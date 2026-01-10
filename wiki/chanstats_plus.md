# chanstats_plus

SQL-backed channel/nick stats optimized for low SQL round-trips.

## Highlights

- Buffers increments in memory and flushes in batches (`INSERT .. ON DUPLICATE KEY UPDATE`).
- Tracks multiple periods (`daily`, `weekly`, `monthly`, `total`).
- Supports per-channel enablement (ChanServ) and per-nick enablement (NickServ).

## Config + commands

See the repository README for the full config example and command blocks.
