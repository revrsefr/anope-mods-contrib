# InfoServ

InfoServ is an Anope 2.1 pseudo-client inspired by Atheme’s InfoServ.

It provides:
- informational messages shown on user connect
- oper-only informational messages shown when a user gains OPER
- admin commands to post/list/move/delete entries
- optional global broadcasts for higher-importance posts
- flatfile persistence in `data/infoserv.db`

## Configuration

See the shipped example config:
- `InfoServ/infoserv.example.conf`

Key options (module block):
- `client` — the service nick to use (e.g. `InfoServ`)
- `reply_method` — `notice` (default) or `privmsg`
- `notify_priv` — privilege required to change `reply_method` at runtime using `NOTIFY`
- `admin_priv` — privilege required for admin commands
- `show_on_connect` — show public messages on connect (`yes`/`no`)
- `show_on_oper` — show oper-only messages when a user gains OPER (`yes`/`no`)
- `logoninfo_count` — how many public messages to show on connect
- `logoninfo_reverse` — if enabled, show most recent first
- `logoninfo_show_metadata` — show poster + timestamp

## Commands

Public:
- `INFO` — show current public messages (and oper-only messages if you’re oper)

Admin (usually hidden):
- `POST <importance 0-4> <subject> <message>` — post a message
- `LIST` / `MOVE <from> <to>` / `DEL <id>` — manage public message list
- `OLIST` / `OMOVE <from> <to>` / `ODEL <id>` — manage oper-only message list
- `NOTIFY [notice|privmsg]` — change reply transport at runtime

### POST importance levels

- `0` — oper-only (stored, displayed on oper-up)
- `1` — public (stored, displayed on connect)
- `2` — global notice (broadcast immediately, not stored)
- `3` — public + global notice (stored and broadcast)
- `4` — critical global notice (broadcast immediately, not stored)

### Subject formatting

`subject` is a single token. Use `_` for spaces (it will be displayed as spaces).

Examples:
- `/msg InfoServ POST 1 Network_Upgrade The network will reboot at 03:00 UTC.`
- `/msg InfoServ POST 0 Staff_Memo Remember to update your fingerprints.`

## Data storage

Messages are stored in a flatfile at `data/infoserv.db` (relative to your Anope data dir).
