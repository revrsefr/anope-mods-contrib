/*
 * ChanFix module for Anope 2.1.
 *
 * Inspired by Atheme's chanfix, but implemented using Anope's Channel/ChanServ
 * registration APIs (ChannelInfo::Find / Channel::ci).
 */

#pragma once

#include "module.h"

#include <filesystem>
#include <unordered_map>
#include <vector>

struct CFOpRecord final
{
	Anope::string account; // empty or "*" means no account
	Anope::string user;
	Anope::string host;
	time_t firstseen = 0;
	time_t lastevent = 0;
	unsigned int age = 0;
};

struct CFChannelRecord final
{
	Anope::string name;
	time_t ts = 0;
	time_t lastupdate = 0;
	time_t fix_started = 0;
	bool fix_requested = false;

	bool marked = false;
	Anope::string mark_setter;
	Anope::string mark_reason;
	time_t mark_time = 0;

	bool nofix = false;
	Anope::string nofix_setter;
	Anope::string nofix_reason;
	time_t nofix_time = 0;

	Anope::unordered_map<CFOpRecord> oprecords;
};

class ChanFixCore final
{
public:
	explicit ChanFixCore(Module* owner);
	~ChanFixCore();

	void OnReload(Configuration::Conf& conf);

	void GatherTick();
	void ExpireTick();
	void AutoFixTick();
	void SaveDB() const;

	bool IsAdmin(CommandSource& source) const;
	bool IsAuspex(CommandSource& source) const;

	bool RequestFix(CommandSource& source, const Anope::string& chname);
	bool RequestFixFromChanServ(CommandSource& source, const Anope::string& chname);
	bool SetMark(CommandSource& source, const Anope::string& chname, bool on, const Anope::string& reason);
	bool SetNoFix(CommandSource& source, const Anope::string& chname, bool on, const Anope::string& reason);

	void ShowScores(CommandSource& source, const Anope::string& chname, unsigned int count);
	void ShowInfo(CommandSource& source, const Anope::string& chname);
	void ListChannels(CommandSource& source, const Anope::string& pattern);

	time_t GetGatherInterval() const { return this->gather_interval; }
	time_t GetExpireInterval() const { return this->expire_interval; }
	time_t GetAutofixInterval() const { return this->autofix_interval; }
	time_t GetSaveInterval() const { return this->save_interval; }

private:
	Module* module;
	BotInfo* chanfix = nullptr;

	bool do_autofix = false;
	bool join_to_fix = false;

	unsigned int op_threshold = 3;
	unsigned int min_fix_score = 12;
	double account_weight = 1.5;
	double initial_step = 0.70;
	double final_step = 0.30;

	time_t retention_time = 4 * 7 * 24 * 60 * 60;
	time_t fix_time = 60 * 60;
	time_t gather_interval = 5 * 60;
	time_t expire_interval = 60 * 60;
	time_t autofix_interval = 60;
	time_t save_interval = 10 * 60;
	unsigned int expire_divisor = 672;

	char op_status_char = 'o';

	Anope::string admin_priv = "chanfix/admin";
	Anope::string auspex_priv = "chanfix/auspex";

	Anope::unordered_map<CFChannelRecord> channels;

	static constexpr const char* DB_MAGIC = "chanfix";
	static constexpr unsigned DB_VERSION = 1;

	void LoadDB();
	Anope::string GetDBPath() const;

	static Anope::string EscapeValue(const Anope::string& in);
	static Anope::string UnescapeValue(const Anope::string& in);

	bool IsRegistered(Channel* c) const;
	static bool IsValidChannelName(const Anope::string& name);

	CFChannelRecord* GetRecord(const Anope::string& chname);
	CFChannelRecord& GetOrCreateRecord(Channel* c);

	unsigned int CountOps(Channel* c) const;
	Anope::string KeyForUser(User* u) const;
	CFOpRecord* FindRecord(CFChannelRecord& rec, User* u);
	void UpdateOpRecord(CFChannelRecord& rec, User* u);

	unsigned int CalculateScore(const CFOpRecord& orec) const;
	unsigned int GetHighScore(const CFChannelRecord& rec) const;
	unsigned int GetThreshold(const CFChannelRecord& rec, time_t now) const;

	bool ShouldHandle(CFChannelRecord& rec, Channel* c) const;
	bool CanStartFix(const CFChannelRecord& rec, Channel* c) const;
	bool FixChannel(CFChannelRecord& rec, Channel* c);
	void ClearBans(Channel* c);
};
