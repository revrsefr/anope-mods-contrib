/*
 * (C) 2026 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Channel: #development
 *
 * GroupServ module created by reverse for Anope 2.1
 *
 * Core logic + persistence:
 * - Groups named like !group
 * - Member access flags + join flags
 * - Invites (with expiry)
 * - Flatfile persistence in data/groupserv.db (atomic .tmp + rename)
 */

#include "groupserv.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
	constexpr const char* LEGACY_DB_MAGIC = "groupserv";
	constexpr uint64_t DB_VERSION = 1;
	constexpr time_t INVITE_TTL = 7 * 24 * 60 * 60; // 7 days
	constexpr time_t DROP_CHALLENGE_TTL = 30 * 60; // 30 minutes

	bool ParseU64(const Anope::string& in, uint64_t& out)
	{
		try
		{
			out = Anope::Convert<uint64_t>(in, 0);
			return true;
		}
		catch (...)
		{
			out = 0;
			return false;
		}
	}
}

GroupServCore::GroupServCore(Module* owner)
	: module(owner)
{
	if (!IRCD)
		throw ModuleException("IRCd protocol module not loaded");
}

GroupServCore::~GroupServCore()
{
	if (this->initialized)
		this->SaveDB();
}

void GroupServCore::PurgeExpiredState()
{
	// Expired invites should not remain in memory forever.
	for (auto it = this->invites.begin(); it != this->invites.end();)
	{
		if (it->second.expires && it->second.expires < Anope::CurTime)
			it = this->invites.erase(it);
		else
			++it;
	}

	// Drop challenges are transient; keep the map bounded.
	for (auto it = this->drop_challenges.begin(); it != this->drop_challenges.end();)
	{
		if (it->second.created && (it->second.created + DROP_CHALLENGE_TTL) < Anope::CurTime)
			it = this->drop_challenges.erase(it);
		else
			++it;
	}
}

bool GroupServCore::DoesGroupExist(const Anope::string& groupname) const
{
	const auto key = NormalizeKey(groupname);
	return this->groups.find(key) != this->groups.end();
}

bool GroupServCore::IsMemberOfGroup(const Anope::string& groupname, const NickCore* nc) const
{
	if (!nc)
		return false;

	const auto key = NormalizeKey(groupname);
	auto it = this->groups.find(key);
	if (it == this->groups.end())
		return false;

	const auto flags = this->GetAccessFor(it->second, nc);
	if (flags == GSAccessFlags::NONE)
		return false;

	if (HasFlag(flags, GSAccessFlags::BAN))
		return false;

	return true;
}

void GroupServCore::SetChanAccessItem(ExtensibleItem<GSChanAccessData>* item)
{
	this->chanaccess_item = item;
}

Anope::string GroupServCore::GetDBPath() const
{
	return Anope::ExpandData("groupserv.db");
}

Anope::string GroupServCore::EscapeValue(const Anope::string& in)
{
	Anope::string out;
	for (const char ch : in)
	{
		switch (ch)
		{
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n"; break;
			case '\r': break;
			case '|': out += "\\|"; break;
			default: out += ch; break;
		}
	}
	return out;
}

Anope::string GroupServCore::UnescapeValue(const Anope::string& in)
{
	Anope::string out;
	for (size_t i = 0; i < in.length(); ++i)
	{
		const char ch = in[i];
		if (ch != '\\' || i + 1 >= in.length())
		{
			out += ch;
			continue;
		}

		const char next = in[i + 1];
		if (next == 'n')
		{
			out += '\n';
			++i;
		}
		else if (next == '\\')
		{
			out += '\\';
			++i;
		}
		else if (next == '|')
		{
			out += '|';
			++i;
		}
		else
		{
			out += next;
			++i;
		}
	}
	return out;
}

Anope::string GroupServCore::NormalizeKey(const Anope::string& s)
{
	Anope::string out = s;
	out.trim();
	out = out.lower();
	return out;
}

Anope::string GroupServCore::DropChallengeKey(const Anope::string& account, const Anope::string& group)
{
	return NormalizeKey(account) + "|" + NormalizeKey(group);
}

