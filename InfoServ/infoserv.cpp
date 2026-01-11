/*
 * InfoServ module created for Anope 2.1.
 *
 * Summary:
 * - Stores informational logon messages (public + oper-only)
 * - Shows public messages on connect (optional)
 * - Shows oper-only messages when a user gains OPER (optional)
 * - Atheme-inspired POST/LIST/MOVE/DEL workflow
 * - Flatfile persistence in data/infoserv.db
 *
 * Example configuration is shipped as infoserv.example.conf.
 */

#include "infoserv.h"

#include "language.h"

#include <algorithm>

namespace
{
	void ReplySyntaxAndMoreInfo(InfoServCore& is, CommandSource& source, const Anope::string& syntax)
	{
		if (syntax.empty())
			is.Reply(source, Anope::Format("Syntax: %s", source.command.c_str()));
		else
			is.Reply(source, Anope::Format("Syntax: %s %s", source.command.c_str(), syntax.c_str()));

		if (!source.service)
			return;

		auto it = std::find_if(source.service->commands.begin(), source.service->commands.end(), [](const auto& cmd)
		{
			return cmd.second.name == "generic/help";
		});
		if (it != source.service->commands.end())
			is.Reply(source, Anope::Format(MORE_INFO, source.service->GetQueryCommand("generic/help", source.command).c_str()));
	}
}

CommandInfoServInfo::CommandInfoServInfo(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/info", 0, 0)
	, is(parent)
{
	this->SetDesc(_("Show InfoServ messages."));
	this->AllowUnregistered(true);
}

void CommandInfoServInfo::Execute(CommandSource& source, const std::vector<Anope::string>&)
{
	User* u = source.GetUser();
	if (!u)
		return;
	this->is.DisplayAllToUser(u);
}

bool CommandInfoServInfo::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Shows informational messages posted by staff."));
	this->is.Reply(source, _("Example: INFO"));
	return true;
}

CommandInfoServPost::CommandInfoServPost(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/post", 3, 3)
	, is(parent)
{
	this->SetDesc(_("Post a new InfoServ message."));
	this->SetSyntax(_("<importance 0-4> <subject> <message>"));
}

void CommandInfoServPost::OnSyntaxError(CommandSource& source, const Anope::string&)
{
	ReplySyntaxAndMoreInfo(this->is, source, _("<importance 0-4> <subject> <message>"));
}

void CommandInfoServPost::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "InfoServ: Access denied.");
		return;
	}

	unsigned imp = 0;
	try
	{
		imp = Anope::Convert<unsigned>(params[0], 0);
	}
	catch (...)
	{
		imp = 99;
	}

	if (imp > 4)
	{
		this->is.Reply(source, "Importance must be a number between 0 and 4.");
		return;
	}

	this->is.Post(source, imp, params[1], params[2]);
}

bool CommandInfoServPost::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Posts a new InfoServ message."));
	this->is.Reply(source, _("Importance levels:"));
	this->is.Reply(source, _("0 = oper-only (stored, shown when a user becomes OPER)"));
	this->is.Reply(source, _("1 = public (stored, shown on connect)"));
	this->is.Reply(source, _("2 = global notice (sent immediately, not stored)"));
	this->is.Reply(source, _("3 = public + global notice (stored and sent)"));
	this->is.Reply(source, _("4 = critical global notice (sent immediately, not stored)"));
	this->is.Reply(source, _("Subject is a single token; use '_' for spaces."));
	return true;
}

CommandInfoServList::CommandInfoServList(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/list", 0, 0)
	, is(parent)
{
	this->SetDesc(_("List public InfoServ messages."));
}

void CommandInfoServList::Execute(CommandSource& source, const std::vector<Anope::string>&)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}
	this->is.List(source, false);
}

bool CommandInfoServList::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Lists the stored public messages."));
	this->is.Reply(source, _("Example: LIST"));
	return true;
}

CommandInfoServMove::CommandInfoServMove(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/move", 2, 2)
	, is(parent)
{
	this->SetDesc(_("Move a public InfoServ message to another position."));
	this->SetSyntax(_("<from> <to>"));
}

void CommandInfoServMove::OnSyntaxError(CommandSource& source, const Anope::string&)
{
	ReplySyntaxAndMoreInfo(this->is, source, _("<from> <to>"));
}

void CommandInfoServMove::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}

	unsigned from_id = 0;
	unsigned to_id = 0;
	try
	{
		from_id = Anope::Convert<unsigned>(params[0], 0);
		to_id = Anope::Convert<unsigned>(params[1], 0);
	}
	catch (...)
	{
		from_id = to_id = 0;
	}

	this->is.Move(source, false, from_id, to_id);
}

