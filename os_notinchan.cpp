/*
 * Operserv notinchanlist command
 *
 * This command can list, kill, akill, or tempshun all users not in any channels.
 *
 * Note that the 'TSHUN' option only works on ircd's that suport tempshun. (like Unreal 3.2.x)
 *
 * Syntax: NOTINCHAN {LIST|JOIN|KILL|AKILL|TSHUN} [reason]
 *
 * If a reason is not specified then the default reason from the module configuration will be used.  If those 
 * are missing or left blank then a generaic "Not In Channel Management" reason it automaticly appeneded.
 *
 * tshunreason : Used as the default message when shunning users not in channel.
 * killreason  : Used as the default reason when killing users not in channel.
 * akillreason : Used as the default reason when akilling users not in channel.
 * akillexpire : Time used for the akills.  This is not intended for permanent akills, just a temporary one
 *               so that the staff has time to deal with the issue.
 * idlechan    : Channel to force join non-channeled users into with using the JOIN option. Defaults to
 *               #idle if not specified.
 *
 * Modify the following as necessary and put it in your operserv config:

module { 
	name = "os_notinchan"
	tshunreason = "Rejoin us when you are willing to join us publicly."
	killreason = "Non-Channel kill"
	akillreason = "Not-in-channel ban"
	akillexpire = "7m"
	idlechan = "#idle"
}
command { service = "OperServ"; name="NOTINCHAN"; command = "operserv/notinchan"; permission = "operserv/akill"; }    

 *
 */

#include "module.h"

static ServiceReference<XLineManager> akills("XLineManager", "xlinemanager/sgline");