bool GroupServCore::IsValidGroupName(const Anope::string& name)
{
	if (name.length() < 2 || name.length() > 32)
		return false;
	if (name[0] != '!')
		return false;
	for (size_t i = 1; i < name.length(); ++i)
	{
		const char ch = name[i];
		if (!(isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-'))
			return false;
	}
	return true;
}

NickCore* GroupServCore::FindAccount(const Anope::string& account) const
{
	NickAlias* na = NickAlias::Find(account);
	if (na && na->nc)
		return na->nc;
	return NickCore::Find(account);
}

GSGroupRecord* GroupServCore::FindGroup(const Anope::string& name)
{
	auto it = this->groups.find(NormalizeKey(name));
	if (it == this->groups.end())
		return nullptr;
	return &it->second;
}

GSGroupRecord& GroupServCore::GetOrCreateGroup(const Anope::string& name)
{
	auto key = NormalizeKey(name);
	auto it = this->groups.find(key);
	if (it != this->groups.end())
		return it->second;

	GSGroupRecord g;
	g.name = name;
	g.regtime = Anope::CurTime;
	this->groups.emplace(key, g);
	return this->groups.find(key)->second;
}

unsigned int GroupServCore::CountGroupsFoundedBy(const NickCore* nc) const
{
	if (!nc)
		return 0;
	const auto acct = NormalizeKey(nc->display);
	unsigned int count = 0;
	for (const auto& [_, g] : this->groups)
	{
		auto it = g.access.find(acct);
		if (it != g.access.end() && HasFlag(it->second, GSAccessFlags::FOUNDER))
			++count;
	}
	return count;
}

unsigned int GroupServCore::CountGroupAccessEntries(const GSGroupRecord& g) const
{
	return static_cast<unsigned int>(g.access.size());
}

GSAccessFlags GroupServCore::GetAccessFor(const GSGroupRecord& g, const NickCore* nc) const
{
	if (!nc)
		return GSAccessFlags::NONE;
	const auto acct = NormalizeKey(nc->display);
	auto it = g.access.find(acct);
	if (it == g.access.end())
		return GSAccessFlags::NONE;
	return it->second;
}

bool GroupServCore::HasAccess(const GSGroupRecord& g, const NickCore* nc, GSAccessFlags required) const
{
	const auto flags = this->GetAccessFor(g, nc);
	if (HasFlag(flags, GSAccessFlags::BAN))
		return false;
	return (static_cast<unsigned int>(flags) & static_cast<unsigned int>(required)) == static_cast<unsigned int>(required);
}

Anope::string GroupServCore::FlagsToString(GSAccessFlags flags) const
{
	Anope::string out;
	auto add = [&](char c)
	{
		out += c;
	};

	// Atheme-style letters (see ParseFlags).
	if (HasFlag(flags, GSAccessFlags::FOUNDER)) add('F');
	if (HasFlag(flags, GSAccessFlags::FLAGS)) add('f');
	if (HasFlag(flags, GSAccessFlags::ACLVIEW)) add('A');
	if (HasFlag(flags, GSAccessFlags::MEMO)) add('m');
	if (HasFlag(flags, GSAccessFlags::CHANACCESS)) add('c');
	if (HasFlag(flags, GSAccessFlags::VHOST)) add('v');
	if (HasFlag(flags, GSAccessFlags::SET)) add('s');
	if (HasFlag(flags, GSAccessFlags::INVITE)) add('i');
	if (HasFlag(flags, GSAccessFlags::BAN)) add('b');
	if (out.empty())
		out = "-";
	return out;
}

Anope::string GroupServCore::GroupFlagsToString(GSGroupFlags flags) const
{
	Anope::string out;
	auto add = [&](const char* s)
	{
		if (!out.empty())
			out += " ";
		out += s;
	};

	if (HasFlag(flags, GSGroupFlags::REGNOLIMIT)) add("REGNOLIMIT");
	if (HasFlag(flags, GSGroupFlags::ACSNOLIMIT)) add("ACSNOLIMIT");
	if (HasFlag(flags, GSGroupFlags::OPEN)) add("OPEN");
	if (HasFlag(flags, GSGroupFlags::PUBLIC)) add("PUBLIC");
	if (out.empty())
		out = "-";
	return out;
}

GSAccessFlags GroupServCore::ParseFlags(const Anope::string& flagstring, bool allow_minus, GSAccessFlags current) const
{
	GSAccessFlags flags = current;
	std::vector<Anope::string> tokens;
	sepstream(flagstring, ' ').GetTokens(tokens);
	if (tokens.empty() && !flagstring.empty())
		tokens.push_back(flagstring);

	auto apply_one = [&](const Anope::string& up, char dir) {
		GSAccessFlags bit = GSAccessFlags::NONE;
		// Atheme letters (case-insensitive), plus some older aliases for compatibility.
		if (up == "*" || up == "ALL") bit = GSAccessFlags::ALL;
		else if (up == "F" || up == "FOUNDER") bit = GSAccessFlags::FOUNDER;
		else if (up == "f" || up == "FLAGS" || up == "MANAGE" || up == "M") bit = GSAccessFlags::FLAGS;
		else if (up == "A" || up == "ACLVIEW" || up == "VIEW" || up == "V") bit = GSAccessFlags::ACLVIEW;
		else if (up == "m" || up == "MEMO") bit = GSAccessFlags::MEMO;
		else if (up == "c" || up == "CHAN" || up == "CHANACCESS") bit = GSAccessFlags::CHANACCESS;
		else if (up == "v" || up == "VHOST") bit = GSAccessFlags::VHOST;
		else if (up == "s" || up == "SET") bit = GSAccessFlags::SET;
		else if (up == "i" || up == "INVITE" || up == "I") bit = GSAccessFlags::INVITE;
		else if (up == "b" || up == "BAN" || up == "B") bit = GSAccessFlags::BAN;
		else
			return false;

		if (dir == '-')
			flags = static_cast<GSAccessFlags>(static_cast<unsigned int>(flags) & ~static_cast<unsigned int>(bit));
		else
			flags |= bit;
		return true;
	};

	for (auto token : tokens)
	{
		token.trim();
		if (token.empty())
			continue;

		char dir = '+';
		if (token[0] == '+' || token[0] == '-')
		{
			dir = token[0];
			token = token.substr(1);
		}
		if (dir == '-' && !allow_minus)
			continue;

		// Special:
		// - "+" means "add all permissions except founder" (Atheme behavior)
		// - "-" means "remove all permissions including founder"
		if (token.empty())
		{
			if (dir == '-')
				flags = GSAccessFlags::NONE;
			else
				flags |= GSAccessFlags::ALL_NOFOUNDER;
			continue;
		}

		Anope::string up = token;
		if (apply_one(up, dir))
			continue;

		// Support compact Atheme-style strings like +VI or -MS.
		Anope::string up2 = token.upper();
		if (up2.length() > 1)
		{
			bool any = false;
			for (const auto ch : up2)
			{
				if (!isalpha(static_cast<unsigned char>(ch)))
					continue;
				Anope::string one;
				one += ch;
				any |= apply_one(one, dir);
			}
			if (!any)
				continue;
		}
	}

	// Founder implies management powers.
	if (HasFlag(flags, GSAccessFlags::FOUNDER))
		flags |= GSAccessFlags::ALL_NOFOUNDER;

	return flags;
}

bool GroupServCore::ListChans(CommandSource& source, const Anope::string& groupname)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}

	if (!source.GetAccount())
	{
		this->Reply(source, "You must be identified to use LISTCHANS.");
		return false;
	}
	if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::ACLVIEW) && !this->IsAuspex(source))
	{
		this->Reply(source, "Access denied.");
		return false;
	}

	// Atheme semantics are "channels the group has access to".
	// In this module we implement that as explicit channel association via:
	//   /msg ChanServ SET #channel GROUP <!group>
	if (!this->chanaccess_item)
	{
		this->Reply(source, "LISTCHANS is not available (channel association storage not initialized).");
		return false;
	}

	std::vector<Anope::string> chans;
	for (const auto& [_, ci] : *RegisteredChannelList)
	{
		if (!ci)
			continue;

		auto* d = this->chanaccess_item->Get(ci);
		if (!d)
			continue;
		if (NormalizeKey(d->group) != NormalizeKey(g->name))
			continue;
		chans.push_back(ci->name);
	}
	std::sort(chans.begin(), chans.end());

	this->ReplyF(source, "Channels associated with %s:", g->name.c_str());
	if (chans.empty())
	{
		this->Reply(source, "(none)");
		return true;
	}

	for (const auto& ch : chans)
		this->ReplyF(source, "- %s", ch.c_str());
	this->ReplyF(source, "End of list - %u channel(s) shown.", static_cast<unsigned int>(chans.size()));
	return true;
}

