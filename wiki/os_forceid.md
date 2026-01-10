# os_forceid

Allows an operator to force-identify a user to their matching nick.

Config:

```conf
module { name = "os_forceid" }
command { service = "OperServ"; name = "FORCEID"; command = "operserv/forceid"; permission = "operserv/forceid"; }
```
