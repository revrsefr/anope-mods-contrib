/*
 * m_newregmsg module for Anope 2.1
 * Original author: Scott Seufert (katsklaw)
 * For 2.1 updated by reverse mike.chevronnet@gmail.com
 *
 * Sends configurable notices when a nickname or channel is registered.
 *
 * Configuration example:
 * module
 * {
 * 	name = "m_newregmsg"
 * 	CSRegMsg = "Your channel has been registered."
 * 	NSRegMsg = "Your nickname has been registered."
 * }
 */

#include "module.h"

class MNewRegMsg final : public Module
{
	Anope::string cs_reg_msg;
	Anope::string ns_reg_msg;

	void LoadConfig()
	{
		Configuration::Block block = Config->GetModule(this);
		cs_reg_msg = block.Get<Anope::string>("CSRegMsg", "");
		ns_reg_msg = block.Get<Anope::string>("NSRegMsg", "");
	}

 public:
	MNewRegMsg(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, THIRD)
	{
		if (Anope::VersionMajor() != 2 || Anope::VersionMinor() != 1)
			throw ModuleException("Requires version 2.1.x of Anope.");

		this->SetAuthor("katsklaw");
		this->SetVersion("2.1.0");
		LoadConfig();
	}

	void OnReload(Configuration::Conf &) override
	{
		LoadConfig();
	}

	void OnNickRegister(User *user, NickAlias *, const Anope::string &) override
	{
		if (!user || ns_reg_msg.empty())
			return;

		BotInfo *nickserv = Config->GetClient("NickServ");
		if (!nickserv)
			return;

		user->SendMessage(nickserv, "%s", ns_reg_msg.c_str());
	}

	void OnChanRegistered(ChannelInfo *ci) override
	{
		if (!ci || cs_reg_msg.empty())
			return;

		NickCore *founder = ci->GetFounder();
		BotInfo *chanserv = Config->GetClient("ChanServ");
		if (!founder || !chanserv)
			return;

		for (const auto &[_, u] : UserListByNick)
		{
			if (u && u->Account() == founder)
				u->SendMessage(chanserv, "%s", cs_reg_msg.c_str());
		}
	}
};

MODULE_INIT(MNewRegMsg)
