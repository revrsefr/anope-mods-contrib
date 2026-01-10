# os_regset

Modify the registration time of a nick or channel.

Config:

```conf
module { name = "os_regset" }
command { service = "OperServ"; name = "REGSET"; command = "operserv/regset"; permission = "operserv/regset"; }
```
