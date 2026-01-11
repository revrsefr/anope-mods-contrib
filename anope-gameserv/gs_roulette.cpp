/* GameServ: Roulette
 *
 * (C) 2019 Michael Hazell
 * Contact me at michaelhazell@hotmail.com
 *
 * You may use this module under the terms of docs/LICENSE, in the Anope source directory
 *
 * Configuration:
 * module { name = "gs_roulette"; }
 * command { name = "ROULETTE"; service = "GameServ"; command = "gameserv/roulette"; }
 * fantasy { name = "ROULETTE"; command = "gameserv/roulette"; prepend_channel = true; }
 */

#include "module.h"

class CommandGSRoulette : public Command
{
 public:
	CommandGSRoulette(Module* creator, const Anope::string& sname = "gameserv/roulette") : Command(creator, sname, 1, 2)
	{
		this->SetDesc(_("Perform a round of roulette (live or die)"));
		this->SetSyntax("\037#channel\037");
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		const Anope::string& chan = params[0];
		ChannelInfo* ci = ChannelInfo::Find(chan);
		BotInfo *gs = Config->GetClient("GameServ");

		if (!ci)
		{
			source.Reply("That channel (\002%s\002) is not registered.", chan.c_str());
			return;
		}

		if (!ci->bi)
		{
			source.Reply("I can't play roulette in \002%s\002 because no BotServ bot is assigned to it.", ci->name.c_str());
			return;
		}

		if (!ci->c || !ci->c->FindUser(ci->bi))
		{
			source.Reply("I can't speak in \002%s\002 right now (the BotServ bot isn't in the channel).", ci->name.c_str());
			return;
		}

		if (!gs)
		{
			source.Reply("GameServ isn't available right now.");
			return;
		}

		User* u = source.GetUser();
		if (!u)
		{
			source.Reply("This command can only be used by a user.");
			return;
		}
		int random = rand() % 1000;
		if(random > 499)
		{
			u->Kill(gs, "BANG!");
		}
		else
		{
			IRCD->SendPrivmsg(gs, ci->name, "*click*");
		}
	}

	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Perform a round of roulette. The gun will click or go off with a bang!"));
		return true;
	}
};

class GSRoulette : public Module
{
	CommandGSRoulette commandgsroulette;

 public:
	GSRoulette(const Anope::string& modname, const Anope::string& creator) : Module(modname, creator, THIRD),
		commandgsroulette(this)
	{
		this->SetAuthor("Techman");
		this->SetVersion("0.1");

		if (!ModuleManager::FindModule("gameserv"))
		{
			throw ModuleException("This module requires the GameServ core module to be loaded in order to function.");
		}
	}
};

MODULE_INIT(GSRoulette)
