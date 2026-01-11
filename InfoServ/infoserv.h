/*
 * InfoServ (Anope 2.1) module header.
 *
 * Implements an InfoServ pseudo-client similar in spirit to Atheme's InfoServ:
 * - Stores informational logon messages (public + oper-only)
 * - Displays public messages on connect
 * - Displays oper-only messages when a user becomes an IRC operator
 * - Provides commands to post/list/move/delete messages
 *
 * Command implementations and MODULE_INIT live in infoserv.cpp.
 */

#pragma once

#include "module.h"

#include <cstdint>
#include <ctime>
#include <vector>

class InfoServCore;

class CommandInfoServInfo final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServInfo(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServPost final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServPost(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
	void OnSyntaxError(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServList final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServList(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServMove final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServMove(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
	void OnSyntaxError(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServDel final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServDel(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
	void OnSyntaxError(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServOList final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServOList(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServOMove final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServOMove(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
	void OnSyntaxError(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServODel final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServODel(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
	void OnSyntaxError(CommandSource& source, const Anope::string& subcommand) override;
};

class CommandInfoServNotify final : public Command
{
	InfoServCore& is;

public:
	CommandInfoServNotify(Module* creator, InfoServCore& parent);
	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override;
	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override;
};

class InfoServCore final : public Module
{
public:
	struct InfoEntry final
	{
		time_t ts = 0;
		Anope::string poster;
		Anope::string subject;
		Anope::string message;
	};

private:
	Reference<BotInfo> infoserv;

	std::vector<InfoEntry> logon_info;
	std::vector<InfoEntry> oper_info;

	// Display config
	unsigned logoninfo_count = 3;
	bool logoninfo_reverse = true;
	bool logoninfo_show_metadata = true;
	bool show_on_connect = true;
	bool show_on_oper = true;

	// Reply config
	bool reply_with_notice = true;
	Anope::string notify_priv = "infoserv/admin";
	Anope::string admin_priv = "infoserv/admin";

	CommandInfoServInfo command_info;
	CommandInfoServPost command_post;
	CommandInfoServList command_list;
	CommandInfoServMove command_move;
	CommandInfoServDel command_del;
	CommandInfoServOList command_olist;
	CommandInfoServOMove command_omove;
	CommandInfoServODel command_odel;
	CommandInfoServNotify command_notify;

public:
	InfoServCore(const Anope::string& modname, const Anope::string& creator);
	~InfoServCore() override;

	void OnReload(Configuration::Conf& conf) override;
	void OnUserConnect(User* user, bool& exempt) override;
	void OnUserModeSet(const MessageSource& setter, User* u, const Anope::string& mname) override;

	void Reply(CommandSource& source, const Anope::string& msg);
	void SendToUser(User* u, const Anope::string& msg);
	bool SetReplyMode(const Anope::string& mode);
	Anope::string ReplyModeString() const;

	bool IsAdmin(CommandSource& source) const;
	bool CanChangeReplyMode(CommandSource& source) const;

	void DisplayInfoToUser(User* u);
	void DisplayOperInfoToUser(User* u);
	void DisplayAllToUser(User* u);

	bool Post(CommandSource& source, unsigned importance, const Anope::string& subject, const Anope::string& message);
	bool Delete(CommandSource& source, bool oper, unsigned id);
	bool Move(CommandSource& source, bool oper, unsigned from_id, unsigned to_id);
	void List(CommandSource& source, bool oper);

private:
	static Anope::string EscapeValue(const Anope::string& in);
	static Anope::string UnescapeValue(const Anope::string& in);
	static Anope::string SubjectForDisplay(const Anope::string& subject);

	Anope::string GetDBPath() const;
	void LoadDB();
	void SaveDB() const;

	void BroadcastGlobal(const Anope::string& msg);
	void DisplayListToUser(User* u, const std::vector<InfoEntry>& list, bool oper);
};
