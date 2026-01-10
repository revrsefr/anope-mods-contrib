# m_apiauth

Authenticate users against an external HTTP API (instead of Anope’s internal DB).
The API is expected to return an `access_token` (JWT) and optionally an email.

Extra deps:
- libcurl
- OpenSSL
- jwt-cpp
- nlohmann-json

Config:

```conf
module {
	name = "m_apiauth"

	api_url = "https://www.example/accounts/api/login_token/"
	api_username_param = "username"
	api_password_param = "password"
	api_method = "POST"  # or GET
	api_email_field = "email"

	# Optional header for the API.
	api_key = ""

	# TLS verification options.
	verify_ssl = "true"  # accepts: true/false/1/0/yes/no
	capath = ""
	cainfo = ""

	# JWT verification controls (used when decoding the returned token).
	jwt_secret = ""
	jwt_issuer = ""

	# Optional: block local REGISTER / SET EMAIL and redirect users elsewhere.
	register_url = "https://www.example/accounts/register/"
	profile_url = "https://www.example/accounts/profile/%s/"
	disable_reason = "To register, use %s"
	disable_email_reason = "To change your email, use %s"
}
```

Notes:
- A more detailed doc lives in `m_apiauth.md`.
---
<!-- nav -->
[Home](https://github.com/revrsefr/anope-mods-contrib/wiki) · [Modules](https://github.com/revrsefr/anope-mods-contrib/wiki/Modules)
<!-- /nav -->
