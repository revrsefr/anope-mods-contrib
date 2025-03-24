/*
m_apiauth.cpp
2025 Jean "reverse" Chevronnet
Module for Anope IRC Services v2.1, lets users authenticate with
credentials stored in an external API endpoint instead of the internal
Anope database.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "module.h"
#include "modules/encryption.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>


// Global module reference
static Module *me;

// For convenience
using json = nlohmann::json;

// Callback for CURL to write response data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Structure to hold API response
struct APIAuthResponse
{
    bool success;
    Anope::string email;
    Anope::string error_message;
};

class APIAuthRequest
{
private:
    Reference<User> user;
    IdentifyRequest *req;
    Anope::string api_url;
    Anope::string api_username_param;
    Anope::string api_password_param;
    Anope::string api_method;
    Anope::string api_success_field;
    Anope::string api_email_field;
    Anope::string api_key;
    Anope::string api_verify_ssl;
    Anope::string api_capath;
    Anope::string api_cainfo;

public:
    APIAuthRequest(User *u, IdentifyRequest *r, const Anope::string &url, 
                 const Anope::string &username_param, const Anope::string &password_param,
                 const Anope::string &method, const Anope::string &success_field,
                 const Anope::string &email_field, const Anope::string &key) 
        : user(u), req(r), api_url(url), api_username_param(username_param),
          api_password_param(password_param), api_method(method),
          api_success_field(success_field), api_email_field(email_field),
          api_key(key)
    {
        req->Hold(me);
    }

    ~APIAuthRequest()
    {
        req->Release(me);
    }

    APIAuthResponse MakeRequest()
    {
        APIAuthResponse response;
        response.success = false;

        CURL *curl;
        CURLcode res;
        std::string readBuffer;
        
        curl = curl_easy_init();
        if (!curl)
        {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: Could not initialize CURL";
            return response;
        }

        // Prepare the API request (only POST allowed)
        std::string postData;
        std::string username_param(api_username_param.c_str());
        std::string password_param(api_password_param.c_str());
        std::string url(api_url.c_str());
        
        postData = username_param + "=" + curl_easy_escape(curl, req->GetAccount().c_str(), 0) + "&" +
                   password_param + "=" + curl_easy_escape(curl, req->GetPassword().c_str(), 0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        // Always use POST URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // Add API key to request (if provided)
        std::string key(api_key.c_str());
        struct curl_slist *headers = NULL;
        if (!key.empty()) {
            std::string header = "X-API-Key: " + key;
            headers = curl_slist_append(headers, header.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        
        // Set standard options
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 seconds timeout
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Anope-API-Auth/1.0");
        
        // SSL Options
        bool verify_ssl = (api_verify_ssl == "true" || api_verify_ssl == "1" || api_verify_ssl == "yes");
        
        if (!verify_ssl)
        {
            // Disable SSL verification (only for development/testing)
            Log(LOG_DEBUG) << "[api_auth]: ⚠️ WARNING: SSL certificate verification disabled";
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        else if (!api_capath.empty())
        {
            // Use custom CA certificates path if specified
            curl_easy_setopt(curl, CURLOPT_CAPATH, api_capath.c_str());
        }
        else if (!api_cainfo.empty())
        {
            // Use custom CA certificate file if specified
            curl_easy_setopt(curl, CURLOPT_CAINFO, api_cainfo.c_str());
        }

        // Perform the request
        Log(LOG_COMMAND) << "[api_auth]: 🔄 Making API request for user @" << req->GetAccount() << "@";
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK)
        {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: CURL failed: " << curl_easy_strerror(res);
            
            // Provide more helpful error message for SSL issues
            if (res == CURLE_PEER_FAILED_VERIFICATION || res == CURLE_SSL_CACERT)
            {
                response.error_message = "SSL certificate verification failed. Check your server's SSL configuration or try setting verify_ssl=false in modules.conf";
                Log(LOG_COMMAND) << "[api_auth]: ℹ️  Try setting 'verify_ssl' to false in your module configuration for testing.";
            }
            else
            {
                response.error_message = "Connection error: Could not reach authentication server";
            }
            
            curl_easy_cleanup(curl);
            return response;
        }

        // Check HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code == 403) {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: API authentication denied (403 Forbidden)";
            response.error_message = "API authentication failed: Check API key and IP restrictions";
            curl_easy_cleanup(curl);
            return response;
        }
        else if (http_code == 429) {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: Rate limit exceeded (429 Too Many Requests)";
            response.error_message = "Too many authentication attempts. Please try again later";
            curl_easy_cleanup(curl);
            return response;
        }
        else if (http_code != 200) {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: API returned HTTP code " << http_code;
            response.error_message = "API error: Unexpected HTTP response code";
            curl_easy_cleanup(curl);
            return response;
        }

        // Parse the JSON response
        try 
        {
            json root = json::parse(readBuffer);
            
            // Check for success field
            std::string success_field(api_success_field.c_str());
            if (root.contains(success_field))
            {
                response.success = root[success_field].get<bool>();
                
                // If successful and email field is present, get email
                if (response.success && !api_email_field.empty())
                {
                    std::string email_field(api_email_field.c_str());
                    if (root.contains(email_field))
                    {
                        response.email = root[email_field].get<std::string>().c_str();
                    }
                }
            }
            else
            {
                response.error_message = "API response missing success field";
            }
        }
        catch (const json::parse_error& e)
        {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: Failed to parse JSON response: " << e.what();
            Log(LOG_COMMAND) << "[api_auth]: Response was: " << readBuffer;
            response.error_message = "JSON parse error";
        }
        catch (const std::exception& e)
        {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: JSON exception: " << e.what();
            response.error_message = "JSON exception";
        }

        // Cleanup headers
        if (headers) {
            curl_slist_free_all(headers);
        }

        curl_easy_cleanup(curl);
        return response;
    }

    void ProcessAuth()
    {
        // Make the API request
        APIAuthResponse response = MakeRequest();

        if (response.success)
        {
            Log(LOG_COMMAND) << "[api_auth]: ✅ SUCCESS: User @" << req->GetAccount() << "@ LOGGED IN!";
            
            NickAlias *na = NickAlias::Find(req->GetAccount());
            BotInfo *NickServ = Config->GetClient("NickServ");

            if (na == NULL)
            {
                na = new NickAlias(req->GetAccount(), new NickCore(req->GetAccount()));
                FOREACH_MOD(OnNickRegister, (user, na, ""));
                if (user && NickServ)
                    user->SendMessage(NickServ, _("Your account \002%s\002 has been confirmed."), na->nick.c_str());
            }

            if (!response.email.empty() && response.email != na->nc->email)
            {
                na->nc->email = response.email;
                if (user && NickServ)
                    user->SendMessage(NickServ, _("E-mail set to \002%s\002."), response.email.c_str());
            }

            req->Success(me);
        }
        else
        {
            Log(LOG_COMMAND) << "[api_auth]: ❌ ERROR: Authentication failed for user " << req->GetAccount() 
                            << " - " << (!response.error_message.empty() ? response.error_message : "Invalid credentials");
        }

        delete this;
    }
};

class ModuleAPIAuth final : public Module
{
    Anope::string api_url;
    Anope::string api_username_param;
    Anope::string api_password_param;
    Anope::string api_method;
    Anope::string api_success_field;
    Anope::string api_email_field;
    Anope::string disable_reason;
    Anope::string disable_email_reason;
    Anope::string api_key;
    Anope::string api_verify_ssl;
    Anope::string api_capath;
    Anope::string api_cainfo;
    Anope::string profile_url;
    Anope::string register_url;

public:
    ModuleAPIAuth(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, EXTRA | VENDOR)
    {
        me = this;
        
        // Initialize CURL globally
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~ModuleAPIAuth()
    {
        curl_global_cleanup();
    }

    void OnReload(Configuration::Conf &conf) override
    {
        Configuration::Block &config = conf.GetModule(this);
        
        this->api_url = config.Get<const Anope::string>("api_url", "http://example.com/api/auth");
        this->api_username_param = config.Get<const Anope::string>("api_username_param", "username");
        this->api_password_param = config.Get<const Anope::string>("api_password_param", "password");
        this->api_method = config.Get<const Anope::string>("api_method", "POST");
        this->api_success_field = config.Get<const Anope::string>("api_success_field", "success");
        this->api_email_field = config.Get<const Anope::string>("api_email_field", "email");
        this->disable_reason = config.Get<const Anope::string>("disable_reason");
        this->disable_email_reason = config.Get<const Anope::string>("disable_email_reason");
        this->api_key = config.Get<const Anope::string>("api_key", "");
        this->api_verify_ssl = config.Get<const Anope::string>("verify_ssl", "true");
        this->api_capath = config.Get<const Anope::string>("capath", "");
        this->api_cainfo = config.Get<const Anope::string>("cainfo", "");
        this->profile_url = config.Get<const Anope::string>("profile_url", "https://www.t-chat.fr/accounts/profile/%s/");
        this->register_url = config.Get<const Anope::string>("register_url", "https://www.t-chat.fr/accounts/register/");
    }

    EventReturn OnPreCommand(CommandSource &source, Command *command, std::vector<Anope::string> &params) override
    {
        if (!this->disable_reason.empty() && (command->name == "nickserv/register" || command->name == "nickserv/group"))
        {
            // Use register_url for the message
            Anope::string formatted_reason = this->disable_reason;
            if (formatted_reason.find("%s") != Anope::string::npos)
                formatted_reason = formatted_reason.replace_all_cs("%s", this->register_url);
            else if (!this->register_url.empty())
                formatted_reason += " " + this->register_url;
                
            source.Reply(formatted_reason);
            return EVENT_STOP;
        }

        if (!this->disable_email_reason.empty() && command->name == "nickserv/set/email")
        {
            // Get account name from NickCore if available, should be with this command :O
            Anope::string account = source.GetAccount() ? source.GetAccount()->display : "";
            Anope::string formatted_url = this->profile_url;
            
            if (account.empty())
                account = ""; // Default if not logged in
                
            if (formatted_url.find("%s") != Anope::string::npos)
                formatted_url = formatted_url.replace_all_cs("%s", account);
                
            Anope::string formatted_reason = this->disable_email_reason;
            if (formatted_reason.find("%s") != Anope::string::npos)
                formatted_reason = formatted_reason.replace_all_cs("%s", formatted_url);
            else
                formatted_reason += " " + formatted_url;
                
            source.Reply(formatted_reason);
            return EVENT_STOP;
        }

        return EVENT_CONTINUE;
    }

    void OnCheckAuthentication(User *u, IdentifyRequest *req) override
    {
        Log(LOG_COMMAND) << "[api_auth]: 🔎 Checking authentication for " << req->GetAccount();
        
        APIAuthRequest *auth_req = new APIAuthRequest(u, req, this->api_url, this->api_username_param, 
                                                    this->api_password_param, this->api_method,
                                                    this->api_success_field, this->api_email_field,
                                                    this->api_key);
        auth_req->ProcessAuth();
    }

    void OnPreNickExpire(NickAlias *na, bool &expire) override
    {
        // Prevent expiration of nicks with active accounts
        if (na->nick == na->nc->display && na->nc->aliases->size() > 1)
            expire = false;
    }
};

MODULE_INIT(ModuleAPIAuth)

