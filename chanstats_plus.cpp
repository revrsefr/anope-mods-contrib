// Anope IRC Services module>
//
// SPDX-License-Identifier: GPL-2.0-only
//
// chanstats_plus: a third-party chanstats module optimized for low SQL round-trips.
//
// Overview:
// - Buffers per-message counters in memory and flushes them periodically in batched UPSERTs.
// - Stores multiple periods as separate rows, keyed by a period start date:
//   - daily:   YYYY-MM-DD (start of day)
//   - weekly:  YYYY-MM-DD (start of week; Monday)
//   - monthly: YYYY-MM-01 (start of month)
//   - total:   1970-01-01
// - Does not require stored procedures or SQL EVENT schedulers.
//
// What it tracks (per channel, and optionally per identified nick):
// - lines, words, letters, actions (/me), smileys (happy/sad/other)
// - kicks given / kicks received, channel mode changes, topic changes
//
// Commands / toggles:
// - ChanServ: /msg ChanServ SET <channel> CHANSTATSPLUS {ON|OFF}
// - NickServ: /msg NickServ SET CHANSTATSPLUS {ON|OFF}
//   Only identified users who enable the NickServ option are recorded as per-nick stats.
//
// Config example:
//
// module {
//   name = "rpc_chanstatsplus"
//   engine = "mysql/dbstats"
//   prefix = "anope_"
//   flushinterval = 5s
//   maxpending = 100000
//   maxrowsperquery = 500
//   smileyshappy = { ":)" ":-)" ":D" }
//   smileyssad = { ":(" ":-(" }
//   smileysother = { ";)" ";-)" }
// }
//

#include "module.h"
#include "modules/sql.h"

#include <cctype>
#include <ctime>


namespace
{
	static constexpr const char *EXT_CS_STATS = "CS_STATS_PLUS";
	static constexpr const char *EXT_NS_STATS = "NS_STATS_PLUS";

	struct StatsDelta final
	{
		uint64_t letters = 0;
		uint64_t words = 0;
		uint64_t lines = 0;
		uint64_t actions = 0;
		uint64_t smileys_happy = 0;
		uint64_t smileys_sad = 0;
		uint64_t smileys_other = 0;
		uint64_t kicks = 0;
		uint64_t kicked = 0;
		uint64_t modes = 0;
		uint64_t topics = 0;

		bool Empty() const
		{
			return letters == 0 && words == 0 && lines == 0 && actions == 0 && smileys_happy == 0 &&
				smileys_sad == 0 && smileys_other == 0 && kicks == 0 && kicked == 0 && modes == 0 && topics == 0;
		}

		void Add(const StatsDelta &other)
		{
			letters += other.letters;
			words += other.words;
			lines += other.lines;
			actions += other.actions;
			smileys_happy += other.smileys_happy;
			smileys_sad += other.smileys_sad;
			smileys_other += other.smileys_other;
			kicks += other.kicks;
			kicked += other.kicked;
			modes += other.modes;
			topics += other.topics;
		}
	};

	static inline bool IsSpace(unsigned char c)
	{
		return std::isspace(c) != 0;
	}

	static size_t CountWords(const Anope::string &msg)
	{
		bool in_word = false;
		size_t words = 0;
		for (const unsigned char c : msg)
		{
			if (IsSpace(c))
			{
				in_word = false;
				continue;
			}
			if (!in_word)
			{
				in_word = true;
				words++;
			}
		}
		return words;
	}

	static size_t CountSmileys(const Anope::string &msg, const Anope::string &smileylist)
	{
		size_t smileys = 0;
		spacesepstream sep(smileylist);
		Anope::string token;
		while (sep.GetToken(token) && !token.empty())
		{
			for (size_t pos = msg.find(token, 0); pos != Anope::string::npos; pos = msg.find(token, pos + 1))
				smileys++;
		}
		return smileys;
	}

	static bool IsCTCPAction(const Anope::string &msg)
	{
		return msg.length() >= 8 && msg[0] == '\x01' && msg.find("ACTION ") == 1;
	}

