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

#define CHANFIX_CHANNEL_DATA_TYPE "ChanFixChannel"

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
	// Deprecated placeholder. The ChanFix DB is now stored via Anope's
	// serialization system using CFChannelData.
};

struct CFChannelData final
	: Serializable
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

	explicit CFChannelData(const Anope::string& chname);
	~CFChannelData() override;
};

class ChanFixChannelDataType final
	: public Serialize::Type
{
public:
	explicit ChanFixChannelDataType(Module* owner);

	void Serialize(Serializable* obj, Serialize::Data& data) const override;
	Serializable* Unserialize(Serializable* obj, Serialize::Data& data) const override;
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

	void LegacyImportIfNeeded();
	bool LegacyImportNeedsSave() const { return this->legacy_import_needs_save; }
	void ClearLegacyImportNeedsSave() { this->legacy_import_needs_save = false; }

	void ScheduleDBSave();

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

private:
	class DeferredSaveTimer;

	Module* module;
	BotInfo* chanfix = nullptr;
	bool legacy_import_needs_save = false;
	bool db_save_pending = false;
	DeferredSaveTimer* db_save_timer = nullptr;

	bool do_autofix = false;
	bool join_to_fix = false;
	bool clear_modes_on_fix = false;
	bool clear_bans_on_fix = false;
	bool clear_moderated_on_fix = false;
	bool deop_below_threshold_on_fix = false;

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
	unsigned int expire_divisor = 672;

	char op_status_char = 'o';

	Anope::string admin_priv = "chanfix/admin";
	Anope::string auspex_priv = "chanfix/auspex";

	bool IsRegistered(Channel* c) const;
	static bool IsValidChannelName(const Anope::string& name);

	CFChannelData* GetRecord(const Anope::string& chname);
	CFChannelData& GetOrCreateRecord(Channel* c);

	unsigned int CountOps(Channel* c) const;
	Anope::string KeyForUser(User* u) const;
	CFOpRecord* FindRecord(CFChannelData& rec, User* u);
	bool UpdateOpRecord(CFChannelData& rec, User* u);

	unsigned int CalculateScore(const CFOpRecord& orec) const;
	unsigned int GetHighScore(const CFChannelData& rec) const;
	unsigned int GetThreshold(const CFChannelData& rec, time_t now) const;

	bool ShouldHandle(CFChannelData& rec, Channel* c) const;
	bool CanStartFix(const CFChannelData& rec, Channel* c) const;
	bool FixChannel(CFChannelData& rec, Channel* c);
	void ClearBans(Channel* c);
};
