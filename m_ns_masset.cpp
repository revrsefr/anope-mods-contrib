/*--------------------------------------------------------------
* Name:    ns_massset
* Author:  LEthaLity 'Lee' <lethality@anope.org>, PeGaSuS
* Date:    4th November 2023
* Version: 0.3
* Updated for 2.1.x 24th June 2024 by reverse
* --------------------------------------------------------------
* This module provides the ability for a Services Oper with the
* nickserv/massset privilege to change NickServ settings for 
* all registered users.
* This module is usually only loaded when needed, to undo something
* you set incorrectly in the configs default options, and wish to
* put right. 
* Note that using this module some time down the line, undoing
* users desired settings, may "annoy" some.
* --------------------------------------------------------------
* This module provides the following command:
* /msg NickServ MASSSET <option> <param>
* Option can be one of PROTECT (alias: KILL), MESSAGE, AUTOOP, PRIVATE, and CHANSTATS
* Available params:
*  - PROTECT: ON, OFF, or a delay in seconds
*  - KILL (alias): ON, OFF, QUICK (= min protect delay), IMMED (= 0s if allowed)
*  - MESSAGE/AUTOOP/PRIVATE/CHANSTATS: ON or OFF
* --------------------------------------------------------------
* Configuration: nickserv.conf
module { name = "ns_massset" }
command { service = "NickServ"; name = "MASSSET"; command = "nickserv/massset"; permission = "nickserv/massset"; }
* --------------------------------------------------------------
*/

#include "module.h"

class CommandNSMassSet : public Command
{
public:
    CommandNSMassSet(Module *creator) : Command(creator, "nickserv/massset", 2, 2)
    {
        this->SetDesc(_("Set options for ALL registered users"));
        this->SetSyntax(_("\037option\037 \037parameter\037"));
    }

