# Anope 2.1 contrib modules

Collection of third-party modules for **Anope IRC Services 2.1.x**.

Wiki pages (in-repo):
- Start here: `wiki/Home.md`
- Index: `wiki/Modules.md`

GitHub note: the `/wiki` tab/URL is GitHub’s built-in Wiki feature. If it’s disabled, use the `wiki/` folder links above.

Each module is independent.

Documentation lives in the wiki:
- GitHub Wiki: https://github.com/revrsefr/anope-mods-contrib/wiki
- Mirror in this repo: `wiki/`

## Contents

- [Build & install](#build--install)
- [Wiki](#wiki)
- [Modules](#modules)

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

## Wiki

All module documentation (description, configuration, required commands) is in the wiki:

- https://github.com/revrsefr/anope-mods-contrib/wiki

This repository also mirrors those pages under `wiki/` for offline browsing.

## Modules

- Wiki index: https://github.com/revrsefr/anope-mods-contrib/wiki/Modules
- Module pages:
	- https://github.com/revrsefr/anope-mods-contrib/wiki/cs_topichistory
	- https://github.com/revrsefr/anope-mods-contrib/wiki/cs_akick_check
	- https://github.com/revrsefr/anope-mods-contrib/wiki/hs_nethost
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_apiauth
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_expirenotice
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_login
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_memo_chanaccess
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_saslJwt
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_secure
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_sqlauth
	- https://github.com/revrsefr/anope-mods-contrib/wiki/m_youtube
	- https://github.com/revrsefr/anope-mods-contrib/wiki/ns_sasl_oauthbearer
	- https://github.com/revrsefr/anope-mods-contrib/wiki/os_chantrap
	- https://github.com/revrsefr/anope-mods-contrib/wiki/os_forceid
	- https://github.com/revrsefr/anope-mods-contrib/wiki/os_notinchan
	- https://github.com/revrsefr/anope-mods-contrib/wiki/os_regset
	- https://github.com/revrsefr/anope-mods-contrib/wiki/chanstats_plus
	- https://github.com/revrsefr/anope-mods-contrib/wiki/rpc_chanstatsplus

## Contact

IRC: irc.irc4fun.net +6697 (tls)
Channel: #development