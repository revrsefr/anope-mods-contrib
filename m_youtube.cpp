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

/// $CompilerFlags: find_compiler_flags("RapidJSON")
/// $LinkerFlags: find_linker_flags("RapidJSON")
/// $CompilerFlags: find_compiler_flags("libcurl")
/// $LinkerFlags: find_linker_flags("libcurl")

#include "module.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <regex>
#include <chrono>
#include <iomanip>
#include <sstream>

class CommandBSYouTube final : public Command
{
private:
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

    void FetchYouTubeDetails(const Anope::string &video_id, Anope::string &title, Anope::string &duration, Anope::string &viewCount)
    {
        const Anope::string api_key = "AIzaSyBWdSIHhcQv7O5smOlCo2bq9QFcEB-zFEE"; // Replace with your new API key
        const Anope::string api_url = "https://www.googleapis.com/youtube/v3/videos?id=" + video_id + "&part=snippet,contentDetails,statistics&key=" + api_key;

        CURL *curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, api_url.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable SSL verification for testing
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            std::string response_string;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK)
            {
                rapidjson::Document jsonData;
                if (jsonData.Parse(response_string.c_str()).HasParseError())
                {
                    Log() << "JSON Parse Error: " << rapidjson::GetParseError_En(jsonData.GetParseError());
                    curl_easy_cleanup(curl);
                    return;
                }

                if (jsonData.HasMember("items") && jsonData["items"].IsArray() && jsonData["items"].Size() > 0)
                {
                    const rapidjson::Value& items = jsonData["items"];
                    const rapidjson::Value& snippet = items[0]["snippet"];
                    const rapidjson::Value& contentDetails = items[0]["contentDetails"];
                    const rapidjson::Value& statistics = items[0]["statistics"];

                    title = snippet["title"].GetString();
                    duration = ParseISO8601Duration(contentDetails["duration"].GetString());
                    viewCount = statistics["viewCount"].GetString();
                }
                else
                {
                    Log() << "Invalid JSON response: 'items' field missing or not an array or empty";
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

    void HandleYouTubeLink(ChannelInfo *ci, const Anope::string &message)
    {
        try
        {
            std::regex youtube_regex("https?://(?:www\\.)?youtube\\.com/watch\\?v=([a-zA-Z0-9_-]+)");
            std::smatch match;
            if (std::regex_search(message.begin(), message.end(), match, youtube_regex) && match.size() > 1)
            {
                Anope::string video_id = match.str(1).c_str();
                Anope::string title, duration, viewCount;
                FetchYouTubeDetails(video_id, title, duration, viewCount);

                if (!title.empty() && !duration.empty() && !viewCount.empty())
                {
                    Anope::string response = "\x02\x03""01,00You\x03""00,04Tube\x0F\x02 " + title + " - Duración: " + duration + " - Visto: " + viewCount + " veces.";
                    Anope::map<Anope::string> tags;
                    IRCD->SendPrivmsg(*ci->bi, ci->name, response, tags);
                }
                else
                {
                    Log() << "Failed to fetch YouTube details for video ID: " << video_id;
                    Anope::string response = "Failed to fetch YouTube details. Please check the logs for more information.";
                    Anope::map<Anope::string> tags;
                    IRCD->SendPrivmsg(*ci->bi, ci->name, response, tags);
                }
            }
        }
        catch (const std::regex_error &e)
        {
            Log() << "Regex error: " << e.what();
        }
    }

public:
    CommandBSYouTube(Module *creator) : Command(creator, "botserv/youtube", 1, 1)
    {
        this->SetDesc(_("Handles YouTube links and fetches video details"));
        this->SetSyntax(_("\037channel\037 \037message\037"));
    }

    void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
    {
        const Anope::string &channel = params[0];
        const Anope::string &message = params[1];

        ChannelInfo *ci = ChannelInfo::Find(channel);
        if (ci == nullptr || !ci->c || !ci->c->FindUser(ci->bi))
        {
            source.Reply("Bot is not on the channel or channel not found.");
            return;
        }

        HandleYouTubeLink(ci, message);
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

public:
    YouTubeModule(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR),
                                                                               commandbsYouTube(this)
    {
    }

    void OnPrivmsg(User *u, Channel *c, Anope::string &msg, const Anope::map<Anope::string> &tags) override
    {
        if (u == nullptr || c == nullptr || c->ci == nullptr || c->ci->bi == nullptr || msg.empty() || msg[0] == '\1')
            return;

        if (msg.find("youtube.com/watch") != Anope::string::npos)
        {
            commandbsYouTube.HandleYouTubeLink(c->ci, msg);
        }
    }
};

MODULE_INIT(YouTubeModule)
