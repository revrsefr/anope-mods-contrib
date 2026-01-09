/* ------------------------------------------------------------------------------------------------
 *  Name:		| OS_FORCEID
 *  Author:		| DukePyrolator & Andromeda
 *  Version:		| 1.0.0 (Anope 2.0.0)
 *  Date:		| April 8th, 2014
 * ------------------------------------------------------------------------------------------------
 *  Original concept by n00bie
 *  Original code created by DukePyrolator for Anope 2.0.0
 *  Modified and enhanced by Andromeda with improved functionality
 * ------------------------------------------------------------------------------------------------
 *  Description:
 *  - Allows an operator of sufficient access to forcefully identify a user to a matching nick.
 *
 *  Tested:
 *  - Unreal 3.2.10.2, Anope 2.0.0
 * ------------------------------------------------------------------------------------------------
 * Default command usage:
 * - /msg OperServ FORCEID <nick>
 * ------------------------------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 1, or (at your option) any later version.
 * ------------------------------------------------------------------------------------------------
 *  To use, place following lines in your services.conf
 *
 *  module { name = "os_forceid" }
 *  command { service = "OperServ"; name = "FORCEID"; command = "operserv/forceid"; permission = "operserv/forceid"; }
 *
 */


#include "module.h"

class CommandOSForceID : public Command
{
 public:
	CommandOSForceID(Module *creator, const Anope::string &sname = "operserv/forceid") : Command(creator, sname, 1, 1)
	{
		this->SetDesc(_("Forcefully identifies a user to their nick"));
		this->SetSyntax(_("\037<nick>\037"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		User *u = User::Find(params[0], true);
		if (!u)
		{
			source.Reply(_("Nick %s is not online."), params[0].c_str());
			return;
		}
		NickAlias *na = NickAlias::Find(u->nick);
		if (!na)
		{
			source.Reply(_("Nick %s is not registered."), params[0].c_str());
			return;
		}
		u->Identify(na);
		source.Reply(_("Done."));
		Log(LOG_ADMIN, source, this) << "on " << params[0];
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Allows a Services Operator to forcefully identify a user\n"
				"to a matching nick."));

		return true;
	}
};

class OSForceID : public Module
{
	CommandOSForceID commandosforceid;
public:
	OSForceID(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD),
		commandosforceid(this)
	{
		if(Anope::VersionMajor() != 2 || (Anope::VersionMinor() != 0 && Anope::VersionMinor() != 1))
			throw ModuleException("Requires version 2.0.x or 2.1.x of Anope.");
		this->SetAuthor("Jens Voss <DukePyrolator@anope.org>");
		this->SetVersion("1.0.1");
	}
};

MODULE_INIT(OSForceID)
