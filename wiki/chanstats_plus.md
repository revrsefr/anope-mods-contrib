# chanstats_plus

SQL-backed chanstats module optimized for low SQL round-trips.

Key behavior:
- Buffers counter deltas in memory and flushes them periodically in batched `INSERT .. ON DUPLICATE KEY UPDATE` queries.
- Tracks multiple periods as separate rows keyed by a period start date (`daily`, `weekly`, `monthly`, `total`).
- Creates/maintains the SQL table automatically on reload.

What it tracks:
- lines, words, letters
- CTCP ACTIONs (`/me`), kicks given / kicked, mode changes, topic changes
- smileys (counts tokens from configured lists; tokens are removed from the word count)

Enabling stats:
- Per-channel stats are recorded only when enabled on that channel via ChanServ.
- Per-nick stats are recorded only for identified users who enable it via NickServ.

Config:

```conf
module {
	name = "chanstats_plus"

	# SQL::Provider service name.
	engine = "mysql/main"

	# Table prefix; table name becomes <prefix>chanstatsplus
	prefix = "anope_"

	# Flush tuning.
	flushinterval = 5s
	maxpending = 100000
	maxrowsperquery = 500

	# Lists of smiley tokens.
	smileyshappy = { ":)" ":-)" ":D" }
	smileyssad = { ":(" ":-(" }
	smileysother = { ";)" ";-)" }
}

command { service = "ChanServ"; name = "SET CHANSTATSPLUS"; command = "chanserv/set/chanstatsplus"; }
command { service = "NickServ"; name = "SET CHANSTATSPLUS"; command = "nickserv/set/chanstatsplus"; }
command { service = "NickServ"; name = "SASET CHANSTATSPLUS"; command = "nickserv/saset/chanstatsplus"; permission = "nickserv/saset"; }
```

SQL schema notes:
- Table: ``<prefix>chanstatsplus``
- Primary key: `(chan, nick, period, period_start)`
- Aggregate rows are stored with either `chan=''` (global-per-nick) or `nick=''` (per-channel aggregates).
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
