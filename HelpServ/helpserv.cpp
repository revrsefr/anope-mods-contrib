/*
 * (C) 2026 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Channel: #development
 *
 * HelpServ module created by reverse for Anope 2.1
 * - Help topics (HELP <topic>) + SEARCH + STATS
 * - Atheme-like escalation: HELPME (page staff immediately)
 * - Ticket queue: REQUEST/CANCEL + staff LIST/CLOSE
 *
 * Example configuration:
 *
 * service
 * {
 *   nick = "HelpServ"
 *   user = "HelpServ"
 *   host = "chaat.services"
 *   gecos = "Help Service"
 *   channels = "@#services"  # optional (HelpServ will join staff_target too)
 * }
 *
 * module
 * {
 *   name = "helpserv"
 *   client = "HelpServ"
 *
 *   # Paging target for HELPME/REQUEST:
 *   # - "globops"   : send to oper globops/wallops
 *   # - "#channel"  : send to a staff channel (HelpServ will join it)
 *   # - empty        : disable paging
 *   staff_target = "#services"
 *
 *   # Cooldowns (seconds)
 *   helpme_cooldown = 120
 *   request_cooldown = 60
 *
 *   # When enabled, REQUEST also pages staff_target
 *   page_on_request = yes
 *
 *   # Privilege required for staff commands LIST/CLOSE
 *   ticket_priv = "helpserv/ticket"
 *
 *   topic
 *   {
 *     name = "register"
 *     line { text = "To register your nickname: /msg NickServ REGISTER <password> <email>" }
 *   }
 * }
 *
 * command { service = "HelpServ"; name = "HELP"; command = "generic/help"; }
 * command { service = "HelpServ"; name = "SEARCH"; command = "helpserv/search"; }
 * command { service = "HelpServ"; name = "STATS"; command = "helpserv/stats"; hide = true; }
 * command { service = "HelpServ"; name = "HELPME"; command = "helpserv/helpme"; }
 * command { service = "HelpServ"; name = "REQUEST"; command = "helpserv/request"; }
 * command { service = "HelpServ"; name = "CANCEL"; command = "helpserv/cancel"; }
 * command { service = "HelpServ"; name = "LIST"; command = "helpserv/list"; hide = true; }
 * command { service = "HelpServ"; name = "CLOSE"; command = "helpserv/close"; hide = true; }
 */

#include "module.h"

#include <filesystem>
#include <fstream>
#include <cstdarg>
#include <utility>

namespace fs = std::filesystem;

namespace
{
	Anope::string NormalizeTopic(const Anope::string& in)
	{
		Anope::string out = in;
		out.trim();
		out = out.lower();
		return out;
	}

	Anope::string EscapeValue(const Anope::string& in)
	{
		Anope::string out;
		for (const char ch : in)
		{
			switch (ch)
			{
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n"; break;
				case '\r': break;
				default: out += ch; break;
			}
		}
		return out;
	}

	Anope::string UnescapeValue(const Anope::string& in)
	{
		Anope::string out;
		for (size_t i = 0; i < in.length(); ++i)
		{
			const char ch = in[i];
			if (ch != '\\' || i + 1 >= in.length())
			{
				out += ch;
				continue;
			}

			const char next = in[i + 1];
			if (next == 'n')
			{
				out += '\n';
				++i;
			}
			else if (next == '\\')
			{
				out += '\\';
				++i;
			}
			else
			{
				out += next;
				++i;
			}
		}
		return out;
	}
}

class HelpServCore;

