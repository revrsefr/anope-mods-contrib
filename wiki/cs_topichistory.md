# cs_topichistory

Keep a per-channel topic history and allow listing/setting from the history.

Commands:

```conf
module { name = "cs_topichistory"; maxhistory = 3; }
command { service = "ChanServ"; name = "SET TOPICHISTORY"; command = "chanserv/set/topichistory"; }
command { service = "ChanServ"; name = "TOPICHISTORY"; command = "chanserv/topichistory"; group = "chanserv/management"; }
```

Config keys:
- `maxhistory` (default: `3`) — max number of historical topics stored per channel.
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
