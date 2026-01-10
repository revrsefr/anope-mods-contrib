// Anope IRC Services <https://www.anope.org/>
//
// SPDX-License-Identifier: GPL-2.0-only
//
// rpc_chanstatsplus: exposes chanstats_plus data via Anope RPC (jsonrpc/xmlrpc).

#include "module.h"
#include "modules/rpc.h"
#include "modules/sql.h"

#include <ctime>

namespace
{
	enum
	{
		ERR_NO_SUCH_STATS = RPC::ERR_CUSTOM_START,
		ERR_DB_ERROR = RPC::ERR_CUSTOM_START + 1,
	};

	static bool IsValidPeriod(const Anope::string &period)
	{
		return period.equals_ci("total") || period.equals_ci("monthly") || period.equals_ci("weekly") || period.equals_ci("daily");
	}

	static bool IsValidMetric(const Anope::string &metric)
	{
		return metric.equals_ci("letters") || metric.equals_ci("words") || metric.equals_ci("lines") || metric.equals_ci("actions") ||
			metric.equals_ci("smileys_happy") || metric.equals_ci("smileys_sad") || metric.equals_ci("smileys_other") ||
			metric.equals_ci("kicks") || metric.equals_ci("kicked") || metric.equals_ci("modes") || metric.equals_ci("topics");
	}

	static Anope::string MetricColumn(const Anope::string &metric)
	{
		// These are literal SQL identifiers. Only return values from IsValidMetric().
		if (metric.equals_ci("letters"))
			return "`letters`";
		if (metric.equals_ci("words"))
			return "`words`";
		if (metric.equals_ci("lines"))
			return "`lines`";
		if (metric.equals_ci("actions"))
			return "`actions`";
		if (metric.equals_ci("smileys_happy"))
			return "`smileys_happy`";
		if (metric.equals_ci("smileys_sad"))
			return "`smileys_sad`";
		if (metric.equals_ci("smileys_other"))
			return "`smileys_other`";
		if (metric.equals_ci("kicks"))
			return "`kicks`";
		if (metric.equals_ci("kicked"))
			return "`kicked`";
		if (metric.equals_ci("modes"))
			return "`modes`";
		if (metric.equals_ci("topics"))
			return "`topics`";
		return "`lines`";
	}

	static bool IsDateYMD(const Anope::string &ymd)
	{
		if (ymd.length() != 10)
			return false;
		for (size_t i = 0; i < 10; ++i)
		{
			const char c = ymd[i];
			if (i == 4 || i == 7)
			{
				if (c != '-')
					return false;
				continue;
			}
			if (c < '0' || c > '9')
				return false;
		}
		return true;
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
		// Week starts Monday (same as chanstats_plus).
		std::tm tmv;
		localtime_r(&ts, &tmv);
		int wday = tmv.tm_wday; // 0=Sun..6=Sat
		int days_from_monday = (wday + 6) % 7;
		time_t day_start = StartOfDay(ts);
		return day_start - static_cast<time_t>(days_from_monday) * 86400;
	}

	static Anope::string DefaultPeriodStart(const Anope::string &period)
	{
		const time_t now = Anope::CurTime;
		if (period.equals_ci("total"))
			return "1970-01-01";
		if (period.equals_ci("daily"))
			return FormatDateYMD(StartOfDay(now));
		if (period.equals_ci("weekly"))
			return FormatDateYMD(StartOfWeek(now));
		if (period.equals_ci("monthly"))
			return FormatDateYMD(StartOfMonth(now));
		return "1970-01-01";
	}

