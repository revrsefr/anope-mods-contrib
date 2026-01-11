# chanfix

ChanFix is an Anope 2.1 pseudo-client that tracks operator history in *unregistered* channels and can attempt to restore ops when a channel becomes opless.

Important:
- ChanFix **will not touch registered channels**. Registration is owned by ChanServ; ChanFix checks Anope’s registration list (`ChannelInfo`) and skips registered channels.
- ChanFix maintains its own small flatfile DB (`data/chanfix.db`) which is *only* score/history + flags for unregistered channels.

## Features

- Periodically gathers a score for users who currently have `+o` in unregistered channels.
- Expires/decays scores over time so old history fades out.
- Manual fix requests (`CHANFIX #channel`) for staff.
- Optional autofix loop (disabled by default).
- Per-channel controls:
  - `NOFIX` — prevent ChanFix from acting on a channel
  - `MARK` — annotate a channel with a note for staff

## Install

1. Copy the module directory into your Anope source tree:
   - Copy `ChanFix/` to `modules/third/ChanFix/`
2. Re-run CMake, then build and install your modules.
3. Add the configuration blocks below.
4. Reload services (`/msg OperServ RELOAD`) or restart.

## Configuration

A complete example is shipped as `ChanFix/chanfix.example.conf`.

Minimal config:

```conf
service
{
  nick = "ChanFix"
  user = "ChanFix"
  host = "services.host"
  gecos = "Channel Fix Service"

  # Optional channels to join.
  channels = "@#services"
}

module
{
  name = "chanfix"
  client = "ChanFix"

  # Privileges
  admin_priv = "chanfix/admin"
  auspex_priv = "chanfix/auspex"

  # Safety defaults
  autofix = no
  join_to_fix = no

  # Algorithm
  op_threshold = 3
  min_fix_score = 12

  # Timers (seconds)
  gather_interval = 300
  expire_interval = 3600
  autofix_interval = 60
  save_interval = 600
}

command { service = "ChanFix"; name = "HELP"; command = "generic/help"; }

# Staff commands (hidden by default)
command { service = "ChanFix"; name = "CHANFIX"; command = "chanfix/chanfix"; hide = true; }
command { service = "ChanFix"; name = "SCORES"; command = "chanfix/scores"; hide = true; }
command { service = "ChanFix"; name = "INFO"; command = "chanfix/info"; hide = true; }
command { service = "ChanFix"; name = "LIST"; command = "chanfix/list"; hide = true; }
command { service = "ChanFix"; name = "MARK"; command = "chanfix/mark"; hide = true; }
command { service = "ChanFix"; name = "NOFIX"; command = "chanfix/nofix"; hide = true; }
```

## Commands

All commands are exposed via the `ChanFix` pseudo-client.

- `CHANFIX <#channel>` — request a fix attempt (requires `admin_priv`)
- `SCORES <#channel> [count]` — show top scores for a channel (requires `auspex_priv`)
- `INFO <#channel>` — show ChanFix status/metadata for a channel (requires `auspex_priv`)
- `LIST [pattern]` — list channels with ChanFix records (requires `auspex_priv`)
- `MARK <#channel> <ON|OFF> [note]` — set/clear a staff note (requires `admin_priv`)
- `NOFIX <#channel> <ON|OFF> [reason]` — disable/enable fixing for a channel (requires `admin_priv`)

## How it decides what to fix

ChanFix only considers channels that are both:
- currently in the live channel list, and
- **unregistered** (no `ChannelInfo` entry)

Then it uses the score history to decide which users are plausible operators, and attempts to `+o` them.

## Persistence (flatfile DB)

ChanFix stores data in Anope’s data directory:
- `data/chanfix.db`

This is a simple flatfile written/read by the module (not Anope’s built-in DB backend).
