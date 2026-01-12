/*
 * (C) 2026 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Channel: #development
 *
 * GroupServ module created by reverse for Anope 2.1
 * - Account groups: REGISTER/INFO/LIST
 * - Membership: JOIN, INVITE
 * - Access control: FLAGS (per-member permissions)
 * - Settings: SET (OPEN/PUBLIC/JOINFLAGS + metadata)
 * - Persistence: flatfile database in data/groupserv.db (atomic .tmp + rename)
 *
 * Notes:
 * - Group names are like !group
 * - Access flags here are GroupServ-specific (NOT IRC user modes)
 *
 * Example configuration:
 *
 * service
 * {
 *   nick = "GroupServ"
 *   user = "GroupServ"
 *   host = "chaat.services"
 *   gecos = "Group Service"
 *   channels = "@#services"  # optional
 * }
 *
 * module
 * {
 *   name = "groupserv"
 *   client = "GroupServ"
 *
 *   # How GroupServ replies to users: "notice" or "privmsg"
 *   reply_method = "notice"
 *
 *   # Privileges
 *   admin_priv = "groupserv/admin"    # FDROP/FFLAGS
 *   auspex_priv = "groupserv/auspex"  # view/list any group
 *   exceed_priv = "groupserv/exceed"  # bypass limits
 *
 *   # Limits
 *   maxgroups = 5
 *   maxgroupacs = 0
 *
 *   # Open groups
 *   enable_open_groups = yes
 *
 *   # Default flags granted when a user JOINs an open group.
 *   # These are GroupServ access flags (NOT IRC user modes).
 *   # Accepts: +V +I, +ACLVIEW +INVITE, or compact +VI.
 *   default_joinflags = "+V"
 *
 *   # Auto-save interval in seconds (0 disables periodic autosave)
 *   save_interval = 600
 * }
 *
 * command { service = "GroupServ"; name = "HELP"; command = "generic/help"; }
 * command { service = "GroupServ"; name = "REGISTER"; command = "groupserv/register"; }
 * command { service = "GroupServ"; name = "DROP"; command = "groupserv/drop"; }
 * command { service = "GroupServ"; name = "LIST"; command = "groupserv/list"; }
 * command { service = "GroupServ"; name = "INFO"; command = "groupserv/info"; }
 * command { service = "GroupServ"; name = "JOIN"; command = "groupserv/join"; }
 * command { service = "GroupServ"; name = "INVITE"; command = "groupserv/invite"; }
 * command { service = "GroupServ"; name = "FLAGS"; command = "groupserv/flags"; }
 * command { service = "GroupServ"; name = "SET"; command = "groupserv/set"; }
 * command { service = "GroupServ"; name = "FDROP"; command = "groupserv/fdrop"; hide = true; }
 * command { service = "GroupServ"; name = "FFLAGS"; command = "groupserv/fflags"; hide = true; }
 */

#include "groupserv.h"

#include "language.h"

#include <algorithm>
#include <memory>

namespace
{
	void ReplySyntaxAndMoreInfo(GroupServCore& gs, CommandSource& source, const Anope::string& syntax)
	{
		if (syntax.empty())
			gs.Reply(source, Anope::Format("Syntax: %s", source.command.c_str()));
		else
			gs.Reply(source, Anope::Format("Syntax: %s %s", source.command.c_str(), syntax.c_str()));

		if (!source.service)
			return;

		auto it = std::find_if(source.service->commands.begin(), source.service->commands.end(), [](const auto& cmd)
		{
			return cmd.second.name == "generic/help";
		});
		if (it != source.service->commands.end())
			gs.Reply(source, Anope::Format(MORE_INFO, source.service->GetQueryCommand("generic/help", source.command).c_str()));
	}
}

