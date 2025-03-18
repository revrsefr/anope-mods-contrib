/*
m_sqlauth.cpp
2025 Jean "reverse" Chevronnet
Module for Anope IRC Services v2.1, lets users authenticate with
credentials stored in a pre-existing SQL server instead of the internal
Anope database.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "module.h"
#include "modules/sql.h"
#include "modules/encryption.h" // Use Anope's bcrypt module

static Module *me;

class SQLAuthResult final : public SQL::Interface
{
    Reference<User> user;
    IdentifyRequest *req;
    Anope::string currPass;

public:
    SQLAuthResult(User *u, const Anope::string &cp, IdentifyRequest *r) : SQL::Interface(me), user(u), req(r)
    {
        req->Hold(me);
        this->currPass = cp;
    }

    ~SQLAuthResult()
    {
        req->Release(me);
    }

    void OnResult(const SQL::Result &r) override
    {
        if (r.Rows() == 0)
        {
            Log(LOG_COMMAND) << "[sql_auth]: ❌ User @" << req->GetAccount() << "@ NOT found in database!";
            delete this;
            return;
        }

        Log(LOG_COMMAND) << "[sql_auth]: ✅ User @" << req->GetAccount() << "@ found. Processing authentication...";

        Anope::string hash, email;

        try
        {
            hash = r.Get(0, "password");
            email = r.Get(0, "email");
        }
        catch (const SQL::Exception &e)
        {
            Log(LOG_COMMAND) << "[sql_auth]: ❌ SQL Exception: " << e.GetReason();
            delete this;
            return;
        }
        // Ensure bcrypt hash format is correct
        if (hash.find("bcrypt$$") == 0)
        {
            hash = hash.substr(8); // Remove "bcrypt$$" prefix
        }
        // Ensure we do not add "$2b$" twice
        if (hash.find("$2b$") != 0 && hash.find("2b$") == 0)
        {
            hash = "$" + hash; // Ensure only one "$2b$" prefix
        }
        // Use Anope's bcrypt provider for password verification
        ServiceReference<Encryption::Provider> bcrypt_provider("Encryption::Provider", "bcrypt");
        if (!bcrypt_provider)
        {
            Log(LOG_COMMAND) << "[sql_auth]: ❌ ERROR: Bcrypt provider not found!";
            delete this;
            return;
        }
        if (bcrypt_provider->Compare(hash.c_str(), currPass.c_str()))
        {
            Log(LOG_COMMAND) << "[sql_auth]: ✅ SUCCESS: User @" << req->GetAccount() << "@ LOGGED IN!";
            
            NickAlias *na = NickAlias::Find(req->GetAccount());
            BotInfo *NickServ = Config->GetClient("NickServ");

            if (na == NULL)
            {
                na = new NickAlias(req->GetAccount(), new NickCore(req->GetAccount()));
                FOREACH_MOD(OnNickRegister, (user, na, ""));
                if (user && NickServ)
                    user->SendMessage(NickServ, _("Your account \002%s\002 has been confirmed."), na->nick.c_str());
            }

            if (!email.empty() && email != na->nc->email)
            {
                na->nc->email = email;
                if (user && NickServ)
                    user->SendMessage(NickServ, _("E-mail set to \002%s\002."), email.c_str());
            }

            req->Success(me);
        }
        else
        {
            Log(LOG_COMMAND) << "[sql_auth]: ❌ ERROR: Incorrect password for user " << req->GetAccount();
        }

        delete this;
    }

    void OnError(const SQL::Result &r) override
    {
        Log(this->owner) << "[sql_auth]: ❌ Error when executing query: " << r.GetError();
        delete this;
    }
};

class ModuleSQLAuth final : public Module
{
    Anope::string engine;
    Anope::string query;
    Anope::string disable_reason, disable_email_reason;

    ServiceReference<SQL::Provider> SQL;

public:
    ModuleSQLAuth(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA | VENDOR)
    {
        me = this;
    }

    void OnReload(Configuration::Conf &conf) override
    {
        Configuration::Block &config = conf.GetModule(this);
        this->engine = config.Get<const Anope::string>("engine");
        this->query = config.Get<const Anope::string>("query");
        this->disable_reason = config.Get<const Anope::string>("disable_reason");
        this->disable_email_reason = config.Get<const Anope::string>("disable_email_reason");

        this->SQL = ServiceReference<SQL::Provider>("SQL::Provider", this->engine);
    }

    EventReturn OnPreCommand(CommandSource &source, Command *command, std::vector<Anope::string> &params) override
    {
        if (!this->disable_reason.empty() && (command->name == "nickserv/register" || command->name == "nickserv/group"))
        {
            source.Reply(this->disable_reason);
            return EVENT_STOP;
        }

        if (!this->disable_email_reason.empty() && command->name == "nickserv/set/email")
        {
            source.Reply(this->disable_email_reason);
            return EVENT_STOP;
        }

        return EVENT_CONTINUE;
    }

    void OnCheckAuthentication(User *u, IdentifyRequest *req) override
    {
        if (!this->SQL)
        {
            Log(this) << "[sql_auth]: ❌ ERROR: Unable to find SQL engine!";
            return;
        }

        SQL::Query q(this->query);
        q.SetValue("a", req->GetAccount());
        q.SetValue("p", req->GetPassword());

        if (u)
        {
            q.SetValue("n", u->nick);
            q.SetValue("i", u->ip.addr());
        }
        else
        {
            q.SetValue("n", "");
            q.SetValue("i", "");
        }

        this->SQL->Run(new SQLAuthResult(u, req->GetPassword(), req), q);
        Log(LOG_COMMAND) << "[sql_auth]: 🔎 Checking authentication for " << req->GetAccount();
    }

    void OnPreNickExpire(NickAlias *na, bool &expire) override
    {
        // Prevent expiration of nicks with active accounts
        if (na->nick == na->nc->display && na->nc->aliases->size() > 1)
            expire = false;
    }
};

MODULE_INIT(ModuleSQLAuth)

