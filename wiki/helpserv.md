# helpserv

HelpServ is a pseudo-client help system for Anope 2.1.

Features:
- Topic-based help: `HELP` / `HELP <topic>`
- Search: `SEARCH <words>`
- Stats: `STATS` (hidden by default)
- Staff escalation:
  - `HELPME <topic> [message]` pages staff immediately
  - Ticket queue: `REQUEST <topic> [message]` + `CANCEL`
  - Staff tools: `LIST [filter]` + `VIEW #id` + `TAKE #id` + `ASSIGN #id <nick|none>` + `NOTE #id <text>` + `CLOSE <#id|nick|account> [reason]`
  - Queue workflow: `NEXT [ALL] [filter]` + `PRIORITY #id <low|normal|high>` + `WAIT #id [reason]` + `UNWAIT #id`

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

  # How HelpServ replies to users:
  # - notice (default)
  # - privmsg
  reply_method = "notice"

  # Expire old tickets (default 0 = disabled). Supports suffixes: s/m/h/d/w.
  # Example: 7d
  ticket_expire = 0

  # Cooldowns (seconds)
  helpme_cooldown = 120
  request_cooldown = 60

  # When enabled, REQUEST also pages staff_target
  page_on_request = yes

  # Privilege required for staff commands LIST/CLOSE
  ticket_priv = "helpserv/ticket"

  # Privilege required to change reply_method at runtime using NOTIFY
  notify_priv = "helpserv/admin"

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
command { service = "HelpServ"; name = "VIEW"; command = "helpserv/view"; hide = true; }
command { service = "HelpServ"; name = "TAKE"; command = "helpserv/take"; hide = true; }
command { service = "HelpServ"; name = "ASSIGN"; command = "helpserv/assign"; hide = true; }
command { service = "HelpServ"; name = "NOTE"; command = "helpserv/note"; hide = true; }
command { service = "HelpServ"; name = "CLOSE"; command = "helpserv/close"; hide = true; }

# Admin command (hidden by default)
command { service = "HelpServ"; name = "NOTIFY"; command = "helpserv/notify"; hide = true; }
```

## Usage

Users:
- `HELP` / `HELP <topic>`
- `SEARCH <words>`
- `HELPME <topic> [message]` (pages staff now)
- `REQUEST <topic> [message]` (opens/updates a queued ticket; requires identify)
- `CANCEL` (cancels your open ticket)

Staff:
- `LIST [filter]` (lists open tickets)
- `VIEW #id` (shows a ticket including notes)
- `TAKE #id` (claim a ticket)
- `ASSIGN #id <nick|none>` (assign/unassign)
- `NOTE #id <text>` (add internal notes)
- `CLOSE <#id|nick|account> [reason]` (closes a ticket)

Queue workflow:
- `NEXT [ALL] [filter]` (shows the next ticket in the queue)
- `PRIORITY #id <low|normal|high>` (set ticket priority)
- `WAIT #id [reason]` (mark ticket waiting for the user)
- `UNWAIT #id` (mark ticket open again)

Admin:
- `NOTIFY [notice|privmsg]` (controls how HelpServ replies; requires `notify_priv`)

Stats:
- `STATS` also includes ticket queue stats (open vs waiting, priority breakdown, assignment counts, oldest tickets).

## Notes

- `reply_method = "privmsg"` forces HelpServ command output to be sent via `PRIVMSG` (instead of being influenced by user-side preferences).

## Queue behavior

Tickets are ordered like a queue:
- State first: `open` tickets before `waiting` tickets
- Priority next: `high` before `normal` before `low`
- Age last: older tickets first (by last update time when present)

Notes:
- `LIST` shows both `open` and `waiting` tickets, with a `[priority/state]` marker.
- `NEXT` shows only `open` tickets by default; use `NEXT ALL` to include `waiting` tickets.
- `WAIT` optionally records the reason as a note and moves the ticket to `waiting`.
- `UNWAIT` clears the wait reason and returns the ticket to `open`.

## Persistence (flatfile DB)

HelpServ stores data in Anope’s data directory:
- `data/helpserv.db` — stats counters
- `data/helpserv_tickets.db` — open tickets + `next_ticket_id`

These are simple flatfiles written/read by the module (not Anope’s built-in DB backend).