bool GroupServCore::SetGroupFlag(CommandSource& source, const Anope::string& groupname, GSGroupFlags flag, bool enabled)
{
	if (!this->IsAdmin(source))
	{
		this->Reply(source, "Access denied.");
		return false;
	}

	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}

	if (enabled)
		g->flags |= flag;
	else
		g->flags = static_cast<GSGroupFlags>(static_cast<unsigned int>(g->flags) & ~static_cast<unsigned int>(flag));

	this->SaveDB();
	return true;
}

void GroupServCore::SetReplyMode(const Anope::string& mode)
{
	if (mode.equals_ci("privmsg"))
		this->reply_with_notice = false;
	else
		this->reply_with_notice = true;
}

void GroupServCore::Reply(CommandSource& source, const Anope::string& msg)
{
	if (!this->groupserv.operator bool())
	{
		source.Reply("%s", msg.c_str());
		return;
	}

	User* u = source.GetUser();
	if (!u)
	{
		source.Reply("%s", msg.c_str());
		return;
	}

	Anope::map<Anope::string> tags;
	if (!source.msgid.empty())
		tags["+draft/reply"] = source.msgid;

	LineWrapper lw(Language::Translate(u, msg.c_str()));
	for (Anope::string line; lw.GetLine(line); )
	{
		if (this->reply_with_notice)
			IRCD->SendNotice(*this->groupserv, u->GetUID(), line, tags);
		else
			IRCD->SendPrivmsg(*this->groupserv, u->GetUID(), line, tags);
	}
}

void GroupServCore::SendToUser(User* u, const Anope::string& msg)
{
	if (!this->groupserv.operator bool() || !u)
		return;

	Anope::map<Anope::string> tags;
	LineWrapper lw(Language::Translate(u, msg.c_str()));
	for (Anope::string line; lw.GetLine(line); )
	{
		if (this->reply_with_notice)
			IRCD->SendNotice(*this->groupserv, u->GetUID(), line, tags);
		else
			IRCD->SendPrivmsg(*this->groupserv, u->GetUID(), line, tags);
	}
}

void GroupServCore::ReplyF(CommandSource& source, const char* fmt, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	this->Reply(source, buf);
}

bool GroupServCore::IsAdmin(CommandSource& source) const
{
	return !this->admin_priv.empty() && source.HasPriv(this->admin_priv);
}

bool GroupServCore::IsAuspex(CommandSource& source) const
{
	return (!this->auspex_priv.empty() && source.HasPriv(this->auspex_priv)) || this->IsAdmin(source);
}

bool GroupServCore::CanExceedLimits(CommandSource& source) const
{
	return (!this->exceed_priv.empty() && source.HasPriv(this->exceed_priv)) || this->IsAdmin(source);
}