	static Anope::string StripCTCPAction(const Anope::string &msg)
	{
		// "\x01ACTION <text>\x01" -> "<text>"
		if (!IsCTCPAction(msg))
			return msg;
		Anope::string out = msg;
		if (!out.empty() && out[out.length() - 1] == '\x01')
			out.erase(out.length() - 1);
		// remove leading "\x01ACTION "
		if (out.length() >= 8)
			out.erase(0, 8);
		return out;
	}

	static Anope::string FormatDateYMD(time_t ts)
	{
		std::tm tmv;
		localtime_r(&ts, &tmv);
		char buf[11];
		std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tmv);
		return buf;
	}

	static time_t StartOfDay(time_t ts)
	{
		std::tm tmv;
		localtime_r(&ts, &tmv);
		tmv.tm_hour = 0;
		tmv.tm_min = 0;
		tmv.tm_sec = 0;
		return std::mktime(&tmv);
	}

	static time_t StartOfMonth(time_t ts)
	{
		std::tm tmv;
		localtime_r(&ts, &tmv);
		tmv.tm_mday = 1;
		tmv.tm_hour = 0;
		tmv.tm_min = 0;
		tmv.tm_sec = 0;
		return std::mktime(&tmv);
	}

	static time_t StartOfWeek(time_t ts)
	{
		// Week starts Monday (ISO-ish). If you want Sunday, make it configurable.
		std::tm tmv;
		localtime_r(&ts, &tmv);
		int wday = tmv.tm_wday; // 0=Sun..6=Sat
		int days_from_monday = (wday + 6) % 7;
		time_t day_start = StartOfDay(ts);
		return day_start - static_cast<time_t>(days_from_monday) * 86400;
	}

	static Anope::string MakeKey(const Anope::string &chan, const Anope::string &nick, const Anope::string &period,
		const Anope::string &period_start)
	{
		Anope::string key(chan);
		key.push_back('\x1f');
		key += nick;
		key.push_back('\x1f');
		key += period;
		key.push_back('\x1f');
		key += period_start;
		return key;
	}

	static void SplitKey(const Anope::string &key, Anope::string &chan, Anope::string &nick, Anope::string &period,
		Anope::string &period_start)
	{
		size_t p1 = key.find('\x1f');
		size_t p2 = key.find('\x1f', p1 + 1);
		size_t p3 = key.find('\x1f', p2 + 1);
		chan = key.substr(0, p1);
		nick = key.substr(p1 + 1, p2 - (p1 + 1));
		period = key.substr(p2 + 1, p3 - (p2 + 1));
		period_start = key.substr(p3 + 1);
	}

}

class CommandCSSetChanstatsPlus final
	: public Command
{
public:
	CommandCSSetChanstatsPlus(Module *creator) : Command(creator, "chanserv/set/chanstatsplus", 2, 2)
	{
		this->SetDesc(_("Turn chanstats+ statistics on or off"));
		this->SetSyntax(_("\037channel\037 {ON | OFF}"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		ChannelInfo *ci = ChannelInfo::Find(params[0]);
		if (!ci)
		{
			source.Reply(CHAN_X_NOT_REGISTERED, params[0].c_str());
			return;
		}

		EventReturn MOD_RESULT;
		FOREACH_RESULT(OnSetChannelOption, MOD_RESULT, (source, this, ci, params[1]));
		if (MOD_RESULT == EVENT_STOP)
			return;

		if (MOD_RESULT != EVENT_ALLOW && !source.AccessFor(ci).HasPriv("SET") && source.permission.empty() &&
			!source.HasPriv("chanserv/administration"))
		{
			source.Reply(ACCESS_DENIED);
			return;
		}

		if (params[1].equals_ci("ON"))
		{
			ci->Extend<bool>(EXT_CS_STATS);
			source.Reply(_("Chanstats+ statistics are now enabled for this channel."));
			Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci)
				<< "to enable chanstatsplus";
		}
		else if (params[1].equals_ci("OFF"))
		{
			Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci)
				<< "to disable chanstatsplus";
			ci->Shrink<bool>(EXT_CS_STATS);
			source.Reply(_("Chanstats+ statistics are now disabled for this channel."));
		}
		else
			this->OnSyntaxError(source, "");
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Turns chanstats+ statistics ON or OFF."));
		return true;
	}
};

