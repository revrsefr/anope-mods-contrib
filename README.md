# Anope 2.1 contrib modules

Collection of third-party modules for **Anope IRC Services 2.1.x**.

Each module is independent. This README documents:
- what each module does
- required `command { ... }` blocks (if any)
- module configuration keys (as implemented in the code)

## Contents

- [Build & install](#build--install)
- [Modules](#modules)
	- [cs_topichistory](#cs_topichistory)
	- [hs_nethost](#hs_nethost)
	- [m_apiauth](#m_apiauth)
	- [m_expirenotice](#m_expirenotice)
	- [m_login](#m_login)
	- [m_memo_chanaccess](#m_memo_chanaccess)
	- [m_saslJwt](#m_sasljwt)
	- [m_secure](#m_secure)
	- [m_sqlauth](#m_sqlauth)
	- [m_youtube](#m_youtube)
	- [ns_sasl_oauthbearer](#ns_sasl_oauthbearer)
	- [os_chantrap](#os_chantrap)
	- [os_forceid](#os_forceid)
	- [os_notinchan](#os_notinchan)
	- [os_regset](#os_regset)

## Build & install

General workflow (recommended):

1. Copy the module `.cpp` into Anope's modules build tree (commonly `modules/third/`).
2. Rebuild modules.
3. Add the `module { name = "..." }` block, plus any `command { ... }` blocks.
4. `/msg OperServ REHASH` or restart services.

Some modules include extra dependencies (SQL, libcurl, OpenSSL, jwt-cpp, nlohmann-json, rapidjson).
On Debian/Ubuntu you’ll generally need:

- `libssl-dev`
- `libcurl4-openssl-dev`
- headers for `jwt-cpp` and JSON libraries (these are often header-only, but packaging varies)

If a module has a `/// $LinkerFlags:` comment or a `/// BEGIN CMAKE` section, that’s a hint about link requirements.

## Modules

### cs_topichistory

Keep a per-channel topic history and allow listing/setting from the history.

Commands:

```conf
module { name = "cs_topichistory"; maxhistory = 3; }
command { service = "ChanServ"; name = "SET TOPICHISTORY"; command = "chanserv/set/topichistory"; }
command { service = "ChanServ"; name = "TOPICHISTORY"; command = "chanserv/topichistory"; group = "chanserv/management"; }
```

Config keys:
- `maxhistory` (default: `3`) — max number of historical topics stored per channel.

### hs_nethost

Automatically assign default vhosts based on the account name (HostServ required).

Config:

```conf
module {
	name = "hs_nethost"

	# If true, set a vhost on identify if the account has none.
	setifnone = false

	# Format: <prefix><nick><suffix>
	prefix = ""
	suffix = ""

	# If the nick contained invalid chars (replaced with '-'), append a hash.
	hashprefix = ""
}
```

Config keys:
- `setifnone` (default: `false`)
- `prefix` (default: empty)
- `suffix` (default: empty)
- `hashprefix` (default: empty)

### m_apiauth

Authenticate users against an external HTTP API (instead of Anope’s internal DB).
The API is expected to return an `access_token` (JWT) and optionally an email.

Extra deps:
- libcurl
- OpenSSL
- jwt-cpp
- nlohmann-json

Config:

```conf
module {
	name = "m_apiauth"

	api_url = "https://www.example/accounts/api/login_token/"
	api_username_param = "username"
	api_password_param = "password"
	api_method = "POST"  # or GET
	api_email_field = "email"

	# Optional header for the API.
	api_key = ""

	# TLS verification options.
	verify_ssl = "true"  # accepts: true/false/1/0/yes/no
	capath = ""
	cainfo = ""

	# JWT verification controls (used when decoding the returned token).
	jwt_secret = ""
	jwt_issuer = ""

	# Optional: block local REGISTER / SET EMAIL and redirect users elsewhere.
	register_url = "https://www.example/accounts/register/"
	profile_url = "https://www.example/accounts/profile/%s/"
	disable_reason = "To register, use %s"
	disable_email_reason = "To change your email, use %s"
}
```

Notes:
- A more detailed doc lives in `m_apiauth.md`.

### m_expirenotice

Send notices via email and/or MemoServ when nicks/channels are expiring soon or have expired.

Config (example from the module header):

```conf
module {
	name = "m_expirenotice"

	ns_notice_expiring = yes
	ns_notice_expired = yes
	ns_notice_time = 7d
	ns_notice_mail = yes
	ns_notice_memo = no

	cs_notice_expiring = yes
	cs_notice_expired = yes
	cs_notice_time = 3d
	cs_notice_mail = yes
	cs_notice_memo = no

	ns_expiring_subject = "Nickname expiring"
	ns_expiring_message = "Your nickname %n will expire %t.\n%N IRC Administration"
	ns_expiring_memo = "Your nickname %n will expire %t."

	ns_expired_subject = "Nickname expired"
	ns_expired_message = "Your nickname %n has expired.\n%N IRC Administration"
	ns_expired_memo = "Your nickname %n has expired."

	cs_expiring_subject = "Channel expiring"
	cs_expiring_message = "Your channel %c will expire %t.\n%N IRC Administration"
	cs_expiring_memo = "Your channel %c will expire %t."

	cs_expired_subject = "Channel expired"
	cs_expired_message = "Your channel %c has expired.\n%N IRC Administration"
	cs_expired_memo = "Your channel %c has expired."
}
```

Template variables:
- `%n` nickname
- `%c` channel
- `%t` expiry time
- `%N` network name

### m_login

Adds `NickServ LOGIN` command to “recover” your nick and identify.

Config:

```conf
module { name = "m_login" }
command { service = "NickServ"; name = "LOGIN"; command = "nickserv/login"; }
```

Config key (in `ns_login` module config):
- `restoreonrecover` (boolean) — if enabled, preserves channel status when regaining nick.

### m_memo_chanaccess

Notify users when they’re added to a channel access list, or when founder/successor changes.
Supports MemoServ memos, email (Anope mail system), and online notices.
Also supports mask entries (e.g. `nick!ident@host` or `*!*@*`).

Config:

```conf
module {
	name = "m_memo_chanaccess"

	# MemoServ (memos)
	notify_access_add = yes
	notify_founder_change = yes
	notify_successor_change = yes

	# Email (Anope mail system)
	email_access_add = no
	email_founder_change = no
	email_successor_change = no

	# If a mask (non-account) is added, reply to the command source.
	notice_unregistered_access_add = no

	# If a mask (including *!*@*) is added and the channel currently exists,
	# notify matching online users in the channel.
	notice_mask_access_add = no

	# If the added account is currently online/identified, send them a notice too.
	notice_online_access_add = no

	# If "no", don’t notify when you change your own access/founder/successor.
	notify_self = no

	# Optional: force memo sender nick (otherwise uses WhoSends()/service/ChanServ).
	sender = "ChanServ"
}
```

### m_saslJwt

Adds SASL mechanisms for JWT (and an enhanced `PLAIN` handler that can accept JWT-like tokens).
This module also has a configurable SASL “agent” pseudoclient.

Config:

```conf
module {
	name = "m_saslJwt"
	agent = "NickServ"  # default
}
```

Important note:
- This module currently hardcodes the JWT secret and issuer inside the source. You must edit the module source to change them.

### m_secure

Proxy detection using proxycheck.io. Kills users detected as proxies and can log to a channel.
Creates a `SeCuRe` service bot.

Extra deps:
- libcurl
- nlohmann-json

Config:

```conf
module {
	name = "m_secure"

	# Required.
	proxycheck_api_key = "your-proxycheck-io-key"

	# Optional logging channel (SeCuRe bot joins it).
	log_channel = "#services"  # default

	# Optional: whitelist specific server names (space or comma separated).
	whitelist_servers = "irc1.example.net irc2.example.net"

	# Optional: wildcard whitelist, only supports leading "*." style.
	wildcard_server = "*.example.net"
}
```

### m_sqlauth

Authenticate users against an SQL database (and auto-create/update Anope accounts).

Config keys (from code):
- `engine` (required) — SQL engine name (e.g. `mysql/main`)
- `password_field` (default: `password`)
- `email_field` (default: `email`)
- `username_field` (default: `username`)
- `table_name` (default: `users`)
- `query` (optional) — custom SQL query
- `disable_reason` (optional) — message shown on `NickServ REGISTER` / `GROUP`
- `disable_email_reason` (optional) — message shown on `NickServ SET EMAIL`
- `kill_message` (default: `Error: Too many failed login attempts. Please try again later. ID:`)
- `max_attempts` (default: `5`)

Config (recommended explicit `query`):

```conf
module {
	name = "m_sqlauth"
	engine = "mysql/main"

	# Custom query (the module substitutes @a@ with the account name).
	query = "SELECT `password`, `email` FROM `users` WHERE `username` = @a@"

	# Optional: block local register/email commands and redirect users elsewhere.
	disable_reason = "To register a new account navigate to https://example.net/register/"
	disable_email_reason = "To change your email navigate to https://example.net/profile/"

	kill_message = "Error: Too many failed login attempts. Please try again later. ID:"
	max_attempts = 5
}
```

### m_youtube

BotServ command that detects YouTube links and replies with metadata (title/duration/views).

Extra deps:
- libcurl
- rapidjson

Important note:
- The YouTube API key is currently hardcoded in the source (`api_key = "API-KEY"`). You must edit the module source to set a real API key.

### ns_sasl_oauthbearer

Implements IRCv3 SASL `OAUTHBEARER` and the newer IRCv3 bearer format, validating JWTs server-side.

Extra deps:
- OpenSSL
- jwt-cpp

Config:

```conf
module {
	name = "ns_sasl_oauthbearer"

	jwt_secret = "..."
	jwt_issuer = "..."

	# Auto-create accounts when the token subject (sub) doesn't exist yet.
	autocreate = true

	# For IRCv3 bearer format: allow token_type=oauth2 to be treated as jwt.
	allow_oauth2_type_as_jwt = false
}
```

### os_chantrap

Create trap channels (exact or wildcard masks) to catch unwanted users/botnets; can KILL or AKILL when users join.

Config:

```conf
module {
	name = "os_chantrap"
	killreason = "I know what you did last join!"
	akillreason = "You found yourself a disappearing act!"

	# Optional limits/behavior.
	maxbots = 5
	createbots = no
}

command { service = "OperServ"; name = "CHANTRAP"; command = "operserv/chantrap"; permission = "operserv/chantrap"; }
```

### os_forceid

Allows an operator to force-identify a user to their matching nick.

Config:

```conf
module { name = "os_forceid" }
command { service = "OperServ"; name = "FORCEID"; command = "operserv/forceid"; permission = "operserv/forceid"; }
```

### os_notinchan

OperServ command to list/kill/akill/tempshun/join users who are not in any channel.

Config:

```conf
module {
	name = "os_notinchan"

	tshunreason = "Rejoin us when you are willing to join us publicly."
	killreason = "Not In Channel Management"
	akillreason = "Not In Channel Management"

	akillexpire = "5m"
	idlechan = "#idle"
}

command { service = "OperServ"; name = "NOTINCHAN"; command = "operserv/notinchan"; permission = "operserv/akill"; }
```

### os_regset

Modify the registration time of a nick or channel.

Config:

```conf
module { name = "os_regset" }
command { service = "OperServ"; name = "REGSET"; command = "operserv/regset"; permission = "operserv/regset"; }
```

## Contact

IRC: irc.irc4fun.net +6697 (tls)
Channel: #development