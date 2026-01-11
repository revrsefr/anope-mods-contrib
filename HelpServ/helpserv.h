/*
 * HelpServ (Anope 2.1) module header.
 *
 * This file exists to keep helpserv.cpp smaller:
 * - Declares the HelpServ command classes
 * - Contains the HelpServCore module class (state/config/helpers)
 *
 * Command implementations and MODULE_INIT live in helpserv.cpp.
 * Configuration/examples are documented in helpserv.cpp and helpserv.example.conf.
 */

#pragma once


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

class CommandHelpServNext final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServNext(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServPriority final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServPriority(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServWait final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServWait(Module* creator, HelpServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandHelpServUnwait final : public Command
{
	HelpServCore& hs;

public:
	CommandHelpServUnwait(Module* creator, HelpServCore& parent);
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
		int priority = 1; // 0=low, 1=normal, 2=high
		Anope::string state = "open"; // open|waiting
		Anope::string wait_reason;
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
	CommandHelpServNext command_next;
	CommandHelpServPriority command_priority;
	CommandHelpServWait command_wait;
	CommandHelpServUnwait command_unwait;
	CommandHelpServNotify command_notify;

	static int ClampPriority(int p)
	{
		if (p < 0)
			return 0;
		if (p > 2)
			return 2;
		return p;
	}

	static Anope::string PriorityString(int p)
	{
		switch (ClampPriority(p))
		{
			case 0: return "low";
			case 2: return "high";
			default: return "normal";
		}
	}

	static bool ParsePriority(const Anope::string& in, int& out)
	{
		Anope::string s = in;
		s.trim();
		if (s.empty())
			return false;
		if (s.equals_ci("low"))
		{
			out = 0;
			return true;
		}
		if (s.equals_ci("normal") || s.equals_ci("med") || s.equals_ci("medium"))
		{
			out = 1;
			return true;
		}
		if (s.equals_ci("high"))
		{
			out = 2;
			return true;
		}

		bool all_digits = true;
		for (size_t i = 0; i < s.length(); ++i)
			if (s[i] < '0' || s[i] > '9')
				all_digits = false;
		if (!all_digits)
			return false;
		uint64_t n = 0;
		if (!ParseU64(s, n))
			return false;
		out = ClampPriority(static_cast<int>(n));
		return true;
	}

	static int StateSortKey(const Anope::string& state)
	{
		if (state.equals_ci("waiting") || state.equals_ci("wait"))
			return 1;
		return 0;
	}

	static Anope::string NormalizeState(const Anope::string& state)
	{
		if (state.equals_ci("waiting") || state.equals_ci("wait"))
			return "waiting";
		return "open";
	}

	static bool TicketMatchesFilter(const Ticket& t, const Anope::string& filter)
	{
		if (filter.empty())
			return true;
		const auto match = ContainsCaseInsensitive;
		if (match(t.nick, filter) || match(t.account, filter) || match(t.topic, filter)
			|| match(t.message, filter) || match(t.requester, filter) || match(t.assigned, filter)
			|| match(t.state, filter) || match(t.wait_reason, filter))
			return true;
		return false;
	}

	std::vector<Ticket*> GetSortedTickets(const Anope::string& filter, bool include_waiting)
	{
		std::vector<Ticket*> out;
		out.reserve(this->tickets_by_id.size());
		for (auto& [_, t] : this->tickets_by_id)
		{
			if (!TicketMatchesFilter(t, filter))
				continue;
			if (!include_waiting && NormalizeState(t.state).equals_ci("waiting"))
				continue;
			out.push_back(&t);
		}
		std::sort(out.begin(), out.end(), [](const Ticket* a, const Ticket* b) {
			const int as = StateSortKey(a->state);
			const int bs = StateSortKey(b->state);
			if (as != bs)
				return as < bs;
			const int ap = ClampPriority(a->priority);
			const int bp = ClampPriority(b->priority);
			if (ap != bp)
				return ap > bp;
			const auto at = (a->updated ? a->updated : a->created);
			const auto bt = (b->updated ? b->updated : b->created);
			if (at != bt)
				return at < bt;
			return a->id < b->id;
		});
		return out;
	}

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

	void SendToUser(User* u, const Anope::string& msg)
	{
		if (!this->HelpServ.operator bool() || !u)
			return;

		Anope::map<Anope::string> tags;
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

		this->PruneExpiredTickets();
		const auto now = Anope::CurTime;
		if (!this->tickets_by_id.empty())
		{
			size_t open = 0;
			size_t waiting = 0;
			size_t assigned = 0;
			size_t pri_low = 0;
			size_t pri_normal = 0;
			size_t pri_high = 0;

			uint64_t oldest_open_id = 0;
			time_t oldest_open_created = 0;
			time_t oldest_open_updated = 0;
			uint64_t oldest_wait_id = 0;
			time_t oldest_wait_created = 0;
			time_t oldest_wait_updated = 0;

			std::map<Anope::string, uint64_t> ticket_topics;
			auto consider_oldest = [&](bool is_waiting, const Ticket& t) {
				const auto created = t.created;
				if (!created)
					return;
				uint64_t& oldest_id = is_waiting ? oldest_wait_id : oldest_open_id;
				time_t& oldest_created = is_waiting ? oldest_wait_created : oldest_open_created;
				time_t& oldest_updated = is_waiting ? oldest_wait_updated : oldest_open_updated;
				if (!oldest_created || created < oldest_created)
				{
					oldest_id = t.id;
					oldest_created = created;
					oldest_updated = t.updated;
				}
			};

			for (const auto& [_, t] : this->tickets_by_id)
			{
				const auto state = NormalizeState(t.state);
				const bool is_waiting = state.equals_ci("waiting");
				if (is_waiting)
					++waiting;
				else
					++open;

				if (!t.assigned.empty())
					++assigned;

				switch (ClampPriority(t.priority))
				{
					case 0: ++pri_low; break;
					case 2: ++pri_high; break;
					default: ++pri_normal; break;
				}

				if (!t.topic.empty())
					++ticket_topics[t.topic];

				consider_oldest(is_waiting, t);
			}

			this->ReplyF(source, "Tickets: %zu (open: %zu, waiting: %zu)", this->tickets_by_id.size(), open, waiting);
			this->ReplyF(source, "Tickets by priority: high: %zu, normal: %zu, low: %zu", pri_high, pri_normal, pri_low);
			this->ReplyF(source, "Tickets assigned: %zu (unassigned: %zu)", assigned, this->tickets_by_id.size() - assigned);

			auto age = [&](time_t when) {
				if (!when || now <= when)
					return Anope::string("0s");
				return Anope::Duration(now - when, source.GetAccount());
			};
			if (oldest_open_id)
				this->ReplyF(source, "Oldest open ticket: \002#%llu\002 (created %s ago%s)",
					static_cast<unsigned long long>(oldest_open_id),
					age(oldest_open_created).c_str(),
					oldest_open_updated ? Anope::Format(", updated %s ago", age(oldest_open_updated).c_str()).c_str() : "");
			if (oldest_wait_id)
				this->ReplyF(source, "Oldest waiting ticket: \002#%llu\002 (created %s ago%s)",
					static_cast<unsigned long long>(oldest_wait_id),
					age(oldest_wait_created).c_str(),
					oldest_wait_updated ? Anope::Format(", updated %s ago", age(oldest_wait_updated).c_str()).c_str() : "");

			if (!ticket_topics.empty())
			{
				std::vector<std::pair<Anope::string, uint64_t>> top;
				top.reserve(ticket_topics.size());
				for (const auto& kv : ticket_topics)
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
					this->ReplyF(source, "Ticket topics: %s", buf.c_str());
			}
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
			else if (field.equals_ci("priority"))
			{
				uint64_t n2 = 0;
				if (ParseU64(val, n2))
					t.priority = ClampPriority(static_cast<int>(n2));
			}
			else if (field.equals_ci("state"))
				t.state = NormalizeState(val);
			else if (field.equals_ci("wait_reason"))
				t.wait_reason = val;
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
		out << "version=2\n";
		out << "next_ticket_id=" << this->next_ticket_id << "\n";
		for (const auto& [id, t] : this->tickets_by_id)
		{
			out << "ticket." << id << ".account=" << EscapeValue(t.account) << "\n";
			out << "ticket." << id << ".nick=" << EscapeValue(t.nick) << "\n";
			out << "ticket." << id << ".requester=" << EscapeValue(t.requester) << "\n";
			out << "ticket." << id << ".topic=" << EscapeValue(t.topic) << "\n";
			out << "ticket." << id << ".message=" << EscapeValue(t.message) << "\n";
			out << "ticket." << id << ".priority=" << static_cast<uint64_t>(ClampPriority(t.priority)) << "\n";
			out << "ticket." << id << ".state=" << EscapeValue(NormalizeState(t.state)) << "\n";
			out << "ticket." << id << ".wait_reason=" << EscapeValue(t.wait_reason) << "\n";
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
		, command_next(this, *this)
		, command_priority(this, *this)
		, command_wait(this, *this)
		, command_unwait(this, *this)
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
	friend class CommandHelpServNext;
	friend class CommandHelpServPriority;
	friend class CommandHelpServWait;
	friend class CommandHelpServUnwait;
	friend class CommandHelpServNotify;
};

