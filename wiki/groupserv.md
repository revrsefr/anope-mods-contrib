# GroupServ

GroupServ is an Anope 2.1 pseudo-client inspired by Atheme’s GroupServ.

It provides **account groups** named like `!group` which contain member accounts, along with per-group access flags (invite/manage/set/view) and basic group settings.

This module is **Atheme-like**, not a byte-for-byte port.

## What users can do with it

- Create groups like `!staff`, `!helpers`, `!devs`
- Invite accounts to a group, or make a group open to allow self-joins
- Maintain a group member list with per-member permissions
- Set “join flags” so new joiners automatically receive specific permissions
- (Optional) Mark registered channels as “group-only” so non-members are auto-kicked

## Important note about `V`

`V` in GroupServ is accepted as an alias for the **GroupServ access flag** `A` (ACLVIEW: allowed to view the group access list).

You may see ACLVIEW displayed as `A` in outputs.

It is **not** an IRC user mode. If you try `/mode Nick +V`, InspIRCd will reject it.

## Configuration

See the shipped example config:
- `GroupServ/groupserv.example.conf`

Key module options:
- `client` — the service nick to use (e.g. `GroupServ`)
- `reply_method` — `notice` (default) or `privmsg`
- `admin_priv` — privilege required for forced admin commands (`FDROP`, `FFLAGS`)
- `auspex_priv` — privilege required to view/list any group
- `exceed_priv` — privilege required to bypass limits
- `maxgroups` — max groups a single account may found (create)
- `maxgroupacs` — max access list size per group (`0 = unlimited`)
- `enable_open_groups` — allow `SET OPEN ON`
- `default_joinflags` — flags granted when a user joins an open group
- `save_interval` — autosave interval in seconds (`0` disables periodic autosave)

### `default_joinflags` format

These are **GroupServ access flags**, not channel modes.

Accepted formats:
- space-separated: `+V +I +S`
- long names: `+ACLVIEW +INVITE +SET`
- compact (Atheme-style): `+VI` or `+fAsivb`

Notes:
- `V` is accepted as an alias for `A` (ACLVIEW).

## Commands

All commands are used as:

- `/msg GroupServ <COMMAND> ...`

### Channel restrictions (ChanServ)

GroupServ can associate a registered channel with a group and (optionally) enforce “group-only” membership.

- `SET #channel GROUP <!group>`
  - Associates the channel with the group.
  - By default, this also enables GROUPONLY enforcement.
  - Example: `/msg ChanServ SET #dev GROUP !devs`

- `SET #channel GROUPONLY <ON|OFF>`
  - Toggles enforcement for the channel.
  - When enabled, users who join the channel but are not members of the group are automatically kicked.
  - Example: `/msg ChanServ SET #dev GROUPONLY ON`

Important: Anope does not provide a true “pre-join deny” hook for this, so the behavior is “join then kick” (Atheme-like in practice).

### Public

- `HELP`
  - Shows help for GroupServ commands.

- `LISTCHANS <!group>`
  - Lists registered channels explicitly associated with the group via `ChanServ SET #channel GROUP`.
  - Example: `/msg GroupServ LISTCHANS !devs`

- `REGISTER <!group>`
  - Creates a group and makes your account the founder.
  - Example: `/msg GroupServ REGISTER !staff`
    - Meaning: create `!staff` and give you founder permissions.

- `INFO <!group>`
  - Shows group information (flags/settings/metadata).
  - Example: `/msg GroupServ INFO !staff`

- `LIST <pattern>`
  - Lists groups matching a pattern.
  - Example: `/msg GroupServ LIST !*`
    - Meaning: list all groups starting with `!`.

- `JOIN <!group>`
  - Joins a group (must be identified).
  - Works if the group is open, or you have a pending invite.
  - Example: `/msg GroupServ JOIN !staff`

### Membership / invites

- `INVITE <!group> <account>`
  - Invites a NickServ account to a group.
  - Requires you have the group access flag `I` (INVITE) in that group.
  - Example: `/msg GroupServ INVITE !staff Alice`
    - Meaning: create an invite for account `Alice`.
    - The invited user accepts with: `/msg GroupServ JOIN !staff`

