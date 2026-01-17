/*
 * HostServ NetHost: Add default vhosts for new users using their account names
 *
 * Copyright (C) 2016-2021 Michael Hazell <michaelhazell@hotmail.com>
 *
 * You may use this module as long as you follow the terms of the GPLv3
 * Originally forked from https://gist.github.com/7330c12ce7a03c030871
 *
 * Configuration:
 * module { name = "hs_nethost"; prefix = "user/"; suffix = ""; hashprefix = "/x-"; setifnone = true; }
 *
 * This will format a user's vhost as user/foo (or user/foo-bar/x-XXXX), and broadcast the vhost change to
 * other modules.
 */

#include "module.h"

class HSNetHost : public Module
{
	bool setifnone;
	Anope::string hashprefix;
	Anope::string prefix;
	Anope::string suffix;

 private:

	NickAlias* GetMatchingNAFromNC(NickCore *nc)
	{
		for (std::vector<NickAlias *>::iterator it = (*nc->aliases).begin(); it != (*nc->aliases).end(); ++it)
		{
			if ((*it)->nick == nc->display)
				return ((*it));
		}
		return NULL;
	}

	Anope::string GenerateHash(NickCore *nc)
	{
		std::stringstream ss;
		ss << std::hex << nc->GetId();
		return ss.str();
	}

	void SetNetHost(NickAlias *na)
	{
		// If the NickAlias has an existing host not set by this module, do not touch it
		// This avoids overwriting manually set/requested vhosts
		if (na->HasVHost() && (na->GetVHostCreator() != "HostServ"))
			return;

		Anope::string nick = na->nick;
		Anope::string vhost;
		bool usehash = false;

		Anope::string valid_nick_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-";

		// This operates on nick, not na->nick, so that changes can be made later.
		for (Anope::string::iterator it = nick.begin(); it != nick.end(); ++it)
		{
			if (valid_nick_chars.find((*it)) == Anope::string::npos)
			{
				usehash = true;
				(*it) = '-';
			}
		}

		// Construct vhost
		vhost = prefix + nick + suffix;

		// If the nickname contained invalid characters, append a hash since those were replaced with a -
		if (usehash)
			vhost += hashprefix + GenerateHash(na->nc);

		if (!IRCD->IsHostValid(vhost))
		{
			Log(Config->GetClient("HostServ"), "nethost") << "Tried setting vhost " << vhost << " on " << na->nick << ", but it was not valid!";
			Log(Config->GetClient("HostServ"), "nethost") << "Check if your IRCd supports all of the characters in the vhost, and that Anope is configured to allow them (check vhost_chars in services.conf).";
			return;
		}

		na->SetVHost(na->GetVHostIdent(), vhost, "HostServ");
		FOREACH_MOD(OnSetVHost, (na));
	}

 public:
	HSNetHost(const Anope::string &modname, const Anope::string &creator):
	Module(modname, creator, THIRD)
	{
		this->SetAuthor("Techman");
		this->SetVersion("2.0.4");

		if (!IRCD || !IRCD->CanSetVHost)
			throw ModuleException("Your IRCd does not support vhosts");

		if (!ModuleManager::FindModule("hostserv"))
			throw ModuleException("HostServ is required for this module to be effective.");
	}

	void Prioritize() override
	{
		// We set ourselves as priority last because we don't want vhosts being added to
		// ns_access on registration if that's enabled by the network
		ModuleManager::SetPriority(this, PRIORITY_LAST);
	}

	void OnChangeCoreDisplay(NickCore *nc, const Anope::string &newdisplay) override
	{
		// Send the matching NickAlias so that nick is set
		// This requires newdisplay, so this loop is different
		for (std::vector<NickAlias *>::const_iterator it = (*nc->aliases).begin(); it != (*nc->aliases).end(); ++it)
		{
			if ((*it)->nick == newdisplay)
			{
				SetNetHost((*it));
				break;
			}
		}
	}

	void OnNickRegister(User *user, NickAlias *na, const Anope::string &) override
	{
		// If it's anything else, we assume they have a nick confirmation system
		if (Config->GetModule("ns_register").Get<const Anope::string>("registration") == "none")
			SetNetHost(na);
	}

	void OnNickConfirm(User *user, NickCore *nc) override
	{
		// It is assumed there is only one NickAlias in the account
		NickAlias *na = (*nc->aliases)[0];
		SetNetHost(na);
	}

	void OnNickIdentify(User *u) override
	{
		// If not configured to set a nethost when no vhost is present, just quit
		if (!setifnone)
			return;

		NickCore *nc = u->Account();
		if (!nc)
			return;

		// If the account is marked as unconfirmed, quit
		if (nc->HasExt("UNCONFIRMED"))
			return;

		// Send the NickAlias that matches the account name
		NickAlias *na = GetMatchingNAFromNC(nc);
		if (!na->HasVHost())
			SetNetHost(na);
	}

	void OnUserLogin(User *u) override
	{
		OnNickIdentify(u);
	}

	void OnReload(Configuration::Conf &conf) override
	{
		Configuration::Block block = conf.GetModule(this);
		setifnone = block.Get<bool>("setifnone", "false");
		hashprefix = block.Get<Anope::string>("hashprefix", "");
		prefix = block.Get<Anope::string>("prefix", "");
		suffix = block.Get<Anope::string>("suffix", "");
	}
};

MODULE_INIT(HSNetHost)