void GroupServCore::LoadDB()
{
	std::map<Anope::string, GSGroupRecord> new_groups;
	std::map<Anope::string, GSInvite> new_invites;
	std::map<Anope::string, Anope::string> joinflags_tmp; // groupkey -> raw joinflags string

	const auto path = this->GetDBPath();
	std::ifstream in(path.c_str());
	if (!in.is_open())
		return;

	auto load_legacy = [&]() {
		std::string raw;
		Anope::string line;
		bool header_ok = false;
		while (std::getline(in, raw))
		{
			line = Anope::string(raw);
			line.trim();
			if (line.empty())
				continue;

			if (!header_ok)
			{
				std::vector<Anope::string> parts;
				sepstream(line, '|').GetTokens(parts);
				if (parts.size() >= 2 && parts[0].equals_ci(LEGACY_DB_MAGIC))
				{
					header_ok = true;
					continue;
				}
				return;
			}

			std::vector<Anope::string> parts;
			sepstream(line, '|').GetTokens(parts);
			if (parts.size() < 2)
				continue;

			const Anope::string type = parts[0];
			if (type.equals_ci("G"))
			{
				if (parts.size() < 9)
					continue;
				GSGroupRecord g;
				g.name = UnescapeValue(parts[1]);
				uint64_t reg = 0;
				uint64_t gflags = 0;
				ParseU64(parts[2], reg);
				ParseU64(parts[3], gflags);
				g.regtime = static_cast<time_t>(reg);
				g.flags = static_cast<GSGroupFlags>(static_cast<unsigned int>(gflags));
				g.description = UnescapeValue(parts[4]);
				g.url = UnescapeValue(parts[5]);
				g.email = UnescapeValue(parts[6]);
				g.channel = UnescapeValue(parts[7]);
				const auto joinflags_raw = UnescapeValue(parts[8]);
				g.joinflags = this->ParseFlags(joinflags_raw, false, GSAccessFlags::NONE);
				new_groups.emplace(NormalizeKey(g.name), g);
			}
			else if (type.equals_ci("A"))
			{
				if (parts.size() < 4)
					continue;
				auto git = new_groups.find(NormalizeKey(UnescapeValue(parts[1])));
				if (git == new_groups.end())
					continue;
				const auto acct = NormalizeKey(UnescapeValue(parts[2]));
				uint64_t af = 0;
				ParseU64(parts[3], af);
				git->second.access[acct] = static_cast<GSAccessFlags>(static_cast<unsigned int>(af));
			}
			else if (type.equals_ci("I"))
			{
				if (parts.size() < 5)
					continue;
				GSInvite inv;
				const auto acct = NormalizeKey(UnescapeValue(parts[1]));
				inv.group = UnescapeValue(parts[2]);
				uint64_t created = 0;
				uint64_t expires = 0;
				ParseU64(parts[3], created);
				ParseU64(parts[4], expires);
				inv.created = static_cast<time_t>(created);
				inv.expires = static_cast<time_t>(expires);
				new_invites[acct] = inv;
			}
		}
	};

	// Detect legacy pipe-delimited format (first meaningful line starts with "groupserv|").
	{
		auto pos = in.tellg();
		std::string raw;
		for (; std::getline(in, raw); )
		{
			Anope::string s(raw.c_str());
			s.trim();
			if (s.empty() || s[0] == '#')
				continue;
			if (s.find('|') != Anope::string::npos)
			{
				std::vector<Anope::string> parts;
				sepstream(s, '|').GetTokens(parts);
				if (parts.size() >= 2 && parts[0].equals_ci(LEGACY_DB_MAGIC))
				{
					in.clear();
					in.seekg(pos);
					load_legacy();
					this->groups.swap(new_groups);
					this->invites.swap(new_invites);
					this->PurgeExpiredState();
					return;
				}
			}
			break;
		}
		in.clear();
		in.seekg(pos);
	}

	// HelpServ-style key=value flatfile.
	uint64_t version = 0;
	std::map<Anope::string, std::map<uint64_t, std::pair<Anope::string, uint64_t>>> access_tmp; // groupkey -> idx -> (acct, flags)
	struct InviteTmp { Anope::string account; Anope::string group; uint64_t created = 0; uint64_t expires = 0; };
	std::map<uint64_t, InviteTmp> invites_tmp;

	for (std::string raw; std::getline(in, raw); )
	{
		Anope::string s(raw.c_str());
		s.trim();
		if (s.empty() || s[0] == '#')
			continue;

		auto eq = s.find('=');
		if (eq == Anope::string::npos)
			continue;

		Anope::string key = s.substr(0, eq);
		Anope::string val = s.substr(eq + 1);
		key.trim();
		val = UnescapeValue(val);
		val.trim();

		if (key.equals_ci("version"))
		{
			ParseU64(val, version);
			continue;
		}

		std::vector<Anope::string> parts;
		sepstream(key, '.').GetTokens(parts);
		if (parts.size() < 3)
			continue;

		if (parts[0].equals_ci("group"))
		{
			const auto gkey = NormalizeKey(parts[1]);
			const auto field = parts[2];
			auto& g = new_groups[gkey];
			if (field.equals_ci("name"))
				g.name = val;
			else if (field.equals_ci("regtime"))
			{
				uint64_t ts = 0;
				if (ParseU64(val, ts))
					g.regtime = static_cast<time_t>(ts);
			}
			else if (field.equals_ci("flags"))
			{
				uint64_t n = 0;
				if (ParseU64(val, n))
					g.flags = static_cast<GSGroupFlags>(static_cast<unsigned int>(n));
			}
			else if (field.equals_ci("description"))
				g.description = val;
			else if (field.equals_ci("url"))
				g.url = val;
			else if (field.equals_ci("email"))
				g.email = val;
			else if (field.equals_ci("channel"))
				g.channel = val;
			else if (field.equals_ci("joinflags"))
				joinflags_tmp[gkey] = val;
			else if (field.equals_ci("access") && parts.size() >= 5)
			{
				uint64_t idx = 0;
				if (!ParseU64(parts[3], idx) || idx > 100000)
					continue;
				const auto afield = parts[4];
				auto& entry = access_tmp[gkey][idx];
				if (afield.equals_ci("account"))
					entry.first = NormalizeKey(val);
				else if (afield.equals_ci("flags"))
					ParseU64(val, entry.second);
			}
		}
		else if (parts[0].equals_ci("invite") && parts.size() >= 3)
		{
			uint64_t idx = 0;
			if (!ParseU64(parts[1], idx) || idx > 100000)
				continue;
			auto& inv = invites_tmp[idx];
			const auto field = parts[2];
			if (field.equals_ci("account"))
				inv.account = NormalizeKey(val);
			else if (field.equals_ci("group"))
				inv.group = val;
			else if (field.equals_ci("created"))
				ParseU64(val, inv.created);
			else if (field.equals_ci("expires"))
				ParseU64(val, inv.expires);
		}
	}

	if (version != 0 && version != DB_VERSION)
		return;

	// Finalize access entries and computed fields.
	for (auto& [gkey, g] : new_groups)
	{
		if (g.name.empty())
			g.name = gkey;
		const auto it = joinflags_tmp.find(gkey);
		g.joinflags = this->ParseFlags(it != joinflags_tmp.end() ? it->second : Anope::string(), false, GSAccessFlags::NONE);
		g.access.clear();

		auto ait = access_tmp.find(gkey);
		if (ait == access_tmp.end())
			continue;
		for (const auto& [_, entry] : ait->second)
		{
			if (entry.first.empty())
				continue;
			g.access[NormalizeKey(entry.first)] = static_cast<GSAccessFlags>(static_cast<unsigned int>(entry.second));
		}
	}

	for (const auto& [_, inv] : invites_tmp)
	{
		if (inv.account.empty() || inv.group.empty())
			continue;
		GSInvite out;
		out.group = inv.group;
		out.created = static_cast<time_t>(inv.created);
		out.expires = static_cast<time_t>(inv.expires);
		new_invites[NormalizeKey(inv.account)] = out;
	}

	this->groups.swap(new_groups);
	this->invites.swap(new_invites);
	this->PurgeExpiredState();
}