    void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
    {
        const Anope::string &option = params[0]; // Our SET option, Kill, AutoOP, etc
        const Anope::string &param = params[1];  // Setting for the above option - on/off
        int count = 0;

        if (Anope::ReadOnly)
        {
            source.Reply(READ_ONLY_MODE);
            return;
        }
        else if (!option[0] || !param[0])
        {
            this->SendSyntax(source);
            source.Reply(_("Type \002/msg %s HELP MASSSET\002 for more information."), source.service->nick.c_str());
        }
        else if (option.equals_ci("KILL") || option.equals_ci("PROTECT"))
        {
            if (Config->GetModule("nickserv").Get<bool>("nonicknameownership"))
            {
                source.Reply(_("This command may not be used on this network because nickname ownership is disabled."));
                return;
            }

            auto &block = Config->GetModule("nickserv");
            const auto minprotect = block.Get<time_t>("minprotect", "10s");
            const auto maxprotect = block.Get<time_t>("maxprotect", "10m");

            auto ApplyProtect = [&](NickCore *nc, const Anope::string &why, bool enable, const time_t *delay)
            {
                if (enable)
                {
                    nc->Extend<bool>("PROTECT");
                    if (delay)
                        nc->Extend<time_t>("PROTECT_AFTER", *delay);
                    else
                        nc->Shrink<time_t>("PROTECT_AFTER");
                }
                else
                {
                    nc->Shrink<bool>("PROTECT");
                    nc->Shrink<time_t>("PROTECT_AFTER");
                }
            };

            if (param.equals_ci("ON"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    ApplyProtect(nc, "enable", true, nullptr);
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable protection for " << count << " users";
                source.Reply(_("Successfully enabled \002protection\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("OFF"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    ApplyProtect(nc, "disable", false, nullptr);
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to disable protection for " << count << " users";
                source.Reply(_("Successfully disabled \002protection\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("QUICK") && option.equals_ci("KILL"))
            {
                const time_t delay = minprotect;
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    ApplyProtect(nc, "enable (quick)", true, &delay);
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable protection (quick) for " << count << " users";
                source.Reply(_("Successfully enabled \002quick protection\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("IMMED") && option.equals_ci("KILL"))
            {
                if (!Config->GetModule(this->owner).Get<bool>("allowkillimmed"))
                {
                    source.Reply(_("The \002IMMED\002 option is not available on this network."));
                    return;
                }

                const time_t delay = 0;
                if (delay < minprotect || delay > maxprotect)
                {
                    source.Reply(_("The \002IMMED\002 option is not available because protection delay must be between %s and %s."),
                        Anope::Duration(minprotect, source.GetAccount()).c_str(),
                        Anope::Duration(maxprotect, source.GetAccount()).c_str());
                    return;
                }

                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    ApplyProtect(nc, "enable (immed)", true, &delay);
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable protection (immed) for " << count << " users";
                source.Reply(_("Successfully enabled \002immediate protection\002 for \002%d\002 users."), count);
            }
            else
            {
                auto iparam = Anope::TryConvert<time_t>(param);
                if (!iparam)
                {
                    source.Reply(_("Syntax: \002MASSSET PROTECT \037on|off|delay\037\002"));
                    return;
                }

                if (*iparam < minprotect || *iparam > maxprotect)
                {
                    source.Reply(_("Protection delay must be between %s and %s."),
                        Anope::Duration(minprotect, source.GetAccount()).c_str(),
                        Anope::Duration(maxprotect, source.GetAccount()).c_str());
                    return;
                }

                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    ApplyProtect(nc, "enable (delayed)", true, &*iparam);
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable protection (delay " << *iparam << "s) for " << count << " users";
                source.Reply(_("Successfully enabled \002protection\002 (delay %lu seconds) for \002%d\002 users."), *iparam, count);
            }
        }
        else if (option.equals_ci("AUTOOP"))
        {
            if (param.equals_ci("ON"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Extend<bool>("AUTOOP");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable autoop for \002" << count << " users";
                source.Reply(_("Successfully enabled \002autoop\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("OFF"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Shrink<bool>("AUTOOP");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to disable autoop for \002" << count << " users";
                source.Reply(_("Successfully disabled \002autoop\002 for \002%d\002 users."), count);
            }
            else
                source.Reply(_("Syntax: \002MASSSET AUTOOP \037on|off\037\002"));
        }
        else if (option.equals_ci("PRIVATE"))
        {
            if (param.equals_ci("ON"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Extend<bool>("NS_PRIVATE");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable private for \002" << count << " users";
                source.Reply(_("Successfully enabled \002private\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("OFF"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Shrink<bool>("NS_PRIVATE");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to disable private for \002" << count << " users";
                source.Reply(_("Successfully disabled \002private\002 for \002%d\002 users."), count);
            }
            else
                source.Reply(_("Syntax: \002MASSSET PRIVATE \037on|off\037\002"));
        }
        else if (option.equals_ci("MESSAGE"))
        {
            if (!Config->GetBlock("options").Get<bool>("useprivmsg"))
            {
                source.Reply(_("useprivmsg is disabled on this network."));
                return;
            }

            if (param.equals_ci("ON"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Extend<bool>("MSG");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable privmsg replies for \002" << count << " users";
                source.Reply(_("Successfully enabled \002privmsg replies\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("OFF"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    nc->Shrink<bool>("MSG");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable notice replies for \002" << count << " users";
                source.Reply(_("Successfully enabled \002notice replies\002 for \002%d\002 users."), count);
            }
            else
                source.Reply(_("Syntax: \002MASSSET MESSAGE \037on|off\037\002"));
        }
        else if (option.equals_ci("CHANSTATS"))
        {
            const bool has_chanstats = !!ModuleManager::FindModule("chanstats");
            const bool has_chanstats_plus = !!ModuleManager::FindModule("chanstats_plus");
            if (!has_chanstats && !has_chanstats_plus)
            {
                source.Reply(_("Chanstats module is not loaded"));
                return;
            }

            if (param.equals_ci("ON"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    if (has_chanstats)
                        nc->Extend<bool>("NS_STATS");
                    if (has_chanstats_plus)
                        nc->Extend<bool>("NS_STATS_PLUS");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to enable chanstats for \002" << count << " users";
                source.Reply(_("Successfully enabled \002chanstats\002 for \002%d\002 users."), count);
            }
            else if (param.equals_ci("OFF"))
            {
                for (nickcore_map::const_iterator it = NickCoreList->begin(), it_end = NickCoreList->end(); it != it_end; ++it)
                {
                    NickCore *nc = it->second;
                    count++;

                    EventReturn MOD_RESULT;
                    FOREACH_RESULT(OnSetNickOption, MOD_RESULT, (source, this, nc, param));
                    if (MOD_RESULT == EVENT_STOP)
                        return;

                    if (has_chanstats)
                        nc->Shrink<bool>("NS_STATS");
                    if (has_chanstats_plus)
                        nc->Shrink<bool>("NS_STATS_PLUS");
                }
                Log(source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, this) << "to disable chanstats for \002" << count << " users";
                source.Reply(_("Successfully disabled \002chanstats\002 for \002%d\002 users."), count);
            }
            else
                source.Reply(_("Syntax: \002MASSSET CHANSTATS \037on|off\037\002"));
        }
        else {
            source.Reply(_("Unknown \002MASSSET\002 option \002%s\002\n"
                "\002/msg %s HELP MASSSET\002 for more information"), option.c_str(), source.service->nick.c_str());
        }
        return;
    }

    bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
    {
        this->SendSyntax(source);
        source.Reply(" ");
        source.Reply(_("Allows options to be mass-set for ALL registered users\n"
            "	PROTECT    Change NickServ's protection setting to on/off/delay\n"
            "              for all registered nicks. (Alias: KILL)\n"
            "	AUTOOP	   Turns NickServ's autoop features on/off\n"
            "              for all registered nicks.\n"
            "	PRIVATE    Turns NickServ's private feature on/off\n"
            "              for all registered nicks.\n"
            "	MESSAGE	   Set whether NickServ privmsg or notices,\n"
            "              for all registered nicks.\n"
            "	CHANSTATS	Turns Chanstats statistics on/off\n"
            "              for all registered nicks.\n"));
        return true;
    }

};


class NSMassSet : public Module
{
    CommandNSMassSet commandnsmassset;

public:
    NSMassSet(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD),
        commandnsmassset(this)
    {
        if (Anope::VersionMajor() != 2 || Anope::VersionMinor() != 1)
            throw ModuleException("Requires version 2.1.x of Anope.");

        this->SetAuthor("LEthaLity, PeGaSuS");
        this->SetVersion("0.3");
    }
};

MODULE_INIT(NSMassSet)