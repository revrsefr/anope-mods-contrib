# os_notinchan

OperServ command to list/kill/akill/tempshun/join users who are not in any channel.

Config:

```conf
module {
	name = "os_notinchan"

	tshunreason = "Rejoin us when you are willing to join us publicly."
	killreason = "Not In Channel Management"
	akillreason = "Not In Channel Management"

	akillexpire = "5m"
	idlechan = "#idle"
}

command { service = "OperServ"; name = "NOTINCHAN"; command = "operserv/notinchan"; permission = "operserv/akill"; }
```