	static void ReplyRow(const SQL::Result &res, size_t row, RPC::Map &out)
	{
		out.Reply("chan", res.Get(row, "chan"));
		out.Reply("nick", res.Get(row, "nick"));
		out.Reply("period", res.Get(row, "period"));
		out.Reply("period_start", res.Get(row, "period_start"));

		out.Reply("letters", Anope::Convert<uint64_t>(res.Get(row, "letters"), 0));
		out.Reply("words", Anope::Convert<uint64_t>(res.Get(row, "words"), 0));
		out.Reply("lines", Anope::Convert<uint64_t>(res.Get(row, "lines"), 0));
		out.Reply("actions", Anope::Convert<uint64_t>(res.Get(row, "actions"), 0));
		out.Reply("smileys_happy", Anope::Convert<uint64_t>(res.Get(row, "smileys_happy"), 0));
		out.Reply("smileys_sad", Anope::Convert<uint64_t>(res.Get(row, "smileys_sad"), 0));
		out.Reply("smileys_other", Anope::Convert<uint64_t>(res.Get(row, "smileys_other"), 0));
		out.Reply("kicks", Anope::Convert<uint64_t>(res.Get(row, "kicks"), 0));
		out.Reply("kicked", Anope::Convert<uint64_t>(res.Get(row, "kicked"), 0));
		out.Reply("modes", Anope::Convert<uint64_t>(res.Get(row, "modes"), 0));
		out.Reply("topics", Anope::Convert<uint64_t>(res.Get(row, "topics"), 0));
	}
}

