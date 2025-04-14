/*
m_sqlauth.cpp
2025-04-10 Jean "reverse" Chevronnet
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
#include "modules/encryption.h"

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
    Anope::string password_field;
    Anope::string email_field;
    Anope::string username_field;
    Anope::string table_name;
    Anope::string disable_reason, disable_email_reason;
    Anope::string kill_message;
    unsigned max_attempts;
    Anope::string query;

    ServiceReference<SQL::Provider> SQL;

    // Store failed login attempts
    std::map<Anope::string, std::pair<time_t, unsigned>> failed_logins;

public:
    ModuleSQLAuth(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA | VENDOR)
    {
        me = this;
        this->SetAuthor("revrsefrJean \"reverse\" Chevronnet");
        this->SetVersion("1.1.0");
    }

    void OnReload(Configuration::Conf &conf) override
    {
        Configuration::Block &config = conf.GetModule(this);
        this->engine = config.Get<const Anope::string>("engine");
        this->password_field = config.Get<const Anope::string>("password_field", "password");
        this->email_field = config.Get<const Anope::string>("email_field", "email");
        this->username_field = config.Get<const Anope::string>("username_field", "username");
        this->table_name = config.Get<const Anope::string>("table_name", "users");
        this->disable_reason = config.Get<const Anope::string>("disable_reason");
        this->disable_email_reason = config.Get<const Anope::string>("disable_email_reason");
        this->kill_message = config.Get<const Anope::string>("kill_message", "Error: Too many failed login attempts. Please try again later. ID:");
        this->max_attempts = config.Get<unsigned>("max_attempts", 5);
        
        // Get the custom query from config if available, otherwise construct it from fields
        Anope::string config_query = config.Get<const Anope::string>("query", "");
        if (!config_query.empty())
        {
            this->query = config_query;
            Log(this) << "[sql_auth]: Using custom query from config: " << this->query;
        }
        else
        {
            // Build a default query using the configured field names
            this->query = "SELECT `" + this->password_field + "`, `" + this->email_field + 
                        "` FROM `" + this->table_name + "` WHERE `" + this->username_field + "` = :account";
            Log(this) << "[sql_auth]: Using default query: " << this->query;
        }

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

        // Check if user has too many failed login attempts
        if (u)
        {
            auto it = failed_logins.find(u->ip.addr());
            if (it != failed_logins.end())
            {
                auto &[last_time, attempts] = it->second;
                
                // Reset attempts if more than an hour has passed
                if (Anope::CurTime - last_time > 3600)
                {
                    attempts = 0;
                }
                else if (attempts >= this->max_attempts)
                {
                    // Generate a unique ID for tracking
                    Anope::string unique_id = Anope::Random(16);
                    Log(LOG_COMMAND) << "[sql_auth]: 🛑 Too many failed login attempts from " << u->ip.addr() << " ID: " << unique_id;
                    
                    BotInfo *NickServ = Config->GetClient("NickServ");
                    if (NickServ)
                    {
                        IRCD->SendKill(NickServ, u->GetUID(), this->kill_message + " " + unique_id);
                    }
                    
                    return;
                }
            }
        }

        // Build a SQL query directly with the account name substituted into it
        // Since we control the account name and the query construction, this is safe
        Anope::string actualQuery = this->query;
        
        // Replace @a@ with the actual account name
        if (actualQuery.find("@a@") != Anope::string::npos)
        {
            actualQuery = actualQuery.replace_all_cs("@a@", "'" + req->GetAccount().replace_all_cs("'", "''") + "'");
        }
        
        SQL::Query sqlQuery(actualQuery);
        
        // Execute the query
        this->SQL->Run(new SQLAuthResult(u, req->GetPassword(), req), sqlQuery);
        Log(LOG_COMMAND) << "[sql_auth]: 🔎 Checking authentication for " << req->GetAccount();
    }

    void OnLoginFail(User *u)
    {
        if (u)
        {
            auto &record = failed_logins[u->ip.addr()];
            record.first = Anope::CurTime;
            record.second++;
            
            Log(LOG_DEBUG) << "[sql_auth]: Failed login attempt from " << u->ip.addr() 
                          << " - " << record.second << "/" << this->max_attempts;
        }
    }

    void OnPreNickExpire(NickAlias *na, bool &expire) override
    {
        // Prevent expiration of nicks with active accounts
        if (na->nick == na->nc->display && na->nc->aliases->size() > 1)
            expire = false;
    }
    
    void OnUserQuit(User *u, const Anope::string &msg) override
    {
        // Clean up failed login tracking when a user quits
        if (u && !u->ip.addr().empty())
        {
            auto it = failed_logins.find(u->ip.addr());
            if (it != failed_logins.end() && it->second.second < this->max_attempts)
            {
                failed_logins.erase(it);
            }
        }
    }
    
    // Clean up old records periodically
    void OnRestart() override
    {
        failed_logins.clear();
    }
    
    // Periodic cleanup of expired records
    void OnBackupDatabase()
    {
        time_t current = Anope::CurTime;
        for (auto it = failed_logins.begin(); it != failed_logins.end();)
        {
            if (current - it->second.first > 3600)
            {
                it = failed_logins.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
};

MODULE_INIT(ModuleSQLAuth)

