# helpserv

HelpServ is a pseudo-client help system for Anope 2.1.

Features:
- Topic-based help: `HELP` / `HELP <topic>`
- Search: `SEARCH <words>`
- Stats: `STATS` (hidden by default)
- Staff escalation:
  - `HELPME <topic> [message]` pages staff immediately
  - Ticket queue: `REQUEST <topic> [message]` + `CANCEL`
  - Staff tools: `LIST` + `CLOSE <nick|account> [reason]`

## Install

1. Copy the module directory into your Anope source tree:
   - Copy `HelpServ/` to `modules/third/HelpServ/`
2. Re-run CMake, then build and install your modules.
3. Add the configuration blocks below.
4. Reload services (`/msg OperServ RELOAD`) or restart.

## Configuration

A complete example is shipped as `HelpServ/helpserv.example.conf`.

Minimal config:

```conf
service
{
  nick = "HelpServ"
  user = "HelpServ"
  host = "chaat.services"
  gecos = "Help Service"

  # Optional channels to join.
  # (HelpServ will also join staff_target if it is a #channel)
  channels = "@#services"
}

module
{
  name = "helpserv"
  client = "HelpServ"

  # Paging target for HELPME/REQUEST:
  # - "globops"   : send to oper globops/wallops
  # - "#channel"  : send to a staff channel (HelpServ will join it)
  # - empty        : disable paging
  staff_target = "#services"

  # Cooldowns (seconds)
  helpme_cooldown = 120
  request_cooldown = 60

  # When enabled, REQUEST also pages staff_target
  page_on_request = yes

  # Privilege required for staff commands LIST/CLOSE
  ticket_priv = "helpserv/ticket"

  # Help topics
  topic
  {
    name = "register"
    line { text = "To register your nickname: /msg NickServ REGISTER <password> <email>" }
    line { text = "Then follow the instructions sent to you." }
  }
}

command { service = "HelpServ"; name = "HELP"; command = "generic/help"; }
command { service = "HelpServ"; name = "SEARCH"; command = "helpserv/search"; }
command { service = "HelpServ"; name = "STATS"; command = "helpserv/stats"; hide = true; }

command { service = "HelpServ"; name = "HELPME"; command = "helpserv/helpme"; }
command { service = "HelpServ"; name = "REQUEST"; command = "helpserv/request"; }
command { service = "HelpServ"; name = "CANCEL"; command = "helpserv/cancel"; }

# Staff commands (hidden by default)
command { service = "HelpServ"; name = "LIST"; command = "helpserv/list"; hide = true; }
command { service = "HelpServ"; name = "CLOSE"; command = "helpserv/close"; hide = true; }
```

## Usage

Users:
- `HELP` / `HELP <topic>`
- `SEARCH <words>`
- `HELPME <topic> [message]` (pages staff now)
- `REQUEST <topic> [message]` (opens/updates a queued ticket; requires identify)
- `CANCEL` (cancels your open ticket)

Staff:
- `LIST` (lists open tickets)
- `CLOSE <nick|account> [reason]` (closes a ticket)

## Persistence (flatfile DB)

HelpServ stores data in Anope’s data directory:
- `data/helpserv.db` — stats counters
- `data/helpserv_tickets.db` — open tickets + `next_ticket_id`

These are simple flatfiles written/read by the module (not Anope’s built-in DB backend).
