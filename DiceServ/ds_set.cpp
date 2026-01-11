/* ----------------------------------------------------------------------------
 * Name    : ds_set.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The SET and SET IGNORE commands of DiceServ. See diceserv.cpp for more
 * information about DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

ServiceReference<DiceServService> DiceServ("DiceServService", "DiceServ");

/** SET command
 *
 * This base command has no real functionality, it is mainly here to display the help for the sub-commands.
 */
class DSSetCommand : public Command
{
public:
	DSSetCommand(Module *creator) : Command(creator, "diceserv/set", 3, 3)
	{
		this->SetDesc(Anope::Format(_("Set options for %s access"), Config->GetClient("DiceServ")->nick.c_str()));
		this->SetSyntax(_("\037option\037 \037parameters\037"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &) override
	{
		this->OnSyntaxError(source, "");
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Currently allows you to set who has %s access.\n"), source.service->nick.c_str());
		source.Reply(" ");
		source.Reply(_("Available options:"));
		source.Reply(" ");
		Anope::string this_name = source.command;
		bool hide_privileged_commands = Config->GetBlock("options").Get<bool>("hideprivilegedcommands"),
			hide_registered_commands = Config->GetBlock("options").Get<bool>("hideregisteredcommands");

		HelpWrapper help;
		for (CommandInfo::map::const_iterator it = source.service->commands.begin(), it_end = source.service->commands.end(); it != it_end; ++it)
		{
			const Anope::string &c_name = it->first;
			const CommandInfo &info = it->second;
			if (c_name.find_ci(this_name + " ") == 0)
			{
				ServiceReference<Command> c("Command", info.name);

				if (!c)
					continue;
				else if (hide_registered_commands && !c->AllowUnregistered() && !source.GetAccount())
					continue;
				else if (hide_privileged_commands && !info.permission.empty() && !source.HasCommand(info.permission))
					continue;

				source.command = it->first;
				c->OnServHelp(source, help);
			}
		}
		help.SendTo(source);
		source.Reply(" ");
		source.Reply(_("Type \002%s%s HELP %s \037option\037\002 for more information on a\n"
			"particular option.\n"), "/msg ", source.service->nick.c_str(), this_name.c_str());
		source.Reply(" ");
		source.Reply(_("Note: Access to these commands are limited. See help on each\n"
			"option for details."));
		return true;
	}
};

/** SET IGNORE command
 *
 * Sets a channel or nickname/user to be ignored by DiceServ.
 */
class DSSetIgnoreCommand : public Command
{
	/** When this value is true, then channel operators set in ChanServ can ignore channels in addition to the channel's founder(s) */
	bool ChanOpCanIgnore;

	/** Sets the actual syntax based on the access the user has.
	 */
	void SetRealSyntax(CommandSource &source)
	{
		this->ClearSyntax();
		if (source.GetAccount() && source.HasCommand("diceserv/set"))
			this->SetSyntax(_("{\037channel\037|\037nick\037} {ON|OFF}"));
		else
			this->SetSyntax(_("\037channel\037 {ON|OFF}"));
	}

public:
	DSSetIgnoreCommand(Module *creator) : Command(creator, "diceserv/set/ignore", 2, 2)
	{
		this->SetDesc(_("Change ignored/allowed setting"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		if (Anope::ReadOnly)
		{
			source.Reply(_("Sorry, dice ignore option setting is temporarily disabled."));
			return;
		}
		Anope::string where = params[0], param = params[1];
		// The parameter must be ON or OFF, all others should stop processing
		enum
		{
			IGNORE_ADD,
			IGNORE_DEL
		} mode;
		if (param.equals_ci("ON"))
			mode = IGNORE_ADD;
		else if (param.equals_ci("OFF"))
			mode = IGNORE_DEL;
		else
		{
			this->OnSyntaxError(source, "");
			return;
		}
		bool is_servoper = source.HasCommand("diceserv/set");
		// If the thing to ignore starts with a #, assume it's a channel
		if (where[0] == '#')
		{
			// Find the Channel record and ChannelInfo record from ChanServ
			Channel *c = Channel::Find(where);
			ChannelInfo *ci = ChannelInfo::Find(where);
			// If the channel wasn't found and a ChanServ entry wasn't found (or the channel is suspended), display an error
			if (!c && (!ci || ci->HasExt("SUSPENDED")))
				source.Reply(CHAN_X_INVALID, where.c_str());
			// If we found a registered channel, we will store the ignore there
			else if (ci)
			{
				/* The only ones who can set an ignore on a registered channel are Services Admins or the channel's founder (unless chanopcanignore is set, then
				 * channel operators can as well) */
				if (is_servoper || (this->ChanOpCanIgnore ? ci->AccessFor(source.GetUser()).HasPriv("AUTOOP") : false) ||
					(ci->HasExt("SECUREFOUNDER") ? source.IsFounder(ci) : source.AccessFor(ci).HasPriv("FOUNDER")))
				{
					/* Either add or delete the ignore data */
					if (mode == IGNORE_ADD)
					{
						DiceServ->Ignore(ci);
						if (c)
							DiceServ->Ignore(c);
					}
					else
					{
						DiceServ->Unignore(ci);
						if (c)
							DiceServ->Unignore(c);
					}
					source.Reply(mode == IGNORE_ADD ? _("\002%s\002 will now ignore all dice rolls sent to \037%s\037.") :
						_("\002%s\002 will now allow all dice rolls sent to \037%s\037."), source.service->nick.c_str(), where.c_str());
				}
				// Otherwise, deny access
				else
					source.Reply(ACCESS_DENIED);
			}
			// No registered channel was found, but a channel was, so store temporary data on the channel
			else
			{
				// The only ones who can set an ignore on an unregistered channel are Services Operators with diceserv/set or channel operators
				if (is_servoper || c->HasUserStatus(source.GetUser(), "OP"))
				{
					// Either add or delete the ignore data
					if (mode == IGNORE_ADD)
						DiceServ->Ignore(c);
					else
						DiceServ->Unignore(c);
					source.Reply(mode == IGNORE_ADD ? _("\002%s\002 will now ignore all dice rolls sent to \037%s\037.") :
						_("\002%s\002 will now allow all dice rolls sent to \037%s\037."), source.service->nick.c_str(), where.c_str());
				}
				// Otherwise, deny access
				else
					source.Reply(ACCESS_DENIED);
			}
		}
		// Otherwise, the thing to ignore is going to be assumed to be a nick
		else
		{
			// Only Services Operators with diceserv/set can set ignores on nicks, deny access to those who aren't
			if (!is_servoper)
				source.Reply(ACCESS_DENIED);
			// Find the User record and the NickAlias record from NickServ
			User *nu = User::Find(where);
			NickAlias *na = NickAlias::Find(where);
			BotInfo *bot = BotInfo::Find(where);
			// If the nick wasn't found and a NickServ entry wasn't found (or the nick is suspended), display an error
			if (bot || (!nu && (!na || (na->nc && na->nc->HasExt("SUSPENDED")))))
				source.Reply(_("I couldn't find a user or registered nick named \037%s\037."), where.c_str());
			// If we found a registered nick, we will store the ignore there
			else if (na)
			{
				// If a User record was not found, we will check all the users to find one that has a matching NickCore
				if (!nu)
				{
					user_map::const_iterator it = UserListByNick.begin(), it_end = UserListByNick.end();
					for (; it != it_end; ++it)
					{
						nu = it->second;
						if (nu->Account() == na->nc)
							break;
					}
					if (it == it_end)
						nu = NULL;
				}
				// Either add or delete the ignore data
				if (mode == IGNORE_ADD)
				{
					DiceServ->Ignore(na->nc);
					if (nu)
						DiceServ->Ignore(nu);
				}
				else
				{
					DiceServ->Unignore(na->nc);
					if (nu)
						DiceServ->Unignore(nu);
				}
				source.Reply(mode == IGNORE_ADD ? _("\002%s\002 will now ignore all dice rolls by \037%s\037.") :
					_("\002%s\002 will now allow all dice rolls by \037%s\037."), source.service->nick.c_str(), where.c_str());
			}
			// No registered nick was found, but a user was, so store temporary data on the user
			else
			{
				// Either add or delete the ignore data
				if (mode == IGNORE_ADD)
					nu->Extend("diceserv_ignore", 1);
				else
					DiceServ->Unignore(nu);
				source.Reply(mode == IGNORE_ADD ? _("\002%s\002 will now ignore all dice rolls by \037%s\037.") :
					_("\002%s\002 will now allow all dice rolls by \037%s\037."), source.service->nick.c_str(), where.c_str());
			}
		}
	}

	/** Sets the value for if channel operators can set ignores on channels.
	 * @param chanOpCanIgnore true if channel operators can set ignores, false otherwise
	 */
	void SetChanOpCanIgnore(bool chanOpCanIgnore)
	{
		this->ChanOpCanIgnore = chanOpCanIgnore;
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SetRealSyntax(source);
		this->SendSyntax(source);
		source.Reply(" ");
		BotInfo *ChanServ = Config->GetClient("ChanServ");
		if (source.GetAccount() && source.HasCommand("diceserv/set"))
		{
			BotInfo *NickServ = Config->GetClient("NickServ");
			source.Reply(_("This will allow a channel or nick to be set to ignore the\n"
				"use of %s commands inside the channel or by that user.\n"
				"If ON is given, then %s will be ignored, otherwise, it\n"
				"will be allowed.\n"
				" \n"
				"If the channel in question is registered, then only a\n"
				"services admin or the channel's founder (or someone with\n"
				"founder-level access) can use this option. The option set\n"
				"will be persistent as long as the channel stays registered\n"
				"in %s. If the channel is unregistered, then any ops in\n"
				"the channel can set this option, but it will only last as\n"
				"long as the channel is active.\n"
				" \n"
				"A nick may also be given, but this option is limited to\n"
				"services operators and up. If the nick in question is\n"
				"registered, then the option will be set in %s so it\n"
				"will stay persistent as long as the nick stays registered.\n"
				"If the nick is unregistered, then it will only last as long\n"
				"as the user is online."), source.service->nick.c_str(), source.service->nick.c_str(), ChanServ ? ChanServ->nick.c_str() : "ChanServ",
				NickServ ? NickServ->nick.c_str() : "NickServ");
		}
		else
			source.Reply(_("This will allow a channel to be set to ignore the use of\n"
				"%s commands inside the channel. If ON is given, then\n"
				"%s will be ignored, otherwise, it will be allowed.\n"
				" \n"
				"If the channel in question is registered, then only the\n"
				"channel's founder (or someone with founder-level access) can\n"
				"use this option. The option set will be persistent as long\n"
				"as the channel stays registered in %s. If the channel\n"
				"is unregistered, then any ops in the channel can set this\n"
				"option, but it will only last as long as the channel is\n"
				"active."), source.service->nick.c_str(), source.service->nick.c_str(), ChanServ ? ChanServ->nick.c_str() : "ChanServ");
		return true;
	}

	/** This is being overridden so we can replace the syntax string depending on the access the user has.
	 */
	void OnSyntaxError(CommandSource &source, const Anope::string &subcommand) override
	{
		this->SetRealSyntax(source);
		Command::OnSyntaxError(source, subcommand);
	}
};

class DSSet : public Module
{
	DSSetCommand set_cmd;
	DSSetIgnoreCommand set_ignore_cmd;

public:
	DSSet(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), set_cmd(this), set_ignore_cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServ)
			throw ModuleException("No interface for DiceServ");
	}

	void OnReload(Configuration::Conf &conf) override
	{
		this->set_ignore_cmd.SetChanOpCanIgnore(conf.GetModule("diceserv").Get<bool>("chanopcanignore"));
	}
};

MODULE_INIT(DSSet)