### Access list (member flags)

- `FLAGS <!group>`
  - Shows the group access list.
  - Example: `/msg GroupServ FLAGS !staff`

- `FLAGS <!group> <account> <flags>`
  - Adds/removes flags for a member (requires `M`/manage or founder).
  - Examples:
    - `/msg GroupServ FLAGS !staff Alice +IV`
      - Meaning: give Alice INVITE (`I`) and ACLVIEW (`V`).
    - `/msg GroupServ FLAGS !staff Alice -I`
      - Meaning: remove INVITE (`I`) from Alice.
    - `/msg GroupServ FLAGS !staff Alice +F`
      - Meaning: make Alice a founder (and she will automatically gain management powers).

### Group settings

- `SET <!group> DESCRIPTION <text>`
  - Example: `/msg GroupServ SET !staff DESCRIPTION Network staff group`

- `SET <!group> URL <url>`
  - Example: `/msg GroupServ SET !staff URL https://example.org/staff`

- `SET <!group> EMAIL <email>`
  - Example: `/msg GroupServ SET !staff EMAIL staff@example.org`

- `SET <!group> CHANNEL <#channel>`
  - Example: `/msg GroupServ SET !staff CHANNEL #staff`

- `SET <!group> OPEN <ON|OFF>`
  - Turns open-join on/off (requires founder).
  - Example: `/msg GroupServ SET !helpers OPEN ON`
    - Meaning: anyone identified can `/msg GroupServ JOIN !helpers`.

- `SET <!group> PUBLIC <ON|OFF>`
  - Marks the group public/private (how it’s displayed by INFO/LIST).

- `SET <!group> JOINFLAGS <flags|OFF|NONE>`
  - Controls what flags are granted when someone joins the group.
  - Examples:
    - `/msg GroupServ SET !helpers JOINFLAGS +V`
      - Meaning: new joiners can view the group member list.
    - `/msg GroupServ SET !helpers JOINFLAGS +VI`
      - Meaning: new joiners can view and invite.
    - `/msg GroupServ SET !helpers JOINFLAGS OFF`
      - Meaning: clear group-specific join flags (falls back to `default_joinflags`).

### Drop

- `DROP <!group> [key]`
  - Drops a group (normally requires a confirmation key).
  - Example flow:
    - `/msg GroupServ DROP !staff`
    - GroupServ replies with a key
    - `/msg GroupServ DROP !staff <key>`

### Oper/admin (usually hidden)

- `FDROP <!group> [key]`
  - Forced drop (requires `admin_priv`).

- `FFLAGS <!group> <account> <flags>`
  - Force-set flags bypassing normal permission checks (requires `admin_priv`).

## Access flag letters

- `F` — FOUNDER (implies management powers)
- `f` — FLAGS/MANAGE (edit members/flags)
- `A` (or `V`) — ACLVIEW (view group access list)
- `m` — MEMO
- `c` — CHANACCESS
- `v` — VHOST
- `s` — SET (change group settings)
- `i` — INVITE (invite accounts)
- `b` — BAN (blocks access)

## Data storage

Group data is stored as a flatfile in the Anope data directory (e.g. `data/groupserv.db`) and written atomically (via `.tmp` then rename).

Channel ↔ group association (and GROUPONLY state) are stored on the registered channel so they persist across restarts and module reloads.

## Memory usage notes

GroupServ persists long-lived state to disk (group data in `groupserv.db`, and channel association/GROUPONLY state on the registered channel), but it also keeps a working set in memory while services are running.

In memory it keeps:
- groups + access lists (loaded from `groupserv.db`)
- pending invites (transient; not intended to survive restart)
- pending DROP confirmation challenges (transient)

Keeping this in memory avoids doing slow on-demand disk/DB reads for common operations (including join-time enforcement).

To keep memory usage bounded over time:
- expired invites are purged
- DROP challenges expire automatically (TTL) and are purged
- JOINFLAGS are stored internally as parsed flags (not duplicated raw strings)
