# Modules

This page lists every module in the repository.

Notes:
- If a module has its own page here, it’s linked.
- Otherwise, see the repository README for full config/usage.

## ChanServ

- [cs_akick_check](cs_akick_check.md) — re-check AKICKs when user identity/account changes.
- [cs_topichistory](cs_topichistory.md) — keep a per-channel topic history and allow listing/setting.

## HostServ

- [hs_nethost](hs_nethost.md) — automatically set vhosts based on account name.

## NickServ

- [m_login](m_login.md) — adds `NickServ LOGIN` (recover/switch to a nickname + identify in one step).
- [ns_sasl_oauthbearer](ns_sasl_oauthbearer.md) — SASL OAUTHBEARER / IRCv3 bearer token auth using JWT validation.

## OperServ

- [os_chantrap](os_chantrap.md) — trap channels to catch unwanted joins (kill/akill).
- [os_forceid](os_forceid.md) — force-identify a user to their matching nick.
- [os_notinchan](os_notinchan.md) — list/act on users not in any channel.
- [os_regset](os_regset.md) — adjust registration time for nicks/channels.

## SASL

- [m_saslJwt](m_saslJwt.md) — JWT-based SASL mechanisms/agent behavior.

## Security / auth

- [m_apiauth](m_apiauth.md) — authenticate against an external HTTP API (JWT + optional email).
- [m_secure](m_secure.md) — proxy detection using proxycheck.io.
- [m_sqlauth](m_sqlauth.md) — authenticate against an SQL database (and auto-create/update accounts).

## Messaging / notices

- [m_expirenotice](m_expirenotice.md) — email/memo notices for expiring/expired nicks/channels.
- [m_memo_chanaccess](m_memo_chanaccess.md) — notify on channel access list changes (memos/email/notices).

## Stats

- [chanstats_plus](chanstats_plus.md) — SQL-backed channel/nick stats with batched flushing.
- [rpc_chanstatsplus](rpc_chanstatsplus.md) — RPC methods to query `chanstats_plus`.

## Fun

- [m_youtube](m_youtube.md) — BotServ helper that replies with YouTube metadata.
