// Anope IRC Services <https://www.anope.org/>
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Implements the IRCv3 SASL OAUTHBEARER mechanism for Anope.
// This is intended for validating *existing* bearer tokens (typically JWTs)
// server-side, without sending account passwords.
//
// NOTE: This module does not embed or copy the IRCv3 specification text.
// See: https://github.com/ircv3/ircv3-specifications/blob/master/extensions/ircv3bearer.md

/// BEGIN CMAKE
/// find_package("OpenSSL" REQUIRED)
/// target_link_libraries(${SO} PRIVATE OpenSSL::Crypto OpenSSL::SSL)
/// END CMAKE

#include "module.h"
#include "modules/nickserv/sasl.h"

#include <jwt-cpp/jwt.h>
#include <chrono>

namespace
{
	Anope::string JoinAuthenticateData(const std::vector<Anope::string> &parts)
	{
		Anope::string out;
		for (const auto &p : parts)
			out += p;
		return out;
	}

	Anope::string ExtractBearerToken(const Anope::string &decoded)
	{
		// Expected shape (simplified):
		// "n,a=<authzid>,\x01auth=Bearer <token>\x01\x01"
		// We look for "auth=Bearer " then read until the next \x01.
		const Anope::string marker = "auth=Bearer ";
		auto pos = decoded.find(marker);
		if (pos == Anope::string::npos)
			return "";

		pos += marker.length();
		auto end = decoded.find('\x01', pos);
		if (end == Anope::string::npos)
			end = decoded.length();

		auto token = decoded.substr(pos, end - pos);
		token.trim();
		return token;
	}

	struct IRCV3BearerFields final
	{
		Anope::string authzid;
		Anope::string token_type;
		Anope::string token;
	};

	bool ParseIRCV3BearerMessage(const Anope::string &decoded, IRCV3BearerFields &out)
	{
		// <message> ::= [authzid] NUL <token_type> NUL <token>
		const auto first_nul = decoded.find('\0');
		if (first_nul == Anope::string::npos)
			return false;

		const auto second_nul = decoded.find('\0', first_nul + 1);
		if (second_nul == Anope::string::npos)
			return false;

		out.authzid = decoded.substr(0, first_nul);
		out.token_type = decoded.substr(first_nul + 1, second_nul - first_nul - 1);
		out.token = decoded.substr(second_nul + 1);

		out.authzid.trim();
		out.token_type.trim();
		out.token.trim();

		if (out.token_type.empty() || out.token.empty())
			return false;

		// Bearer tokens must not contain NUL.
		if (out.token.find('\0') != Anope::string::npos)
			return false;

		// Basic hygiene.
		if (out.token.find_first_of("\r\n") != Anope::string::npos)
			return false;

		return true;
	}

	Anope::string JwtSubjectFromBearerToken(const Anope::string &token, const Anope::string &jwt_secret, const Anope::string &jwt_issuer)
	{
		std::string token_str(token.c_str());
		auto decoded = jwt::decode(token_str);

		// Verify signature + issuer.
		jwt::verify()
			.allow_algorithm(jwt::algorithm::hs256{std::string(jwt_secret.c_str())})
			.with_issuer(std::string(jwt_issuer.c_str()))
			.verify(decoded);

		// Validate exp if present.
		if (decoded.has_expires_at())
		{
			const auto exp = decoded.get_expires_at();
			const auto now = std::chrono::system_clock::now();
			if (now >= exp)
				throw std::runtime_error("token expired");
		}

		std::string subject;
		try
		{
			subject = decoded.get_payload_claim("sub").as_string();
		}
		catch (...)
		{
			// Missing or non-string sub.
		}

		return subject.empty() ? "" : Anope::string(subject.c_str());
	}

	bool VerifyAndLogin(SASL::Session *sess, Module *owner, const Anope::string &jwt_secret, const Anope::string &jwt_issuer, bool autocreate,
		const Anope::string &token, const Anope::string &authzid)
	{
		Anope::string subject;
		try
		{
			subject = JwtSubjectFromBearerToken(token, jwt_secret, jwt_issuer);
		}
		catch (const std::exception &ex)
		{
			Log(owner, "sasl", Config->GetClient("NickServ")) << sess->GetUserInfo() << " failed SASL bearer token verification: " << ex.what();
			return false;
		}

		if (subject.empty() || !IRCD->IsNickValid(subject))
			return false;

		// If an authzid was provided, require it to match the derived account.
		if (!authzid.empty() && authzid != subject)
			return false;

		NickAlias *na = NickAlias::Find(subject);
		if (!na)
		{
			if (!autocreate)
				return false;

			na = new NickAlias(subject, new NickCore(subject));
			FOREACH_MOD(OnNickRegister, (nullptr, na, ""));
		}

		NickCore *nc = na->nc;
		if (!nc || nc->HasExt("NS_SUSPENDED") || nc->HasExt("UNCONFIRMED"))
			return false;

		Log(owner, "sasl", Config->GetClient("NickServ")) << sess->GetUserInfo() << " identified to account " << nc->display << " using SASL";
		SASL::service->Succeed(sess, nc);
		delete sess;
		return true;
	}
}


