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
	bool HasChanServSetAccess(CommandSource& source, ChannelInfo* ci)
	{
		return (ci && (source.AccessFor(ci).HasPriv("SET") || source.HasPriv("chanserv/administration")));
	}
}

namespace
{
	struct GSChanAccessDataType final
		: Serialize::Type
	{
		ExtensibleItem<GSChanAccessData>& item;

		explicit GSChanAccessDataType(ExtensibleItem<GSChanAccessData>& it)
			: Serialize::Type("GSChanAccessData")
			, item(it)
		{
		}

		void Serialize(Serializable* obj, Serialize::Data& data) const override
		{
			const auto* d = static_cast<const GSChanAccessData*>(obj);
			data.Store("ci", d->object);
			data.Store("group", d->group);
			data.Store("group_only", d->group_only ? "1" : "0");
		}

		Serializable* Unserialize(Serializable* obj, Serialize::Data& data) const override
		{
			Anope::string sci, sgroup, sonly;
			data["ci"] >> sci;
			data["group"] >> sgroup;
			data["group_only"] >> sonly;

			ChannelInfo* ci = ChannelInfo::Find(sci);
			if (!ci)
				return nullptr;

			const bool only = (sonly == "1" || sonly.equals_ci("true") || sonly.equals_ci("yes") || sonly.equals_ci("on"));

			if (obj)
			{
				auto* d = anope_dynamic_static_cast<GSChanAccessData*>(obj);
				d->object = ci->name;
				d->group = sgroup;
				d->group_only = only;
				return d;
			}

			return this->item.Set(ci, GSChanAccessData(ci, sgroup, only));
		}
	};
}

class CommandCSSetGroup final
	: public Command
{
	GroupServCore& gs;
	ExtensibleItem<GSChanAccessData>& item;

public:
	CommandCSSetGroup(Module* creator, GroupServCore& core, ExtensibleItem<GSChanAccessData>& it)
		: Command(creator, "chanserv/set/group", 1, 2)
		, gs(core)
		, item(it)
	{
		this->SetDesc(_("Associate a registered channel with a GroupServ group."));
		this->SetSyntax(_("\037channel\037 [\037!group|OFF\037]"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		if (Anope::ReadOnly)
		{
			source.Reply(READ_ONLY_MODE);
			return;
		}

		ChannelInfo* ci = ChannelInfo::Find(params[0]);
		if (!ci)
		{
			source.Reply(CHAN_X_NOT_REGISTERED, params[0].c_str());
			return;
		}

		if (!HasChanServSetAccess(source, ci) && source.permission.empty())
		{
			source.Reply(ACCESS_DENIED);
			return;
		}

		if (params.size() == 1)
		{
			auto* d = this->item.Get(ci);
			if (!d || d->group.empty())
				source.Reply(_("Group restriction for %s is not set."), ci->name.c_str());
			else
				source.Reply(_("Group restriction for %s is %s (%s)."), ci->name.c_str(), d->group.c_str(), d->group_only ? "GROUPONLY" : "not group-only");
			return;
		}

		const auto& val = params[1];
		if (val.equals_ci("OFF"))
		{
			this->item.Unset(ci);
			Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci) << "to unset GROUP";
			source.Reply(CHAN_SETTING_UNSET, "GROUP", ci->name.c_str());
			return;
		}

		if (!this->gs.DoesGroupExist(val))
		{
			source.Reply(_("Group %s does not exist."), val.c_str());
			return;
		}

		bool only = false;
		if (auto* cur = this->item.Get(ci))
			only = cur->group_only;
		else
			only = true; // default to enforcement when a group is set

		this->item.Set(ci, GSChanAccessData(ci, val, only));
		Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci) << "to set GROUP to " << val;
		source.Reply(CHAN_SETTING_CHANGED, "GROUP", ci->name.c_str(), val.c_str());
	}
};