void GroupServCore::SaveDB()
{
	if (!this->initialized)
		return;
	if (Anope::ReadOnly)
		return;

	this->PurgeExpiredState();

	const auto path = this->GetDBPath();
	const auto tmp = path + ".tmp";

	std::ofstream out(tmp.c_str(), std::ios::out | std::ios::trunc);
	if (!out.is_open())
	{
		Log(this->module) << "Unable to write " << tmp;
		return;
	}

	out << "# GroupServ database\n";
	out << "version=" << DB_VERSION << "\n";

	std::vector<Anope::string> gkeys;
	gkeys.reserve(this->groups.size());
	for (const auto& [k, _] : this->groups)
		gkeys.push_back(k);
	std::sort(gkeys.begin(), gkeys.end());

	uint64_t invite_idx = 0;
	for (const auto& gkey : gkeys)
	{
		const auto& g = this->groups.at(gkey);
		out << "group." << gkey << ".name=" << EscapeValue(g.name) << "\n";
		out << "group." << gkey << ".regtime=" << static_cast<uint64_t>(g.regtime) << "\n";
		out << "group." << gkey << ".flags=" << static_cast<uint64_t>(static_cast<unsigned int>(g.flags)) << "\n";
		out << "group." << gkey << ".description=" << EscapeValue(g.description) << "\n";
		out << "group." << gkey << ".url=" << EscapeValue(g.url) << "\n";
		out << "group." << gkey << ".email=" << EscapeValue(g.email) << "\n";
		out << "group." << gkey << ".channel=" << EscapeValue(g.channel) << "\n";
		out << "group." << gkey << ".joinflags=" << EscapeValue(this->FlagsToString(g.joinflags)) << "\n";

		std::vector<std::pair<Anope::string, GSAccessFlags>> a;
		a.reserve(g.access.size());
		for (const auto& [acct, flags] : g.access)
			a.emplace_back(acct, flags);
		std::sort(a.begin(), a.end(), [](const auto& x, const auto& y) { return x.first < y.first; });
		for (size_t i = 0; i < a.size(); ++i)
		{
			out << "group." << gkey << ".access." << static_cast<uint64_t>(i) << ".account=" << EscapeValue(a[i].first) << "\n";
			out << "group." << gkey << ".access." << static_cast<uint64_t>(i) << ".flags=" << static_cast<uint64_t>(static_cast<unsigned int>(a[i].second)) << "\n";
		}
	}

	// Invites.
	std::vector<std::pair<Anope::string, GSInvite>> invs;
	invs.reserve(this->invites.size());
	for (const auto& kv : this->invites)
		invs.emplace_back(kv.first, kv.second);
	std::sort(invs.begin(), invs.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
	for (const auto& [acct, inv] : invs)
	{
		out << "invite." << invite_idx << ".account=" << EscapeValue(acct) << "\n";
		out << "invite." << invite_idx << ".group=" << EscapeValue(inv.group) << "\n";
		out << "invite." << invite_idx << ".created=" << static_cast<uint64_t>(inv.created) << "\n";
		out << "invite." << invite_idx << ".expires=" << static_cast<uint64_t>(inv.expires) << "\n";
		++invite_idx;
	}

	out.close();

	std::error_code ec;
	fs::rename(tmp.c_str(), path.c_str(), ec);
	if (ec)
	{
		fs::remove(path.c_str(), ec);
		ec.clear();
		fs::rename(tmp.c_str(), path.c_str(), ec);
		if (ec)
			Log(this->module) << "Unable to replace " << path << ": " << ec.message();
	}
}

void GroupServCore::OnReload(Configuration::Conf& conf)
{
	const Configuration::Block* mod = &conf.GetModule(this->module);
	Anope::string nick = mod->Get<const Anope::string>("client");
	if (nick.empty())
	{
		mod = &conf.GetModule("groupserv");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
	{
		mod = &conf.GetModule("groupserv.so");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
		throw ConfigException(this->module->name + ": <client> must be defined");

	BotInfo* bi = BotInfo::Find(nick, true);
	if (!bi)
		throw ConfigException(this->module->name + ": no bot named " + nick);
	this->groupserv = bi;

	this->admin_priv = mod->Get<Anope::string>("admin_priv", "groupserv/admin");
	this->auspex_priv = mod->Get<Anope::string>("auspex_priv", "groupserv/auspex");
	this->exceed_priv = mod->Get<Anope::string>("exceed_priv", "groupserv/exceed");

	this->maxgroups = mod->Get<unsigned int>("maxgroups", "5");
	this->maxgroupacs = mod->Get<unsigned int>("maxgroupacs", "0");
	this->enable_open_groups = mod->Get<bool>("enable_open_groups", "yes");

	this->default_joinflags = this->ParseFlags(mod->Get<Anope::string>("default_joinflags", ""), false, GSAccessFlags::NONE);
	// Atheme default: JOIN grants no privileges unless joinflags are configured.

	this->save_interval = mod->Get<time_t>("save_interval", "600");

	this->SetReplyMode(mod->Get<Anope::string>("reply_method", "notice"));

	if (!this->initialized)
	{
		this->LoadDB();
		this->initialized = true;
	}
}

bool GroupServCore::RegisterGroup(CommandSource& source, const Anope::string& groupname)
{
	if (!source.GetAccount())
	{
		this->Reply(source, "You must be identified to register a group.");
		return false;
	}
	if (!IsValidGroupName(groupname))
	{
		this->Reply(source, "Syntax: REGISTER <!group>");
		return false;
	}
	if (this->FindGroup(groupname))
	{
		this->ReplyF(source, "The group %s already exists.", groupname.c_str());
		return false;
	}

	if (this->maxgroups > 0 && !this->CanExceedLimits(source))
	{
		const auto founded = this->CountGroupsFoundedBy(source.GetAccount());
		if (founded >= this->maxgroups)
		{
			this->Reply(source, "You have too many groups registered.");
			return false;
		}
	}

	GSGroupRecord& g = this->GetOrCreateGroup(groupname);
	g.name = groupname;
	g.regtime = Anope::CurTime;
	g.flags = GSGroupFlags::NONE;
	g.joinflags = GSAccessFlags::NONE;
	g.access.clear();

	const auto acct = NormalizeKey(source.GetAccount()->display);
	g.access[acct] = GSAccessFlags::ALL | GSAccessFlags::FOUNDER;

	this->SaveDB();
	this->ReplyF(source, "The group %s has been registered to %s.", g.name.c_str(), source.GetAccount()->display.c_str());
	return true;
}

bool GroupServCore::DropGroup(CommandSource& source, const Anope::string& groupname, const Anope::string& key, bool force)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "Group %s does not exist.", groupname.c_str());
		return false;
	}

	if (force)
	{
		if (!this->IsAdmin(source))
		{
			this->Reply(source, "Access denied.");
			return false;
		}
	}
	else
	{
		if (!source.GetAccount())
		{
			this->Reply(source, "You must be identified to drop a group.");
			return false;
		}
		if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::FOUNDER))
		{
			this->Reply(source, "Access denied.");
			return false;
		}

		const auto chal_key = DropChallengeKey(source.GetAccount()->display, g->name);
		if (key.empty())
		{
			const auto token = Anope::Random(12);
			this->drop_challenges[chal_key] = { token, Anope::CurTime };
			this->ReplyF(source, "This will DESTROY the group %s.", g->name.c_str());
			this->ReplyF(source, "To confirm: /msg %s DROP %s %s", this->groupserv ? this->groupserv->nick.c_str() : "GroupServ", g->name.c_str(), token.c_str());
			return false;
		}

		auto it = this->drop_challenges.find(chal_key);
		if (it != this->drop_challenges.end() && it->second.created && (it->second.created + DROP_CHALLENGE_TTL) < Anope::CurTime)
		{
			this->drop_challenges.erase(it);
			it = this->drop_challenges.end();
		}
		if (it == this->drop_challenges.end() || it->second.token != key)
		{
			this->Reply(source, "Invalid key for DROP.");
			return false;
		}
		this->drop_challenges.erase(it);
	}

	// Remove any pending invites to this group.
	for (auto it = this->invites.begin(); it != this->invites.end();)
	{
		if (it->second.group.equals_ci(g->name))
			it = this->invites.erase(it);
		else
			++it;
	}

	this->groups.erase(NormalizeKey(g->name));
	this->SaveDB();
	this->ReplyF(source, "The group %s has been dropped.", groupname.c_str());
	return true;
}

