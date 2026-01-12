/*
 * (C) 2026 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Channel: #development
 *
 * GroupServ module created by reverse for Anope 2.1
 *
 * Public types and core interface for GroupServ.
 *
 * This provides account groups (named like !group) which contain member
 * accounts with per-group access flags.
 *
 * Note: This intentionally does not attempt to replace Anope's NickServ GROUP
 * (nick grouping) feature.
 */

#pragma once

#include "module.h"

#include <filesystem>
#include <map>
#include <vector>

enum class GSGroupFlags : unsigned int
{
	NONE = 0,
	OPEN = 1 << 0,
	PUBLIC = 1 << 1,
	REGNOLIMIT = 1 << 2,
	ACSNOLIMIT = 1 << 3,
};

inline GSGroupFlags operator|(GSGroupFlags a, GSGroupFlags b)
{
	return static_cast<GSGroupFlags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline GSGroupFlags& operator|=(GSGroupFlags& a, GSGroupFlags b)
{
	a = a | b;
	return a;
}

inline GSGroupFlags operator&(GSGroupFlags a, GSGroupFlags b)
{
	return static_cast<GSGroupFlags>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline bool HasFlag(GSGroupFlags value, GSGroupFlags flag)
{
	return (static_cast<unsigned int>(value) & static_cast<unsigned int>(flag)) != 0;
}

enum class GSAccessFlags : unsigned int
{
	NONE = 0,
	FOUNDER = 1 << 0,   // Atheme: +F
	FLAGS = 1 << 1,     // Atheme: +f (modify ACL)
	ACLVIEW = 1 << 2,   // Atheme: +A
	MEMO = 1 << 3,      // Atheme: +m (group memos; stored only)
	CHANACCESS = 1 << 4,// Atheme: +c (stored only)
	VHOST = 1 << 5,     // Atheme: +v (stored only)
	SET = 1 << 6,       // Atheme: +s
	INVITE = 1 << 7,    // Atheme: +i
	BAN = 1 << 8,       // Atheme: +b

	ALL = (FOUNDER | FLAGS | ACLVIEW | MEMO | CHANACCESS | VHOST | SET | INVITE | BAN),
	ALL_NOFOUNDER = (FLAGS | ACLVIEW | MEMO | CHANACCESS | VHOST | SET | INVITE | BAN),
};

inline GSAccessFlags operator|(GSAccessFlags a, GSAccessFlags b)
{
	return static_cast<GSAccessFlags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

inline GSAccessFlags& operator|=(GSAccessFlags& a, GSAccessFlags b)
{
	a = a | b;
	return a;
}

inline GSAccessFlags operator&(GSAccessFlags a, GSAccessFlags b)
{
	return static_cast<GSAccessFlags>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

inline bool HasFlag(GSAccessFlags value, GSAccessFlags flag)
{
	return (static_cast<unsigned int>(value) & static_cast<unsigned int>(flag)) != 0;
}

struct GSGroupRecord final
{
	Anope::string name;
	time_t regtime = 0;
	GSGroupFlags flags = GSGroupFlags::NONE;

	Anope::string description;
	Anope::string url;
	Anope::string email;
	Anope::string channel;

	Anope::string joinflags_raw;
	GSAccessFlags joinflags = GSAccessFlags::NONE;

	// account (lowercased) -> flags
	std::map<Anope::string, GSAccessFlags> access;
};

struct GSInvite final
{
	Anope::string group;
	time_t created = 0;
	time_t expires = 0;
};

struct GSChanAccessData final
	: Serializable
{
	Anope::string object;
	Anope::string group;
	bool group_only = false;

	GSChanAccessData(Extensible*) : Serializable("GSChanAccessData") { }
	GSChanAccessData(ChannelInfo* ci, const Anope::string& g, bool only)
		: Serializable("GSChanAccessData")
	{
		object = ci->name;
		group = g;
		group_only = only;
	}
};

class GroupServCore final
{
public:
	explicit GroupServCore(Module* owner);
	~GroupServCore();

	void OnReload(Configuration::Conf& conf);

	void Reply(CommandSource& source, const Anope::string& msg);
	void ReplyF(CommandSource& source, const char* fmt, ...) ATTR_FORMAT(3, 4);

	bool DoesGroupExist(const Anope::string& groupname) const;
	bool IsMemberOfGroup(const Anope::string& groupname, const NickCore* nc) const;
	void SetChanAccessItem(ExtensibleItem<GSChanAccessData>* item);

	bool IsAdmin(CommandSource& source) const;
	bool IsAuspex(CommandSource& source) const;
	bool CanExceedLimits(CommandSource& source) const;

	bool RegisterGroup(CommandSource& source, const Anope::string& groupname);
	bool DropGroup(CommandSource& source, const Anope::string& groupname, const Anope::string& key, bool force);
	bool ShowInfo(CommandSource& source, const Anope::string& groupname);
	bool ListGroups(CommandSource& source, const Anope::string& pattern);
	bool ListChans(CommandSource& source, const Anope::string& groupname);

	bool JoinGroup(CommandSource& source, const Anope::string& groupname);
	bool InviteToGroup(CommandSource& source, const Anope::string& groupname, const Anope::string& account);

	bool ShowFlags(CommandSource& source, const Anope::string& groupname);
	bool SetFlags(CommandSource& source, const Anope::string& groupname, const Anope::string& account, const Anope::string& changes, bool force);

	bool SetOption(CommandSource& source, const Anope::string& groupname, const Anope::string& setting, const Anope::string& value);
	bool SetGroupFlag(CommandSource& source, const Anope::string& groupname, GSGroupFlags flag, bool enabled);

	void SaveDB() const;
	void LoadDB();
	time_t GetSaveInterval() const { return this->save_interval; }

private:
	Module* module;
	Serialize::Reference<BotInfo> groupserv;
	bool reply_with_notice = true;
	ExtensibleItem<GSChanAccessData>* chanaccess_item = nullptr;

	void SendToUser(User* u, const Anope::string& msg);

	Anope::string admin_priv;
	Anope::string auspex_priv;
	Anope::string exceed_priv;

	unsigned int maxgroups = 5;
	unsigned int maxgroupacs = 0;
	bool enable_open_groups = true;
	GSAccessFlags default_joinflags = GSAccessFlags::NONE;
	Anope::string default_joinflags_raw;
	
	time_t save_interval = 600;

	std::map<Anope::string, GSGroupRecord> groups; // key = lowercased name
	std::map<Anope::string, GSInvite> invites; // key = lowercased account
	std::map<Anope::string, Anope::string> drop_challenges; // key = lowercased account+"|"+group => token
	bool initialized = false;

	Anope::string GetDBPath() const;
	static Anope::string EscapeValue(const Anope::string& in);
	static Anope::string UnescapeValue(const Anope::string& in);

	static bool IsValidGroupName(const Anope::string& name);
	static Anope::string NormalizeKey(const Anope::string& s);
	static Anope::string DropChallengeKey(const Anope::string& account, const Anope::string& group);

	GSGroupRecord* FindGroup(const Anope::string& name);
	GSGroupRecord& GetOrCreateGroup(const Anope::string& name);
	NickCore* FindAccount(const Anope::string& account) const;

	unsigned int CountGroupsFoundedBy(const NickCore* nc) const;
	unsigned int CountGroupAccessEntries(const GSGroupRecord& g) const;

	GSAccessFlags GetAccessFor(const GSGroupRecord& g, const NickCore* nc) const;
	bool HasAccess(const GSGroupRecord& g, const NickCore* nc, GSAccessFlags required) const;

	GSAccessFlags ParseFlags(const Anope::string& flagstring, bool allow_minus, GSAccessFlags current) const;
	Anope::string FlagsToString(GSAccessFlags flags) const;
	Anope::string GroupFlagsToString(GSGroupFlags flags) const;

	void SetReplyMode(const Anope::string& mode);
};