class CommandOSnotinchan : public Command
{
 public:
        CommandOSnotinchan(Module *creator) : Command(creator, "operserv/notinchan", 0, 2)
        {
		this->SetDesc(_("List, or apply command to, anyone not in a channel."));
		if(IRCD->GetProtocolName().find_ci("unreal") != Anope::string::npos)
			this->SetSyntax(_("{\037LIST\037|\037GECOS\037|\037JOIN\037|\037TSHUN\037|\037KILL\037|\037AKILL\037} [\037reason\037]"));
		else
			this->SetSyntax(_("{\037LIST\037|\037GECOS\037|\037JOIN\037|\037KILL\037|\037AKILL\037} [\037reason\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		const Anope::string &subcommand = params.size() > 0 ? params[0] : "LIST";
		const Anope::string &reason = params.size() > 1 ? params[1] : "";
		Anope::string rreason = "";
		Anope::string chan = Config->GetModule(this->owner).Get<Anope::string>("idlechan","#idle");
		std::set<Anope::string> modes;
		Anope::map<User *> ordered_map;
		time_t expires = Config->GetModule(this->owner).Get<time_t>("akillexpire", "5m");
		
		expires += Anope::CurTime;
                ListFormatter list(source.GetAccount());
		Log(LOG_ADMIN, source, this) << subcommand << " " << reason;

		if (!subcommand.equals_ci("TSHUN") && !subcommand.equals_ci("LIST") && 
		    !subcommand.equals_ci("KILL") && !subcommand.equals_ci("AKILL") && 
		    !subcommand.equals_ci("JOIN") && !subcommand.equals_ci("GECOS"))
		{
			source.Reply(_("You must specify a valid option.\n"
					" \n"));
			this->SendSyntax(source);
			return;
		}

		if (!reason.empty()) 
		{
			rreason = reason;
		}
		else if (subcommand.equals_ci("LIST"))
		{
                	list.AddColumn(_("Name")).AddColumn(_("Mask"));
		}
		else if (subcommand.equals_ci("GECOS"))
		{
                	list.AddColumn(_("Name")).AddColumn(_("Realname"));
		}
		else if (subcommand.equals_ci("KILL"))
		{
			rreason = Config->GetModule(this->owner).Get<Anope::string>("killreason","Not In Channel Management");
		}
		else if (subcommand.equals_ci("AKILL"))
		{
			rreason = Config->GetModule(this->owner).Get<Anope::string>("akillreason","Not In Channel Management");
		}
		else if (subcommand.equals_ci("TSHUN") and IRCD->GetProtocolName().find_ci("unreal") != Anope::string::npos)
		{
			rreason = Config->GetModule(this->owner).Get<Anope::string>("tshunreason","Not In Channel Management");
		}

		if (subcommand.equals_ci("JOIN"))
		{
			if (!IRCD->CanSVSJoin)
			{
				source.Reply(_("Your IRCd does not support SVSJOIN."));
				return;
			}
		}

		for (user_map::const_iterator it = UserListByNick.begin(); it != UserListByNick.end(); ++it)
		{
			ordered_map[it->first] = it->second;
		}
		int counter = 0;
		for (Anope::map<User *>::const_iterator it = ordered_map.begin(); it != ordered_map.end(); ++it)
		{
			User *u2 = it->second;
			if (u2->chans.empty() && !(u2->server == Me) )
			{
				counter++;
				if (subcommand.equals_ci("GECOS"))
				{
					ListFormatter::ListEntry entry;
					entry["Name"] = u2->nick;
					entry["Realname"] = u2->realname;
					list.AddEntry(entry);
				}
				if (subcommand.equals_ci("LIST"))
				{
					ListFormatter::ListEntry entry;
					entry["Name"] = u2->nick;
					entry["Mask"] = u2->GetIdent() + "@" + u2->GetDisplayedHost();
					list.AddEntry(entry);
				}
				else if (subcommand.equals_ci("KILL"))
				{
/*					Log(LOG_ADMIN,source,this) << " Killing " << u2->nick.c_str(); */
					u2->Kill(Me, rreason);
				}
				else if (subcommand.equals_ci("AKILL"))
				{
/*					Log(LOG_ADMIN,source,this) << " Akilling " << u2->nick.c_str(); */
					u2->Kill(Me, rreason);
				XLine *x = new XLine("*@"+u2->host, source.GetNick(), expires, rreason, XLineManager::GenerateUID());
					akills->AddXLine(x);
				}
				else if (subcommand.equals_ci("TSHUN"))
				{
/*					Log(LOG_ADMIN,source,this) << "Tempshunning " << u2->nick.c_str(); */
					Uplink::Send(Me, "TEMPSHUN", u2->nick, rreason);

				}
				else if (subcommand.equals_ci("JOIN"))
				{
/*					Log(LOG_ADMIN,source,this) << "joining " << u2->nick << " to " << chan ;  */
					IRCD->SendSVSJoin(*source.service, u2, chan, "");
				}

			}
		}
		if (subcommand.equals_ci("LIST") || subcommand.equals_ci("GECOS"))
		{
			if (list.IsEmpty())
				source.Reply(_("No users to list."));
			else
			{
				source.Reply(_("Users list:"));
				list.SendTo(source);
				source.Reply(_("End of users list."));
			}
		}
		else
		{
			source.Reply(_("%s affected %d users."),subcommand.c_str(),counter); 
		}

	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		Anope::string chan = Config->GetModule(this->owner).Get<Anope::string>("idlechan","#idle");
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("\037LIST\037 will list the users, and their hostmask, that are not"
				"in any channel.\n"
				" \n"));
		if(IRCD->GetProtocolName().find_ci("unreal") != Anope::string::npos)
			source.Reply(_("\037TSHUN\037 will locate and TEMPSHUN each and ever user not in a channel.\n \n"));
		source.Reply(_("\037KILL\037 will kill all users not in a channel.\n"
				" \n"
				"\037AKILL\037 will AKILL all users not in a channel.\n"
			        " \n"
				"\037JOIN\037 will foce join all the users to %s. \n"
				" \n"),chan.c_str());
		return true;
	}
};

class OSnotinchan : public Module
{
	CommandOSnotinchan commandosnotinchan;

 public:
	OSnotinchan(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA),
		commandosnotinchan(this)
	{
		if(Anope::VersionMajor() != 2 || (Anope::VersionMinor() != 0 && Anope::VersionMinor() != 1))
		{
			throw ModuleException("Requires version 2.1.x of Anope.");
		}
		this->SetAuthor("Azander");
		this->SetVersion("1.0.4");
	}

};

MODULE_INIT(OSnotinchan)
