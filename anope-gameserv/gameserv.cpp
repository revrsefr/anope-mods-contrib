/* GameServ core functions
 *
 * (C) 2019 Michael Hazell
 * Contact me at michaelhazell@hotmail.com
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "module.h"

class GameServCore : public Module
{
	Reference<BotInfo> GameServ;

 public:
	GameServCore(const Anope::string& modname, const Anope::string& creator) : Module(modname, creator, PSEUDOCLIENT | THIRD)
	{
		if(!IRCD)
		{
			throw ModuleException("IRCd protocol module not loaded");
		}
	}

	void OnReload(Configuration::Conf& conf) override
	{
		const Anope::string& gsnick = conf.GetModule(this).Get<const Anope::string>("client");

		if (gsnick.empty())
		{
			throw ConfigException(Module::name + ": <client> must be defined");
		}

		BotInfo* bi = BotInfo::Find(gsnick, true);
		if (!bi)
		{
			throw ConfigException(Module::name + ": no bot named " + gsnick);
		}

		GameServ = bi;
	}

	EventReturn OnPreHelp(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		if (!params.empty() || source.c || source.service != *GameServ)
		{
			return EVENT_CONTINUE;
		}
		source.Reply(_("%s commands:"), GameServ->nick.c_str());
		return EVENT_CONTINUE;
	}
};

MODULE_INIT(GameServCore)
