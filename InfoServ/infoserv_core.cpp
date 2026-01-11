#include "infoserv.h"

#include <algorithm>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
	constexpr const char* DB_MAGIC = "infoserv";
	constexpr uint64_t DB_VERSION = 1;
}

InfoServCore::InfoServCore(const Anope::string& modname, const Anope::string& creator)
	: Module(modname, creator, VENDOR)
	, command_info(this, *this)
	, command_post(this, *this)
	, command_list(this, *this)
	, command_move(this, *this)
	, command_del(this, *this)
	, command_olist(this, *this)
	, command_omove(this, *this)
	, command_odel(this, *this)
	, command_notify(this, *this)
{
	if (!IRCD)
		throw ModuleException("IRCd protocol module not loaded");

	this->LoadDB();
}

InfoServCore::~InfoServCore()
{
	this->SaveDB();
}

Anope::string InfoServCore::GetDBPath() const
{
	return Anope::ExpandData("infoserv.db");
}

Anope::string InfoServCore::EscapeValue(const Anope::string& in)
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

Anope::string InfoServCore::UnescapeValue(const Anope::string& in)
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

Anope::string InfoServCore::SubjectForDisplay(const Anope::string& subject)
{
	Anope::string out = subject;
	for (size_t i = 0; i < out.length(); ++i)
		if (out[i] == '_')
			out[i] = ' ';
	return out;
}

void InfoServCore::LoadDB()
{
	this->logon_info.clear();
	this->oper_info.clear();

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
			// infoserv|1
			std::vector<Anope::string> parts;
			sepstream(line, '|').GetTokens(parts);
			if (parts.size() >= 2 && parts[0].equals_ci(DB_MAGIC))
			{
				header_ok = true;
				continue;
			}
			// Unknown format.
			return;
		}

		std::vector<Anope::string> parts;
		sepstream(line, '|').GetTokens(parts);
		if (parts.size() < 5)
			continue;

		const Anope::string type = parts[0];
		uint64_t ts_u64 = 0;
		try
		{
			ts_u64 = Anope::Convert<uint64_t>(parts[1], 0);
		}
		catch (...)
		{
			ts_u64 = 0;
		}

		InfoEntry e;
		e.ts = static_cast<time_t>(ts_u64);
		e.poster = UnescapeValue(parts[2]);
		e.subject = UnescapeValue(parts[3]);
		e.message = UnescapeValue(parts[4]);

		if (type.equals_ci("O"))
			this->oper_info.push_back(e);
		else if (type.equals_ci("P"))
			this->logon_info.push_back(e);
	}
}

void InfoServCore::SaveDB() const
{
	const fs::path path(this->GetDBPath().c_str());
	std::error_code ec;
	fs::create_directories(path.parent_path(), ec);

	const fs::path tmp = path.string() + ".tmp";
	std::ofstream out(tmp, std::ios::out | std::ios::trunc);
	if (!out.is_open())
		return;

	out << DB_MAGIC << "|" << DB_VERSION << "\n";
	for (const auto& e : this->logon_info)
	{
		out << "P|" << static_cast<uint64_t>(e.ts) << "|" << EscapeValue(e.poster) << "|" << EscapeValue(e.subject)
			<< "|" << EscapeValue(e.message) << "\n";
	}
	for (const auto& e : this->oper_info)
	{
		out << "O|" << static_cast<uint64_t>(e.ts) << "|" << EscapeValue(e.poster) << "|" << EscapeValue(e.subject)
			<< "|" << EscapeValue(e.message) << "\n";
	}
	out.close();

	fs::rename(tmp, path, ec);
	if (ec)
	{
		fs::remove(path, ec);
		fs::rename(tmp, path, ec);
	}
}

bool InfoServCore::SetReplyMode(const Anope::string& mode)
{
	if (mode.equals_ci("privmsg"))
	{
		this->reply_with_notice = false;
		return true;
	}
	if (mode.equals_ci("notice"))
	{
		this->reply_with_notice = true;
		return true;
	}
	return false;
}

Anope::string InfoServCore::ReplyModeString() const
{
	return this->reply_with_notice ? "notice" : "privmsg";
}

void InfoServCore::Reply(CommandSource& source, const Anope::string& msg)
{
	if (!this->infoserv.operator bool())
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
			IRCD->SendNotice(*this->infoserv, u->GetUID(), line, tags);
		else
			IRCD->SendPrivmsg(*this->infoserv, u->GetUID(), line, tags);
	}
}

