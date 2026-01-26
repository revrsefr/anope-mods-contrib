/*
2024 Jean "reverse" Chevronnet
Module for Anope IRC Services v2.1.
Botserv read and answer youtube links.

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

/// $LinkerFlags: -lcurl -lssl -lcrypto

#include "module.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <regex>
#include <chrono>
#include <sstream>
#include <cctype>

class CommandBSYouTube final : public Command
{
private:
    Anope::string api_key;
    Anope::string prefix;
    Anope::string duration_text;
    Anope::string seen_text;
    Anope::string times_text;
    Anope::string response_format;

    static Anope::string UnescapeConfigString(const Anope::string &in)
    {
        // Anope's config parser only treats \" specially. We optionally support common
        // escape sequences here so users can configure IRC formatting codes.
        Anope::string out;

        for (size_t i = 0; i < in.length(); ++i)
        {
            const char ch = in[i];
            if (ch != '\\' || i + 1 >= in.length())
            {
                out.push_back(ch);
                continue;
            }

            const char next = in[i + 1];
            if (next == 'n')
            {
                out.push_back('\n');
                ++i;
            }
            else if (next == 'r')
            {
                out.push_back('\r');
                ++i;
            }
            else if (next == 't')
            {
                out.push_back('\t');
                ++i;
            }
            else if (next == '\\' || next == '"')
            {
                out.push_back(next);
                ++i;
            }
            else if (next == 'x' && i + 3 < in.length() && std::isxdigit(static_cast<unsigned char>(in[i + 2])) && std::isxdigit(static_cast<unsigned char>(in[i + 3])))
            {
                auto hexval = [](char c) -> int
                {
                    if (c >= '0' && c <= '9')
                        return c - '0';
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                    if (c >= 'a' && c <= 'f')
                        return 10 + (c - 'a');
                    return 0;
                };

                const int value = (hexval(in[i + 2]) << 4) | hexval(in[i + 3]);
                out.push_back(static_cast<char>(value));
                i += 3;
            }
            else if (next >= '0' && next <= '7')
            {
                // Octal \NNN (up to 3 digits)
                int value = 0;
                size_t j = i + 1;
                for (int digits = 0; digits < 3 && j < in.length(); ++digits, ++j)
                {
                    char oc = in[j];
                    if (oc < '0' || oc > '7')
                        break;
                    value = (value << 3) | (oc - '0');
                }
                out.push_back(static_cast<char>(value & 0xFF));
                i = j - 1;
            }
            else
            {
                // Unknown escape; keep the next char and drop the backslash.
                out.push_back(next);
                ++i;
            }
        }

        return out;
    }

    Anope::string BuildResponse(const Anope::string &title, const Anope::string &duration, const Anope::string &viewCount) const
    {
        if (!this->response_format.empty())
        {
            // Simple token replacement (no printf-style formatting).
            Anope::string out = this->response_format;
            out = out.replace_all_cs("{title}", title);
            out = out.replace_all_cs("{duration}", duration);
            out = out.replace_all_cs("{views}", viewCount);
            out = out.replace_all_cs("{viewCount}", viewCount);
            return out;
        }

        // Backwards-compatible default.
        return this->prefix + " " + title + " - " + this->duration_text + duration + " - " + this->seen_text + viewCount + this->times_text;
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string ParseISO8601Duration(const std::string &duration)
    {
        std::chrono::seconds seconds(0);
        std::smatch match;
        std::regex duration_regex("PT((\\d+)H)?((\\d+)M)?((\\d+)S)?");

        if (std::regex_match(duration, match, duration_regex))
        {
            if (match[2].matched)
                seconds += std::chrono::hours(std::stoi(match[2].str()));
            if (match[4].matched)
                seconds += std::chrono::minutes(std::stoi(match[4].str()));
            if (match[6].matched)
                seconds += std::chrono::seconds(std::stoi(match[6].str()));
        }

        auto h = std::chrono::duration_cast<std::chrono::hours>(seconds).count();
        auto m = std::chrono::duration_cast<std::chrono::minutes>(seconds).count() % 60;
        auto s = seconds.count() % 60;

        std::ostringstream oss;
        if (h > 0) oss << h << "h";
        if (m > 0) oss << m << "m";
        if (s > 0) oss << s << "s";

        return oss.str();
    }

    void FetchYouTubeDetails(const Anope::string &api_key, const Anope::string &video_id, Anope::string &title, Anope::string &duration, Anope::string &viewCount)
    {
        if (api_key.empty())
        {
            Log(LOG_DEBUG) << "YouTube API key is not configured.";
            return;
        }

        // Keep payload small.
        const Anope::string api_url = "https://www.googleapis.com/youtube/v3/videos?id=" + video_id
            + "&part=snippet,contentDetails,statistics"
            + "&fields=items(snippet(title),contentDetails(duration),statistics(viewCount))"
            + "&key=" + api_key;

        CURL *curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Anope-m_youtube/1.0");

            std::string response_string;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK)
            {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code != 200)
                {
                    Log(LOG_DEBUG) << "YouTube API HTTP status " << http_code << " for video ID: " << video_id;
                    curl_easy_cleanup(curl);
                    return;
                }

                rapidjson::Document jsonData;
                if (jsonData.Parse(response_string.c_str()).HasParseError())
                {
                    Log() << "JSON Parse Error: " << rapidjson::GetParseError_En(jsonData.GetParseError());
                }
                else if (jsonData.HasMember("items") && jsonData["items"].IsArray() && !jsonData["items"].Empty())
                {
                    const auto& item = jsonData["items"][0];
                    if (item.HasMember("snippet") && item.HasMember("contentDetails") && item.HasMember("statistics")
                        && item["snippet"].IsObject() && item["contentDetails"].IsObject() && item["statistics"].IsObject())
                    {
                        const auto& snippet = item["snippet"];
                        const auto& contentDetails = item["contentDetails"];
                        const auto& statistics = item["statistics"];

                        if (snippet.HasMember("title") && snippet["title"].IsString()
                            && contentDetails.HasMember("duration") && contentDetails["duration"].IsString()
                            && statistics.HasMember("viewCount"))
                        {
                            title = snippet["title"].GetString();
                            duration = ParseISO8601Duration(contentDetails["duration"].GetString());

                            if (statistics["viewCount"].IsString())
                                viewCount = statistics["viewCount"].GetString();
                            else if (statistics["viewCount"].IsUint64())
                                viewCount = Anope::ToString(statistics["viewCount"].GetUint64());
                            else if (statistics["viewCount"].IsInt64())
                                viewCount = Anope::ToString(statistics["viewCount"].GetInt64());
                            else
                                Log() << "Invalid JSON response: statistics.viewCount is not a string or integer";
                        }
                        else
                        {
                            Log() << "Invalid JSON response: missing/invalid members in snippet, contentDetails, or statistics";
                        }
                    }
                    else
                    {
                        Log() << "Invalid JSON response: missing/invalid snippet, contentDetails, or statistics";
                    }
                }
                else
                {
                    Log() << "Invalid JSON response: 'items' field missing, not an array, or empty";
                }
            }
            else
            {
                Log() << "CURL Error: " << curl_easy_strerror(res);
            }

            curl_easy_cleanup(curl);
        }
        else
        {
            Log() << "CURL Initialization Error";
        }
    }

    void HandleYouTubeLink(const Anope::string &api_key, ChannelInfo *ci, const Anope::string &message)
    {
        try
        {
            // Supports:
            // - https://www.youtube.com/watch?v=VIDEOID
            // - https://youtu.be/VIDEOID
            // - https://www.youtube.com/shorts/VIDEOID
            // - https://www.youtube.com/embed/VIDEOID
            // Also accepts m.youtube.com and trailing query/fragment.
            std::regex youtube_regex(
                "https?://(?:www\\.|m\\.)?(?:youtube\\.com/(?:watch\\?v=|shorts/|embed/)|youtu\\.be/)([A-Za-z0-9_-]{11})(?:[?&#/].*)?",
                std::regex::icase);
            std::smatch match;
            if (std::regex_search(message.begin(), message.end(), match, youtube_regex) && match.size() > 1)
            {
                Anope::string video_id = match.str(1).c_str();
                Anope::string title, duration, viewCount;
                FetchYouTubeDetails(api_key, video_id, title, duration, viewCount);

                if (!title.empty() && !duration.empty() && !viewCount.empty())
                {
                    IRCD->SendPrivmsg(*ci->bi, ci->name, this->BuildResponse(title, duration, viewCount));
                    ci->bi->lastmsg = Anope::CurTime;
                }
                else
                {
                    Log(LOG_DEBUG) << "Failed to fetch YouTube details for video ID: " << video_id;
                }
            }
        }
        catch (const std::regex_error &e)
        {
            Log() << "Regex error: " << e.what();
        }
    }

public:
    CommandBSYouTube(Module *creator) : Command(creator, "botserv/youtube", 2, 2)
    {
        this->SetDesc(_("Handles YouTube links and fetches video details"));
        this->SetSyntax(_("\037channel\037 \037message\037"));

        // Reasonable defaults (can be overridden in config).
        this->prefix = "\x02\x03""01,00You\x03""00,04Tube\x0F\x02";
        this->duration_text = "Duration: ";
        this->seen_text = "Seen: ";
        this->times_text = " times.";
    }

    void SetAPIKey(const Anope::string &key)
    {
        this->api_key = key;
    }

    void SetResponseConfig(const Anope::string &prefix,
                           const Anope::string &duration_text,
                           const Anope::string &seen_text,
                           const Anope::string &times_text,
                           const Anope::string &response_format)
    {
        if (!prefix.empty())
            this->prefix = UnescapeConfigString(prefix);
        if (!duration_text.empty())
            this->duration_text = UnescapeConfigString(duration_text);
        if (!seen_text.empty())
            this->seen_text = UnescapeConfigString(seen_text);
        if (!times_text.empty())
            this->times_text = UnescapeConfigString(times_text);
        this->response_format = UnescapeConfigString(response_format);
    }

    void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
    {
        const Anope::string &channel = params[0];
        const Anope::string &message = params[1];

        ChannelInfo *ci = ChannelInfo::Find(channel);
        if (ci == NULL)
        {
            source.Reply(CHAN_X_NOT_REGISTERED, channel.c_str());
            return;
        }

        if (!source.AccessFor(ci).HasPriv("SAY") && !source.HasPriv("botserv/administration"))
        {
            source.Reply(ACCESS_DENIED);
            return;
        }

        if (!ci->bi)
        {
            source.Reply(BOT_NOT_ASSIGNED);
            return;
        }

        if (!ci->c || !ci->c->FindUser(ci->bi))
        {
            source.Reply(BOT_NOT_ON_CHANNEL, ci->name.c_str());
            return;
        }

        if (!message.empty() && message[0] == '\001')
        {
            this->OnSyntaxError(source, "");
            return;
        }

        HandleYouTubeLink(this->api_key, ci, message);

        bool override = !source.AccessFor(ci).HasPriv("SAY");
        Log(override ? LOG_OVERRIDE : LOG_COMMAND, source, this, ci) << "to fetch YouTube info";
    }

    bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
    {
        this->SendSyntax(source);
        source.Reply(" ");
        source.Reply(_("Handles YouTube links and fetches video details when a YouTube link is posted in the channel."));
        return true;
    }

    friend class YouTubeModule;
};

class YouTubeModule final : public Module
{
    CommandBSYouTube commandbsYouTube;
    Anope::string api_key;
    Anope::string prefix;
    Anope::string duration_text;
    Anope::string seen_text;
    Anope::string times_text;
    Anope::string response_format;

    void LoadConfig(Configuration::Conf &conf)
    {
        const auto &modconf = conf.GetModule(this);

        // Prefer youtube_api_key but accept api_key for convenience.
        this->api_key = modconf.Get<Anope::string>("youtube_api_key");
        if (this->api_key.empty())
            this->api_key = modconf.Get<Anope::string>("api_key");

        if (this->api_key.empty())
            Log() << "m_youtube: youtube_api_key not set in modules.conf; video info lookups are disabled.";

        // Response text customization.
        this->prefix = modconf.Get<Anope::string>("prefix", "\x02\x03""01,00You\x03""00,04Tube\x0F\x02");

        // Allow short key names for convenience.
        this->duration_text = modconf.Get<Anope::string>("duration_text");
        if (this->duration_text.empty())
            this->duration_text = modconf.Get<Anope::string>("duration", "Duration: ");

        this->seen_text = modconf.Get<Anope::string>("seen_text");
        if (this->seen_text.empty())
            this->seen_text = modconf.Get<Anope::string>("seen", "Seen: ");

        this->times_text = modconf.Get<Anope::string>("times_text");
        if (this->times_text.empty())
            this->times_text = modconf.Get<Anope::string>("times", " times.");

        this->response_format = modconf.Get<Anope::string>("response_format");
        if (this->response_format.empty())
            this->response_format = modconf.Get<Anope::string>("format");

        commandbsYouTube.SetAPIKey(this->api_key);
        commandbsYouTube.SetResponseConfig(this->prefix, this->duration_text, this->seen_text, this->times_text, this->response_format);
    }

public:
    YouTubeModule(const Anope::string &modname, const Anope::string &creator)
        : Module(modname, creator, VENDOR)
        , commandbsYouTube(this)
    {
        // Configuration is loaded via OnReload.
    }

    void OnReload(Configuration::Conf &conf) override
    {
        this->LoadConfig(conf);
    }

    void OnPrivmsg(User *u, Channel *c, Anope::string &msg, const Anope::map<Anope::string> &tags) override
    {
        if (u == nullptr || c == nullptr || c->ci == nullptr || c->ci->bi == nullptr || msg.empty() || msg[0] == '\1')
            return;

        commandbsYouTube.HandleYouTubeLink(this->api_key, c->ci, msg);
    }
};

MODULE_INIT(YouTubeModule)