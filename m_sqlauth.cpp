/*
 * 2025 Jean "reverse" Chevronnet
 * 
 * Module for Anope IRC Services v2.1, lets users authenticate with
 * credentials stored in a pre-existing SQL server instead of the internal
 * database using bcrypt/crypt_blowfish from Anope.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "module.h"
#include "modules/sql.h"
#include "modules/encryption.h"
#include "bcrypt/crypt_blowfish.c"

#include <random>
#include <mutex>
#include <map>
#include <chrono>

static Module *me;

struct FailedAttemptInfo
{
    int attempts;
    time_t last_attempt_time;
};

static std::map<Anope::string, FailedAttemptInfo> failed_attempts;
static std::mutex failed_attempts_mutex;

class SQLAuthResult final : public SQL::Interface
{
    Reference<User> user;
    IdentifyRequest *req;
    Anope::string currPass;
    int max_attempts;
    Anope::string kill_message_base;

public:
    SQLAuthResult(User *u, const Anope::string &cp, IdentifyRequest *r, int attempts, const Anope::string &message_base)
        : SQL::Interface(me), user(u), req(r), max_attempts(attempts), kill_message_base(message_base)
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
            Log(LOG_COMMAND) << "[sql_auth]: User @" << req->GetAccount() << "@ NOT found";
            HandleFailedAttempt();
            delete this;
            return;
        }

        Log(LOG_COMMAND) << "[sql_auth]: User @" << req->GetAccount() << "@ found";
        Log(LOG_COMMAND) << "[sql_auth]: Authentication for user @" << req->GetAccount() << "@ processing...";

        Anope::string hash;
        Anope::string email;

        try
        {
            hash = r.Get(0, "password");
            email = r.Get(0, "email");
        }
        catch (const SQL::Exception &) { }

        // Normalize bcrypt hash prefix
        if (hash.find("bcrypt$$") == 0)
        {
            hash = "$" + hash.substr(8);
        }

        char hash_output[64];
        if (!_crypt_blowfish_rn(currPass.c_str(), hash.c_str(), hash_output, sizeof(hash_output)))
        {
            Log(LOG_COMMAND) << "[sql_auth]: Bcrypt comparison failed";
            HandleFailedAttempt();
            delete this;
            return;
        }

        if (hash != hash_output)
        {
            Log(LOG_COMMAND) << "[sql_auth]: ERROR: hash NOT EQUAL pass";
            HandleFailedAttempt();
            delete this;
            return;
        }

        Log(LOG_COMMAND) << "[sql_auth]: User @" << req->GetAccount() << "@ LOGGED IN";

        {
            std::lock_guard<std::mutex> lock(failed_attempts_mutex);
            failed_attempts.erase(req->GetAccount());
        }

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
        delete this;
    }

    void OnError(const SQL::Result &r) override
    {
        Log(this->owner) << "[sql_auth]: Error when executing query " << r.GetQuery().query << ": " << r.GetError();
        HandleFailedAttempt();
        delete this;
    }

private:
    void HandleFailedAttempt()
    {
        if (!user)
            return;

        Anope::string account = req->GetAccount();
        {
            std::lock_guard<std::mutex> lock(failed_attempts_mutex);
            failed_attempts[account].attempts++;
            failed_attempts[account].last_attempt_time = Anope::CurTime;

            if (failed_attempts[account].attempts >= max_attempts)
            {
                Log(LOG_COMMAND) << "[sql_auth]: User " << account << " exceeded maximum authentication attempts.";

                // Generate a random ID using std::mt19937
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<unsigned long long> dist(100000, 999999);

                unsigned long long id = static_cast<unsigned long long>(Anope::CurTime) * 100000 + dist(gen);
                Anope::string idString = std::to_string(id);

                // Ensure the kill message fits within 512 characters
                size_t maxBaseLength = 512 - idString.length();
                Anope::string kill_message = kill_message_base;
                if (kill_message.length() > maxBaseLength)
                    kill_message = kill_message.substr(0, maxBaseLength);

                kill_message += idString;

                MessageSource source = MessageSource(Config->GetClient("NickServ"));
                user->Kill(source, kill_message);

                failed_attempts.erase(account);
            }
        }
    }
};

class ModuleSQLAuth final : public Module
{
    Anope::string engine;
    Anope::string query;
    Anope::string disable_reason, disable_email_reason;
    Anope::string kill_message_base;
    int max_attempts;

    ServiceReference<SQL::Provider> SQL;

public:
    ModuleSQLAuth(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA | VENDOR)
    {
        me = this;
    }

    void OnReload(Configuration::Conf *conf) override
    {
        Configuration::Block *config = conf->GetModule(this);
        this->engine = config->Get<const Anope::string>("engine");
        this->query = config->Get<const Anope::string>("query");
        this->disable_reason = config->Get<const Anope::string>("disable_reason");
        this->disable_email_reason = config->Get<Anope::string>("disable_email_reason");
        this->kill_message_base = config->Get<const Anope::string>("kill_message_base", "Error: Too many failed login attempts. Please try again. ID:");
        this->max_attempts = config->Get<int>("max_attempts", 3);

        this->SQL = ServiceReference<SQL::Provider>("SQL::Provider", this->engine);
    }

    void OnCheckAuthentication(User *u, IdentifyRequest *req) override
    {
        if (!this->SQL)
        {
            Log(this) << "[sql_auth]: Unable to find SQL engine";
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

        this->SQL->Run(new SQLAuthResult(u, req->GetPassword(), req, this->max_attempts, this->kill_message_base), q);
        Log(LOG_COMMAND) << "[sql_auth]: Checking authentication for " << req->GetAccount();
    }

    void OnPreNickExpire(NickAlias *na, bool &expire) override
    {
        if (na->nick == na->nc->display && na->nc->aliases->size() > 1)
            expire = false;
    }
};

MODULE_INIT(ModuleSQLAuth)
