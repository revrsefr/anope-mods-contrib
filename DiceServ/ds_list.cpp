/* ----------------------------------------------------------------------------
 * Name    : ds_list.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The LIST command of DiceServ. See diceserv.cpp for more information about
 * DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

ServiceReference<DiceServService> DiceServ("DiceServService", "DiceServ");

/** LIST command
 *
 * This will allow Services Operators to list all the nicknames/users or channels (either registered or not) matching a mask that are either ignored, allowed,
 * or both.
 */
class DSListCommand : public Command
{
public:
	DSListCommand(Module *creator) : Command(creator, "diceserv/list", 3, 4)
	{
		this->SetDesc(Anope::Format("Gives list of %s access", Config->GetClient("DiceServ")->nick.c_str()));
		this->SetSyntax(_("{IGNORE|ALLOW|ALL} \037what\037 \037pattern\037 [{REG|UNREG}]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		Anope::string showtype = params[0], what = params[1], pattern = params[2];
		// Ignore type must be one of the following, otherwise we stop processing
		enum
		{
			SHOW_IGNORED,
			SHOW_ALLOWED,
			SHOW_ALL
		} show;
		if (showtype.equals_ci("IGNORE"))
			show = SHOW_IGNORED;
		else if (showtype.equals_ci("ALLOW"))
			show = SHOW_ALLOWED;
		else if (showtype.equals_ci("ALL"))
			show = SHOW_ALL;
		else
		{
			this->OnSyntaxError(source, "");
			return;
		}
		// The type of entries to find must be one of the following, otherwise we stop processing
		if (!what.equals_ci("CHANNELS") && !what.equals_ci("NICKS"))
		{
			this->OnSyntaxError(source, "");
			return;
		}
		enum
		{
			REG_SHOW_ALL,
			REG_SHOW_REG,
			REG_SHOW_UNREG
		} reg = REG_SHOW_ALL;
		// If the optional regtype argument is given, it must be one of the following, otherwise we stop processing
		if (params.size() > 3)
		{
			if (params[3].equals_ci("REG"))
				reg = REG_SHOW_REG;
			if (params[3].equals_ci("UNREG"))
				reg = REG_SHOW_UNREG;
			else
			{
				this->OnSyntaxError(source, "");
				return;
			}
		}
		// Show the header
		source.Reply(_("List of \037%s %s\037 entries matching \002%s\002%s:"),
			show == SHOW_IGNORED ? _("ignored") : (show == SHOW_ALLOWED ? _("allowed") : _("all")), what.equals_ci("CHANNELS") ? _("channels") : _("nicks"),
			pattern.c_str(), reg == REG_SHOW_ALL ? "" : (reg == REG_SHOW_REG ? _(" (registered only)") : _(" (unregistered only)")));
		bool display;
		int shown = 0;
		// If we are to show channels, we do so
		if (what.equals_ci("CHANNELS"))
		{
			// If no regtype argument is given or we want to look only at unregistered channels, we process the channels list
			if (reg == REG_SHOW_UNREG || reg == REG_SHOW_ALL)
				for (channel_map::const_iterator it = ChannelList.begin(), it_end = ChannelList.end(); it != it_end; ++it)
				{
					Channel *c = it->second;
					// Skip the channel if it's registered
					if (c->ci)
						continue;
					display = false;
					// We will only show the channel if it was part of the ignore type request
					bool diceserv_ignore = DiceServ->IsIgnored(c);
					if (diceserv_ignore && (show == SHOW_IGNORED || show == SHOW_ALL))
						display = true;
					else if (!diceserv_ignore && (show == SHOW_ALLOWED || show == SHOW_ALL))
						display = true;
					// If we are to display the channel, make sure it matches the pattern first, and we haven't shown too many entries already
					if (display && (pattern.equals_ci(c->name) || Anope::Match(c->name, pattern)) && ++shown <= 100)
					{
						if (reg == REG_SHOW_ALL)
							source.Reply("   %-20s  %-5s  %s", c->name.c_str(), _("Unreg"), diceserv_ignore ? _("Ignored") : _("Allowed"));
						else
							source.Reply("   %-20s  %s", c->name.c_str(), diceserv_ignore ? _("Ignored") : _("Allowed"));
					}
				}
			// If no regtype argument is given or we want to look only at registered channels, we process the ChanServ list
			if (reg == REG_SHOW_REG || reg == REG_SHOW_ALL)
				for (registered_channel_map::const_iterator it = RegisteredChannelList->begin(), it_end = RegisteredChannelList->end(); it != it_end; ++it)
				{
					ChannelInfo *ci = it->second;
					// Skip the channel if it's suspended
					if (ci->HasExt("SUSPENDED"))
						continue;
					display = false;
					// We will only show the channel if it was part of the ignore type request
					bool diceserv_ignore = DiceServ->IsIgnored(ci);
					if (diceserv_ignore && (show == SHOW_IGNORED || show == SHOW_ALL))
						display = true;
					else if (!diceserv_ignore && (show == SHOW_ALLOWED || show == SHOW_ALL))
						display = true;
					// If we are to display the channel, make sure it matches the pattern first, and we haven't shown too many entries already
					if (display && (pattern.equals_ci(ci->name) || Anope::Match(ci->name, pattern)) && ++shown <= 100)
					{
						if (reg == REG_SHOW_ALL)
							source.Reply("   %-20s  %-5s  %s", ci->name.c_str(), _("Reg"), diceserv_ignore ? _("Ignored") : _("Allowed"));
						else
							source.Reply("   %-20s  %s", ci->name.c_str(), diceserv_ignore ? _("Ignored") : _("Allowed"));
					}
				}
		}
		// Otherwise, we are to show nicks
		else
		{
			// If no regtype argument is given or we want to look only at unregistered nicks, we process the users list
			if (reg == REG_SHOW_UNREG || reg == REG_SHOW_ALL)
				for (user_map::const_iterator it = UserListByNick.begin(), it_end = UserListByNick.end(); it != it_end; ++it)
				{
					// Check if the nick is a bot, and skip it if it is
					BotInfo *bot = BotInfo::Find(it->second->nick);
					if (bot)
						continue;
					User *nu = it->second;
					// Skip the nick if it's registered
					if (nu->Account())
						continue;
					display = false;
					// We will only show the channel if it was part of the ignore type request
					bool diceserv_ignore = DiceServ->IsIgnored(nu);
					if (diceserv_ignore && (show == SHOW_IGNORED || show == SHOW_ALL))
						display = true;
					else if (!diceserv_ignore && (show == SHOW_ALLOWED || show == SHOW_ALL))
						display = true;
					// If we are to display the nick, make sure it matches the pattern first, and we haven't shown too many entries already
					if (display && (pattern.equals_ci(nu->nick) || Anope::Match(nu->nick, pattern)) && ++shown <= 100)
					{
						if (reg == REG_SHOW_ALL)
							source.Reply("   %-20s  %-5s  %s", nu->nick.c_str(), _("Unreg"), diceserv_ignore ? _("Ignored") : _("Allowed"));
						else
							source.Reply("   %-20s  %s", nu->nick.c_str(), diceserv_ignore ? _("Ignored") : _("Allowed"));
					}
				}
			// If no regtype argument is given or we want to look only at registered nicks, we process the NickServ list
			if (reg == REG_SHOW_REG || reg == REG_SHOW_ALL)
				for (nickalias_map::const_iterator it = NickAliasList->begin(), it_end = NickAliasList->end(); it != it_end; ++it)
				{
					NickAlias *na = it->second;
					// Skip the nick if it's suspended
					if (na->nc && na->nc->HasExt("SUSPENDED"))
						continue;
					display = false;
					// We will only show the channel if it was part of the ignore type request
					bool diceserv_ignore = DiceServ->IsIgnored(na->nc);
					if (diceserv_ignore && (show == SHOW_IGNORED || show == SHOW_ALL))
						display = true;
					else if (!diceserv_ignore && (show == SHOW_ALLOWED || show == SHOW_ALL))
						display = true;
					// If we are to display the nick, make sure it matches the pattern first, and we haven't shown too many entries already
					if (display && (pattern.equals_ci(na->nick) || Anope::Match(na->nick, pattern, false)) && ++shown <= 100)
					{
						if (reg == REG_SHOW_ALL)
							source.Reply("   %-20s  %-5s  %s", na->nick.c_str(), _("Reg"), diceserv_ignore ? _("Ignored") : _("Allowed"));
						else
							source.Reply("   %-20s  %s", na->nick.c_str(), diceserv_ignore ? _("Ignored") : _("Allowed"));
					}
				}
		}
		// Show the footer
		source.Reply(_("End of list - %d/%d matches shown."), shown > 100 ? 100 : shown, shown);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("This will display a list of channels or nicks depending on\n"
			"what options you give. The first parameter is what access\n"
			"types to show, either all ignored, all allowed, or just all.\n"
			" \n"
			"\037what\037 MUST be one of the following:\n"
			" \n"
			"    CHANNELS       Shows channels based on the display type\n"
			"    NICKS          Shows nicks based on the display type\n"
			" \n"
			"\037pattern\037 is the mask you want to view.\n"
			" \n"
			"The final parameter is optional, if given, it will allow you\n"
			"to choose if only registered or unregistered entries are\n"
			"shown on the list."));
		return true;
	}
};

class DSList : public Module
{
	DSListCommand cmd;

public:
	DSList(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServ)
			throw ModuleException("No interface for DiceServ");
	}
};

MODULE_INIT(DSList)
