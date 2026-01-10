# cs_akick_check

Re-check channel AKICKs after services startup/uplink sync, and when a user’s account state or visible identity changes.
This helps ensure AKICKs apply immediately after identify/logout/group/nick/host/ident changes.

Config:

```conf
module { name = "cs_akick_check" }
```
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
