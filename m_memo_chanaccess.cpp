/*
 * (C) 2025 reverse Juean Chevronnet
 * Contact me at mike.chevronnet@gmail.com
 * IRC: irc.irc4fun.net Port:+6697 (tls)
 * Salon: #development
 *
 * Module m_memo_chanaccess created by reverse to send memos on chanaccess changes
 *
 * module { name = "m_memo_chanaccess" }
 * 
 * MemoServ: [NOTICE] You have 1 new memo.
 * MemoServ: [NOTICE] Memo 1 from tChatCop (Fri Jan  9 12:49:53 2026 (37 seconds ago)).
 * MemoServ: [NOTICE] To delete, type: /msg MemoServ DEL 1
 * MemoServ: [NOTICE] You have been added to the access list for #!accueil by reverse (access: 5).
 */

#include "module.h"
#include "modules/memoserv/service.h"

class ModuleMemoChanAccess final
	: public Module
{
	bool notify_access_add = true;
	bool notify_founder_change = true;
	bool notify_successor_change = true;
	bool notify_self = false;
	Anope::string sender;

	void LoadConfig(Configuration::Block &block)
	{
		notify_access_add = block.Get<bool>("notify_access_add", "yes");
		notify_founder_change = block.Get<bool>("notify_founder_change", "yes");
		notify_successor_change = block.Get<bool>("notify_successor_change", "yes");
		notify_self = block.Get<bool>("notify_self", "no");
		sender = block.Get<const Anope::string>("sender", "");
	}

	Anope::string GetSenderNick(CommandSource &source, ChannelInfo *ci) const
	{
		if (!sender.empty())
			return sender;
		if (ci && ci->WhoSends())
			return ci->WhoSends()->nick;
		if (source.service)
			return source.service->nick;
		if (auto *cs = Config->GetClient("ChanServ"))
			return cs->nick;
		return "ChanServ";
	}

	void TrySendMemo(CommandSource &source, ChannelInfo *ci, NickCore *target, const Anope::string &text)
	{
		if (!target)
			return;
		if (!MemoServ::service)
			return;
		if (!notify_self && source.GetAccount() && source.GetAccount() == target)
			return;

		const auto result = MemoServ::service->Send(GetSenderNick(source, ci), target->display, text, true);
		if (result != MemoServ::MEMO_SUCCESS)
			Log(LOG_DEBUG, "m_memo_chanaccess") << "Unable to send memo to " << target->display << " (result=" << result << ")";
	}

public:
	ModuleMemoChanAccess(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator)
	{
		this->SetAuthor("reverse");
		this->SetVersion("1.0");
		LoadConfig(Config->GetModule(this));
	}

	void OnReload(Configuration::Conf &conf) override
	{
		LoadConfig(conf.GetModule(this));
	}

	void OnAccessAdd(ChannelInfo *ci, CommandSource &source, ChanAccess *access) override
	{
		if (!notify_access_add || !ci || !access)
			return;

		auto *target = access->GetAccount();
		if (!target)
			return;

		Anope::string msg = "You have been added to the access list for " + ci->name
			+ " by " + source.GetNick() + " (access: " + access->AccessSerialize() + ").";

		TrySendMemo(source, ci, target, msg);
	}

	void OnPostCommand(CommandSource &source, Command *command, const std::vector<Anope::string> &params) override
	{
		if (!command)
			return;

		if (notify_founder_change && command->name.equals_ci("chanserv/set/founder"))
		{
			if (params.size() < 2)
				return;
			auto *ci = ChannelInfo::Find(params[0]);
			auto *na = NickAlias::Find(params[1]);
			if (!ci || !na || !na->nc)
				return;
			if (ci->GetFounder() != na->nc)
				return;

			Anope::string msg = "You have been set as founder of " + ci->name + " by " + source.GetNick() + ".";
			TrySendMemo(source, ci, na->nc, msg);
			return;
		}

		if (notify_successor_change && command->name.equals_ci("chanserv/set/successor"))
		{
			if (params.empty())
				return;
			// Successor can be unset by passing no nick.
			if (params.size() < 2 || params[1].empty())
				return;
			auto *ci = ChannelInfo::Find(params[0]);
			auto *na = NickAlias::Find(params[1]);
			if (!ci || !na || !na->nc)
				return;
			if (ci->GetSuccessor() != na->nc)
				return;

			Anope::string msg = "You have been set as successor of " + ci->name + " by " + source.GetNick() + ".";
			TrySendMemo(source, ci, na->nc, msg);
			return;
		}
	}
};

MODULE_INIT(ModuleMemoChanAccess)