bool CommandInfoServMove::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Reorders stored public messages (1-based positions)."));
	this->is.Reply(source, _("Example: MOVE 5 1"));
	return true;
}

CommandInfoServDel::CommandInfoServDel(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/del", 1, 1)
	, is(parent)
{
	this->SetDesc(_("Delete a public InfoServ message."));
	this->SetSyntax(_("<id>"));
}

void CommandInfoServDel::OnSyntaxError(CommandSource& source, const Anope::string&)
{
	ReplySyntaxAndMoreInfo(this->is, source, _("<id>"));
}

void CommandInfoServDel::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}

	unsigned id = 0;
	try
	{
		id = Anope::Convert<unsigned>(params[0], 0);
	}
	catch (...)
	{
		id = 0;
	}

	this->is.Delete(source, false, id);
}

bool CommandInfoServDel::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Deletes a stored public message by id (see LIST)."));
	this->is.Reply(source, _("Example: DEL 3"));
	return true;
}

CommandInfoServOList::CommandInfoServOList(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/olist", 0, 0)
	, is(parent)
{
	this->SetDesc(_("List oper-only InfoServ messages."));
}

void CommandInfoServOList::Execute(CommandSource& source, const std::vector<Anope::string>&)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}
	this->is.List(source, true);
}

bool CommandInfoServOList::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Lists the stored oper-only messages."));
	return true;
}

CommandInfoServOMove::CommandInfoServOMove(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/omove", 2, 2)
	, is(parent)
{
	this->SetDesc(_("Move an oper-only InfoServ message."));
	this->SetSyntax(_("<from> <to>"));
}

void CommandInfoServOMove::OnSyntaxError(CommandSource& source, const Anope::string&)
{
	ReplySyntaxAndMoreInfo(this->is, source, _("<from> <to>"));
}

void CommandInfoServOMove::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}

	unsigned from_id = 0;
	unsigned to_id = 0;
	try
	{
		from_id = Anope::Convert<unsigned>(params[0], 0);
		to_id = Anope::Convert<unsigned>(params[1], 0);
	}
	catch (...)
	{
		from_id = to_id = 0;
	}

	this->is.Move(source, true, from_id, to_id);
}

bool CommandInfoServOMove::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Reorders stored oper-only messages (1-based positions)."));
	return true;
}

CommandInfoServODel::CommandInfoServODel(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/odel", 1, 1)
	, is(parent)
{
	this->SetDesc(_("Delete an oper-only InfoServ message."));
	this->SetSyntax(_("<id>"));
}

void CommandInfoServODel::OnSyntaxError(CommandSource& source, const Anope::string&)
{
	ReplySyntaxAndMoreInfo(this->is, source, _("<id>"));
}

void CommandInfoServODel::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (!this->is.IsAdmin(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}

	unsigned id = 0;
	try
	{
		id = Anope::Convert<unsigned>(params[0], 0);
	}
	catch (...)
	{
		id = 0;
	}

	this->is.Delete(source, true, id);
}

bool CommandInfoServODel::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Deletes a stored oper-only message by id (see OLIST)."));
	return true;
}

CommandInfoServNotify::CommandInfoServNotify(Module* creator, InfoServCore& parent)
	: Command(creator, "infoserv/notify", 0, 1)
	, is(parent)
{
	this->SetDesc(_("Change how InfoServ replies (NOTICE/PRIVMSG)."));
	this->SetSyntax(_("NOTIFY [notice|privmsg]"));
}

void CommandInfoServNotify::Execute(CommandSource& source, const std::vector<Anope::string>& params)
{
	if (params.empty())
	{
		this->is.Reply(source, Anope::Format("Current reply method: %s", this->is.ReplyModeString().c_str()));
		this->is.Reply(source, "Change it with: \2NOTIFY privmsg\2 or \2NOTIFY notice\2");
		return;
	}

	if (!this->is.CanChangeReplyMode(source))
	{
		this->is.Reply(source, "Access denied.");
		return;
	}

	if (!this->is.SetReplyMode(params[0]))
	{
		this->is.Reply(source, "Syntax: NOTIFY [notice|privmsg]");
		return;
	}

	this->is.Reply(source, Anope::Format("OK. Reply method is now: %s", this->is.ReplyModeString().c_str()));
}

bool CommandInfoServNotify::OnHelp(CommandSource& source, const Anope::string&)
{
	this->is.Reply(source, " ");
	this->is.Reply(source, _("Controls whether InfoServ replies using NOTICE or PRIVMSG."));
	return true;
}

MODULE_INIT(InfoServCore)