class MRPCChanstatsPlus final
	: public Module
{
	Anope::string engine;
	Anope::string prefix;
	size_t max_limit = 100;
	ServiceReference<SQL::Provider> sql;

	Anope::string Table() const
	{
		return prefix + "chanstatsplus";
	}

	bool EnsureSQL(RPC::Request &request)
	{
		if (sql)
			return true;

		request.Error(ERR_DB_ERROR, "No SQL provider is configured");
		return false;
	}

	bool SelectOne(RPC::Request &request, const Anope::string &chan, const Anope::string &nick, const Anope::string &period,
		const Anope::string &period_start)
	{
		if (!EnsureSQL(request))
			return true;

		SQL::Query q;
		q.query = "SELECT `chan`,`nick`,`period`,DATE_FORMAT(`period_start`,'%Y-%m-%d') AS `period_start`,"
			"`letters`,`words`,`lines`,`actions`,"
			"`smileys_happy`,`smileys_sad`,`smileys_other`,"
			"`kicks`,`kicked`,`modes`,`topics` "
			"FROM `" + Table() + "` "
			"WHERE `chan`=@chan@ AND `nick`=@nick@ AND `period`=@period@ AND `period_start`=@pstart@ "
			"LIMIT 1";
		q.SetValue("chan", chan);
		q.SetValue("nick", nick);
		q.SetValue("period", period);
		q.SetValue("pstart", period_start);

		auto res = sql->RunQuery(q);
		if (!res)
		{
			request.Error(ERR_DB_ERROR, res.GetError());
			return true;
		}

		if (res.Rows() < 1)
		{
			request.Error(ERR_NO_SUCH_STATS, "No stats found");
			return true;
		}

		ReplyRow(res, 0, request.Root());
		return true;
	}

	class GetChannelEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		GetChannelEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.getChannel", 1)
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			const Anope::string &channel = request.data[0];
			const Anope::string nick = request.data.size() > 1 ? request.data[1] : "";
			Anope::string period = request.data.size() > 2 && !request.data[2].empty() ? request.data[2] : "total";
			Anope::string period_start = request.data.size() > 3 ? request.data[3] : "";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			return parent->SelectOne(request, channel, nick, period, period_start);
		}
	};

	class GetNickEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		GetNickEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.getNick", 1)
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			const Anope::string &nick = request.data[0];
			const Anope::string channel = request.data.size() > 1 ? request.data[1] : "";
			Anope::string period = request.data.size() > 2 && !request.data[2].empty() ? request.data[2] : "total";
			Anope::string period_start = request.data.size() > 3 ? request.data[3] : "";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			return parent->SelectOne(request, channel, nick, period, period_start);
		}
	};

	class TopEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		TopEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.top")
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			const Anope::string channel = request.data.size() > 0 ? request.data[0] : "";
			Anope::string period = request.data.size() > 1 && !request.data[1].empty() ? request.data[1] : "total";
			Anope::string metric = request.data.size() > 2 && !request.data[2].empty() ? request.data[2] : "lines";
			Anope::string limitstr = request.data.size() > 3 && !request.data[3].empty() ? request.data[3] : "10";
			Anope::string period_start = request.data.size() > 4 ? request.data[4] : "";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			if (!IsValidMetric(metric))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid metric");
				return true;
			}

			auto limit = Anope::Convert<size_t>(limitstr, 10);
			if (!limit)
				limit = 10;
			if (limit > parent->max_limit)
				limit = parent->max_limit;

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			if (!parent->EnsureSQL(request))
				return true;

			const Anope::string ordercol = MetricColumn(metric);
			SQL::Query q;
			q.query = "SELECT `chan`,`nick`,`period`,DATE_FORMAT(`period_start`,'%Y-%m-%d') AS `period_start`,"
				"`letters`,`words`,`lines`,`actions`,"
				"`smileys_happy`,`smileys_sad`,`smileys_other`,"
				"`kicks`,`kicked`,`modes`,`topics` "
				"FROM `" + parent->Table() + "` "
				"WHERE `chan`=@chan@ AND `period`=@period@ AND `period_start`=@pstart@ AND `nick` != '' "
				"ORDER BY " + ordercol + " DESC, `nick` ASC "
				"LIMIT @limit@";
			q.SetValue("chan", channel);
			q.SetValue("period", period);
			q.SetValue("pstart", period_start);
			q.SetValue("limit", Anope::ToString(limit), false);

			auto res = parent->sql->RunQuery(q);
			if (!res)
			{
				request.Error(ERR_DB_ERROR, res.GetError());
				return true;
			}

			auto &root = request.Root<RPC::Array>();
			for (int i = 0; i < res.Rows(); ++i)
			{
				auto &row = root.ReplyMap();
				row.Reply("rank", static_cast<uint64_t>(i + 1));
				ReplyRow(res, i, row);
			}
			return true;
		}
	};

	class TopChannelsEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		TopChannelsEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.topChannels")
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			Anope::string period = request.data.size() > 0 && !request.data[0].empty() ? request.data[0] : "total";
			Anope::string metric = request.data.size() > 1 && !request.data[1].empty() ? request.data[1] : "lines";
			Anope::string limitstr = request.data.size() > 2 && !request.data[2].empty() ? request.data[2] : "10";
			Anope::string period_start = request.data.size() > 3 ? request.data[3] : "";

			if (!IsValidPeriod(period))	
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			if (!IsValidMetric(metric))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid metric");
				return true;
			}

			auto limit = Anope::Convert<size_t>(limitstr, 10);
			if (!limit)
				limit = 10;
			if (limit > parent->max_limit)
				limit = parent->max_limit;

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			if (!parent->EnsureSQL(request))
				return true;

			const Anope::string ordercol = MetricColumn(metric);
			SQL::Query q;
			q.query = "SELECT `chan`,`nick`,`period`,DATE_FORMAT(`period_start`,'%Y-%m-%d') AS `period_start`,"
				"`letters`,`words`,`lines`,`actions`,"
				"`smileys_happy`,`smileys_sad`,`smileys_other`,"
				"`kicks`,`kicked`,`modes`,`topics` "
				"FROM `" + parent->Table() + "` "
				"WHERE `nick`='' AND `chan` != '' AND `period`=@period@ AND `period_start`=@pstart@ "
				"ORDER BY " + ordercol + " DESC, `chan` ASC "
				"LIMIT @limit@";
			q.SetValue("period", period);
			q.SetValue("pstart", period_start);
			q.SetValue("limit", Anope::ToString(limit), false);

			auto res = parent->sql->RunQuery(q);
			if (!res)
			{
				request.Error(ERR_DB_ERROR, res.GetError());
				return true;
			}

			auto &root = request.Root<RPC::Array>();
			for (int i = 0; i < res.Rows(); ++i)
			{
				auto &row = root.ReplyMap();
				row.Reply("rank", static_cast<uint64_t>(i + 1));
				ReplyRow(res, i, row);
			}
			return true;
		}
	};

	class TopNicksGlobalEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		TopNicksGlobalEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.topNicksGlobal")
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			Anope::string period = request.data.size() > 0 && !request.data[0].empty() ? request.data[0] : "total";
			Anope::string metric = request.data.size() > 1 && !request.data[1].empty() ? request.data[1] : "lines";
			Anope::string limitstr = request.data.size() > 2 && !request.data[2].empty() ? request.data[2] : "10";
			Anope::string period_start = request.data.size() > 3 ? request.data[3] : "";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			if (!IsValidMetric(metric))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid metric");
				return true;
			}

			auto limit = Anope::Convert<size_t>(limitstr, 10);
			if (!limit)
				limit = 10;
			if (limit > parent->max_limit)
				limit = parent->max_limit;

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			if (!parent->EnsureSQL(request))
				return true;

			const Anope::string ordercol = MetricColumn(metric);
			SQL::Query q;
			q.query = "SELECT `chan`,`nick`,`period`,DATE_FORMAT(`period_start`,'%Y-%m-%d') AS `period_start`,"
				"`letters`,`words`,`lines`,`actions`,"
				"`smileys_happy`,`smileys_sad`,`smileys_other`,"
				"`kicks`,`kicked`,`modes`,`topics` "
				"FROM `" + parent->Table() + "` "
				"WHERE `chan`='' AND `nick` != '' AND `period`=@period@ AND `period_start`=@pstart@ "
				"ORDER BY " + ordercol + " DESC, `nick` ASC "
				"LIMIT @limit@";
			q.SetValue("period", period);
			q.SetValue("pstart", period_start);
			q.SetValue("limit", Anope::ToString(limit), false);

			auto res = parent->sql->RunQuery(q);
			if (!res)
			{
				request.Error(ERR_DB_ERROR, res.GetError());
				return true;
			}

			auto &root = request.Root<RPC::Array>();
			for (int i = 0; i < res.Rows(); ++i)
			{
				auto &row = root.ReplyMap();
				row.Reply("rank", static_cast<uint64_t>(i + 1));
				ReplyRow(res, i, row);
			}
			return true;
		}
	};

	class ListNicksInChannelEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		ListNicksInChannelEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.listNicksInChannel", 1)
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			const Anope::string &channel = request.data[0];
			Anope::string period = request.data.size() > 1 && !request.data[1].empty() ? request.data[1] : "total";
			Anope::string period_start = request.data.size() > 2 ? request.data[2] : "";
			Anope::string limitstr = request.data.size() > 3 && !request.data[3].empty() ? request.data[3] : "100";
			Anope::string offsetstr = request.data.size() > 4 && !request.data[4].empty() ? request.data[4] : "0";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			auto limit = Anope::Convert<size_t>(limitstr, 100);
			if (!limit)
				limit = 100;
			if (limit > parent->max_limit)
				limit = parent->max_limit;

			auto offset = Anope::Convert<size_t>(offsetstr, 0);

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			if (!parent->EnsureSQL(request))
				return true;

			SQL::Query q;
			q.query = "SELECT `nick` "
				"FROM `" + parent->Table() + "` "
				"WHERE `chan`=@chan@ AND `nick` != '' AND `period`=@period@ AND `period_start`=@pstart@ "
				"GROUP BY `nick` "
				"ORDER BY `nick` ASC "
				"LIMIT @limit@ OFFSET @offset@";
			q.SetValue("chan", channel);
			q.SetValue("period", period);
			q.SetValue("pstart", period_start);
			q.SetValue("limit", Anope::ToString(limit), false);
			q.SetValue("offset", Anope::ToString(offset), false);

			auto res = parent->sql->RunQuery(q);
			if (!res)
			{
				request.Error(ERR_DB_ERROR, res.GetError());
				return true;
			}

			auto &root = request.Root<RPC::Array>();
			for (int i = 0; i < res.Rows(); ++i)
				root.Reply(res.Get(i, "nick"));
			return true;
		}
	};

	class ListChannelsForNickEvent final
		: public RPC::Event
	{
		MRPCChanstatsPlus *parent;

	public:
		ListChannelsForNickEvent(MRPCChanstatsPlus *p)
			: RPC::Event(p, "anope.chanstatsplus.listChannelsForNick", 1)
			, parent(p)
		{
		}

		bool Run(RPC::ServiceInterface *iface, HTTP::Client *client, RPC::Request &request) override
		{
			const Anope::string &nick = request.data[0];
			Anope::string period = request.data.size() > 1 && !request.data[1].empty() ? request.data[1] : "total";
			Anope::string period_start = request.data.size() > 2 ? request.data[2] : "";
			Anope::string limitstr = request.data.size() > 3 && !request.data[3].empty() ? request.data[3] : "100";
			Anope::string offsetstr = request.data.size() > 4 && !request.data[4].empty() ? request.data[4] : "0";

			if (!IsValidPeriod(period))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period (expected total/monthly/weekly/daily)");
				return true;
			}

			auto limit = Anope::Convert<size_t>(limitstr, 100);
			if (!limit)
				limit = 100;
			if (limit > parent->max_limit)
				limit = parent->max_limit;

			auto offset = Anope::Convert<size_t>(offsetstr, 0);

			if (period_start.empty())
				period_start = DefaultPeriodStart(period);
			else if (!IsDateYMD(period_start))
			{
				request.Error(RPC::ERR_INVALID_PARAMS, "Invalid period_start (expected YYYY-MM-DD)");
				return true;
			}

			if (!parent->EnsureSQL(request))
				return true;

			SQL::Query q;
			q.query = "SELECT `chan` "
				"FROM `" + parent->Table() + "` "
				"WHERE `nick`=@nick@ AND `chan` != '' AND `period`=@period@ AND `period_start`=@pstart@ "
				"GROUP BY `chan` "
				"ORDER BY `chan` ASC "
				"LIMIT @limit@ OFFSET @offset@";
			q.SetValue("nick", nick);
			q.SetValue("period", period);
			q.SetValue("pstart", period_start);
			q.SetValue("limit", Anope::ToString(limit), false);
			q.SetValue("offset", Anope::ToString(offset), false);

			auto res = parent->sql->RunQuery(q);
			if (!res)
			{
				request.Error(ERR_DB_ERROR, res.GetError());
				return true;
			}

			auto &root = request.Root<RPC::Array>();
			for (int i = 0; i < res.Rows(); ++i)
				root.Reply(res.Get(i, "chan"));
			return true;
		}
	};

	GetChannelEvent event_get_channel;
	GetNickEvent event_get_nick;
	TopEvent event_top;
	TopChannelsEvent event_top_channels;
	TopNicksGlobalEvent event_top_nicks_global;
	ListNicksInChannelEvent event_list_nicks_in_channel;
	ListChannelsForNickEvent event_list_channels_for_nick;

public:
	MRPCChanstatsPlus(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, EXTRA | VENDOR)
		, sql("", "")
		, event_get_channel(this)
		, event_get_nick(this)
		, event_top(this)
		, event_top_channels(this)
		, event_top_nicks_global(this)
		, event_list_nicks_in_channel(this)
		, event_list_channels_for_nick(this)
	{
	}

	void OnReload(Configuration::Conf &conf) override
	{
		const auto &block = conf.GetModule(this);
		engine = block.Get<const Anope::string>("engine");
		prefix = block.Get<const Anope::string>("prefix", "anope_");
		max_limit = block.Get<size_t>("maxlimit", "100");
		if (!max_limit)
			max_limit = 100;

		sql = ServiceReference<SQL::Provider>("SQL::Provider", engine);
		if (!sql)
			Log(this) << "rpc_chanstatsplus: no database connection to " << engine;
	}
};

MODULE_INIT(MRPCChanstatsPlus)