class CommandNSSetChanstatsPlus
	: public Command
{
public:
	CommandNSSetChanstatsPlus(Module *creator, const Anope::string &sname = "nickserv/set/chanstatsplus", size_t min = 1)
		: Command(creator, sname, min, min + 1)
	{
		this->SetDesc(_("Turn chanstats+ statistics on or off"));
		this->SetSyntax("{ON | OFF}");
	}

	void Run(CommandSource &source, const Anope::string &user, const Anope::string &param, bool saset = false)
	{
		NickAlias *na = NickAlias::Find(user);
		if (!na)
		{
			source.Reply(NICK_X_NOT_REGISTERED, user.c_str());
			return;
		}

		EventReturn MOD_RESULT;
		FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, na->nc, param));
		if (MOD_RESULT == EVENT_STOP)
			return;

		if (param.equals_ci("ON"))
		{
			Log(na->nc == source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this)
				<< "to enable chanstatsplus for " << na->nc->display;
			na->nc->Extend<bool>(EXT_NS_STATS);
			if (saset)
				source.Reply(_("Chanstats+ statistics are now enabled for %s"), na->nc->display.c_str());
			else
				source.Reply(_("Chanstats+ statistics are now enabled for your nick."));
		}
		else if (param.equals_ci("OFF"))
		{
			Log(na->nc == source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this)
				<< "to disable chanstatsplus for " << na->nc->display;
			na->nc->Shrink<bool>(EXT_NS_STATS);
			if (saset)
				source.Reply(_("Chanstats+ statistics are now disabled for %s"), na->nc->display.c_str());
			else
				source.Reply(_("Chanstats+ statistics are now disabled for your nick."));
		}
		else
			this->OnSyntaxError(source, "CHANSTATPLUS");
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		this->Run(source, source.nc->display, params[0]);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Turns chanstats+ statistics ON or OFF."));
		return true;
	}
};