class CommandHelpServSearch final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServSearch(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServStats final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServStats(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServHelpMe final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServHelpMe(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServRequest final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServRequest(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServCancel final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServCancel(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServList final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServList(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServClose final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServClose(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServTake final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServTake(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServAssign final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServAssign(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServNote final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServNote(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServView final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServView(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServNotify final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServNotify(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class HelpServCore final : public Module
{
	struct Ticket final
	{
		uint64_t id = 0;
		Anope::string account;
		Anope::string nick;
		Anope::string requester;
		Anope::string topic;
		Anope::string message;
		Anope::string assigned;
		std::vector<Anope::string> notes;
		time_t created = 0;
		time_t updated = 0;
	};

	Reference<BotInfo> HelpServ;
	std::map<Anope::string, std::vector<Anope::string>> topics;
	std::map<Anope::string, uint64_t> topic_requests;
	std::map<Anope::string, uint64_t> loaded_topic_requests;

	// Ticket queue.
	uint64_t next_ticket_id = 1;
	std::map<uint64_t, Ticket> tickets_by_id;
	std::map<Anope::string, uint64_t> open_ticket_by_account;
	Anope::map<time_t> last_helpme_by_key;
	Anope::map<time_t> last_request_by_key;

	// Escalation settings.
	Anope::string staff_target;
	time_t helpme_cooldown = 120;
	time_t request_cooldown = 60;
	bool page_on_request = false;
	Anope::string ticket_priv = "helpserv/ticket";
	time_t ticket_expire = 0;

	// Reply mode for user-facing output.
	bool reply_with_notice = true;
	Anope::string notify_priv = "helpserv/admin";

	uint64_t help_requests = 0;
	uint64_t search_requests = 0;
	uint64_t search_hits = 0;
	uint64_t search_misses = 0;
	uint64_t unknown_topics = 0;
	time_t last_reload = 0;
	time_t last_stats_save = 0;
	uint64_t pending_stat_updates = 0;

	CommandHelpServSearch command_search;
	CommandHelpServStats command_stats;
	CommandHelpServHelpMe command_helpme;
	CommandHelpServRequest command_request;
	CommandHelpServCancel command_cancel;
	CommandHelpServList command_list;
	CommandHelpServClose command_close;
	CommandHelpServTake command_take;
	CommandHelpServAssign command_assign;
	CommandHelpServNote command_note;
	CommandHelpServView command_view;
	CommandHelpServNotify command_notify;

	void Reply(CommandSource& source, const Anope::string& msg)
	{
		if (!this->HelpServ.operator bool())
		{
			source.Reply("%s", msg.c_str());
			return;
		}

		User* u = source.GetUser();
		if (!u)
		{
			source.Reply("%s", msg.c_str());
			return;
		}

		Anope::map<Anope::string> tags;
		if (!source.msgid.empty())
			tags["+draft/reply"] = source.msgid;

		LineWrapper lw(Language::Translate(u, msg.c_str()));
		for (Anope::string line; lw.GetLine(line); )
		{
			if (this->reply_with_notice)
				IRCD->SendNotice(*this->HelpServ, u->GetUID(), line, tags);
			else
				IRCD->SendPrivmsg(*this->HelpServ, u->GetUID(), line, tags);
		}
	}

	void Reply(CommandSource& source, const char* text)
	{
		this->Reply(source, Anope::string(text));
	}

	void ReplyF(CommandSource& source, const char* fmt, ...) ATTR_FORMAT(3, 4)
	{
		va_list args;
		va_start(args, fmt);
		Anope::string msg = Anope::Format(args, fmt);
		va_end(args);
		this->Reply(source, msg);
	}

	Anope::string ReplyModeString() const
	{
		return this->reply_with_notice ? "notice" : "privmsg";
	}

	bool SetReplyMode(const Anope::string& mode)
	{
		if (mode.equals_ci("notice"))
		{
			this->reply_with_notice = true;
			return true;
		}
		if (mode.equals_ci("privmsg") || mode.equals_ci("msg"))
		{
			this->reply_with_notice = false;
			return true;
		}
		return false;
	}

	void SendIndex(CommandSource& source)
	{
		this->Reply(source, "Hi! I'm HelpServ. Try: \002HELP\002 or \002HELP <topic>\002");
		this->Reply(source, "You can also try: \002SEARCH <words>\002");
		this->Reply(source, "Need staff? Try: \002HELPME <topic> [message]\002 or \002REQUEST <topic> [message]\002");

		if (this->topics.empty())
		{
			this->Reply(source, "No help topics are configured.");
			return;
		}

		Anope::string topic_list;
		for (const auto& [name, _] : this->topics)
		{
			if (!topic_list.empty())
				topic_list += ", ";
			topic_list += name;
		}

		this->ReplyF(source, "Available topics: %s", topic_list.c_str());
	}

	bool SendTopic(CommandSource& source, const Anope::string& topic_key)
	{
		auto it = this->topics.find(topic_key);
		if (it == this->topics.end())
			return false;

		this->ReplyF(source, "Help for \002%s\002:", topic_key.c_str());
		for (const auto& line : it->second)
			this->Reply(source, line);
		return true;
	}

	static bool ContainsCaseInsensitive(const Anope::string& haystack, const Anope::string& needle)
	{
		if (needle.empty())
			return true;
		return haystack.lower().find(needle.lower()) != Anope::string::npos;
	}

	void SendStats(CommandSource& source)
	{
		size_t line_count = 0;
		for (const auto& [_, lines] : this->topics)
			line_count += lines.size();

		this->Reply(source, "HelpServ stats:");
		this->ReplyF(source, "Topics: %zu, Lines: %zu", this->topics.size(), line_count);
		this->ReplyF(source, "HELP requests: %llu (unknown topics: %llu)",
			static_cast<unsigned long long>(this->help_requests),
			static_cast<unsigned long long>(this->unknown_topics));
		this->ReplyF(source, "SEARCH requests: %llu (hits: %llu, misses: %llu)",
			static_cast<unsigned long long>(this->search_requests),
			static_cast<unsigned long long>(this->search_hits),
			static_cast<unsigned long long>(this->search_misses));

		if (!this->topic_requests.empty())
		{
			// Show top 5 topics.
			std::vector<std::pair<Anope::string, uint64_t>> top;
			top.reserve(this->topic_requests.size());
			for (const auto& kv : this->topic_requests)
				top.push_back(kv);
			std::sort(top.begin(), top.end(), [](const auto& a, const auto& b) {
				return a.second > b.second;
			});

			Anope::string buf;
			size_t shown = 0;
			for (const auto& [name, count] : top)
			{
				if (shown++ >= 5)
					break;
				if (!buf.empty())
					buf += ", ";
				buf += Anope::Format("%s(%llu)", name.c_str(), static_cast<unsigned long long>(count));
			}
			if (!buf.empty())
				this->ReplyF(source, "Top topics: %s", buf.c_str());
		}

		if (this->last_reload)
			this->ReplyF(source, "Last reload: %s", Anope::strftime(this->last_reload, source.GetAccount()).c_str());
	}

	static Anope::string GetStatsPath()
	{
		// Stored in Anope data directory (requested: data/helpserv.db).
		return Anope::ExpandData("helpserv.db");
	}

	static Anope::string GetTicketsPath()
	{
		return Anope::ExpandData("helpserv_tickets.db");
	}

	static bool ParseU64(const Anope::string& in, uint64_t& out)
	{
		try
		{
			out = Anope::Convert<uint64_t>(in, 0);
			return true;
		}
		catch (...)
		{
			out = 0;
			return false;
		}
	}

	static bool TryParseTicketId(const Anope::string& in, uint64_t& out)
	{
		Anope::string s = in;
		s.trim();
		if (s.empty())
			return false;
		if (s[0] == '#')
			s = s.substr(1);
		s.trim();
		if (s.empty())
			return false;
		for (size_t i = 0; i < s.length(); ++i)
			if (s[i] < '0' || s[i] > '9')
				return false;
		return ParseU64(s, out) && out != 0;
	}

	static time_t ParseDurationSeconds(const Anope::string& in, time_t fallback)
	{
		Anope::string s = in;
		s.trim();
		if (s.empty())
			return fallback;

		// Pure number = seconds.
		bool all_digits = true;
		for (size_t i = 0; i < s.length(); ++i)
			if (s[i] < '0' || s[i] > '9')
				all_digits = false;
		if (all_digits)
		{
			uint64_t n = 0;
			if (!ParseU64(s, n))
				return fallback;
			return static_cast<time_t>(n);
		}

		// Number + suffix (s/m/h/d/w).
		char suffix = s[s.length() - 1];
		Anope::string num = s.substr(0, s.length() - 1);
		num.trim();
		if (num.empty())
			return fallback;
		for (size_t i = 0; i < num.length(); ++i)
			if (num[i] < '0' || num[i] > '9')
				return fallback;
		uint64_t n = 0;
		if (!ParseU64(num, n))
			return fallback;
		uint64_t mult = 1;
		switch (suffix)
		{
			case 's': case 'S': mult = 1; break;
			case 'm': case 'M': mult = 60; break;
			case 'h': case 'H': mult = 60 * 60; break;
			case 'd': case 'D': mult = 60 * 60 * 24; break;
			case 'w': case 'W': mult = 60 * 60 * 24 * 7; break;
			default: return fallback;
		}
		return static_cast<time_t>(n * mult);
	}

	Ticket* FindTicketById(uint64_t id)
	{
		auto it = this->tickets_by_id.find(id);
		if (it == this->tickets_by_id.end())
			return nullptr;
		return &it->second;
	}

	void PruneExpiredTickets()
	{
		if (!this->ticket_expire)
			return;
		const auto now = Anope::CurTime;
		if (now <= 0)
			return;
		const auto cutoff = now - this->ticket_expire;
		std::vector<uint64_t> expired;
		for (const auto& [id, t] : this->tickets_by_id)
		{
			const auto last = t.updated ? t.updated : t.created;
			if (last && last < cutoff)
				expired.push_back(id);
		}
		if (expired.empty())
			return;

		for (const auto id : expired)
		{
			auto it = this->tickets_by_id.find(id);
			if (it == this->tickets_by_id.end())
				continue;
			this->open_ticket_by_account.erase(NormalizeTopic(it->second.account));
			this->tickets_by_id.erase(it);
		}
		this->SaveTicketsToFile();
		Log(this) << "Expired " << expired.size() << " old tickets";
	}

	void LoadStatsFromFile()
	{
		this->loaded_topic_requests.clear();

		const auto path = GetStatsPath();
		std::ifstream in(path.c_str());
		if (!in.is_open())
			return;

		for (std::string line; std::getline(in, line);)
		{
			Anope::string s(line.c_str());
			s.trim();
			if (s.empty() || s[0] == '#')
				continue;

			auto eq = s.find('=');
			if (eq == Anope::string::npos)
				continue;

			Anope::string key = s.substr(0, eq);
			Anope::string val = s.substr(eq + 1);
			key.trim();
			val.trim();

			uint64_t n = 0;
			if (!ParseU64(val, n))
				continue;

			if (key.equals_ci("help_requests"))
				this->help_requests = n;
			else if (key.equals_ci("search_requests"))
				this->search_requests = n;
			else if (key.equals_ci("search_hits"))
				this->search_hits = n;
			else if (key.equals_ci("search_misses"))
				this->search_misses = n;
			else if (key.equals_ci("unknown_topics"))
				this->unknown_topics = n;
			else if (key.length() > 6 && key.substr(0, 6).equals_ci("topic."))
			{
				const auto t = NormalizeTopic(key.substr(6));
				if (!t.empty())
					this->loaded_topic_requests[t] = n;
			}
		}
	}

	void SaveStatsToFile()
	{
		const auto path = GetStatsPath();
		const auto tmp = path + ".tmp";

		std::ofstream out(tmp.c_str(), std::ios::out | std::ios::trunc);
		if (!out.is_open())
		{
			Log(this) << "Unable to write " << tmp;
			return;
		}

		out << "# HelpServ stats\n";
		out << "version=1\n";
		out << "help_requests=" << this->help_requests << "\n";
		out << "unknown_topics=" << this->unknown_topics << "\n";
		out << "search_requests=" << this->search_requests << "\n";
		out << "search_hits=" << this->search_hits << "\n";
		out << "search_misses=" << this->search_misses << "\n";
		for (const auto& [topic, count] : this->topic_requests)
			out << "topic." << topic << "=" << count << "\n";
		out.close();

		std::error_code ec;
		fs::rename(tmp.c_str(), path.c_str(), ec);
		if (ec)
		{
			fs::remove(path.c_str(), ec);
			ec.clear();
			fs::rename(tmp.c_str(), path.c_str(), ec);
			if (ec)
				Log(this) << "Unable to replace " << path << ": " << ec.message();
		}
	}

	void MaybeSaveStats()
	{
		++this->pending_stat_updates;
		const auto now = Anope::CurTime;
		if (this->last_stats_save == 0)
			this->last_stats_save = now;

		if (now - this->last_stats_save >= 30 || this->pending_stat_updates >= 50)
		{
			this->SaveStatsToFile();
			this->last_stats_save = now;
			this->pending_stat_updates = 0;
		}
	}

	bool IsStaff(CommandSource& source) const
	{
		return source.HasPriv(this->ticket_priv);
	}

	Anope::string GetRequesterKey(CommandSource& source) const
	{
		if (source.GetAccount())
			return source.GetAccount()->display;
		if (source.GetUser())
			return source.GetUser()->GetUID();
		return source.GetNick();
	}

	void PageStaff(const Anope::string& msg)
	{
		if (!this->HelpServ || this->staff_target.empty())
			return;

		if (this->staff_target.equals_ci("globops"))		
		{
			IRCD->SendGlobops(*this->HelpServ, msg);
			return;
		}

		if (this->staff_target[0] == '#')
		{
			this->HelpServ->Join(this->staff_target);
			IRCD->SendPrivmsg(*this->HelpServ, this->staff_target, msg);
		}
	}

	void LoadTicketsFromFile()
	{
		this->tickets_by_id.clear();
		this->open_ticket_by_account.clear();
		this->next_ticket_id = 1;

		const auto path = GetTicketsPath();
		std::ifstream in(path.c_str());
		if (!in.is_open())
			return;

		for (std::string line; std::getline(in, line);)
		{
			Anope::string s(line.c_str());
			s.trim();
			if (s.empty() || s[0] == '#')
				continue;

			auto eq = s.find('=');
			if (eq == Anope::string::npos)
				continue;

			Anope::string key = s.substr(0, eq);
			Anope::string val = s.substr(eq + 1);
			key.trim();
			val = UnescapeValue(val);
			val.trim();

			uint64_t n = 0;
			if (key.equals_ci("next_ticket_id"))
			{
				if (ParseU64(val, n) && n)
					this->next_ticket_id = n;
				continue;
			}

			if (!key.length() || !key.substr(0, 7).equals_ci("ticket."))
				continue;

			// ticket.<id>.<field>
			const auto rest = key.substr(7);
			auto dot = rest.find('.');
			if (dot == Anope::string::npos)
				continue;
			const auto id_s = rest.substr(0, dot);
			const auto field = rest.substr(dot + 1);
			if (!ParseU64(id_s, n) || n == 0)
				continue;

			auto& t = this->tickets_by_id[n];
			t.id = n;
			if (field.equals_ci("account"))
				t.account = val;
			else if (field.equals_ci("nick"))
				t.nick = val;
			else if (field.equals_ci("requester"))
				t.requester = val;
			else if (field.equals_ci("topic"))
				t.topic = val;
			else if (field.equals_ci("message"))
				t.message = val;
			else if (field.equals_ci("assigned"))
				t.assigned = val;
			else if (field.length() > 5 && field.substr(0, 5).equals_ci("note."))
			{
				uint64_t idx = 0;
				if (ParseU64(field.substr(5), idx))
				{
					if (idx < 1000)
					{
						if (t.notes.size() <= idx)
							t.notes.resize(static_cast<size_t>(idx) + 1);
						t.notes[static_cast<size_t>(idx)] = val;
					}
				}
			}
			else if (field.equals_ci("created"))
			{
				uint64_t ts;
				if (ParseU64(val, ts))
					t.created = static_cast<time_t>(ts);
			}
			else if (field.equals_ci("updated"))
			{
				uint64_t ts;
				if (ParseU64(val, ts))
					t.updated = static_cast<time_t>(ts);
			}
		}

		// Build account index.
		for (const auto& [id, t] : this->tickets_by_id)
		{
			if (!t.account.empty())
				this->open_ticket_by_account[NormalizeTopic(t.account)] = id;
			if (id >= this->next_ticket_id)
				this->next_ticket_id = id + 1;
		}

		// If all tickets were closed, reset the counter so the flatfile doesn't
		// keep drifting upwards forever (requested behavior).
		if (this->tickets_by_id.empty() && this->next_ticket_id != 1)
		{
			this->next_ticket_id = 1;
			if (!Anope::ReadOnly)
				this->SaveTicketsToFile();
		}
	}

	void SaveTicketsToFile()
	{
		const auto path = GetTicketsPath();
		const auto tmp = path + ".tmp";

		// When there are no open tickets, reset next_ticket_id for clean files.
		if (this->tickets_by_id.empty())
			this->next_ticket_id = 1;

		std::ofstream out(tmp.c_str(), std::ios::out | std::ios::trunc);
		if (!out.is_open())
		{
			Log(this) << "Unable to write " << tmp;
			return;
		}

		out << "# HelpServ tickets\n";
		out << "version=1\n";
		out << "next_ticket_id=" << this->next_ticket_id << "\n";
		for (const auto& [id, t] : this->tickets_by_id)
		{
			out << "ticket." << id << ".account=" << EscapeValue(t.account) << "\n";
			out << "ticket." << id << ".nick=" << EscapeValue(t.nick) << "\n";
			out << "ticket." << id << ".requester=" << EscapeValue(t.requester) << "\n";
			out << "ticket." << id << ".topic=" << EscapeValue(t.topic) << "\n";
			out << "ticket." << id << ".message=" << EscapeValue(t.message) << "\n";
			out << "ticket." << id << ".assigned=" << EscapeValue(t.assigned) << "\n";
			out << "ticket." << id << ".created=" << static_cast<uint64_t>(t.created) << "\n";
			out << "ticket." << id << ".updated=" << static_cast<uint64_t>(t.updated) << "\n";
			for (size_t i = 0; i < t.notes.size(); ++i)
			{
				if (t.notes[i].empty())
					continue;
				out << "ticket." << id << ".note." << static_cast<uint64_t>(i) << "=" << EscapeValue(t.notes[i]) << "\n";
			}
		}
		out.close();

		std::error_code ec;
		fs::rename(tmp.c_str(), path.c_str(), ec);
		if (ec)
		{
			fs::remove(path.c_str(), ec);
			ec.clear();
			fs::rename(tmp.c_str(), path.c_str(), ec);
			if (ec)
				Log(this) << "Unable to replace " << path << ": " << ec.message();
		}
	}

	Ticket* FindOpenTicketByAccountKey(const Anope::string& account_key)
	{
		auto it = this->open_ticket_by_account.find(NormalizeTopic(account_key));
		if (it == this->open_ticket_by_account.end())
			return nullptr;
		auto it2 = this->tickets_by_id.find(it->second);
		if (it2 == this->tickets_by_id.end())
			return nullptr;
		return &it2->second;
	}

	Ticket* FindOpenTicketByNickOrAccount(const Anope::string& who)
	{
		// Try account key first.
		if (auto* t = this->FindOpenTicketByAccountKey(who))
			return t;

		// Fall back to nick match.
		for (auto& [_, t] : this->tickets_by_id)
			if (t.nick.equals_ci(who))
				return &t;
		return nullptr;
	}

	void NotifyTicketEvent(const Anope::string& msg)
	{
		this->PageStaff(msg);
		Log(this) << msg;
	}

	void Search(CommandSource& source, const Anope::string& query)
	{
		++this->search_requests;
		this->MaybeSaveStats();
		Anope::string q = query;
		q.trim();
		if (q.empty())
		{
			this->Reply(source, "What should I search for? Try: \002SEARCH vhost\002");
			return;
		}

		std::vector<Anope::string> matches;
		for (const auto& [name, lines] : this->topics)
		{
			if (ContainsCaseInsensitive(name, q))
			{
				matches.push_back(name);
				continue;
			}
			for (const auto& line : lines)
			{
				if (ContainsCaseInsensitive(line, q))
				{
					matches.push_back(name);
					break;
				}
			}
		}

		if (matches.empty())
		{
			++this->search_misses;
			this->MaybeSaveStats();
			this->ReplyF(source, "No topics matched \002%s\002.", q.c_str());
			this->Reply(source, "Try: \002HELP\002 to list topics.");
			return;
		}

		++this->search_hits;
		this->MaybeSaveStats();
		std::sort(matches.begin(), matches.end());

		const size_t limit = 10;
		Anope::string out;
		for (size_t i = 0; i < matches.size() && i < limit; ++i)
		{
			if (!out.empty())
				out += ", ";
			out += matches[i];
		}
		this->ReplyF(source, "Matches: %s", out.c_str());
		if (matches.size() > limit)
			this->ReplyF(source, "(%zu more not shown)", matches.size() - limit);
		this->Reply(source, "Use: \002HELP <topic>\002");
	}

	void LoadTopics(const Configuration::Block& mod)
	{
		this->topics.clear();
		this->topic_requests.clear();

		const int topic_count = mod.CountBlock("topic");
		for (int i = 0; i < topic_count; ++i)
		{
			const auto& topic_block = mod.GetBlock("topic", i);
			Anope::string topic_name = NormalizeTopic(topic_block.Get<const Anope::string>("name"));
			if (topic_name.empty())
				continue;

			std::vector<Anope::string> lines;
			const int line_count = topic_block.CountBlock("line");
			for (int j = 0; j < line_count; ++j)
			{
				const auto& line_block = topic_block.GetBlock("line", j);
				Anope::string text = line_block.Get<const Anope::string>("text");
				text.trim();
				if (!text.empty())
					lines.push_back(text);
			}

			if (!lines.empty())
			{
				uint64_t existing = 0;
				auto it = this->loaded_topic_requests.find(topic_name);
				if (it != this->loaded_topic_requests.end())
					existing = it->second;
				this->topic_requests.emplace(topic_name, existing);
				this->topics.emplace(std::move(topic_name), std::move(lines));
			}
		}
	}

public:
	HelpServCore(const Anope::string& modname, const Anope::string& creator)
		: Module(modname, creator, PSEUDOCLIENT | THIRD)
		, command_search(this, *this)
		, command_stats(this, *this)
		, command_helpme(this, *this)
		, command_request(this, *this)
		, command_cancel(this, *this)
		, command_list(this, *this)
		, command_close(this, *this)
		, command_take(this, *this)
		, command_assign(this, *this)
		, command_note(this, *this)
		, command_view(this, *this)
		, command_notify(this, *this)
	{
		if (!IRCD)
			throw ModuleException("IRCd protocol module not loaded");
	}

	~HelpServCore() override
	{
		this->SaveStatsToFile();
	}

	void OnReload(Configuration::Conf& conf) override
	{
		// Reload persisted stats first so topic counters can be preserved.
		this->LoadStatsFromFile();
		this->LoadTicketsFromFile();

		const Configuration::Block* mod = &conf.GetModule(this);
		Anope::string nick = mod->Get<const Anope::string>("client");
		if (nick.empty())
		{
			mod = &conf.GetModule("helpserv");
			nick = mod->Get<const Anope::string>("client");
		}
		if (nick.empty())
		{
			mod = &conf.GetModule("helpserv.so");
			nick = mod->Get<const Anope::string>("client");
		}
		if (nick.empty())
			throw ConfigException(Module::name + ": <client> must be defined");

		BotInfo* bi = BotInfo::Find(nick, true);
		if (!bi)
			throw ConfigException(Module::name + ": no bot named " + nick);

		this->HelpServ = bi;
		this->last_reload = Anope::CurTime;

		// Escalation/ticket configuration.
		this->staff_target = mod->Get<Anope::string>("staff_target", "");
		this->helpme_cooldown = mod->Get<time_t>("helpme_cooldown", "120");
		this->request_cooldown = mod->Get<time_t>("request_cooldown", "60");
		this->page_on_request = mod->Get<bool>("page_on_request", "yes");
		this->ticket_priv = mod->Get<Anope::string>("ticket_priv", "helpserv/ticket");
		this->ticket_expire = ParseDurationSeconds(mod->Get<Anope::string>("ticket_expire", "0"), 0);
		this->notify_priv = mod->Get<Anope::string>("notify_priv", "helpserv/admin");
		this->SetReplyMode(mod->Get<Anope::string>("reply_method", "notice"));

		if (!this->staff_target.empty() && this->staff_target[0] == '#')
			this->HelpServ->Join(this->staff_target);

		this->PruneExpiredTickets();

		this->LoadTopics(*mod);
	}

	EventReturn OnBotPrivmsg(User* u, BotInfo* bi, Anope::string& message, const Anope::map<Anope::string>& tags) override
	{
		if (bi != *this->HelpServ || !u)
			return EVENT_CONTINUE;

		Anope::string trimmed = message;
		trimmed.trim();
		if (trimmed.empty())
			return EVENT_CONTINUE;

		// Let users type "help ..." in any case; normalize it to HELP so generic/help routing works.
		if (trimmed.length() >= 4 && trimmed.substr(0, 4).equals_ci("help"))
		{
			message = "HELP" + trimmed.substr(4);
		}

		return EVENT_CONTINUE;
	}

	EventReturn OnPreHelp(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		if (!this->HelpServ || source.service != *this->HelpServ)
			return EVENT_CONTINUE;

		++this->help_requests;
		this->MaybeSaveStats();

		if (params.empty())
		{
			this->SendIndex(source);
			return EVENT_STOP;
		}

		const Anope::string topic_key = NormalizeTopic(params[0]);
		if (!topic_key.empty() && this->SendTopic(source, topic_key))
		{
			auto it = this->topic_requests.find(topic_key);
			if (it != this->topic_requests.end())
				++it->second;
			this->MaybeSaveStats();
			return EVENT_STOP;
		}

		++this->unknown_topics;
		this->MaybeSaveStats();
		this->Reply(source, "I don't know that topic. Try: \002HELP\002 or \002SEARCH <words>\002");
		return EVENT_STOP;
	}

	friend class CommandHelpServSearch;
	friend class CommandHelpServStats;
	friend class CommandHelpServHelpMe;
	friend class CommandHelpServRequest;
	friend class CommandHelpServCancel;
	friend class CommandHelpServList;
	friend class CommandHelpServClose;
	friend class CommandHelpServTake;
	friend class CommandHelpServAssign;
	friend class CommandHelpServNote;
	friend class CommandHelpServView;
	friend class CommandHelpServNotify;
};

CommandHelpServSearch::CommandHelpServSearch(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/search", 1, 1)
	, hs(parent)
{
	this->SetDesc(_("Search HelpServ topics"));
	this->SetSyntax(_("\037words\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServSearch::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	this->hs.Search(source, params.empty() ? Anope::string() : params[0]);
}

bool CommandHelpServSearch::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Searches HelpServ topics by name and content."));
	this->hs.Reply(source, _("Example: SEARCH vhost"));
	return true;
}

CommandHelpServStats::CommandHelpServStats(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/stats", 0, 0)
	, hs(parent)
{
	this->SetDesc(_("Show HelpServ statistics"));
	this->AllowUnregistered(true);
}

void CommandHelpServStats::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	this->hs.SendStats(source);
}

bool CommandHelpServStats::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Shows topic counts and usage statistics."));
	return true;
}

CommandHelpServHelpMe::CommandHelpServHelpMe(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/helpme", 1, 2)
	, hs(parent)
{
	this->SetDesc(_("Page network staff for help"));
	this->SetSyntax(_("\037topic\037 [\037message\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServHelpMe::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.HelpServ)
		return;

	Anope::string topic = params[0];
	topic.trim();
	Anope::string msg = params.size() > 1 ? params[1] : "";
	msg.trim();

	if (topic.empty())
	{
		this->hs.Reply(source, "Syntax: HELPME <topic> [message]");
		return;
	}

	const auto key = this->hs.GetRequesterKey(source);
	const auto now = Anope::CurTime;
	auto& last = this->hs.last_helpme_by_key[key];
	if (last && now - last < this->hs.helpme_cooldown)
	{
		this->hs.ReplyF(source, "Please wait %lld seconds before using \002HELPME\002 again.",
			static_cast<long long>(this->hs.helpme_cooldown - (now - last)));
		return;
	}
	last = now;

	Anope::string who = source.GetNick();
	if (source.GetUser())
		who = source.GetUser()->GetDisplayedMask();

	Anope::string staff_msg;
	if (msg.empty())
		staff_msg = Anope::Format("HELPME: %s needs help about \002%s\002.", who.c_str(), topic.c_str());
	else
		staff_msg = Anope::Format("HELPME: %s needs help about \002%s\002: %s", who.c_str(), topic.c_str(), msg.c_str());

	this->hs.NotifyTicketEvent(staff_msg);
	this->hs.Reply(source, "Thanks! I've alerted the staff.");
}

bool CommandHelpServHelpMe::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Pages network staff immediately."));
	this->hs.Reply(source, _("Example: HELPME vhost I need help requesting one"));
	return true;
}

CommandHelpServRequest::CommandHelpServRequest(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/request", 1, 2)
	, hs(parent)
{
	this->SetDesc(_("Open or update a help ticket"));
	this->SetSyntax(_("\037topic\037 [\037message\037]"));
	this->AllowUnregistered(false);
}

void CommandHelpServRequest::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.HelpServ)
		return;

	this->hs.PruneExpiredTickets();

	if (!source.GetAccount())
	{
		this->hs.Reply(source, "You must be identified to open a ticket. Try: /msg NickServ IDENTIFY <password>");
		return;
	}

	Anope::string topic = params[0];
	topic.trim();
	Anope::string msg = params.size() > 1 ? params[1] : "";
	msg.trim();

	if (topic.empty())
	{
		this->hs.Reply(source, "Syntax: REQUEST <topic> [message]");
		return;
	}

	const auto key = this->hs.GetRequesterKey(source);
	const auto now = Anope::CurTime;
	auto& last = this->hs.last_request_by_key[key];
	if (last && now - last < this->hs.request_cooldown)
	{
		this->hs.ReplyF(source, "Please wait %lld seconds before creating another ticket.",
			static_cast<long long>(this->hs.request_cooldown - (now - last)));
		return;
	}
	last = now;

	const auto account_key = NormalizeTopic(source.GetAccount()->display);
	auto* existing = this->hs.FindOpenTicketByAccountKey(account_key);
	if (existing)
	{
		this->hs.ReplyF(source, "You already have an open ticket (\002#%llu\002) about \002%s\002; updating it.",
			static_cast<unsigned long long>(existing->id), existing->topic.c_str());

		existing->topic = topic;
		existing->message = msg;
		existing->nick = source.GetNick();
		existing->requester = source.GetNick();
		if (source.GetUser())
			existing->requester = source.GetUser()->GetDisplayedMask();
		existing->updated = now;
		this->hs.SaveTicketsToFile();
		this->hs.ReplyF(source, "Your ticket has been updated (\002#%llu\002).", static_cast<unsigned long long>(existing->id));

		Anope::string who = source.GetNick();
		if (source.GetUser())
			who = source.GetUser()->GetDisplayedMask();
		Anope::string staff_msg;
		if (msg.empty())
			staff_msg = Anope::Format("REQUEST: %s needs help about \002%s\002. (ticket \002#%llu\002)", who.c_str(), topic.c_str(), static_cast<unsigned long long>(existing->id));
		else
			staff_msg = Anope::Format("REQUEST: %s needs help about \002%s\002: %s (ticket \002#%llu\002)", who.c_str(), topic.c_str(), msg.c_str(), static_cast<unsigned long long>(existing->id));
		this->hs.NotifyTicketEvent(staff_msg);
		return;
	}

	HelpServCore::Ticket t;
	t.id = this->hs.next_ticket_id++;
	t.account = source.GetAccount()->display;
	t.nick = source.GetNick();
	t.requester = source.GetNick();
	t.topic = topic;
	t.message = msg;
	if (source.GetUser())
		t.requester = source.GetUser()->GetDisplayedMask();
	t.created = now;
	t.updated = now;

	this->hs.tickets_by_id[t.id] = t;
	this->hs.open_ticket_by_account[NormalizeTopic(t.account)] = t.id;
	this->hs.SaveTicketsToFile();

	this->hs.ReplyF(source, "Your ticket has been opened (\002#%llu\002) about \002%s\002.", static_cast<unsigned long long>(t.id), topic.c_str());

	Anope::string who = source.GetNick();
	if (source.GetUser())
		who = source.GetUser()->GetDisplayedMask();
	Anope::string staff_msg;
	if (msg.empty())
		staff_msg = Anope::Format("REQUEST: %s needs help about \002%s\002. (ticket \002#%llu\002)", who.c_str(), topic.c_str(), static_cast<unsigned long long>(t.id));
	else
		staff_msg = Anope::Format("REQUEST: %s needs help about \002%s\002: %s (ticket \002#%llu\002)", who.c_str(), topic.c_str(), msg.c_str(), static_cast<unsigned long long>(t.id));
	this->hs.NotifyTicketEvent(staff_msg);
}

bool CommandHelpServRequest::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Opens a help ticket for staff to review."));
	this->hs.Reply(source, _("If you already have an open ticket, this updates it."));
	return true;
}

CommandHelpServCancel::CommandHelpServCancel(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/cancel", 0, 0)
	, hs(parent)
{
	this->SetDesc(_("Cancel your open help ticket"));
	this->AllowUnregistered(false);
}

void CommandHelpServCancel::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!source.GetAccount())
	{
		this->hs.Reply(source, "You must be identified to cancel a ticket.");
		return;
	}

	this->hs.PruneExpiredTickets();

	auto* t = this->hs.FindOpenTicketByAccountKey(source.GetAccount()->display);
	if (!t)
	{
		this->hs.Reply(source, "You have no open ticket.");
		return;
	}

	const auto id = t->id;
	this->hs.open_ticket_by_account.erase(NormalizeTopic(t->account));
	this->hs.tickets_by_id.erase(id);
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: cancelled \002#%llu\002 by %s", static_cast<unsigned long long>(id), source.GetNick().c_str()));
	this->hs.Reply(source, "Your ticket has been cancelled.");
}

bool CommandHelpServCancel::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Cancels your currently open help ticket."));
	return true;
}

CommandHelpServList::CommandHelpServList(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/list", 0, 1)
	, hs(parent)
{
	this->SetDesc(_("List open help tickets"));
	this->SetSyntax(_("[\037filter\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServList::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	Anope::string filter = params.empty() ? "" : params[0];
	filter.trim();

	if (this->hs.tickets_by_id.empty())
	{
		this->hs.Reply(source, "No open tickets.");
		return;
	}

	this->hs.Reply(source, "Open tickets:");
	for (const auto& [id, t] : this->hs.tickets_by_id)
	{
		if (!filter.empty())
		{
			const auto match = HelpServCore::ContainsCaseInsensitive;
			if (!match(t.nick, filter) && !match(t.account, filter) && !match(t.topic, filter)
				&& !match(t.message, filter) && !match(t.requester, filter) && !match(t.assigned, filter))
				continue;
		}

		const auto age = t.created ? (Anope::CurTime - t.created) : 0;
		const auto upd = t.updated ? (Anope::CurTime - t.updated) : age;
		Anope::string line = Anope::Format("\002#%llu\002 Topic: \002%s\002 Nick: \002%s\002 Account: \002%s\002 (%s old, updated %s ago)",
			static_cast<unsigned long long>(id),
			t.topic.c_str(),
			t.nick.c_str(),
			t.account.c_str(),
			Anope::Duration(age, source.GetAccount()).c_str(),
			Anope::Duration(upd, source.GetAccount()).c_str());
		this->hs.Reply(source, line);
		if (!t.assigned.empty())
			this->hs.ReplyF(source, "  Assigned: \002%s\002", t.assigned.c_str());
		if (!t.message.empty())
			this->hs.ReplyF(source, "  Message: %s", t.message.c_str());
		if (!t.notes.empty())
		{
			size_t note_count = 0;
			for (const auto& n : t.notes)
				if (!n.empty())
					++note_count;
			if (note_count)
				this->hs.ReplyF(source, "  Notes: %zu (use \002VIEW #id\002)", note_count);
		}
	}
	this->hs.Reply(source, "End of list.");
}

bool CommandHelpServList::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Lists currently open tickets."));
	this->hs.Reply(source, _("Requires the helpserv/ticket privilege (configurable)."));
	return true;
}

CommandHelpServClose::CommandHelpServClose(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/close", 1, 2)
	, hs(parent)
{
	this->SetDesc(_("Close a help ticket"));
	this->SetSyntax(_("\037#id|nick|account\037 [\037reason\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServClose::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	Anope::string who = params[0];
	who.trim();
	Anope::string reason = params.size() > 1 ? params[1] : "Closed by staff.";
	reason.trim();

	if (who.empty())
	{
		this->hs.Reply(source, "Syntax: CLOSE <nick|account> [reason]");
		return;
	}

	HelpServCore::Ticket* t = nullptr;
	uint64_t by_id = 0;
	if (HelpServCore::TryParseTicketId(who, by_id))
		t = this->hs.FindTicketById(by_id);
	else
		t = this->hs.FindOpenTicketByNickOrAccount(who);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket found for \002%s\002.", who.c_str());
		return;
	}

	const auto id = t->id;
	const auto account = t->account;
	const auto nick = t->nick;

	this->hs.open_ticket_by_account.erase(NormalizeTopic(account));
	this->hs.tickets_by_id.erase(id);
	this->hs.SaveTicketsToFile();

	this->hs.NotifyTicketEvent(Anope::Format("TICKET: closed \002#%llu\002 by %s (account \002%s\002): %s",
		static_cast<unsigned long long>(id), source.GetNick().c_str(), account.c_str(), reason.c_str()));

	// Notify any online users identified to the account.
	bool notified = false;
	if (!account.empty())
	{
		if (NickCore* nc = NickCore::Find(account))
		{
			for (auto* u : nc->users)
			{
				if (!u)
					continue;
				u->SendMessage(*this->hs.HelpServ, Anope::Format("Your help ticket \002#%llu\002 was closed: %s",
					static_cast<unsigned long long>(id), reason.c_str()));
				notified = true;
			}
		}
	}
	if (!notified)
	{
		if (User* u = User::Find(nick, true))
			u->SendMessage(*this->hs.HelpServ, Anope::Format("Your help ticket \002#%llu\002 was closed: %s",
				static_cast<unsigned long long>(id), reason.c_str()));
	}

	this->hs.ReplyF(source, "Closed ticket \002#%llu\002.", static_cast<unsigned long long>(id));
}

bool CommandHelpServClose::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Closes an open ticket by ticket id, nick, or account name."));
	return true;
}

CommandHelpServTake::CommandHelpServTake(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/take", 1, 1)
	, hs(parent)
{
	this->SetDesc(_("Claim a help ticket"));
	this->SetSyntax(_("\037#id\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServTake::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: TAKE #id");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	t->assigned = source.GetNick();
	t->updated = Anope::CurTime;
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 claimed by \002%s\002", static_cast<unsigned long long>(id), source.GetNick().c_str()));
	this->hs.ReplyF(source, "Claimed ticket \002#%llu\002.", static_cast<unsigned long long>(id));
}

bool CommandHelpServTake::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Marks a ticket as being handled by you."));
	return true;
}

CommandHelpServAssign::CommandHelpServAssign(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/assign", 2, 2)
	, hs(parent)
{
	this->SetDesc(_("Assign a help ticket"));
	this->SetSyntax(_("\037#id\037 \037nick|none\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServAssign::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: ASSIGN #id <nick|none>");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	Anope::string assignee = params[1];
	assignee.trim();
	if (assignee.equals_ci("none") || assignee == "-" || assignee.empty())
		assignee.clear();

	t->assigned = assignee;
	t->updated = Anope::CurTime;
	this->hs.SaveTicketsToFile();

	if (assignee.empty())
		this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 unassigned by \002%s\002", static_cast<unsigned long long>(id), source.GetNick().c_str()));
	else
		this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 assigned to \002%s\002 by \002%s\002", static_cast<unsigned long long>(id), assignee.c_str(), source.GetNick().c_str()));

	this->hs.ReplyF(source, "Updated ticket \002#%llu\002 assignment.", static_cast<unsigned long long>(id));
}

bool CommandHelpServAssign::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Assigns a ticket to a staff member (or clears assignment)."));
	return true;
}

CommandHelpServNote::CommandHelpServNote(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/note", 2, 2)
	, hs(parent)
{
	this->SetDesc(_("Add a note to a help ticket"));
	this->SetSyntax(_("\037#id\037 \037text\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServNote::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: NOTE #id <text>");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	Anope::string text = params[1];
	text.trim();
	if (text.empty())
	{
		this->hs.Reply(source, "Syntax: NOTE #id <text>");
		return;
	}

	const auto ts = Anope::strftime(Anope::CurTime, source.GetAccount());
	Anope::string note = Anope::Format("[%s] %s: %s", ts.c_str(), source.GetNick().c_str(), text.c_str());
	t->notes.push_back(note);
	t->updated = Anope::CurTime;
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 noted by \002%s\002", static_cast<unsigned long long>(id), source.GetNick().c_str()));
	this->hs.ReplyF(source, "Added note to ticket \002#%llu\002.", static_cast<unsigned long long>(id));
}

bool CommandHelpServNote::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Adds an internal staff note to a ticket."));
	return true;
}

CommandHelpServView::CommandHelpServView(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/view", 1, 1)
	, hs(parent)
{
	this->SetDesc(_("View details of a help ticket"));
	this->SetSyntax(_("\037#id\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServView::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: VIEW #id");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	this->hs.ReplyF(source, "Ticket \002#%llu\002", static_cast<unsigned long long>(id));
	this->hs.ReplyF(source, "Topic: \002%s\002", t->topic.c_str());
	this->hs.ReplyF(source, "Nick: \002%s\002  Account: \002%s\002", t->nick.c_str(), t->account.c_str());
	if (!t->requester.empty())
		this->hs.ReplyF(source, "Requester: %s", t->requester.c_str());
	if (!t->assigned.empty())
		this->hs.ReplyF(source, "Assigned: \002%s\002", t->assigned.c_str());
	if (!t->message.empty())
		this->hs.ReplyF(source, "Message: %s", t->message.c_str());
	if (t->created)
		this->hs.ReplyF(source, "Created: %s", Anope::strftime(t->created, source.GetAccount()).c_str());
	if (t->updated)
		this->hs.ReplyF(source, "Updated: %s", Anope::strftime(t->updated, source.GetAccount()).c_str());

	if (!t->notes.empty())
	{
		this->hs.Reply(source, "Notes:");
		for (const auto& n : t->notes)
			if (!n.empty())
				this->hs.ReplyF(source, "- %s", n.c_str());
	}
	this->hs.Reply(source, "End of ticket.");
}

bool CommandHelpServView::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Shows details and notes for a ticket."));
	return true;
}

CommandHelpServNotify::CommandHelpServNotify(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/notify", 0, 1)
	, hs(parent)
{
	this->SetDesc(_("Set HelpServ reply method"));
	this->SetSyntax(_("[\037notice|privmsg\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServNotify::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.HelpServ)
		return;

	if (!source.HasPriv(this->hs.notify_priv))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	if (params.empty())
	{
		this->hs.ReplyF(source, "Current reply method: \002%s\002", this->hs.ReplyModeString().c_str());
		this->hs.Reply(source, "Change it with: \002NOTIFY privmsg\002 or \002NOTIFY notice\002");
		return;
	}

	Anope::string mode = params[0];
	mode.trim();
	if (!this->hs.SetReplyMode(mode))
	{
		this->hs.Reply(source, "Syntax: NOTIFY <privmsg|notice>");
		return;
	}

	this->hs.ReplyF(source, "Reply method set to \002%s\002.", this->hs.ReplyModeString().c_str());
}

bool CommandHelpServNotify::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->hs.Reply(source, "Syntax: NOTIFY [notice|privmsg]");
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Controls whether HelpServ replies via NOTICE (default) or PRIVMSG."));
	this->hs.Reply(source, _("This affects HelpServ command output to users."));
	return true;
}

MODULE_INIT(HelpServCore)
