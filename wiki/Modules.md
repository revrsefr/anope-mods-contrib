# Modules

This page lists every module in the repository.

Notes:
- If a module has its own page here, it’s linked.
- Otherwise, see the repository README for full config/usage.

## ChanServ

- [cs_akick_check](https://github.com/revrsefr/anope-mods-contrib/wiki/cs_akick_check) — re-check AKICKs when user identity/account changes.
- [cs_topichistory](https://github.com/revrsefr/anope-mods-contrib/wiki/cs_topichistory) — keep a per-channel topic history and allow listing/setting.

## HostServ

- [hs_nethost](https://github.com/revrsefr/anope-mods-contrib/wiki/hs_nethost) — automatically set vhosts based on account name.

## NickServ

- [m_login](https://github.com/revrsefr/anope-mods-contrib/wiki/m_login) — adds `NickServ LOGIN` (recover/switch to a nickname + identify in one step).
- [ns_sasl_oauthbearer](https://github.com/revrsefr/anope-mods-contrib/wiki/ns_sasl_oauthbearer) — SASL OAUTHBEARER / IRCv3 bearer token auth using JWT validation.

## OperServ

- [os_chantrap](https://github.com/revrsefr/anope-mods-contrib/wiki/os_chantrap) — trap channels to catch unwanted joins (kill/akill).
- [os_forceid](https://github.com/revrsefr/anope-mods-contrib/wiki/os_forceid) — force-identify a user to their matching nick.
- [os_notinchan](https://github.com/revrsefr/anope-mods-contrib/wiki/os_notinchan) — list/act on users not in any channel.
- [os_regset](https://github.com/revrsefr/anope-mods-contrib/wiki/os_regset) — adjust registration time for nicks/channels.

## SASL

- [m_saslJwt](https://github.com/revrsefr/anope-mods-contrib/wiki/m_saslJwt) — JWT-based SASL mechanisms/agent behavior.

## Security / auth

- [m_apiauth](https://github.com/revrsefr/anope-mods-contrib/wiki/m_apiauth) — authenticate against an external HTTP API (JWT + optional email).
- [m_secure](https://github.com/revrsefr/anope-mods-contrib/wiki/m_secure) — proxy detection using proxycheck.io.
- [m_sqlauth](https://github.com/revrsefr/anope-mods-contrib/wiki/m_sqlauth) — authenticate against an SQL database (and auto-create/update accounts).

## Messaging / notices

- [m_expirenotice](https://github.com/revrsefr/anope-mods-contrib/wiki/m_expirenotice) — email/memo notices for expiring/expired nicks/channels.
- [m_memo_chanaccess](https://github.com/revrsefr/anope-mods-contrib/wiki/m_memo_chanaccess) — notify on channel access list changes (memos/email/notices).

## Stats

- [chanstats_plus](https://github.com/revrsefr/anope-mods-contrib/wiki/chanstats_plus) — SQL-backed channel/nick stats with batched flushing.
- [rpc_chanstatsplus](https://github.com/revrsefr/anope-mods-contrib/wiki/rpc_chanstatsplus) — RPC methods to query `chanstats_plus`.

## Fun

- [m_youtube](https://github.com/revrsefr/anope-mods-contrib/wiki/m_youtube) — BotServ helper that replies with YouTube metadata.

## Services

- [helpserv](https://github.com/revrsefr/anope-mods-contrib/wiki/helpserv) — HelpServ pseudo-client (help topics + search + stats + tickets/paging; configurable NOTICE/PRIVMSG replies; cooldown-map pruning; STATS usage counters).
- [infoserv](https://github.com/revrsefr/anope-mods-contrib/wiki/infoserv) — InfoServ pseudo-client (connect/oper informational messages; admin-managed posts; optional global broadcasts; configurable NOTICE/PRIVMSG replies).
