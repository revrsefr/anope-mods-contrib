#include "chanfix.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace fs = std::filesystem;

ChanFixCore::ChanFixCore(Module* owner)
	: module(owner)
{
	this->LoadDB();
}

ChanFixCore::~ChanFixCore()
{
	this->SaveDB();
}

Anope::string ChanFixCore::GetDBPath() const
{
	return Anope::ExpandData("chanfix.db");
}

Anope::string ChanFixCore::EscapeValue(const Anope::string& in)
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

Anope::string ChanFixCore::UnescapeValue(const Anope::string& in)
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

void ChanFixCore::LoadDB()
{
	this->channels.clear();

	const fs::path path(this->GetDBPath().c_str());
	std::error_code ec;
	if (!fs::exists(path, ec))
		return;

	std::ifstream in(path, std::ios::in);
	if (!in.is_open())
		return;

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
			if (parts.size() >= 2 && parts[0].equals_ci(DB_MAGIC))
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
		if (type.equals_ci("C"))
		{
			if (parts.size() < 6)
				continue;

			CFChannelRecord rec;
			rec.name = UnescapeValue(parts[1]);
			try { rec.ts = static_cast<time_t>(Anope::Convert<uint64_t>(parts[2], 0)); } catch (...) { rec.ts = 0; }
			try { rec.lastupdate = static_cast<time_t>(Anope::Convert<uint64_t>(parts[3], 0)); } catch (...) { rec.lastupdate = 0; }
			try { rec.fix_started = static_cast<time_t>(Anope::Convert<uint64_t>(parts[4], 0)); } catch (...) { rec.fix_started = 0; }
			rec.fix_requested = (parts[5] == "1");

			// Optional metadata.
			if (parts.size() >= 9)
			{
				rec.marked = (parts[6] == "1");
				rec.mark_setter = UnescapeValue(parts[7]);
				try { rec.mark_time = static_cast<time_t>(Anope::Convert<uint64_t>(parts[8], 0)); } catch (...) { rec.mark_time = 0; }
				if (parts.size() >= 10)
					rec.mark_reason = UnescapeValue(parts[9]);
			}
			if (parts.size() >= 13)
			{
				rec.nofix = (parts[10] == "1");
				rec.nofix_setter = UnescapeValue(parts[11]);
				try { rec.nofix_time = static_cast<time_t>(Anope::Convert<uint64_t>(parts[12], 0)); } catch (...) { rec.nofix_time = 0; }
				if (parts.size() >= 14)
					rec.nofix_reason = UnescapeValue(parts[13]);
			}

			this->channels[rec.name] = std::move(rec);
		}
		else if (type.equals_ci("O"))
		{
			if (parts.size() < 8)
				continue;
			Anope::string chname = UnescapeValue(parts[1]);
			auto it = this->channels.find(chname);
			if (it == this->channels.end())
			{
				CFChannelRecord rec;
				rec.name = chname;
				this->channels[chname] = std::move(rec);
				it = this->channels.find(chname);
			}

			CFOpRecord o;
			o.account = UnescapeValue(parts[2]);
			o.user = UnescapeValue(parts[3]);
			o.host = UnescapeValue(parts[4]);
			try { o.firstseen = static_cast<time_t>(Anope::Convert<uint64_t>(parts[5], 0)); } catch (...) { o.firstseen = 0; }
			try { o.lastevent = static_cast<time_t>(Anope::Convert<uint64_t>(parts[6], 0)); } catch (...) { o.lastevent = 0; }
			try { o.age = Anope::Convert<unsigned int>(parts[7], 0); } catch (...) { o.age = 0; }

			Anope::string key = (o.account.empty() || o.account == "*") ? (o.user + "@" + o.host) : o.account;
			it->second.oprecords[key] = std::move(o);
		}
	}
}

