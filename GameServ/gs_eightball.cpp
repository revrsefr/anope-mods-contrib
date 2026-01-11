/* GameServ: 8 ball
 *
 * (C) 2019 Michael Hazell
 * Contact me at michaelhazell@hotmail.com
 *
 * You may use this module under the terms of docs/LICENSE, in the Anope source directory
 *
 * Configuration:
 * module { name = "gs_eightball"; }
 * command { name = "EIGHTBALL"; service = "GameServ"; command = "gameserv/eightball"; hide = true; }
 * command { name = "8BALL"; service = "GameServ"; command = "gameserv/eightball"; }
 * fantasy { name = "EIGHTBALL"; command = "gameserv/eightball"; prepend_channel = true; hide = true; }
 * fantasy { name = "8BALL"; command = "gameserv/eightball"; prepend_channel = true; }
 */

#include "module.h"

class CommandGSEightBall : public Command
{
 public:
	CommandGSEightBall(Module* creator, const Anope::string& sname = "gameserv/eightball") : Command(creator, sname, 1, 2)
	{
		this->SetDesc(_("Ask the 8-ball a question"));
		this->SetSyntax(_("\037#channel\037"));
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		std::vector<Anope::string> responses;
		responses.reserve(50);
		responses.push_back("Absolutely yes!");
		responses.push_back("Prospect looks hopeful.");
		responses.push_back("I'd like to think so.");
		responses.push_back("Yes, yes, yes, and yes again.");
		responses.push_back("Most likely.");
		responses.push_back("All signs point to yes.");
		responses.push_back("Yes.");
		responses.push_back("Without a doubt.");
		responses.push_back("Sometime in the near future.");
		responses.push_back("Of course!");
		responses.push_back("Definitely.");
		responses.push_back("Answer hazy.");
		responses.push_back("Prospect looks bleak.");
		responses.push_back("That's a question you should ask yourself.");
		responses.push_back("Maybe.");
		responses.push_back("That question is better remained unanswered.");
		responses.push_back("The stars would have to align for that to happen.");
		responses.push_back("No.");
		responses.push_back("Not even on a GOOD day.");
		responses.push_back("It would take a disturbed person to even ask.");
		responses.push_back("You wish.");
		responses.push_back("Not bloody likely.");
		responses.push_back("No way.");
		responses.push_back("Never.");
		responses.push_back("NO!");
		responses.push_back("Over my dead body.");
		responses.push_back("We won't go there.");
		responses.push_back("No chance at all!");
		responses.push_back("No chance in hell!");

		const Anope::string& chan = params[0];
		ChannelInfo* ci = ChannelInfo::Find(chan);
		BotInfo* gs = Config->GetClient("GameServ");
		if (!ci)
		{
			source.Reply("That channel (\002%s\002) is not registered.", chan.c_str());
			return;
		}

		if (!ci->bi)
		{
			source.Reply("I can't answer in \002%s\002 because no BotServ bot is assigned to it.", ci->name.c_str());
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

		int pick = rand() % responses.size();
		Anope::string text = responses[pick];
		IRCD->SendPrivmsg(gs, ci->name, text.c_str());
	}

	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Ask the 8-ball a question"));
		return true;
	}
};

class GSEightBall : public Module
{
	CommandGSEightBall commandgseightball;

 public:
	GSEightBall(const Anope::string& modname, const Anope::string& creator) : Module(modname, creator, THIRD),
		commandgseightball(this)
	{
		this->SetAuthor("Techman");
		this->SetVersion("0.1");

		if (!ModuleManager::FindModule("gameserv"))
		{
			throw ModuleException("This module requires the GameServ core module to be loaded in order to function.");
		}
	}
};

MODULE_INIT(GSEightBall)
