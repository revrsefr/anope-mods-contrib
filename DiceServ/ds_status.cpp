/* ----------------------------------------------------------------------------
 * Name    : ds_status.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The STATUS command of DiceServ. See diceserv.cpp for more information about
 * DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

ServiceReference<DiceServService> DiceServ("DiceServService", "DiceServ");

/** STATUS command
 *
 * This will allow Services Operators to view the ignore status of a single channel or a single nickname/user.
 */
class DSStatusCommand : public Command
{
public:
	DSStatusCommand(Module *creator) : Command(creator, "diceserv/status", 1, 1)
	{
		this->SetDesc(_("Shows allow status of channel or nick"));
		this->SetSyntax(_("{\037channel\037|\037nick\037}"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		Anope::string what = params[0];
		// If the argument starts with a #, assume it's a channel
		if (what[0] == '#')
		{
			// Find the Channel record and ChannelInfo record from ChanServ
			Channel *c = Channel::Find(what);
			ChannelInfo *ci = ChannelInfo::Find(what);
			// If the channel wasn't found and a ChanServ entry wasn't found (or the channel is suspended), display an error
			if (!c && (!ci || ci->HasExt("SUSPENDED")))
				source.Reply(CHAN_X_INVALID, what.c_str());
			// If we found a registered channel, show the data for that
			else if (ci)
				source.Reply(_("Status for registered channel \037%s\037: %s"), what.c_str(), DiceServ->IsIgnored(ci) ? _("Ignore") : _("Allow"));
			// No registered channel was found, but a channel was, so show the data for that
			else
				source.Reply(_("Status for unregistered channel \037%s\037: %s"), what.c_str(), DiceServ->IsIgnored(c) ? _("Ignore") : _("Allow"));
		}
		// Otherwise, the argument is going to be assumed to be a nick
		else
		{
			// Find the User record and the NickAlias record from NickServ
			User *nu = User::Find(what);
			NickAlias *na = NickAlias::Find(what);
			BotInfo *bot = BotInfo::Find(what);
			// If the nick wasn't found and a NickServ entry wasn't found (or the nick is suspended), display an error
			if (bot || (!nu && (!na || (na->nc && na->nc->HasExt("SUSPENDED")))))
				source.Reply(_("Nick %s is not a valid nick."), what.c_str());
			// If we found a registered nick, show the data for that
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
				// If we have a User record, then the given nick is online from another nick in their group, show that
				if (nu && nu->nick != na->nick)
					source.Reply(_("Status for registered nick \037%s\037: %s\n  (online as \037%s\037)"), what.c_str(),
						DiceServ->IsIgnored(na->nc) ? _("Ignore") : _("Allow"), nu->nick.c_str());
				// Otherwise, they are not online, just show that it is registered and its data
				else
					source.Reply(_("Status for registered nick \037%s\037: %s"), what.c_str(), DiceServ->IsIgnored(na->nc) ? _("Ignore") : _("Allow"));
			}
			// No registered nick was found, but a user was, so show the data for that
			else
				source.Reply(_("Status for unregistered nick \037%s\037: %s"), what.c_str(), DiceServ->IsIgnored(nu) ? _("Ignore") : _("Allow"));
		}
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("This will give you the allowed or ignored status of a\n"
			"channel or a nick, depending on which one you give. It will\n"
			"also tell you if that status is on an online nick/channel,\n"
			"or set in services due to the nick not being online."));
		return true;
	}
};

class DSStatus : public Module
{
	DSStatusCommand cmd;

public:
	DSStatus(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServ)
			throw ModuleException("No interface for DiceServ");
	}
};

MODULE_INIT(DSStatus)