void ChanFixCore::SaveDB() const
{
	const fs::path path(this->GetDBPath().c_str());
	std::error_code ec;
	fs::create_directories(path.parent_path(), ec);

	const fs::path tmp = path.string() + ".tmp";
	std::ofstream out(tmp, std::ios::out | std::ios::trunc);
	if (!out.is_open())
		return;

	out << DB_MAGIC << "|" << DB_VERSION << "\n";
	for (const auto& [_, rec] : this->channels)
	{
		out << "C|" << EscapeValue(rec.name)
			<< "|" << static_cast<uint64_t>(rec.ts)
			<< "|" << static_cast<uint64_t>(rec.lastupdate)
			<< "|" << static_cast<uint64_t>(rec.fix_started)
			<< "|" << (rec.fix_requested ? "1" : "0")
			<< "|" << (rec.marked ? "1" : "0")
			<< "|" << EscapeValue(rec.mark_setter)
			<< "|" << static_cast<uint64_t>(rec.mark_time)
			<< "|" << EscapeValue(rec.mark_reason)
			<< "|" << (rec.nofix ? "1" : "0")
			<< "|" << EscapeValue(rec.nofix_setter)
			<< "|" << static_cast<uint64_t>(rec.nofix_time)
			<< "|" << EscapeValue(rec.nofix_reason)
			<< "\n";

		for (const auto& [key, o] : rec.oprecords)
		{
			out << "O|" << EscapeValue(rec.name)
				<< "|" << EscapeValue(o.account.empty() ? "*" : o.account)
				<< "|" << EscapeValue(o.user)
				<< "|" << EscapeValue(o.host)
				<< "|" << static_cast<uint64_t>(o.firstseen)
				<< "|" << static_cast<uint64_t>(o.lastevent)
				<< "|" << o.age
				<< "\n";
		}
	}

	out.close();

	fs::rename(tmp, path, ec);
	if (ec)
	{
		fs::remove(path, ec);
		fs::rename(tmp, path, ec);
	}
}

bool ChanFixCore::IsValidChannelName(const Anope::string& name)
{
	return !name.empty() && name[0] == '#';
}

bool ChanFixCore::IsRegistered(Channel* c) const
{
	if (!c)
		return false;
	if (c->ci)
		return true;
	return ChannelInfo::Find(c->name) != NULL;
}

CFChannelRecord* ChanFixCore::GetRecord(const Anope::string& chname)
{
	auto it = this->channels.find(chname);
	if (it == this->channels.end())
		return nullptr;
	return &it->second;
}

CFChannelRecord& ChanFixCore::GetOrCreateRecord(Channel* c)
{
	auto it = this->channels.find(c->name);
	if (it != this->channels.end())
		return it->second;

	CFChannelRecord rec;
	rec.name = c->name;
	rec.ts = c->created;
	rec.lastupdate = Anope::CurTime;
	this->channels[rec.name] = std::move(rec);
	return this->channels[c->name];
}

unsigned int ChanFixCore::CountOps(Channel* c) const
{
	if (!c)
		return 0;

	unsigned int n = 0;
	for (const auto& [u, cuc] : c->users)
	{
		if (!u || !cuc)
			continue;
		if (cuc->status.HasMode(this->op_status_char))
			++n;
	}
	return n;
}

Anope::string ChanFixCore::KeyForUser(User* u) const
{
	if (!u)
		return "";
	if (u->Account())
		return u->Account()->display;
	return u->GetVIdent() + "@" + u->GetDisplayedHost();
}

CFOpRecord* ChanFixCore::FindRecord(CFChannelRecord& rec, User* u)
{
	Anope::string key = this->KeyForUser(u);
	if (key.empty())
		return nullptr;

	auto it = rec.oprecords.find(key);
	if (it != rec.oprecords.end())
		return &it->second;

	// If user gained an account, try to upgrade from hostkey.
	if (u->Account())
	{
		Anope::string hostkey = u->GetVIdent() + "@" + u->GetDisplayedHost();
		auto it2 = rec.oprecords.find(hostkey);
		if (it2 != rec.oprecords.end())
		{
			it2->second.account = u->Account()->display;
			rec.oprecords[key] = it2->second;
			rec.oprecords.erase(it2);
			auto it3 = rec.oprecords.find(key);
			if (it3 != rec.oprecords.end())
				return &it3->second;
		}
	}

	return nullptr;
}