bool GroupServCore::ShowInfo(CommandSource& source, const Anope::string& groupname)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "Group %s does not exist.", groupname.c_str());
		return false;
	}

	NickCore* nc = source.GetAccount();
	const auto access = this->GetAccessFor(*g, nc);
	const bool allowed = this->IsAuspex(source) || HasFlag(g->flags, GSGroupFlags::PUBLIC) || (nc && access != GSAccessFlags::NONE && !HasFlag(access, GSAccessFlags::BAN));
	if (!allowed)
	{
		this->Reply(source, "Access denied.");
		return false;
	}

	this->ReplyF(source, "Information for %s:", g->name.c_str());
	this->ReplyF(source, "Registered: %s", Anope::strftime(g->regtime, nc).c_str());
	this->ReplyF(source, "Flags: %s", this->GroupFlagsToString(g->flags).c_str());

	// Founders
	Anope::string founders;
	for (const auto& [acct, fl] : g->access)
	{
		if (HasFlag(fl, GSAccessFlags::FOUNDER))
		{
			if (!founders.empty())
				founders += ", ";
			founders += acct;
		}
	}
	if (!founders.empty())
		this->ReplyF(source, "Founders: %s", founders.c_str());

	if (!g->description.empty())
		this->ReplyF(source, "Description: %s", g->description.c_str());
	if (!g->channel.empty())
		this->ReplyF(source, "Channel: %s", g->channel.c_str());
	if (!g->url.empty())
		this->ReplyF(source, "URL: %s", g->url.c_str());
	if (!g->email.empty())
		this->ReplyF(source, "Email: %s", g->email.c_str());

	if (g->joinflags != GSAccessFlags::NONE)
		this->ReplyF(source, "Join flags: +%s", this->FlagsToString(g->joinflags).c_str());

	this->ReplyF(source, "Access entries: %u", static_cast<unsigned int>(g->access.size()));
	this->Reply(source, "*** End of Info ***");
	return true;
}

bool GroupServCore::ListGroups(CommandSource& source, const Anope::string& pattern)
{
	if (!this->IsAuspex(source))
	{
		this->Reply(source, "Access denied.");
		return false;
	}
	if (pattern.empty())
	{
		this->Reply(source, "Syntax: LIST <pattern>");
		return false;
	}

	unsigned int matches = 0;
	this->ReplyF(source, "Groups matching pattern %s:", pattern.c_str());

	std::vector<Anope::string> names;
	names.reserve(this->groups.size());
	for (const auto& [_, g] : this->groups)
		names.push_back(g.name);
	std::sort(names.begin(), names.end());
	for (const auto& name : names)
	{
		if (!Anope::Match(name, pattern))
			continue;
		this->ReplyF(source, "- %s", name.c_str());
		++matches;
	}

	if (!matches)
		this->ReplyF(source, "No groups matched pattern %s", pattern.c_str());
	else
		this->ReplyF(source, "%u match(es) for pattern %s", matches, pattern.c_str());
	return true;
}

bool GroupServCore::JoinGroup(CommandSource& source, const Anope::string& groupname)
{
	if (!source.GetAccount())
	{
		this->Reply(source, "You must be identified to join a group.");
		return false;
	}

	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "Group %s does not exist.", groupname.c_str());
		return false;
	}

	const auto acct = NormalizeKey(source.GetAccount()->display);
	auto existing = g->access.find(acct);
	if (existing != g->access.end())
	{
		if (HasFlag(existing->second, GSAccessFlags::BAN))
		{
			this->ReplyF(source, "You are banned from group %s.", g->name.c_str());
			return false;
		}
		this->ReplyF(source, "You are already a member of group %s.", g->name.c_str());
		return false;
	}

	bool invited = false;
	auto inv_it = this->invites.find(acct);
	if (inv_it != this->invites.end())
	{
		if (inv_it->second.expires && inv_it->second.expires < Anope::CurTime)
			this->invites.erase(inv_it);
		else if (inv_it->second.group.equals_ci(g->name))
			invited = true;
	}

	if (!invited)
	{
		if (!this->enable_open_groups || !HasFlag(g->flags, GSGroupFlags::OPEN))
		{
			this->ReplyF(source, "Group %s is not open to anyone joining.", g->name.c_str());
			return false;
		}
	}

	if (this->maxgroupacs > 0 && !HasFlag(g->flags, GSGroupFlags::ACSNOLIMIT) && !this->CanExceedLimits(source) && !invited)
	{
		if (g->access.size() >= this->maxgroupacs)
		{
			this->ReplyF(source, "Group %s access list is full.", g->name.c_str());
			return false;
		}
	}

	GSAccessFlags joinflags = g->joinflags;
	if (joinflags == GSAccessFlags::NONE)
		joinflags = this->default_joinflags;
	g->access[acct] = joinflags;

	if (invited)
		this->invites.erase(acct);

	this->SaveDB();
	this->ReplyF(source, "You are now a member of %s.", g->name.c_str());
	return true;
}

