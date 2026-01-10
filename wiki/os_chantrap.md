# os_chantrap

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
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