void ChanFixCore::UpdateOpRecord(CFChannelRecord& rec, User* u)
{
	if (!u || u->Quitting())
		return;
	if (u->server && u->server->IsULined())
		return;

	CFOpRecord* existing = this->FindRecord(rec, u);
	if (existing)
	{
		existing->age++;
		existing->lastevent = Anope::CurTime;
		if (existing->account.empty() && u->Account())
			existing->account = u->Account()->display;
		return;
	}

	CFOpRecord o;
	o.account = u->Account() ? u->Account()->display : "*";
	o.user = u->GetVIdent();
	o.host = u->GetDisplayedHost();
	o.firstseen = Anope::CurTime;
	o.lastevent = Anope::CurTime;
	o.age = 1;

	Anope::string key = this->KeyForUser(u);
	rec.oprecords[key] = std::move(o);
	rec.lastupdate = Anope::CurTime;
}

unsigned int ChanFixCore::CalculateScore(const CFOpRecord& orec) const
{
	double base = static_cast<double>(orec.age);
	if (!orec.account.empty() && orec.account != "*")
		base *= this->account_weight;

	if (base < 0)
		base = 0;
	if (base > static_cast<double>(std::numeric_limits<unsigned int>::max()))
		base = static_cast<double>(std::numeric_limits<unsigned int>::max());
	return static_cast<unsigned int>(base);
}

unsigned int ChanFixCore::GetHighScore(const CFChannelRecord& rec) const
{
	unsigned int high = 0;
	for (const auto& [_, o] : rec.oprecords)
	{
		const unsigned int score = this->CalculateScore(o);
		if (score > high)
			high = score;
	}
	return high;
}

unsigned int ChanFixCore::GetThreshold(const CFChannelRecord& rec, time_t now) const
{
	unsigned int highscore = this->GetHighScore(rec);
	if (highscore == 0)
		return 1;

	time_t t = now - rec.fix_started;
	if (t < 0)
		t = 0;
	if (t > this->fix_time)
		t = this->fix_time;

	double step = this->initial_step;
	if (this->fix_time > 0)
		step = this->initial_step + (this->final_step - this->initial_step) * (static_cast<double>(t) / static_cast<double>(this->fix_time));

	double threshold = static_cast<double>(highscore) * step;
	if (threshold < 1.0)
		threshold = 1.0;
	return static_cast<unsigned int>(threshold);
}

bool ChanFixCore::ShouldHandle(CFChannelRecord& rec, Channel* c) const
{
	if (!c)
		return false;
	if (this->IsRegistered(c))
		return false;
	if (rec.nofix)
		return false;

	const unsigned int ops = this->CountOps(c);
	if (ops >= this->op_threshold)
		return false;

	// Only fix opless channels, and consider a fix done after fix_time if any ops were given.
	if (ops > 0 && (rec.fix_started == 0 || (Anope::CurTime - rec.fix_started) > this->fix_time))
		return false;

	return true;
}

bool ChanFixCore::CanStartFix(const CFChannelRecord& rec, Channel* c) const
{
	if (!c)
		return false;
	if (this->CountOps(c) > 0)
		return false;

	const unsigned int highscore = this->GetHighScore(rec);
	if (highscore < this->min_fix_score)
		return false;

	const unsigned int threshold = static_cast<unsigned int>(static_cast<double>(highscore) * this->final_step);
	for (const auto& [u, cuc] : c->users)
	{
		if (!u || !cuc || u == this->chanfix)
			continue;
		if (cuc->status.HasMode(this->op_status_char))
			continue;

		const CFOpRecord* orec = nullptr;
		auto it = rec.oprecords.find(this->KeyForUser(u));
		if (it != rec.oprecords.end())
			orec = &it->second;
		if (!orec)
			continue;

		if (this->CalculateScore(*orec) >= threshold)
			return true;
	}

	return false;
}