bool GroupServCore::InviteToGroup(CommandSource& source, const Anope::string& groupname, const Anope::string& account)
{
	if (!source.GetAccount())
	{
		this->Reply(source, "You must be identified to invite to a group.");
		return false;
	}

	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}

	if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::INVITE))
	{
		this->Reply(source, "Access denied.");
		return false;
	}

	NickCore* target = this->FindAccount(account);
	if (!target)
	{
		this->ReplyF(source, "%s is not a registered account.", account.c_str());
		return false;
	}

	const auto tkey = NormalizeKey(target->display);
	if (g->access.find(tkey) != g->access.end())
	{
		this->ReplyF(source, "%s is already a member of %s.", target->display.c_str(), g->name.c_str());
		return false;
	}

	auto existing = this->invites.find(tkey);
	if (existing != this->invites.end() && !existing->second.group.equals_ci(g->name))
	{
		this->ReplyF(source, "%s already has another invitation pending.", target->display.c_str());
		return false;
	}

	GSInvite inv;
	inv.group = g->name;
	inv.created = Anope::CurTime;
	inv.expires = Anope::CurTime + INVITE_TTL;
	this->invites[tkey] = inv;

	this->SaveDB();
	if (this->groupserv.operator bool())
	{
		User* direct = User::Find(account, true);
		if (direct)
		{
			this->SendToUser(direct, Anope::Format("You have been invited to %s by %s. To accept: /msg %s JOIN %s",
				g->name.c_str(), source.GetNick().c_str(), this->groupserv->nick.c_str(), g->name.c_str()));
		}

		for (User* u : target->users)
		{
			if (!u)
				continue;
			if (u == direct)
				continue;
			this->SendToUser(u, Anope::Format("You have been invited to %s by %s. To accept: /msg %s JOIN %s",
				g->name.c_str(), source.GetNick().c_str(), this->groupserv->nick.c_str(), g->name.c_str()));
		}
	}
	this->ReplyF(source, "%s has been invited to %s.", target->display.c_str(), g->name.c_str());
	return true;
}

bool GroupServCore::ShowFlags(CommandSource& source, const Anope::string& groupname)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}

	const bool operoverride = this->IsAuspex(source) && !this->HasAccess(*g, source.GetAccount(), GSAccessFlags::ACLVIEW);
	if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::ACLVIEW) && !this->IsAuspex(source))
	{
		this->Reply(source, "Access denied.");
		return false;
	}
	if (operoverride && !this->IsAuspex(source))
	{
		this->Reply(source, "Access denied.");
		return false;
	}

	this->ReplyF(source, "Entry  Account                 Flags");
	this->Reply(source, "-----------------------------------------------");

	std::vector<std::pair<Anope::string, GSAccessFlags>> a;
	for (const auto& [acct, flags] : g->access)
		a.emplace_back(acct, flags);
	std::sort(a.begin(), a.end(), [](const auto& x, const auto& y) { return x.first < y.first; });

	unsigned int i = 1;
	for (const auto& [acct, flags] : a)
	{
		this->ReplyF(source, "%-5u  %-22s %s", i, acct.c_str(), this->FlagsToString(flags).c_str());
		++i;
	}

	this->Reply(source, "-----------------------------------------------");
	this->ReplyF(source, "End of %s FLAGS listing.", g->name.c_str());
	return true;
}

bool GroupServCore::SetFlags(CommandSource& source, const Anope::string& groupname, const Anope::string& account, const Anope::string& changes, bool force)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}

	if (force)
	{
		if (!this->IsAdmin(source))
		{
			this->Reply(source, "Access denied.");
			return false;
		}
	}
	else
	{
		if (!source.GetAccount())
		{
			this->Reply(source, "You must be identified to change flags.");
			return false;
		}
		if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::FLAGS))
		{
			this->Reply(source, "Access denied.");
			return false;
		}
	}

	NickCore* target = this->FindAccount(account);
	if (!target)
	{
		this->ReplyF(source, "%s is not a registered account.", account.c_str());
		return false;
	}

	const auto tkey = NormalizeKey(target->display);
	GSAccessFlags cur = GSAccessFlags::NONE;
	auto it = g->access.find(tkey);
	if (it != g->access.end())
		cur = it->second;

	GSAccessFlags updated = this->ParseFlags(changes, true, cur);

	// Non-founders cannot add or remove founder.
	if (!force && source.GetAccount() && !HasFlag(this->GetAccessFor(*g, source.GetAccount()), GSAccessFlags::FOUNDER))
	{
		if (HasFlag(updated, GSAccessFlags::FOUNDER) != HasFlag(cur, GSAccessFlags::FOUNDER))
		{
			this->Reply(source, "You may not change founder status.");
			return false;
		}
	}

	// Prevent removing last founder.
	if (HasFlag(cur, GSAccessFlags::FOUNDER) && !HasFlag(updated, GSAccessFlags::FOUNDER))
	{
		unsigned int founders = 0;
		for (const auto& [_, fl] : g->access)
			if (HasFlag(fl, GSAccessFlags::FOUNDER))
				++founders;
		if (founders <= 1)
		{
			this->Reply(source, "You may not remove the last founder.");
			return false;
		}
	}

	if (updated == GSAccessFlags::NONE)
	{
		if (it != g->access.end())
			g->access.erase(it);
		this->SaveDB();
		this->ReplyF(source, "%s has been removed from %s.", target->display.c_str(), g->name.c_str());
		return true;
	}

	if (it == g->access.end() && this->maxgroupacs > 0 && !HasFlag(g->flags, GSGroupFlags::ACSNOLIMIT) && !this->CanExceedLimits(source))
	{
		if (g->access.size() >= this->maxgroupacs)
		{
			this->ReplyF(source, "Group %s access list is full.", g->name.c_str());
			return false;
		}
	}

	g->access[tkey] = updated;
	this->SaveDB();
	this->ReplyF(source, "%s now has flags %s on %s.", target->display.c_str(), this->FlagsToString(updated).c_str(), g->name.c_str());
	return true;
}