class CommandNSSASetChanstatsPlus final
	: public CommandNSSetChanstatsPlus
{
public:
	CommandNSSASetChanstatsPlus(Module *creator) : CommandNSSetChanstatsPlus(creator, "nickserv/saset/chanstatsplus", 2)
	{
		this->ClearSyntax();
		this->SetSyntax(_("\037nickname\037 {ON | OFF}"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		this->Run(source, params[0], params[1], true);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Turns chanstats+ statistics ON or OFF for this user."));
		return true;
	}
};

class ChanstatsPlusSQLInterface final
	: public SQL::Interface
{
public:
	ChanstatsPlusSQLInterface(Module *m) : SQL::Interface(m) { }

	void OnResult(const SQL::Result &) override { }

	void OnError(const SQL::Result &r) override
	{
		if (!r.GetQuery().query.empty())
			Log(LOG_DEBUG) << "chanstats_plus: SQL error executing query " << r.finished_query << ": " << r.GetError();
		else
			Log(LOG_DEBUG) << "chanstats_plus: SQL error executing query: " << r.GetError();
	}
};

class MChanstatsPlus final
	: public Module
{
	SerializableExtensibleItem<bool> cs_stats;
	SerializableExtensibleItem<bool> ns_stats;

	CommandCSSetChanstatsPlus command_cs_set;
	CommandNSSetChanstatsPlus command_ns_set;
	CommandNSSASetChanstatsPlus command_ns_saset;

	ServiceReference<SQL::Provider> sql;
	ChanstatsPlusSQLInterface sqlinterface;

	Anope::string engine;
	Anope::string prefix;
	Anope::string SmileysHappy;
	Anope::string SmileysSad;
	Anope::string SmileysOther;

	time_t flush_interval = 5;
	size_t max_pending = 100000;
	size_t max_rows_per_query = 500;

	Anope::unordered_map<StatsDelta> pending;

	class FlushTimer final
		: public Timer
	{
		MChanstatsPlus *owner;

	public:
		FlushTimer(MChanstatsPlus *m, time_t interval)
			: Timer(m, interval, true)
			, owner(m)
		{
		}

		void Tick() override
		{
			if (owner)
				owner->Flush(false);
		}
	};

	FlushTimer *flush_timer = nullptr;

	void RunQuery(const SQL::Query &q)
	{
		if (sql)
			sql->Run(&sqlinterface, q);
	}

	Anope::string GetDisplay(User *u) const
	{
		if (u && u->IsIdentified() && ns_stats.HasExt(u->Account()))
			return u->Account()->display;
		return "";
	}

	void EnsureSchema()
	{
		if (!sql)
			return;

		SQL::Query q;
		q = "CREATE TABLE IF NOT EXISTS `" + prefix + "chanstatsplus` ("
			"`chan` varchar(64) NOT NULL DEFAULT '',"
			"`nick` varchar(64) NOT NULL DEFAULT '',"
			"`period` ENUM('total','monthly','weekly','daily') NOT NULL,"
			"`period_start` date NOT NULL,"
			"`letters` bigint unsigned NOT NULL DEFAULT '0',"
			"`words` bigint unsigned NOT NULL DEFAULT '0',"
			"`lines` int unsigned NOT NULL DEFAULT '0',"
			"`actions` int unsigned NOT NULL DEFAULT '0',"
			"`smileys_happy` int unsigned NOT NULL DEFAULT '0',"
			"`smileys_sad` int unsigned NOT NULL DEFAULT '0',"
			"`smileys_other` int unsigned NOT NULL DEFAULT '0',"
			"`kicks` int unsigned NOT NULL DEFAULT '0',"
			"`kicked` int unsigned NOT NULL DEFAULT '0',"
			"`modes` int unsigned NOT NULL DEFAULT '0',"
			"`topics` int unsigned NOT NULL DEFAULT '0',"
			"PRIMARY KEY (`chan`,`nick`,`period`,`period_start`),"
			"KEY `nick_idx` (`nick`),"
			"KEY `chan_idx` (`chan`),"
			"KEY `period_idx` (`period`,`period_start`)"
			") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";
		this->RunQuery(q);
	}

	void AddForPeriods(const Anope::string &chan, const Anope::string &nick, const StatsDelta &delta)
	{
		if (delta.Empty())
			return;

		const time_t now = Anope::CurTime;
		const Anope::string day = FormatDateYMD(StartOfDay(now));
		const Anope::string week = FormatDateYMD(StartOfWeek(now));
		const Anope::string month = FormatDateYMD(StartOfMonth(now));
		const Anope::string total = "1970-01-01";

		struct PeriodRow final
		{
			const char *name;
			const Anope::string *start;
		};

		const PeriodRow rows[] = {
			{"total", &total},
			{"monthly", &month},
			{"weekly", &week},
			{"daily", &day},
		};

		for (const auto &row : rows)
		{
			Anope::string key = MakeKey(chan, nick, row.name, *row.start);
			pending[key].Add(delta);
		}
	}

	void AddEvent(const Anope::string &channel, const Anope::string &nick, const StatsDelta &delta)
	{
		// Channel aggregate
		AddForPeriods(channel, "", delta);

		if (!nick.empty())
		{
			// Per-nick (in channel)
			AddForPeriods(channel, nick, delta);
			// Per-nick (global)
			AddForPeriods("", nick, delta);
		}

		if (pending.size() >= max_pending)
			Flush(false);
	}

	void Flush(bool force_sync)
	{
		if (!sql || pending.empty())
			return;

		size_t processed = 0;
		while (!pending.empty())
		{
			SQL::Query q;
			Anope::string query = "INSERT INTO `" + prefix + "chanstatsplus` ("
				"`chan`,`nick`,`period`,`period_start`,"
				"`letters`,`words`,`lines`,`actions`,"
				"`smileys_happy`,`smileys_sad`,`smileys_other`,"
				"`kicks`,`kicked`,`modes`,`topics`) VALUES ";

			size_t rowcount = 0;
			for (auto it = pending.begin(); it != pending.end() && rowcount < max_rows_per_query;)
			{
				Anope::string chan, nick, period, period_start;
				SplitKey(it->first, chan, nick, period, period_start);

				const StatsDelta delta = it->second;
				it = pending.erase(it);

				const Anope::string idx = Anope::ToString(rowcount);
				if (rowcount)
					query += ",";
				query += "(@chan" + idx + "@,@nick" + idx + "@,@period" + idx + "@,@pstart" + idx + "@,";
				query += "@letters" + idx + "@,@words" + idx + "@,@lines" + idx + "@,@actions" + idx + "@,";
				query += "@smh" + idx + "@,@sms" + idx + "@,@smo" + idx + "@,";
				query += "@kicks" + idx + "@,@kicked" + idx + "@,@modes" + idx + "@,@topics" + idx + "@)";

				q.SetValue("chan" + idx, chan);
				q.SetValue("nick" + idx, nick);
				q.SetValue("period" + idx, period);
				q.SetValue("pstart" + idx, period_start);

				q.SetValue("letters" + idx, Anope::ToString(delta.letters), false);
				q.SetValue("words" + idx, Anope::ToString(delta.words), false);
				q.SetValue("lines" + idx, Anope::ToString(delta.lines), false);
				q.SetValue("actions" + idx, Anope::ToString(delta.actions), false);
				q.SetValue("smh" + idx, Anope::ToString(delta.smileys_happy), false);
				q.SetValue("sms" + idx, Anope::ToString(delta.smileys_sad), false);
				q.SetValue("smo" + idx, Anope::ToString(delta.smileys_other), false);
				q.SetValue("kicks" + idx, Anope::ToString(delta.kicks), false);
				q.SetValue("kicked" + idx, Anope::ToString(delta.kicked), false);
				q.SetValue("modes" + idx, Anope::ToString(delta.modes), false);
				q.SetValue("topics" + idx, Anope::ToString(delta.topics), false);

				rowcount++;
			}

			query += " ON DUPLICATE KEY UPDATE "
				"letters=letters+VALUES(letters),"
				"words=words+VALUES(words),"
				"`lines`=`lines`+VALUES(`lines`),"
				"actions=actions+VALUES(actions),"
				"smileys_happy=smileys_happy+VALUES(smileys_happy),"
				"smileys_sad=smileys_sad+VALUES(smileys_sad),"
				"smileys_other=smileys_other+VALUES(smileys_other),"
				"kicks=kicks+VALUES(kicks),"
				"kicked=kicked+VALUES(kicked),"
				"modes=modes+VALUES(modes),"
				"topics=topics+VALUES(topics);";

			// IMPORTANT: SQL::Query::operator= clears parameters.
			q.query = query;

			if (force_sync)
				sql->RunQuery(q);
			else
				this->RunQuery(q);

			processed += rowcount;
		}

		if (processed)
			Log(LOG_DEBUG) << "chanstats_plus: flushed " << processed << " rows";
	}

public:
	MChanstatsPlus(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, EXTRA | VENDOR)
		, cs_stats(this, EXT_CS_STATS)
		, ns_stats(this, EXT_NS_STATS)
		, command_cs_set(this)
		, command_ns_set(this)
		, command_ns_saset(this)
		, sql("", "")
		, sqlinterface(this)
	{
	}

	~MChanstatsPlus() override
	{
		Flush(true);
		delete flush_timer;
		flush_timer = nullptr;
	}

	void OnReload(Configuration::Conf &conf) override
	{
		const auto &block = conf.GetModule(this);
		prefix = block.Get<const Anope::string>("prefix", "anope_");
		SmileysHappy = block.Get<const Anope::string>("smileyshappy");
		SmileysSad = block.Get<const Anope::string>("smileyssad");
		SmileysOther = block.Get<const Anope::string>("smileysother");
		engine = block.Get<const Anope::string>("engine");

		flush_interval = block.Get<time_t>("flushinterval", "5s");
		max_pending = block.Get<size_t>("maxpending", "100000");
		max_rows_per_query = block.Get<size_t>("maxrowsperquery", "500");

		this->sql = ServiceReference<SQL::Provider>("SQL::Provider", engine);
		if (!sql)
		{
			Log(this) << "chanstats_plus: no database connection to " << engine;
			return;
		}

		EnsureSchema();

		delete flush_timer;
		flush_timer = new FlushTimer(this, flush_interval);
	}

	void OnChanInfo(CommandSource &source, ChannelInfo *ci, InfoFormatter &info, bool show_all) override
	{
		if (!show_all)
			return;
		if (cs_stats.HasExt(ci))
			info.AddOption(_("Chanstats+"));
	}

	void OnNickInfo(CommandSource &source, NickAlias *na, InfoFormatter &info, bool show_hidden) override
	{
		if (!show_hidden)
			return;
		if (ns_stats.HasExt(na->nc))
			info.AddOption(_("Chanstats+"));
	}

	void OnTopicUpdated(User *source, Channel *c, const Anope::string &, const Anope::string &) override
	{
		if (!source || !source->IsIdentified() || !c || !c->ci || !cs_stats.HasExt(c->ci))
			return;

		StatsDelta d;
		d.topics = 1;
		AddEvent(c->name, GetDisplay(source), d);
	}

	EventReturn OnChannelModeSet(Channel *c, MessageSource &setter, ChannelMode *, const ModeData &) override
	{
		OnModeChange(c, setter.GetUser());
		return EVENT_CONTINUE;
	}

	EventReturn OnChannelModeUnset(Channel *c, MessageSource &setter, ChannelMode *, const Anope::string &) override
	{
		OnModeChange(c, setter.GetUser());
		return EVENT_CONTINUE;
	}

	void OnPreUserKicked(const MessageSource &source, ChanUserContainer *cu, const Anope::string &) override
	{
		if (!cu || !cu->chan || !cu->chan->ci || !cs_stats.HasExt(cu->chan->ci))
			return;

		StatsDelta dkicked;
		dkicked.kicked = 1;
		AddEvent(cu->chan->name, GetDisplay(cu->user), dkicked);

		StatsDelta dkicks;
		dkicks.kicks = 1;
		AddEvent(cu->chan->name, GetDisplay(source.GetUser()), dkicks);
	}

	void OnPrivmsg(User *u, Channel *c, Anope::string &msg, const Anope::map<Anope::string> &) override
	{
		if (!c || !c->ci || !cs_stats.HasExt(c->ci))
			return;

		Anope::string effective = msg;
		StatsDelta d;

		if (IsCTCPAction(msg))
		{
			d.actions = 1;
			effective = StripCTCPAction(msg);
		}

		d.lines = 1;
		d.letters = static_cast<uint64_t>(effective.length());
		d.words = static_cast<uint64_t>(CountWords(effective));

		d.smileys_happy = static_cast<uint64_t>(CountSmileys(effective, SmileysHappy));
		d.smileys_sad = static_cast<uint64_t>(CountSmileys(effective, SmileysSad));
		d.smileys_other = static_cast<uint64_t>(CountSmileys(effective, SmileysOther));

		const uint64_t smiley_count = d.smileys_happy + d.smileys_sad + d.smileys_other;
		if (smiley_count >= d.words)
			d.words = 0;
		else
			d.words -= smiley_count;

		AddEvent(c->name, GetDisplay(u), d);
	}

private:
	void OnModeChange(Channel *c, User *u)
	{
		if (!u || !u->IsIdentified() || !c || !c->ci || !cs_stats.HasExt(c->ci))
			return;

		StatsDelta d;
		d.modes = 1;
		AddEvent(c->name, GetDisplay(u), d);
	}
};

MODULE_INIT(MChanstatsPlus)
