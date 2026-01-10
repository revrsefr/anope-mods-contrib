# cs_akick_check

Re-check channel AKICKs after services startup/uplink sync and when a user’s identity/account state changes.

## Config

```conf
module { name = "cs_akick_check" }
```

## Notes

- This module is intentionally low-config: you load it and it starts enforcing AKICK consistency.

See the full README section: [cs_akick_check](../README.md#cs_akick_check)
