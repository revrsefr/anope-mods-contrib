/*
 * (C) 2026 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Channel: #development
 *
 * HelpServ module created by reverse for Anope 2.1
 * - Help topics: HELP <topic>, SEARCH, STATS
 * - Escalation: HELPME (pages staff immediately)
 * - Tickets: REQUEST/CANCEL (users), LIST/VIEW (staff)
 * - Workflow: TAKE/ASSIGN/NOTE/CLOSE, NEXT, PRIORITY, WAIT/UNWAIT
 * - Reply transport: configurable NOTICE/PRIVMSG (reply_method + NOTIFY)
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
 *   # Expire old tickets (0 disables). Supports: 60, 10m, 2h, 7d, 1w
 *   ticket_expire = "0"
 *
 *   # Privilege required for ticket staff commands
 *   ticket_priv = "helpserv/ticket"
 *
 *   # Privilege required to use NOTIFY (change reply_method at runtime)
 *   notify_priv = "helpserv/admin"
 *
 *   # How HelpServ replies to users: "notice" or "privmsg"
 *   reply_method = "notice"
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
 * command { service = "HelpServ"; name = "VIEW"; command = "helpserv/view"; hide = true; }
 * command { service = "HelpServ"; name = "TAKE"; command = "helpserv/take"; hide = true; }
 * command { service = "HelpServ"; name = "ASSIGN"; command = "helpserv/assign"; hide = true; }
 * command { service = "HelpServ"; name = "NOTE"; command = "helpserv/note"; hide = true; }
 * command { service = "HelpServ"; name = "CLOSE"; command = "helpserv/close"; hide = true; }
 * command { service = "HelpServ"; name = "NEXT"; command = "helpserv/next"; hide = true; }
 * command { service = "HelpServ"; name = "PRIORITY"; command = "helpserv/priority"; hide = true; }
 * command { service = "HelpServ"; name = "WAIT"; command = "helpserv/wait"; hide = true; }
 * command { service = "HelpServ"; name = "UNWAIT"; command = "helpserv/unwait"; hide = true; }
 * command { service = "HelpServ"; name = "NOTIFY"; command = "helpserv/notify"; hide = true; }
 */