class CommandCSSetGroupOnly final
	: public Command
{
	GroupServCore& gs;
	ExtensibleItem<GSChanAccessData>& item;

public:
	CommandCSSetGroupOnly(Module* creator, GroupServCore& core, ExtensibleItem<GSChanAccessData>& it)
		: Command(creator, "chanserv/set/grouponly", 2, 2)
		, gs(core)
		, item(it)
	{
		this->SetDesc(_("Require GroupServ group membership to remain in the channel."));
		this->SetSyntax(_("\037channel\037 <ON|OFF>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		if (Anope::ReadOnly)
		{
			source.Reply(READ_ONLY_MODE);
			return;
		}

		ChannelInfo* ci = ChannelInfo::Find(params[0]);
		if (!ci)
		{
			source.Reply(CHAN_X_NOT_REGISTERED, params[0].c_str());
			return;
		}

		if (!HasChanServSetAccess(source, ci) && source.permission.empty())
		{
			source.Reply(ACCESS_DENIED);
			return;
		}

		const auto setting = params[1].upper();
		if (setting != "ON" && setting != "OFF")
		{
			source.Reply(_("Syntax: %s %s"), source.command.c_str(), "\037channel\037 <ON|OFF>");
			return;
		}

		auto* cur = this->item.Get(ci);
		const bool enable = (setting == "ON");

		if (enable)
		{
			if (!cur || cur->group.empty())
			{
				source.Reply(_("You must set GROUP first (use: CHANSERV SET %s GROUP <!group>)."), ci->name.c_str());
				return;
			}

			if (!this->gs.DoesGroupExist(cur->group))
			{
				source.Reply(_("Group %s does not exist."), cur->group.c_str());
				return;
			}

			this->item.Set(ci, GSChanAccessData(ci, cur->group, true));
			Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci) << "to enable GROUPONLY";
			source.Reply(CHAN_SETTING_CHANGED, "GROUPONLY", ci->name.c_str(), "ON");
		}
		else
		{
			if (!cur)
			{
				source.Reply(_("GROUPONLY for %s is already OFF."), ci->name.c_str());
				return;
			}
			this->item.Set(ci, GSChanAccessData(ci, cur->group, false));
			Log(source.AccessFor(ci).HasPriv("SET") ? LOG_COMMAND : LOG_OVERRIDE, source, this, ci) << "to disable GROUPONLY";
			source.Reply(CHAN_SETTING_CHANGED, "GROUPONLY", ci->name.c_str(), "OFF");
		}
	}
};

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

class CommandGroupServListChans final
	: public Command
{
	GroupServCore& gs;

public:
	CommandGroupServListChans(Module* creator, GroupServCore& core)
		: Command(creator, "groupserv/listchans", 1, 1)
		, gs(core)
	{
		this->SetDesc(_("Lists channels associated with a group."));
		this->SetSyntax(_("<!group>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		this->gs.ListChans(source, params[0]);
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

class CommandGroupServToggleGroupFlag final
	: public Command
{
	GroupServCore& gs;
	GSGroupFlags flag;

public:
	CommandGroupServToggleGroupFlag(Module* creator, GroupServCore& core, const Anope::string& name, GSGroupFlags f)
		: Command(creator, name, 2, 2)
		, gs(core)
		, flag(f)
	{
		this->SetDesc(_("Toggle a group limit/flag (Services Operator)."));
		this->SetSyntax(_("<!group> <ON|OFF>"));
		this->AllowUnregistered(true);
	}

	void OnSyntaxError(CommandSource& source, const Anope::string&) override
	{
		ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> <ON|OFF>"));
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		const auto val = params[1].upper();
		if (val != "ON" && val != "OFF")
		{
			ReplySyntaxAndMoreInfo(this->gs, source, _("<!group> <ON|OFF>"));
			return;
		}
		const bool enabled = (val == "ON");
		if (this->gs.SetGroupFlag(source, params[0], this->flag, enabled))
			this->gs.ReplyF(source, "%s for %s set to %s.", source.command.c_str(), params[0].c_str(), enabled ? "ON" : "OFF");
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
	ExtensibleItem<GSChanAccessData> chanaccess;
	GSChanAccessDataType chanaccess_type;
	CommandCSSetGroup cmd_cs_set_group;
	CommandCSSetGroupOnly cmd_cs_set_grouponly;

	CommandGroupServRegister cmd_register;
	CommandGroupServDrop cmd_drop;
	CommandGroupServDrop cmd_fdrop;
	CommandGroupServInfo cmd_info;
	CommandGroupServList cmd_list;
	CommandGroupServJoin cmd_join;
	CommandGroupServInvite cmd_invite;
	CommandGroupServListChans cmd_listchans;
	CommandGroupServFlags cmd_flags;
	CommandGroupServFlags cmd_fflags;
	CommandGroupServSet cmd_set;
	CommandGroupServToggleGroupFlag cmd_acsnolimit;
	CommandGroupServToggleGroupFlag cmd_regnolimit;

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
		, chanaccess(this, "groupserv:chanaccess")
		, chanaccess_type(chanaccess)
		, cmd_cs_set_group(this, core, chanaccess)
		, cmd_cs_set_grouponly(this, core, chanaccess)
		, cmd_register(this, core)
		, cmd_drop(this, core, "groupserv/drop", false)
		, cmd_fdrop(this, core, "groupserv/fdrop", true)
		, cmd_info(this, core)
		, cmd_list(this, core)
		, cmd_join(this, core)
		, cmd_invite(this, core)
		, cmd_listchans(this, core)
		, cmd_flags(this, core, "groupserv/flags", false)
		, cmd_fflags(this, core, "groupserv/fflags", true)
		, cmd_set(this, core)
		, cmd_acsnolimit(this, core, "groupserv/acsnolimit", GSGroupFlags::ACSNOLIMIT)
		, cmd_regnolimit(this, core, "groupserv/regnolimit", GSGroupFlags::REGNOLIMIT)
	{
		this->core.SetChanAccessItem(&this->chanaccess);
	}

	~GroupServ() override
	{
		// Persist serialized channel extensions (GROUP/GROUPONLY) across MODRELOAD.
		// This runs before member destructors, so the extension data still exists.
		this->core.SaveDB();
		Anope::SaveDatabases();
	}

	void OnJoinChannel(User* user, Channel* c) override
	{
		if (!c || !c->ci || !user || !user->server || !user->server->IsSynced() || user->server == Me)
			return;

		auto* d = this->chanaccess.Get(c->ci);
		if (!d || !d->group_only || d->group.empty())
			return;

		NickCore* nc = user->Account();
		if (nc && nc->IsServicesOper() && nc->o && nc->o->ot && (nc->o->ot->HasPriv("chanserv/administration") || nc->o->ot->HasPriv("groupserv/admin")))
			return;

		if (this->core.IsMemberOfGroup(d->group, nc))
			return;

		c->Kick(NULL, user, "You must be a member of %s to join %s.", d->group.c_str(), c->name.c_str());
	}

	void OnReload(Configuration::Conf& conf) override
	{
		this->core.OnReload(conf);
		this->RecreateTimers();
	}
};

MODULE_INIT(GroupServ)