class CommandGroupServRegister final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServRegister(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/register", 1, 1)
		, gs(core)
	{
		this->SetDesc(_("Register a group."));
		this->SetSyntax(_("<!group>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.RegisterGroup(source, params[0]);
	}
};

class CommandGroupServDrop final
	: public Command
{
	GroupServCore& gs;
	bool force;

public:
	CommandGroupServDrop(Module* creator, GroupServCore& core, const Anope::string& sname, bool f)
		: Command(creator, sname, 1, 2)
		, gs(core)
		, force(f)
	{
		this->SetDesc(_("Drop a group."));
		this->SetSyntax(_("<!group> [key]"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> [key]"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.DropGroup(source, params[0], params.size() >= 2 ? params[1] : "", this->force);
	}
};

class CommandGroupServInfo final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServInfo(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/info", 1, 1)
		, gs(core)
	{
		this->SetDesc(_("Show information about a group."));
		this->SetSyntax(_("<!group>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.ShowInfo(source, params[0]);
	}
};

class CommandGroupServList final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServList(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/list", 1, 1)
		, gs(core)
	{
		this->SetDesc(_("List groups."));
		this->SetSyntax(_("<pattern>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<pattern>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.ListGroups(source, params[0]);
	}
};

class CommandGroupServJoin final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServJoin(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/join", 1, 1)
		, gs(core)
	{
		this->SetDesc(_("Join a group."));
		this->SetSyntax(_("<!group>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.JoinGroup(source, params[0]);
	}
};

class CommandGroupServInvite final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServInvite(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/invite", 2, 2)
		, gs(core)
	{
		this->SetDesc(_("Invite an account to a group."));
		this->SetSyntax(_("<!group> <account>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> <account>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.InviteToGroup(source, params[0], params[1]);
	}
};

class CommandGroupServFlags final
	: public Command
{
	GroupServCore& gs;
	bool force;

public:
	CommandGroupServFlags(Module* creator, GroupServCore& core, const Anope::string& sname, bool f)
		: Command(creator, sname, 1, 3)
		, gs(core)
		, force(f)
	{
		this->SetDesc(_("View or modify group access flags."));
		this->SetSyntax(_("<!group>"));
		this->SetSyntax(_("<!group> <account> <flags>"));
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		if (params.size() == 1)
			this->gs.ShowFlags(source, params[0]);
		else if (params.size() == 3)
			this->gs.SetFlags(source, params[0], params[1], params[2], this->force);
		else
			ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> [account flags]"));
	}
};

class CommandGroupServSet final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServSet(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/set", 2, 3)
		, gs(core)
	{
		this->SetDesc(_("Set group options."));
		this->SetSyntax(_("<!group> <setting> [value]"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> <setting> [value]"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.SetOption(source, params[0], params[1], params.size() >= 3 ? params[2] : "");
	}
};

class GroupServTimer final
	: public Timer
{
	GroupServCore& gs;

public:
	GroupServTimer(Module* owner, GroupServCore& core, time_t seconds)
		: Timer(owner, seconds, true)
		, gs(core)
	{
	}

	void Tick() override
	{
		this->gs.SaveDB();
	}
};

class GroupServ final
	: public Module
{
	GroupServCore core;

	CommandGroupServRegister cmd_register;
	CommandGroupServDrop cmd_drop;
	CommandGroupServDrop cmd_fdrop;
	CommandGroupServInfo cmd_info;
	CommandGroupServList cmd_list;
	CommandGroupServJoin cmd_join;
	CommandGroupServInvite cmd_invite;
	CommandGroupServFlags cmd_flags;
	CommandGroupServFlags cmd_fflags;
	CommandGroupServSet cmd_set;

	std::unique_ptr<GroupServTimer> save;

	void RecreateTimers()
	{
		this->save.reset();
		if (this->core.GetSaveInterval() > 0)
			this->save = std::make_unique<GroupServTimer>(this, this->core, this->core.GetSaveInterval());
	}

public:
	GroupServ(const Anope::string& modname, const Anope::string& creator)
		: Module(modname, creator, VENDOR)
		, core(this)
		, cmd_register(this, core)
		, cmd_drop(this, core, "groupserv/drop", false)
		, cmd_fdrop(this, core, "groupserv/fdrop", true)
		, cmd_info(this, core)
		, cmd_list(this, core)
		, cmd_join(this, core)
		, cmd_invite(this, core)
		, cmd_flags(this, core, "groupserv/flags", false)
		, cmd_fflags(this, core, "groupserv/fflags", true)
		, cmd_set(this, core)
	{
	}

	void OnReload(Configuration::Conf& conf) override
	{
		this->core.OnReload(conf);
		this->RecreateTimers();
	}
};

MODULE_INIT(GroupServ)
