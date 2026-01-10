# m_secure

Proxy detection using proxycheck.io. Kills users detected as proxies and can log to a channel.
Creates a `SeCuRe` service bot.

Extra deps:
- libcurl
- nlohmann-json

Config:

```conf
module {
	name = "m_secure"

	# Required.
	proxycheck_api_key = "your-proxycheck-io-key"

	# Optional logging channel (SeCuRe bot joins it).
	log_channel = "#services"  # default

	# Optional: whitelist specific server names (space or comma separated).
	whitelist_servers = "irc1.example.net irc2.example.net"

	# Optional: wildcard whitelist, only supports leading "*." style.
	wildcard_server = "*.example.net"
}
```