#include "helpserv.h"

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

	++this->hs.helpme_requests;
	this->hs.MaybeSaveStats();

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

	++this->hs.request_requests;
	this->hs.MaybeSaveStats();

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

		const auto account_key = HelpServCore::NormalizeTopic(source.GetAccount()->display);
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
		this->hs.open_ticket_by_account[HelpServCore::NormalizeTopic(t.account)] = t.id;
	this->hs.SaveTicketsToFile();

	Anope::string oper = source.GetNick();
	if (source.GetUser())
		oper = source.GetUser()->GetDisplayedMask();
	this->hs.ReplyF(source, "Your ticket has been opened (#%llu) about %s.",
		static_cast<unsigned long long>(t.id), topic.c_str());
	this->hs.ReplyF(source, "Opened by: %s", oper.c_str());

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

	++this->hs.cancel_requests;
	this->hs.MaybeSaveStats();

	this->hs.PruneExpiredTickets();

	auto* t = this->hs.FindOpenTicketByAccountKey(source.GetAccount()->display);
	if (!t)
	{
		this->hs.Reply(source, "You have no open ticket.");
		return;
	}

	const auto id = t->id;
		this->hs.open_ticket_by_account.erase(HelpServCore::NormalizeTopic(t->account));
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

	++this->hs.list_requests;
	this->hs.MaybeSaveStats();

	this->hs.PruneExpiredTickets();

	Anope::string filter = params.empty() ? "" : params[0];
	filter.trim();

	if (this->hs.tickets_by_id.empty())
	{
		this->hs.Reply(source, "No open tickets.");
		return;
	}

	const bool include_waiting = true;
	auto tickets = this->hs.GetSortedTickets(filter, include_waiting);
	if (tickets.empty())
	{
		this->hs.Reply(source, "No matching tickets.");
		return;
	}

	this->hs.Reply(source, "Open tickets (sorted by state/priority/age):");
	for (const auto* t : tickets)
	{
		const auto age = t->created ? (Anope::CurTime - t->created) : 0;
		const auto upd = t->updated ? (Anope::CurTime - t->updated) : age;
		const auto state = HelpServCore::NormalizeState(t->state);
		const auto pri = HelpServCore::PriorityString(t->priority);
		Anope::string line = Anope::Format("\002#%llu\002 [\002%s\002/%s] Topic: \002%s\002 Nick: \002%s\002 Account: \002%s\002 (%s old, updated %s ago)",
			static_cast<unsigned long long>(t->id),
			pri.c_str(),
			state.c_str(),
			t->topic.c_str(),
			t->nick.c_str(),
			t->account.c_str(),
			Anope::Duration(age, source.GetAccount()).c_str(),
			Anope::Duration(upd, source.GetAccount()).c_str());
		this->hs.Reply(source, line);
		if (!t->assigned.empty())
			this->hs.ReplyF(source, "  Assigned: \002%s\002", t->assigned.c_str());
		if (!t->wait_reason.empty() && state.equals_ci("waiting"))
			this->hs.ReplyF(source, "  Waiting: %s", t->wait_reason.c_str());
		if (!t->message.empty())
			this->hs.ReplyF(source, "  Message: %s", t->message.c_str());
		if (!t->notes.empty())
		{
			size_t note_count = 0;
			for (const auto& n : t->notes)
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

	++this->hs.close_requests;
	this->hs.MaybeSaveStats();

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

		this->hs.open_ticket_by_account.erase(HelpServCore::NormalizeTopic(account));
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
				this->hs.SendToUser(u, Anope::Format("Your help ticket \002#%llu\002 was closed: %s",
					static_cast<unsigned long long>(id), reason.c_str()));
				notified = true;
			}
		}
	}
	if (!notified)
	{
		if (User* u = User::Find(nick, true))
			this->hs.SendToUser(u, Anope::Format("Your help ticket \002#%llu\002 was closed: %s",
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

	++this->hs.take_requests;
	this->hs.MaybeSaveStats();

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

	// Notify any online users identified to the account.
	bool notified = false;
	if (!t->account.empty())
	{
		if (NickCore* nc = NickCore::Find(t->account))
		{
			for (auto* u : nc->users)
			{
				if (!u)
					continue;
				this->hs.SendToUser(u, Anope::Format("Your help ticket \002#%llu\002 has been claimed by \002%s\002.",
					static_cast<unsigned long long>(id), source.GetNick().c_str()));
				notified = true;
			}
		}
	}
	if (!notified)
	{
		if (User* u = User::Find(t->nick, true))
			this->hs.SendToUser(u, Anope::Format("Your help ticket \002#%llu\002 has been claimed by \002%s\002.",
				static_cast<unsigned long long>(id), source.GetNick().c_str()));
	}

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

	++this->hs.assign_requests;
	this->hs.MaybeSaveStats();

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

	++this->hs.note_requests;
	this->hs.MaybeSaveStats();

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

	++this->hs.view_requests;
	this->hs.MaybeSaveStats();

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
	this->hs.ReplyF(source, "State: \002%s\002  Priority: \002%s\002",
		HelpServCore::NormalizeState(t->state).c_str(),
		HelpServCore::PriorityString(t->priority).c_str());
	this->hs.ReplyF(source, "Topic: \002%s\002", t->topic.c_str());
	this->hs.ReplyF(source, "Nick: \002%s\002  Account: \002%s\002", t->nick.c_str(), t->account.c_str());
	if (!t->requester.empty())
		this->hs.ReplyF(source, "Requester: %s", t->requester.c_str());
	if (!t->assigned.empty())
		this->hs.ReplyF(source, "Assigned: \002%s\002", t->assigned.c_str());
	if (!t->wait_reason.empty() && HelpServCore::NormalizeState(t->state).equals_ci("waiting"))
		this->hs.ReplyF(source, "Waiting: %s", t->wait_reason.c_str());
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

CommandHelpServNext::CommandHelpServNext(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/next", 0, 2)
	, hs(parent)
{
	this->SetDesc(_("Show the next ticket in the queue"));
	this->SetSyntax(_("[ALL] [\037filter\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServNext::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	++this->hs.next_requests;
	this->hs.MaybeSaveStats();

	this->hs.PruneExpiredTickets();

	bool include_waiting = false;
	Anope::string filter;
	if (!params.empty())
	{
		if (params[0].equals_ci("all"))
			include_waiting = true;
		else
			filter = params[0];
	}
	if (params.size() >= 2)
		filter = params[1];
	filter.trim();

	auto tickets = this->hs.GetSortedTickets(filter, include_waiting);
	if (tickets.empty())
	{
		this->hs.Reply(source, "No matching tickets.");
		return;
	}

	const auto* t = tickets.front();
	this->hs.ReplyF(source, "Next ticket: \002#%llu\002 [\002%s\002/%s] Topic: \002%s\002 (use \002VIEW #%llu\002)",
		static_cast<unsigned long long>(t->id),
		HelpServCore::PriorityString(t->priority).c_str(),
		HelpServCore::NormalizeState(t->state).c_str(),
		t->topic.c_str(),
		static_cast<unsigned long long>(t->id));
}

bool CommandHelpServNext::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Shows the next ticket in the queue (sorted by state, priority, age)."));
	this->hs.Reply(source, _("Use NEXT ALL to include waiting tickets."));
	return true;
}

CommandHelpServPriority::CommandHelpServPriority(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/priority", 0, 2)
	, hs(parent)
{
	this->SetDesc(_("Set ticket priority"));
	this->SetSyntax(_("\037#id\037 \037low|normal|high\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServPriority::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	++this->hs.priority_requests;
	this->hs.MaybeSaveStats();

	if (params.size() < 2)
	{
		this->hs.Reply(source, "Syntax: PRIORITY #id <low|normal|high>");
		this->hs.Reply(source, "Type /msg HelpServ HELP PRIORITY for more information.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: PRIORITY #id <low|normal|high>");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	int pri = 1;
	if (!HelpServCore::ParsePriority(params[1], pri))
	{
		this->hs.Reply(source, "Syntax: PRIORITY #id <low|normal|high>");
		return;
	}

	t->priority = HelpServCore::ClampPriority(pri);
	t->updated = Anope::CurTime;
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 priority set to %s by %s",
		static_cast<unsigned long long>(id),
		HelpServCore::PriorityString(t->priority).c_str(),
		source.GetNick().c_str()));
	this->hs.ReplyF(source, "Ticket \002#%llu\002 priority set to \002%s\002.",
		static_cast<unsigned long long>(id),
		HelpServCore::PriorityString(t->priority).c_str());
}

bool CommandHelpServPriority::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Sets ticket priority which affects queue ordering."));
	return true;
}

CommandHelpServWait::CommandHelpServWait(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/wait", 0, 2)
	, hs(parent)
{
	this->SetDesc(_("Mark a ticket as waiting for user"));
	this->SetSyntax(_("\037#id\037 [\037reason\037]"));
	this->AllowUnregistered(true);
}

void CommandHelpServWait::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	++this->hs.wait_requests;
	this->hs.MaybeSaveStats();

	if (params.empty())
	{
		this->hs.Reply(source, "Syntax: WAIT #id [reason]");
		this->hs.Reply(source, "Type /msg HelpServ HELP WAIT for more information.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: WAIT #id [reason]");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	Anope::string reason = params.size() > 1 ? params[1] : "";
	reason.trim();
	t->state = "waiting";
	t->wait_reason = reason;
	t->updated = Anope::CurTime;
	if (!reason.empty())
	{
		const auto ts = Anope::strftime(Anope::CurTime, source.GetAccount());
		Anope::string note = Anope::Format("[%s] %s: waiting (%s)", ts.c_str(), source.GetNick().c_str(), reason.c_str());
		t->notes.push_back(note);
	}
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 marked waiting by %s",
		static_cast<unsigned long long>(id), source.GetNick().c_str()));
	this->hs.ReplyF(source, "Ticket \002#%llu\002 marked as \002waiting\002.", static_cast<unsigned long long>(id));
}

bool CommandHelpServWait::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Marks a ticket as waiting (moves it down the queue until un-waited)."));
	return true;
}

CommandHelpServUnwait::CommandHelpServUnwait(Module* creator, HelpServCore& parent)
	: Command(creator, "helpserv/unwait", 0, 1)
	, hs(parent)
{
	this->SetDesc(_("Mark a ticket as open (not waiting)"));
	this->SetSyntax(_("\037#id\037"));
	this->AllowUnregistered(true);
}

void CommandHelpServUnwait::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->hs.IsStaff(source))
	{
		this->hs.Reply(source, "Access denied.");
		return;
	}

	++this->hs.unwait_requests;
	this->hs.MaybeSaveStats();

	if (params.empty())
	{
		this->hs.Reply(source, "Syntax: UNWAIT #id");
		this->hs.Reply(source, "Type /msg HelpServ HELP UNWAIT for more information.");
		return;
	}

	this->hs.PruneExpiredTickets();

	uint64_t id = 0;
	if (!HelpServCore::TryParseTicketId(params[0], id))
	{
		this->hs.Reply(source, "Syntax: UNWAIT #id");
		return;
	}

	auto* t = this->hs.FindTicketById(id);
	if (!t)
	{
		this->hs.ReplyF(source, "No open ticket with id \002#%llu\002.", static_cast<unsigned long long>(id));
		return;
	}

	t->state = "open";
	t->wait_reason.clear();
	t->updated = Anope::CurTime;
	this->hs.SaveTicketsToFile();
	this->hs.NotifyTicketEvent(Anope::Format("TICKET: \002#%llu\002 un-waited by %s",
		static_cast<unsigned long long>(id), source.GetNick().c_str()));
	this->hs.ReplyF(source, "Ticket \002#%llu\002 marked as \002open\002.", static_cast<unsigned long long>(id));
}

bool CommandHelpServUnwait::OnHelp(CommandSource& source, const Anope::string& subcommand)
{
	this->SendSyntax(source);
	this->hs.Reply(source, " ");
	this->hs.Reply(source, _("Marks a waiting ticket as open again (returns it to the main queue)."));
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

	++this->hs.notify_requests;
	this->hs.MaybeSaveStats();

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
