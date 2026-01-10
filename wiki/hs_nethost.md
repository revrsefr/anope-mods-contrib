# hs_nethost

Automatically assign default vhosts based on the account name (HostServ required).

Config:

```conf
module {
	name = "hs_nethost"

	# If true, set a vhost on identify if the account has none.
	setifnone = false

	# Format: <prefix><nick><suffix>
	prefix = ""
	suffix = ""

	# If the nick contained invalid chars (replaced with '-'), append a hash.
	hashprefix = ""
}
```

Config keys:
- `setifnone` (default: `false`)
- `prefix` (default: empty)
- `suffix` (default: empty)
- `hashprefix` (default: empty)
