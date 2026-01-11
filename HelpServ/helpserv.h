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

#include <cstdint>
#include <ctime>
#include <map>
#include <vector>

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
	uint64_t helpme_requests = 0;
	uint64_t request_requests = 0;
	uint64_t cancel_requests = 0;
	uint64_t list_requests = 0;
	uint64_t next_requests = 0;
	uint64_t view_requests = 0;
	uint64_t take_requests = 0;
	uint64_t assign_requests = 0;
	uint64_t note_requests = 0;
	uint64_t close_requests = 0;
	uint64_t priority_requests = 0;
	uint64_t wait_requests = 0;
	uint64_t unwait_requests = 0;
	uint64_t notify_requests = 0;
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

	static Anope::string NormalizeTopic(const Anope::string& in);
	static Anope::string EscapeValue(const Anope::string& in);
	static Anope::string UnescapeValue(const Anope::string& in);

	static int ClampPriority(int p);
	static Anope::string PriorityString(int p);
	static bool ParsePriority(const Anope::string& in, int& out);
	static int StateSortKey(const Anope::string& state);
	static Anope::string NormalizeState(const Anope::string& state);
	static bool TicketMatchesFilter(const Ticket& t, const Anope::string& filter);

	std::vector<Ticket*> GetSortedTickets(const Anope::string& filter, bool include_waiting);

	void Reply(CommandSource& source, const Anope::string& msg);
	void SendToUser(User* u, const Anope::string& msg);
	void Reply(CommandSource& source, const char* text);
	void ReplyF(CommandSource& source, const char* fmt, ...) ATTR_FORMAT(3, 4);
	Anope::string ReplyModeString() const;
	bool SetReplyMode(const Anope::string& mode);

	void SendIndex(CommandSource& source);
	bool SendTopic(CommandSource& source, const Anope::string& topic_key);
	static bool ContainsCaseInsensitive(const Anope::string& haystack, const Anope::string& needle);
	void SendStats(CommandSource& source);

	static Anope::string GetStatsPath();
	static Anope::string GetTicketsPath();
	static bool ParseU64(const Anope::string& in, uint64_t& out);
	static bool TryParseTicketId(const Anope::string& in, uint64_t& out);
	static time_t ParseDurationSeconds(const Anope::string& in, time_t fallback);

	Ticket* FindTicketById(uint64_t id);
	void PruneExpiredTickets();
	void LoadStatsFromFile();
	void SaveStatsToFile();
	void MaybeSaveStats();
	bool IsStaff(CommandSource& source) const;
	Anope::string GetRequesterKey(CommandSource& source) const;
	void PageStaff(const Anope::string& msg);

	void LoadTicketsFromFile();
	void SaveTicketsToFile();
	Ticket* FindOpenTicketByAccountKey(const Anope::string& account_key);
	Ticket* FindOpenTicketByNickOrAccount(const Anope::string& who);
	void NotifyTicketEvent(const Anope::string& msg);

	void Search(CommandSource& source, const Anope::string& query);
	void LoadTopics(const Configuration::Block& mod);

public:
	HelpServCore(const Anope::string& modname, const Anope::string& creator);
	~HelpServCore() override;

	void OnReload(Configuration::Conf& conf) override;
	EventReturn OnBotPrivmsg(User* u, BotInfo* bi, Anope::string& message, const Anope::map<Anope::string>& tags) override;
	EventReturn OnPreHelp(CommandSource& source, const std::vector<Anope::string>& params) override;

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

