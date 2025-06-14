/*
2024 Jean "reverse" Chevronnet
Module for Anope IRC Services v2.1.
SeCuRe will ban any proxy IPs using proxycheck.io API.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "module.h"
#include <map>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// Move ProxyCacheEntry and ProxyCache outside the class
struct ProxyCacheEntry {
	bool is_proxy;
	time_t timestamp;
};

class ProxyCache {
	std::map<std::string, ProxyCacheEntry> cache;
	const int expiry_seconds = 86400; // 1 day
public:
	bool get(const std::string& ip, bool& is_proxy) {
		auto it = cache.find(ip);
		if (it != cache.end()) {
			time_t now = std::time(nullptr);
			if (now - it->second.timestamp < expiry_seconds) {
				is_proxy = it->second.is_proxy;
				return true;
			} else {
				cache.erase(it);
			}
		}
		return false;
	}
	void set(const std::string& ip, bool is_proxy) {
		cache[ip] = {is_proxy, std::time(nullptr)};
	}
};

class SeCuReModule : public Module
{
	BotInfo *secureBot = nullptr;
	ProxyCache proxyCache;
	std::string proxycheck_api_key;

public:
	SeCuReModule(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, VENDOR)
	{
		this->SetAuthor("YourName");
		this->SetVersion("1.0");
		// Check API key on module load
		Configuration::Block &config = Config->GetModule(this);
		this->proxycheck_api_key = config.Get<const Anope::string>("proxycheck_api_key", "").str();
		if (this->proxycheck_api_key.empty()) {
			throw ModuleException("m_secure: proxycheck_api_key is not set in anope.conf, refusing to load.");
		}
	}

	void OnReload(Configuration::Conf &conf)
	{
		Configuration::Block &config = conf.GetModule(this);
		this->proxycheck_api_key = config.Get<const Anope::string>("proxycheck_api_key", "").str();
		if (this->proxycheck_api_key.empty()) {
			throw ModuleException("m_secure: proxycheck_api_key is not set in anope.conf, refusing to reload.");
		}
		CreateBot();
	}

	void OnModuleLoad()
	{
		Configuration::Block &config = Config->GetModule(this);
		this->proxycheck_api_key = config.Get<const Anope::string>("proxycheck_api_key", "").str();
		CreateBot();
	}

	void OnModuleUnload()
	{
		if (secureBot)
		{
			delete secureBot;
			secureBot = nullptr;
		}
	}

	void CreateBot()
	{
		if (secureBot)
			return;

		secureBot = BotInfo::Find("SeCuRe");
		if (!secureBot)
		{
			// Create the bot in code, core-style, with all required fields and modes
			secureBot = new BotInfo("SeCuRe", "SeCuRe", "network.services", "SeCuRe Servers", "+oSiI");
			Log(LOG_NORMAL, "m_secure") << "Created SeCuRe bot (core style).";
		}
	}

	// Helper for cURL HTTP GET
	static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
		((std::string*)userp)->append((char*)contents, size * nmemb);
		return size * nmemb;
	}

	// Proxy check logic using proxycheck.io
	bool IsProxyIP(const std::string& ip, ProxyCache& cache, const std::string& api_key) {
		bool cached_result;
		if (cache.get(ip, cached_result))
			return cached_result;

		std::string url = "https://proxycheck.io/v2/" + ip + "?key=" + api_key + "&vpn=1&asn=1&node=1&inf=1";
		CURL* curl = curl_easy_init();
		if (!curl) return false;
		std::string response;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res != CURLE_OK) return false;

		// Parse JSON response robustly
		try {
			auto json = nlohmann::json::parse(response);
			if (json.contains(ip) && json[ip].contains("proxy")) {
				bool is_proxy = json[ip]["proxy"] == "yes";
				cache.set(ip, is_proxy);
				return is_proxy;
			}
		} catch (...) {
			// Fallback to string search if JSON parsing fails
			bool is_proxy = response.find("\"proxy\":\"yes\"") != std::string::npos;
			cache.set(ip, is_proxy);
			return is_proxy;
		}
		cache.set(ip, false);
		return false;
	}

	// In OnUserConnect, check if secureBot is valid before using it
	void OnUserConnect(User *user, bool &exempt) override
	{
		if (exempt || user->Quitting() || !Me->IsSynced() || !user->server->IsSynced())
			return;
		if (!user->ip.valid() || user->ip.sa.sa_family != AF_INET)
			return;
		std::string ip = user->ip.addr().str();
		if (IsProxyIP(ip, proxyCache, proxycheck_api_key)) {
			if (secureBot)
				user->SendMessage(secureBot, "You are connecting from a detected proxy. Please reconnect without a proxy.");
			user->Kill(Me, "Proxy detected by SeCuRe");
		}
	}
};

MODULE_INIT(SeCuReModule)