bool ChanFixCore::FixChannel(CFChannelRecord& rec, Channel* c)
{
	if (!c || !this->chanfix)
		return false;

	const unsigned int threshold = this->GetThreshold(rec, Anope::CurTime);
	unsigned int opped = 0;
	const bool already_in_chan = c->FindUser(this->chanfix) != NULL;
	bool joined = already_in_chan;

	for (const auto& [u, cuc] : c->users)
	{
		if (!u || !cuc || u == this->chanfix)
			continue;
		if (cuc->status.HasMode(this->op_status_char))
			continue;

		const CFOpRecord* orec = nullptr;
		auto it = rec.oprecords.find(this->KeyForUser(u));
		if (it != rec.oprecords.end())
			orec = &it->second;
		if (!orec)
			continue;

		if (this->CalculateScore(*orec) < threshold)
			continue;

		if (this->join_to_fix && !joined)
		{
			this->chanfix->Join(c);
			joined = true;
		}

		c->SetMode(this->chanfix, "OP", u->GetUID(), false);
		opped++;
	}

	if (opped == 0)
		return false;

	ModeManager::ProcessModes();

	if (this->join_to_fix && joined && !already_in_chan)
		this->chanfix->Part(c, "chanfix");

	return true;
}

void ChanFixCore::ClearBans(Channel* c)
{
	if (!c || !this->chanfix)
		return;

	const bool already_in_chan = c->FindUser(this->chanfix) != NULL;
	bool joined = already_in_chan;

	auto join_if_needed = [&]()
	{
		if (this->join_to_fix && !joined)
		{
			this->chanfix->Join(c);
			joined = true;
		}
	};

	if (c->HasMode("INVITE"))
	{
		join_if_needed();
		c->RemoveMode(this->chanfix, "INVITE", "", false);
	}
	if (c->HasMode("LIMIT"))
	{
		join_if_needed();
		c->RemoveMode(this->chanfix, "LIMIT", "", false);
	}
	if (c->HasMode("KEY"))
	{
		Anope::string key;
		if (c->GetParam("KEY", key))
		{
			join_if_needed();
			c->RemoveMode(this->chanfix, "KEY", key, false);
		}
	}

	for (const auto& mask : c->GetModeList("BAN"))
	{
		join_if_needed();
		c->RemoveMode(this->chanfix, "BAN", mask, false);
	}

	if (!joined)
		return;

	ModeManager::ProcessModes();

	if (this->join_to_fix && joined && !already_in_chan)
		this->chanfix->Part(c, "chanfix");
}