class OAuthBearer final
	: public SASL::Mechanism
{
private:
	Anope::string jwt_secret;
	Anope::string jwt_issuer;
	bool autocreate;

public:
	OAuthBearer(Module *o, const Anope::string &secret, const Anope::string &issuer, bool autocreate_)
		: SASL::Mechanism(o, "OAUTHBEARER")
		, jwt_secret(secret)
		, jwt_issuer(issuer)
		, autocreate(autocreate_)
	{
	}

	bool ProcessMessage(SASL::Session *sess, const SASL::Message &m) override
	{
		if (m.type == "S")
		{
			// Prompt for client initial response.
			SASL::service->SendMessage(sess, "C", "+");
			return true;
		}

		if (m.type != "C" || m.data.empty())
			return false;

		// Client abort.
		if (m.data[0] == "*")
			return false;

		Anope::string decoded = Anope::B64Decode(JoinAuthenticateData(m.data));
		if (decoded.empty() || decoded == "+")
			return false;

		Anope::string token = ExtractBearerToken(decoded);
		if (token.empty())
			return false;

		// OAUTHBEARER has no separate token type field.
		return VerifyAndLogin(sess, this->owner, jwt_secret, jwt_issuer, autocreate, token, "");
	}
};

class IRCV3Bearer final
	: public SASL::Mechanism
{
private:
	Anope::string jwt_secret;
	Anope::string jwt_issuer;
	bool autocreate;
	bool allow_oauth2_type_as_jwt;

public:
	IRCV3Bearer(Module *o, const Anope::string &secret, const Anope::string &issuer, bool autocreate_, bool allow_oauth2_type_as_jwt_)
		: SASL::Mechanism(o, "IRCV3BEARER")
		, jwt_secret(secret)
		, jwt_issuer(issuer)
		, autocreate(autocreate_)
		, allow_oauth2_type_as_jwt(allow_oauth2_type_as_jwt_)
	{
	}

	bool ProcessMessage(SASL::Session *sess, const SASL::Message &m) override
	{
		if (m.type == "S")
		{
			SASL::service->SendMessage(sess, "C", "+");
			return true;
		}

		if (m.type != "C" || m.data.empty())
			return false;

		// Client abort.
		if (m.data[0] == "*")
			return false;

		Anope::string decoded = Anope::B64Decode(JoinAuthenticateData(m.data));
		if (decoded.empty() || decoded == "+")
			return false;

		IRCV3BearerFields fields;
		if (!ParseIRCV3BearerMessage(decoded, fields))
			return false;

		// This module validates JWTs locally. Require token_type=jwt by default.
		if (fields.token_type != "jwt")
		{
			if (!(allow_oauth2_type_as_jwt && fields.token_type == "oauth2"))
				return false;
		}

		return VerifyAndLogin(sess, this->owner, jwt_secret, jwt_issuer, autocreate, fields.token, fields.authzid);
	}
};

class ModuleSASLOAuthBearer final
	: public Module
{
private:
	OAuthBearer mech;
	IRCV3Bearer mech_ircv3;

public:
	ModuleSASLOAuthBearer(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, EXTRA | VENDOR)
		, mech(this,
			Config->GetModule(this).Get<Anope::string>("jwt_secret", ""),
			Config->GetModule(this).Get<Anope::string>("jwt_issuer", ""),
			Config->GetModule(this).Get<bool>("autocreate", true))
		, mech_ircv3(this,
			Config->GetModule(this).Get<Anope::string>("jwt_secret", ""),
			Config->GetModule(this).Get<Anope::string>("jwt_issuer", ""),
			Config->GetModule(this).Get<bool>("autocreate", true),
			Config->GetModule(this).Get<bool>("allow_oauth2_type_as_jwt", false))
	{
		if (!SASL::protocol_interface)
			throw ModuleException("Your IRCd does not support SASL");

		if (Config->GetModule(this).Get<Anope::string>("jwt_secret", "").empty() || Config->GetModule(this).Get<Anope::string>("jwt_issuer", "").empty())
			throw ModuleException("ns_sasl_oauthbearer requires jwt_secret and jwt_issuer to be configured");
	}
};

MODULE_INIT(ModuleSASLOAuthBearer)
