# ns_sasl_oauthbearer

Implements IRCv3 SASL `OAUTHBEARER` and the newer IRCv3 bearer format, validating JWTs server-side.

Extra deps:
- OpenSSL
- jwt-cpp

Config:

```conf
module {
	name = "ns_sasl_oauthbearer"

	jwt_secret = "..."
	jwt_issuer = "..."

	# Auto-create accounts when the token subject (sub) doesn't exist yet.
	autocreate = true

	# For IRCv3 bearer format: allow token_type=oauth2 to be treated as jwt.
	allow_oauth2_type_as_jwt = false
}
```
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