void ChanFixCore::OnReload(Configuration::Conf& conf)
{
	const Configuration::Block* mod = &conf.GetModule(this->module);
	Anope::string nick = mod->Get<const Anope::string>("client");
	if (nick.empty())
	{
		mod = &conf.GetModule("chanfix");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
	{
		mod = &conf.GetModule("chanfix.so");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
		throw ConfigException(this->module->name + ": <client> must be defined");

	BotInfo* bi = BotInfo::Find(nick, true);
	if (!bi)
		throw ConfigException(this->module->name + ": no bot named " + nick);
	this->chanfix = bi;

	this->admin_priv = mod->Get<Anope::string>("admin_priv", "chanfix/admin");
	this->auspex_priv = mod->Get<Anope::string>("auspex_priv", "chanfix/auspex");

	this->do_autofix = mod->Get<bool>("autofix", "no");
	this->join_to_fix = mod->Get<bool>("join_to_fix", "no");

	this->op_threshold = mod->Get<unsigned int>("op_threshold", "3");
	this->min_fix_score = mod->Get<unsigned int>("min_fix_score", "12");
	this->account_weight = mod->Get<double>("account_weight", "1.5");
	this->initial_step = mod->Get<double>("initial_step", "0.70");
	this->final_step = mod->Get<double>("final_step", "0.30");

	this->retention_time = mod->Get<time_t>("retention_time", "2419200");
	this->fix_time = mod->Get<time_t>("fix_time", "3600");
	this->gather_interval = mod->Get<time_t>("gather_interval", "300");
	this->expire_interval = mod->Get<time_t>("expire_interval", "3600");
	this->autofix_interval = mod->Get<time_t>("autofix_interval", "60");
	this->save_interval = mod->Get<time_t>("save_interval", "600");
	this->expire_divisor = mod->Get<unsigned int>("expire_divisor", "672");

	ChannelMode* opmode = ModeManager::FindChannelModeByName("OP");
	ChannelModeStatus* cms = anope_dynamic_static_cast<ChannelModeStatus*>(opmode);
	if (cms)
		this->op_status_char = cms->mchar;
	else
		this->op_status_char = 'o';
}

bool ChanFixCore::IsAdmin(CommandSource& source) const
{
	return source.HasPriv(this->admin_priv);
}

bool ChanFixCore::IsAuspex(CommandSource& source) const
{
	return source.HasPriv(this->auspex_priv) || this->IsAdmin(source);
}

void ChanFixCore::GatherTick()
{
	if (!Me->IsSynced())
		return;

	for (auto& [_, c] : ChannelList)
	{
		if (!c)
			continue;
		if (this->IsRegistered(c))
			continue;

		CFChannelRecord& rec = this->GetOrCreateRecord(c);

		for (const auto& [u, cuc] : c->users)
		{
			if (!u || !cuc)
				continue;
			if (!cuc->status.HasMode(this->op_status_char))
				continue;
			this->UpdateOpRecord(rec, u);
		}
	}
}

void ChanFixCore::ExpireTick()
{
	const time_t now = Anope::CurTime;
	for (auto it = this->channels.begin(); it != this->channels.end();)
	{
		CFChannelRecord& rec = it->second;

		for (auto oit = rec.oprecords.begin(); oit != rec.oprecords.end();)
		{
			CFOpRecord& o = oit->second;
			if (o.age > 0)
				o.age -= (o.age + this->expire_divisor - 1) / this->expire_divisor;

			if (o.age > 0 && (now - o.lastevent) < this->retention_time)
			{
				++oit;
				continue;
			}

			oit = rec.oprecords.erase(oit);
		}

		const bool keep = (!rec.oprecords.empty() && (now - rec.lastupdate) < this->retention_time);
		if (keep)
		{
			++it;
			continue;
		}

		it = this->channels.erase(it);
	}
}

void ChanFixCore::AutoFixTick()
{
	if (!Me->IsSynced() || !this->chanfix)
		return;

	for (auto& [name, rec] : this->channels)
	{
		Channel* c = Channel::Find(name);
		if (!c)
			continue;

		if (!this->do_autofix && !rec.fix_requested)
			continue;

		if (this->ShouldHandle(rec, c))
		{
			if (rec.fix_started == 0)
			{
				if (this->CanStartFix(rec, c))
				{
					rec.fix_started = Anope::CurTime;
					if (!this->FixChannel(rec, c))
						this->ClearBans(c);
				}
				else
				{
					this->ClearBans(c);
				}
			}
			else
			{
				if (!this->FixChannel(rec, c) && this->CountOps(c) == 0)
					this->ClearBans(c);
			}
		}
		else
		{
			rec.fix_requested = false;
			rec.fix_started = 0;
		}
	}
}

bool ChanFixCore::RequestFix(CommandSource& source, const Anope::string& chname)
{
	if (!this->IsAdmin(source))
	{
		source.Reply("Access denied.");
		return false;
	}
	if (!IsValidChannelName(chname))
	{
		source.Reply("Invalid channel name.");
		return false;
	}

	Channel* c = Channel::Find(chname);
	if (!c)
	{
		source.Reply("Channel %s does not exist.", chname.c_str());
		return false;
	}
	if (this->IsRegistered(c))
	{
		source.Reply("%s is already registered; ChanFix will not touch it.", chname.c_str());
		return false;
	}

	CFChannelRecord& rec = this->GetOrCreateRecord(c);
	if (rec.nofix)
	{
		source.Reply("%s has NOFIX enabled.", chname.c_str());
		return false;
	}

	const unsigned int highscore = this->GetHighScore(rec);
	if (highscore < this->min_fix_score)
	{
		source.Reply("Scores for %s are too low (< %u) for a fix.", chname.c_str(), this->min_fix_score);
		return false;
	}

	rec.fix_requested = true;
	rec.fix_started = 0;
	source.Reply("Fix request acknowledged for %s.", chname.c_str());
	return true;
}

bool ChanFixCore::RequestFixFromChanServ(CommandSource& source, const Anope::string& chname)
{
	if (!IsValidChannelName(chname))
	{
		source.Reply("Invalid channel name.");
		return false;
	}

	Channel* c = Channel::Find(chname);
	if (!c)
	{
		source.Reply("Channel %s does not exist.", chname.c_str());
		return false;
	}

	if (this->IsRegistered(c))
	{
		source.Reply("%s is registered; ChanFix will not touch it.", chname.c_str());
		return false;
	}

	if (!this->IsAdmin(source))
	{
		User* u = source.GetUser();
		if (!u)
		{
			source.Reply(ACCESS_DENIED);
			return false;
		}

		ChanUserContainer* cuc = c->FindUser(u);
		if (!cuc || !cuc->status.HasMode(this->op_status_char))
		{
			source.Reply(ACCESS_DENIED);
			return false;
		}
	}

	CFChannelRecord& rec = this->GetOrCreateRecord(c);
	if (rec.nofix)
	{
		source.Reply("%s has NOFIX enabled.", chname.c_str());
		return false;
	}

	const unsigned int highscore = this->GetHighScore(rec);
	if (highscore < this->min_fix_score)
	{
		source.Reply("Scores for %s are too low (< %u) for a fix.", chname.c_str(), this->min_fix_score);
		return false;
	}

	rec.fix_requested = true;
	rec.fix_started = 0;
	source.Reply("Fix request acknowledged for %s.", chname.c_str());
	return true;
}

bool ChanFixCore::SetMark(CommandSource& source, const Anope::string& chname, bool on, const Anope::string& reason)
{
	if (!this->IsAuspex(source))
	{
		source.Reply("Access denied.");
		return false;
	}
	if (!IsValidChannelName(chname))
	{
		source.Reply("Invalid channel name.");
		return false;
	}

	CFChannelRecord& rec = this->channels[chname];
	rec.name = chname;
	if (on)
	{
		rec.marked = true;
		rec.mark_setter = source.GetNick();
		rec.mark_reason = reason;
		rec.mark_time = Anope::CurTime;
		source.Reply("%s is now marked.", chname.c_str());
		return true;
	}

	rec.marked = false;
	rec.mark_setter.clear();
	rec.mark_reason.clear();
	rec.mark_time = 0;
	source.Reply("%s is now unmarked.", chname.c_str());
	return true;
}

bool ChanFixCore::SetNoFix(CommandSource& source, const Anope::string& chname, bool on, const Anope::string& reason)
{
	if (!this->IsAdmin(source))
	{
		source.Reply("Access denied.");
		return false;
	}
	if (!IsValidChannelName(chname))
	{
		source.Reply("Invalid channel name.");
		return false;
	}

	CFChannelRecord& rec = this->channels[chname];
	rec.name = chname;
	if (on)
	{
		rec.nofix = true;
		rec.nofix_setter = source.GetNick();
		rec.nofix_reason = reason;
		rec.nofix_time = Anope::CurTime;
		source.Reply("%s is now set to NOFIX.", chname.c_str());
		return true;
	}

	rec.nofix = false;
	rec.nofix_setter.clear();
	rec.nofix_reason.clear();
	rec.nofix_time = 0;
	source.Reply("%s is no longer set to NOFIX.", chname.c_str());
	return true;
}

void ChanFixCore::ShowScores(CommandSource& source, const Anope::string& chname, unsigned int count)
{
	if (!this->IsAuspex(source))
	{
		source.Reply("Access denied.");
		return;
	}

	CFChannelRecord* recp = this->GetRecord(chname);
	if (!recp)
	{
		source.Reply("No CHANFIX record available for %s; try again later.", chname.c_str());
		return;
	}
	CFChannelRecord& rec = *recp;

	std::vector<const CFOpRecord*> list;
	list.reserve(rec.oprecords.size());
	for (const auto& [_, o] : rec.oprecords)
		list.push_back(&o);

	std::sort(list.begin(), list.end(), [&](const CFOpRecord* a, const CFOpRecord* b)
	{
		return this->CalculateScore(*a) > this->CalculateScore(*b);
	});

	if (count == 0)
		count = 20;
	if (count > list.size())
		count = list.size();

	if (count == 0)
	{
		source.Reply("There are no scores in the CHANFIX database for %s.", chname.c_str());
		return;
	}

	source.Reply("Top %u scores for %s:", count, chname.c_str());
	unsigned int i = 0;
	for (const auto* o : list)
	{
		const unsigned int score = this->CalculateScore(*o);
		Anope::string who = (!o->account.empty() && o->account != "*") ? o->account : (o->user + "@" + o->host);
		source.Reply("%u) %s (%u)", ++i, who.c_str(), score);
		if (i >= count)
			break;
	}
	(void)source.Reply("End of SCORES for %s.", chname.c_str());
}

void ChanFixCore::ShowInfo(CommandSource& source, const Anope::string& chname)
{
	if (!this->IsAuspex(source))
	{
		source.Reply("Access denied.");
		return;
	}

	CFChannelRecord* recp = this->GetRecord(chname);
	if (!recp)
	{
		source.Reply("No CHANFIX record available for %s; try again later.", chname.c_str());
		return;
	}
	CFChannelRecord& rec = *recp;

	Channel* c = Channel::Find(chname);
	const unsigned int highscore = this->GetHighScore(rec);

	source.Reply("Information on %s:", chname.c_str());
	source.Reply("Highest score: %u", highscore);
	source.Reply("Usercount: %zu", c ? c->users.size() : 0);
	if (rec.fix_started)
		source.Reply("Now fixing: Yes (started %s)", Anope::strftime(rec.fix_started, source.GetAccount(), true).c_str());
	else
		source.Reply("Now fixing: No");

	if (c)
	{
		source.Reply("Needs fixing: %s", this->ShouldHandle(rec, c) ? "Yes" : "No");
		if (rec.fix_started)
			source.Reply("Current threshold: %u", this->GetThreshold(rec, Anope::CurTime));
	}

	if (rec.marked)
		source.Reply("MARK: set by %s at %s (%s)", rec.mark_setter.c_str(), Anope::strftime(rec.mark_time, source.GetAccount(), true).c_str(), rec.mark_reason.c_str());
	if (rec.nofix)
		source.Reply("NOFIX: set by %s at %s (%s)", rec.nofix_setter.c_str(), Anope::strftime(rec.nofix_time, source.GetAccount(), true).c_str(), rec.nofix_reason.c_str());
}

void ChanFixCore::ListChannels(CommandSource& source, const Anope::string& pattern)
{
	if (!this->IsAuspex(source))
	{
		source.Reply("Access denied.");
		return;
	}

	Anope::string pat = pattern.empty() ? "*" : pattern;
	unsigned int matches = 0;
	for (const auto& [name, rec] : this->channels)
	{
		if (!Anope::Match(pat, name))
			continue;

		Anope::string flags;
		if (rec.marked)
			flags += "[marked]";
		if (rec.nofix)
			flags += "[nofix]";

		source.Reply("- %s %s", name.c_str(), flags.c_str());
		matches++;
	}
	if (matches == 0)
		source.Reply("No channels matched criteria %s", pat.c_str());
	else
		source.Reply("%u matches for criteria %s", matches, pat.c_str());
}
