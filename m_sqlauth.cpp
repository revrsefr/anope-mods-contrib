/*
 * 2025 Jean "reverse" Chevronnet
 *
 * Module for Anope IRC Services v2.1, allows users to authenticate
 * with credentials stored in an external SQL database using bcrypt.
 */

 #include "module.h"
 #include "modules/sql.h"
 #include "modules/encryption.h"
 #include "bcrypt/crypt_blowfish.c"
 
 #include <random>
 #include <mutex>
 #include <map>
 
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
             Log(LOG_DEBUG) << "[m_sqlauth]: User " << req->GetAccount() << " not found.";
             HandleFailedAttempt();
             delete this;
             return;
         }
 
         Log(LOG_DEBUG) << "[m_sqlauth]: User " << req->GetAccount() << " found.";
 
         Anope::string hash, email;
         try
         {
             hash = r.Get(0, "password");
             email = r.Get(0, "email");
         }
         catch (const SQL::Exception &ex)
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: SQL Exception: " << ex.GetReason();
             HandleFailedAttempt();
             delete this;
             return;
         }
 
         if (hash.empty())
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: No password hash found.";
             HandleFailedAttempt();
             delete this;
             return;
         }
 
         // Normalize bcrypt hash prefix
         if (hash.find("bcrypt$$") == 0)
         {
             hash = "$" + hash.substr(8);
         }
 
         char hash_output[64];
         if (!_crypt_blowfish_rn(currPass.c_str(), hash.c_str(), hash_output, sizeof(hash_output)))
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: Bcrypt hashing failed";
             HandleFailedAttempt();
             delete this;
             return;
         }
 
         if (strcmp(hash_output, hash.c_str()) != 0)
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: ERROR: hash NOT EQUAL to expected value";
             HandleFailedAttempt();
             delete this;
             return;
         }
 
         Log(LOG_DEBUG) << "[m_sqlauth]: User " << req->GetAccount() << " successfully authenticated.";
 
         {
             std::lock_guard<std::mutex> lock(failed_attempts_mutex);
             failed_attempts.erase(req->GetAccount());
         }
 
         NickAlias *na = NickAlias::Find(req->GetAccount());
         BotInfo *NickServ = Config->GetClient("NickServ");
 
         if (!na)
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: Registering new user: " << req->GetAccount();
             na = new NickAlias(req->GetAccount(), new NickCore(req->GetAccount()));
             na->nc->aliases->push_back(na);
             na->nc->SetDisplay(na);  // FIXED: SetDisplay expects NickAlias*
         }
 
         if (!email.empty() && email != na->nc->email)
         {
             na->nc->email = email;
             user->SendMessage(NickServ, _("Your email has been updated to \002%s\002."), email.c_str());
         }
 
         req->Success(me);
         delete this;
     }
 
     void OnError(const SQL::Result &r) override
     {
         Log(LOG_DEBUG) << "[m_sqlauth]: SQL Error executing query: " << r.GetQuery().query << ": " << r.GetError();
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
                 Log(LOG_DEBUG) << "[m_sqlauth]: User " << account << " exceeded max authentication attempts.";
 
                 BotInfo *NickServ = Config->GetClient("NickServ");
                 if (NickServ)
                 {
                     MessageSource source(NickServ);
                     user->Kill(source, "Too many failed login attempts.");
                 }
 
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
 
     void OnReload(Configuration::Conf &conf) override
     {
         Configuration::Block &config = conf.GetModule(this);
         this->engine = config.Get<const Anope::string>("engine");
         this->query = config.Get<const Anope::string>("query");
         this->disable_reason = config.Get<const Anope::string>("disable_reason");
         this->disable_email_reason = config.Get<Anope::string>("disable_email_reason");
         this->kill_message_base = config.Get<const Anope::string>("kill_message_base", "Too many failed login attempts.");
         this->max_attempts = config.Get<int>("max_attempts", 3);
 
         this->SQL = ServiceReference<SQL::Provider>("SQL::Provider", this->engine);
     }
 
     void OnCheckAuthentication(User *u, IdentifyRequest *req) override
     {
         if (!this->SQL)
         {
             Log(LOG_DEBUG) << "[m_sqlauth]: Unable to find SQL engine";
             return;
         }
 
         SQL::Query q(this->query);
         q.SetValue("a", req->GetAccount());
         q.SetValue("i", req->GetAddress().str());
         q.SetValue("n", u ? u->nick : "");
         q.SetValue("p", req->GetPassword());
 
         this->SQL->Run(new SQLAuthResult(u, req->GetPassword(), req, this->max_attempts, this->kill_message_base), q);
 
         Log(LOG_DEBUG) << "[m_sqlauth]: Checking authentication for " << req->GetAccount();
     }
 
     void OnPreNickExpire(NickAlias *na, bool &expire) override
     {
         if (na->nick == na->nc->display && na->nc->aliases->size() > 1)
             expire = false;
     }
 };
 
 MODULE_INIT(ModuleSQLAuth)
 