bool GroupServCore::SetOption(CommandSource& source, const Anope::string& groupname, const Anope::string& setting, const Anope::string& value)
{
	GSGroupRecord* g = this->FindGroup(groupname);
	if (!g)
	{
		this->ReplyF(source, "The group %s does not exist.", groupname.c_str());
		return false;
	}
	if (!source.GetAccount())
	{
		this->Reply(source, "You must be identified to use SET.");
		return false;
	}

	const auto up = setting.upper();
	const bool is_founder = this->HasAccess(*g, source.GetAccount(), GSAccessFlags::FOUNDER);
	if (up == "OPEN" || up == "PUBLIC")
	{
		if (!is_founder)
		{
			this->Reply(source, "Access denied.");
			return false;
		}
	}
	else
	{
		if (!this->HasAccess(*g, source.GetAccount(), GSAccessFlags::SET))
		{
			this->Reply(source, "Access denied.");
			return false;
		}
	}

	if (up == "GROUPNAME")
	{
		Anope::string newname = value;
		newname.trim();
		if (!is_founder)
		{
			this->Reply(source, "Access denied.");
			return false;
		}
		if (!IsValidGroupName(newname))
		{
			this->Reply(source, "Syntax: SET <!group> GROUPNAME <!newgroup>");
			return false;
		}
		if (this->FindGroup(newname))
		{
			this->ReplyF(source, "The group %s already exists.", newname.c_str());
			return false;
		}

		const auto oldkey = NormalizeKey(g->name);
		const auto newkey = NormalizeKey(newname);
		GSGroupRecord copy = *g;
		copy.name = newname;

		this->groups.erase(oldkey);
		this->groups.emplace(newkey, copy);

		for (auto& [acct, inv] : this->invites)
		{
			if (inv.group.equals_ci(groupname))
				inv.group = newname;
		}

		for (auto it = this->drop_challenges.begin(); it != this->drop_challenges.end();)
		{
			auto bar = it->first.find('|');
			if (bar != Anope::string::npos)
			{
				Anope::string gpart = it->first.substr(bar + 1);
				if (gpart.equals_ci(oldkey) || gpart.equals_ci(newkey))
				{
					it = this->drop_challenges.erase(it);
					continue;
				}
			}
			++it;
		}

		this->SaveDB();
		this->ReplyF(source, "Group %s has been renamed to %s.", groupname.c_str(), newname.c_str());
		return true;
	}
	if (up == "DESCRIPTION")
	{
		g->description = value;
		this->SaveDB();
		this->ReplyF(source, "Description for %s set.", g->name.c_str());
		return true;
	}
	if (up == "URL")
	{
		g->url = value;
		this->SaveDB();
		this->ReplyF(source, "URL for %s set.", g->name.c_str());
		return true;
	}
	if (up == "EMAIL")
	{
		g->email = value;
		this->SaveDB();
		this->ReplyF(source, "Email for %s set.", g->name.c_str());
		return true;
	}
	if (up == "CHANNEL")
	{
		g->channel = value;
		this->SaveDB();
		this->ReplyF(source, "Channel for %s set.", g->name.c_str());
		return true;
	}
	if (up == "JOINFLAGS")
	{
		Anope::string v = value;
		v.trim();
		if (v.empty() || v.equals_ci("OFF") || v.equals_ci("NONE"))
		{
			g->joinflags = GSAccessFlags::NONE;
			this->SaveDB();
			this->ReplyF(source, "The group-specific join flags for %s have been cleared.", g->name.c_str());
			return true;
		}
		if (v[0] == '-')
		{
			this->Reply(source, "You can't set join flags to be removed.");
			return false;
		}
		g->joinflags = this->ParseFlags(v, false, GSAccessFlags::NONE);
		// Atheme behavior: if invalid, JOINFLAGS becomes "+" (all except founder).
		if (g->joinflags == GSAccessFlags::NONE)
			g->joinflags = GSAccessFlags::ALL_NOFOUNDER;
		this->SaveDB();
		this->ReplyF(source, "Join flags of %s set to %s.", g->name.c_str(), v.c_str());
		return true;
	}
	if (up == "OPEN")
	{
		if (value.equals_ci("ON"))
		{
			if (!this->enable_open_groups)
			{
				this->Reply(source, "Setting groups as open has been administratively disabled.");
				return false;
			}
			g->flags |= GSGroupFlags::OPEN;
			this->SaveDB();
			this->ReplyF(source, "%s is now open to anyone joining.", g->name.c_str());
			return true;
		}
		if (value.equals_ci("OFF"))
		{
			g->flags = static_cast<GSGroupFlags>(static_cast<unsigned int>(g->flags) & ~static_cast<unsigned int>(GSGroupFlags::OPEN));
			this->SaveDB();
			this->ReplyF(source, "%s is no longer open to anyone joining.", g->name.c_str());
			return true;
		}
		this->Reply(source, "Syntax: SET <!group> OPEN <ON|OFF>");
		return false;
	}
	if (up == "PUBLIC")
	{
		if (value.equals_ci("ON"))
		{
			g->flags |= GSGroupFlags::PUBLIC;
			this->SaveDB();
			this->ReplyF(source, "%s is now public.", g->name.c_str());
			return true;
		}
		if (value.equals_ci("OFF"))
		{
			g->flags = static_cast<GSGroupFlags>(static_cast<unsigned int>(g->flags) & ~static_cast<unsigned int>(GSGroupFlags::PUBLIC));
			this->SaveDB();
			this->ReplyF(source, "%s is no longer public.", g->name.c_str());
			return true;
		}
		this->Reply(source, "Syntax: SET <!group> PUBLIC <ON|OFF>");
		return false;
	}

	this->Reply(source, "Unknown setting.");
	return false;
}
