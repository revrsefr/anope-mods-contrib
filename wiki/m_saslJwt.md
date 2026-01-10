# m_saslJwt

Adds SASL mechanisms for JWT (and an enhanced `PLAIN` handler that can accept JWT-like tokens).
This module also has a configurable SASL “agent” pseudoclient.

Config:

```conf
module {
	name = "m_saslJwt"
	agent = "NickServ"  # default
}
```

Important note:
- This module currently hardcodes the JWT secret and issuer inside the source. You must edit the module source to change them.
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
