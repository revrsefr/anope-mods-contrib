# m_login

Adds `NickServ LOGIN` command to “recover” your nick and identify.

Config:

```conf
module { name = "m_login" }
command { service = "NickServ"; name = "LOGIN"; command = "nickserv/login"; }
```

Config key (in `m_login` module config):
- `restoreonrecover` (boolean) — if enabled, preserves channel status when regaining nick.