void InfoServCore::SendToUser(User* u, const Anope::string& msg)
{
	if (!this->infoserv.operator bool() || !u)
		return;

	Anope::map<Anope::string> tags;
	LineWrapper lw(Language::Translate(u, msg.c_str()));
	for (Anope::string line; lw.GetLine(line); )
	{
		if (this->reply_with_notice)
			IRCD->SendNotice(*this->infoserv, u->GetUID(), line, tags);
		else
			IRCD->SendPrivmsg(*this->infoserv, u->GetUID(), line, tags);
	}
}

bool InfoServCore::IsAdmin(CommandSource& source) const
{
	if (this->admin_priv.empty())
		return false;
	return source.HasPriv(this->admin_priv);
}

bool InfoServCore::CanChangeReplyMode(CommandSource& source) const
{
	if (this->notify_priv.empty())
		return false;
	return source.HasPriv(this->notify_priv);
}

void InfoServCore::BroadcastGlobal(const Anope::string& msg)
{
	BotInfo* sender = this->infoserv.operator->();
	if (!sender)
		return;

	for (const auto& [_, serv] : Servers::ByName)
	{
		if (!serv || serv == Me)
			continue;
		if (serv->IsJuped() || serv->IsQuitting() || !serv->IsSynced())
			continue;
		serv->Notice(sender, msg);
	}
}

void InfoServCore::DisplayListToUser(User* u, const std::vector<InfoEntry>& list, bool oper)
{
	if (!u)
		return;
	if (list.empty())
		return;

	const bool show_all = (this->logoninfo_count == 0);
	const unsigned n = static_cast<unsigned>(list.size());
	const unsigned count = show_all ? n : std::min(this->logoninfo_count, n);
	unsigned start = 0;
	unsigned end = n;

	if (!show_all)
	{
		if (this->logoninfo_reverse)
			start = n - count;
		else
			end = count;
	}

	for (unsigned i = start; i < end; ++i)
	{
		const auto& e = list[i];
		const auto subj = SubjectForDisplay(e.subject);
		if (this->logoninfo_show_metadata && !e.poster.empty() && e.ts)
		{
				this->SendToUser(u, Anope::Format("[%s] by %s at %s: %s", subj.c_str(), e.poster.c_str(),
				Anope::strftime(e.ts, u->Account(), true).c_str(), e.message.c_str()));
		}
		else
		{
				this->SendToUser(u, Anope::Format("[%s] %s", subj.c_str(), e.message.c_str()));
		}
	}

	if (!show_all && n > count)
	{
		if (this->logoninfo_reverse)
				this->SendToUser(u, Anope::Format("(%u older %s message(s) not shown)", n - count, oper ? "oper" : "logon"));
		else
				this->SendToUser(u, Anope::Format("(%u additional %s message(s) not shown)", n - count, oper ? "oper" : "logon"));
	}
}

void InfoServCore::DisplayInfoToUser(User* u)
{
	if (!this->infoserv || !u)
		return;
	if (this->logon_info.empty())
		return;

	this->SendToUser(u, "*** \2Message(s) of the Day\2 ***");
	this->DisplayListToUser(u, this->logon_info, false);
	this->SendToUser(u, "*** \2End of Message(s) of the Day\2 ***");
}

void InfoServCore::DisplayOperInfoToUser(User* u)
{
	if (!this->infoserv || !u)
		return;
	if (this->oper_info.empty())
		return;

	this->SendToUser(u, "*** \2Oper Message(s) of the Day\2 ***");
	this->DisplayListToUser(u, this->oper_info, true);
	this->SendToUser(u, "*** \2End of Oper Message(s) of the Day\2 ***");
}

void InfoServCore::DisplayAllToUser(User* u)
{
	this->DisplayInfoToUser(u);
	if (u && u->HasMode("OPER"))
		this->DisplayOperInfoToUser(u);
}

bool InfoServCore::Post(CommandSource& source, unsigned importance, const Anope::string& subject, const Anope::string& message)
{
	if (!this->infoserv)
		return false;

	Anope::string poster = source.GetNick();
	if (source.GetAccount())
		poster = source.GetAccount()->display;

	const auto subj = SubjectForDisplay(subject);
	const auto now = Anope::CurTime;

	// 4 = critical (global)
	if (importance == 4)
	{
			this->BroadcastGlobal(Anope::Format("[CRITICAL NETWORK NOTICE] %s - [%s] %s", poster.c_str(), subj.c_str(), message.c_str()));
		this->Reply(source, "The InfoServ message has been sent.");
		return true;
	}

	// 2 = global notice (no persist)
	if (importance == 2)
	{
			this->BroadcastGlobal(Anope::Format("[Network Notice] %s - [%s] %s", poster.c_str(), subj.c_str(), message.c_str()));
		this->Reply(source, "The InfoServ message has been sent.");
		return true;
	}

	InfoEntry e;
	e.ts = now;
	e.poster = poster;
	e.subject = subject;
	e.message = message;

	if (importance == 0)
		this->oper_info.push_back(e);
	else
		this->logon_info.push_back(e);

	this->SaveDB();
	this->Reply(source, "Added entry.");

	// 3 = persist + global notice
		if (importance == 3)
			this->BroadcastGlobal(Anope::Format("[Network Notice] %s - [%s] %s", poster.c_str(), subj.c_str(), message.c_str()));

	return true;
}

bool InfoServCore::Delete(CommandSource& source, bool oper, unsigned id)
{
	auto& list = oper ? this->oper_info : this->logon_info;
	if (id == 0 || id > list.size())
	{
		this->Reply(source, "Entry not found.");
		return false;
	}
	list.erase(list.begin() + (id - 1));
	this->SaveDB();
	this->Reply(source, "Deleted entry.");
	return true;
}

bool InfoServCore::Move(CommandSource& source, bool oper, unsigned from_id, unsigned to_id)
{
	auto& list = oper ? this->oper_info : this->logon_info;
	if (!from_id || !to_id || from_id > list.size() || to_id > list.size())
	{
		this->Reply(source, "Invalid positions.");
		return false;
	}
	if (from_id == to_id)
	{
		this->Reply(source, "You must specify two different positions.");
		return false;
	}

	InfoEntry moved = list[from_id - 1];
	list.erase(list.begin() + (from_id - 1));
	list.insert(list.begin() + (to_id - 1), moved);
	this->SaveDB();
	this->Reply(source, "Moved entry.");
	return true;
}

void InfoServCore::List(CommandSource& source, bool oper)
{
	const auto& list = oper ? this->oper_info : this->logon_info;
	if (list.empty())
	{
		this->Reply(source, "No entries.");
		return;
	}
	unsigned idx = 0;
	for (const auto& e : list)
	{
		++idx;
		const auto subj = SubjectForDisplay(e.subject);
		if (this->logoninfo_show_metadata && !e.poster.empty() && e.ts)
		{
			this->Reply(source, Anope::Format("%u: [%s] by %s at %s: %s", idx, subj.c_str(), e.poster.c_str(),
				Anope::strftime(e.ts, source.GetAccount(), true).c_str(), e.message.c_str()));
		}
		else
		{
			this->Reply(source, Anope::Format("%u: [%s] %s", idx, subj.c_str(), e.message.c_str()));
		}
	}
	this->Reply(source, "End of list.");
}

void InfoServCore::OnReload(Configuration::Conf& conf)
{
	const Configuration::Block* mod = &conf.GetModule(this);
	Anope::string nick = mod->Get<const Anope::string>("client");
	if (nick.empty())
	{
		mod = &conf.GetModule("infoserv");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
	{
		mod = &conf.GetModule("infoserv.so");
		nick = mod->Get<const Anope::string>("client");
	}
	if (nick.empty())
		throw ConfigException(Module::name + ": <client> must be defined");

	BotInfo* bi = BotInfo::Find(nick, true);
	if (!bi)
		throw ConfigException(Module::name + ": no bot named " + nick);

	this->infoserv = bi;

	this->logoninfo_count = mod->Get<unsigned>("logoninfo_count", "3");
	this->logoninfo_reverse = mod->Get<bool>("logoninfo_reverse", "yes");
	this->logoninfo_show_metadata = mod->Get<bool>("logoninfo_show_metadata", "yes");
	this->show_on_connect = mod->Get<bool>("show_on_connect", "yes");
	this->show_on_oper = mod->Get<bool>("show_on_oper", "yes");

	this->notify_priv = mod->Get<Anope::string>("notify_priv", "infoserv/admin");
	this->admin_priv = mod->Get<Anope::string>("admin_priv", "infoserv/admin");
	this->SetReplyMode(mod->Get<Anope::string>("reply_method", "notice"));
}

void InfoServCore::OnUserConnect(User* user, bool& exempt)
{
	if (exempt || !user || user->Quitting() || !Me->IsSynced() || !user->server || !user->server->IsSynced())
		return;
	if (!this->show_on_connect)
		return;
	this->DisplayInfoToUser(user);
}

void InfoServCore::OnUserModeSet(const MessageSource&, User* u, const Anope::string& mname)
{
	if (!u || u->Quitting() || !u->server || !u->server->IsSynced())
		return;
	if (!this->show_on_oper)
		return;
	if (mname == "OPER")
		this->DisplayOperInfoToUser(u);
}
