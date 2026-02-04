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

## Permissions

These are **GroupServ access flags** (not IRC user modes):

- `+f` - Enables modification of group access list.
- `+F` - Grants full founder access.
- `+A` - Enables viewing of group access list.
- `+m` - Read memos sent to the group.
- `+c` - Have channel access in channels where the group has sufficient privileges.
- `+v` - Take vhosts offered to the group through HostServ.
- `+s` - Ability to use GroupServ SET commands on the group.
- `+b` - Ban a user from the group. The user will not be able to join the group with the JOIN command and it will not show up in their NickServ INFO or anywhere else. NOTE that setting this flag will NOT automatically remove the users' privileges (if applicable).
- `+i` - Grants the ability to invite users to the group.

It is **not** an IRC user mode. If you try `/mode Nick +A`, InspIRCd will reject it.

## Configuration

See the shipped example config:
- `GroupServ/groupserv.example.conf`

Key module options:
- `client` — the service nick to use (e.g. `GroupServ`)
- `reply_method` — `notice` (default) or `privmsg`
- `admin_priv` — privilege required for forced admin commands (`FDROP`, `FFLAGS`)
- `auspex_priv` — privilege required to view/list any group
- `exceed_priv` — privilege required to bypass limits
- `opers_only` — only IRC operators can register groups
- `maxgroups` — max groups a single account may found (create)
- `maxgroupacs` — max access list size per group (`0 = unlimited`)
- `enable_open_groups` — allow `SET OPEN ON`
- `default_joinflags` — flags granted when a user joins an open group
- `vhostauto_default` — default for new groups: `yes` auto-approves vhosts, `no` requires approval
- `save_interval` — autosave interval in seconds (`0` disables periodic autosave)

### `default_joinflags` format

These are **GroupServ access flags**, not channel modes.

Accepted formats:
- space-separated: `+A +i +s`
- long names: `+ACLVIEW +INVITE +SET`
- compact (Atheme-style): `+Ais` or `+fAsivb`

Notes:
- Single-letter flags are case-sensitive (Atheme-style). Use the letters shown in the Permissions section.
- Long names are case-insensitive (e.g. `+invite` works).

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

### Channel access inheritance (+c)

GroupServ can optionally let group members **inherit ChanServ privileges** from a channel access entry that targets the group.

How it works:

- Add a ChanServ access entry for the group on the channel (mask must be exactly the group name, e.g. `!devs`).
- Give a group member the GroupServ flag `+c` (CHANACCESS).
- When ChanServ checks whether the user has a privilege on that channel, GroupServ will allow it **if the privilege is granted by a `!group` entry**.

Example setup:

- Give the group access on the channel:
  - `/msg ChanServ FLAGS #dev !devs +o` (example)
  - or `/msg ChanServ ACCESS #dev ADD !devs <level>`
- Let a member inherit it:
  - `/msg GroupServ FLAGS !devs Alice +c`

Notes:

- `+c` does not auto-op people by itself; it just makes ChanServ privilege checks succeed when the channel’s access list grants the group those privileges.
- This only applies for accounts which are members of the group *and* have GroupServ flag `+c` in that group.

### Group vhosts (+v)

GroupServ supports a simple “group vhost” workflow:

- Founders / SET users can store a vhost on the group with `SET VHOST`.
- Members who have GroupServ flag `+v` (VHOST) can then activate that vhost on their own NickServ account.

Commands:

- `SET <!group> VHOST <hostmask>`
  - Stores the vhost to use for this group.
  - Examples:
    - `/msg GroupServ SET !devs VHOST network.gp.devs`
    - `/msg GroupServ SET !devs VHOST myident@network.gp.devs`
  - Clear it: `/msg GroupServ SET !devs VHOST OFF`

- `VHOST <!group> [OFF]`
  - Activates the group’s stored vhost for your account (requires group flag `+v`).
  - If the group has `VHOSTAUTO` enabled it applies immediately; otherwise it submits a HostServ request.
  - Remove your vhost: `/msg GroupServ VHOST !devs OFF`

- `SET <!group> VHOSTAUTO <ON|OFF>`
  - Enables or disables automatic approval of VHOST requests for this group.
  - Example: `/msg GroupServ SET !devs VHOSTAUTO ON`

### Public

- `HELP`
  - Shows help for GroupServ commands.

- `LISTCHANS <!group>`
  - Lists registered channels explicitly associated with the group via `ChanServ SET #channel GROUP`.
  - Example: `/msg GroupServ LISTCHANS !devs`

- `ACCESS <!group> #channel [priv]`
  - Shows (and optionally diagnoses) ChanServ access entries on `#channel` which target `!group`.
  - With `priv`, it will also say whether the `!group` entry grants that privilege and whether you effectively have it (from all sources).
  - Examples:
    - `/msg GroupServ ACCESS !devs #dev`
    - `/msg GroupServ ACCESS !devs #dev INVITE`

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

### Group memos (MemoServ integration)

GroupServ supports **shared group memos** stored in `groupserv.db`.

These are exposed as **MemoServ commands** (requires MemoServ to be loaded and command blocks added):

- `GSEND <!group> <memo-text>`
  - Sends a memo to the group (requires GroupServ access flag `m`).

- `GLIST <!group>`
  - Lists memos for the group (requires GroupServ access flag `m`).

- `GREAD <!group> <number>`
  - Reads a memo by number (requires GroupServ access flag `m`).
  - Also accepts `#<number>` (e.g. `#1`).

- `GDEL <!group> <number>`
  - Deletes a memo by number (requires GroupServ access flag `s`/SET).
  - Also accepts `#<number>` (e.g. `#1`).

### Membership / invites

- `INVITE <!group> <account>`
  - Invites a NickServ account to a group.
  - Requires you have the group access flag `i` (INVITE) in that group.
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
    - `/msg GroupServ FLAGS !staff Alice +iA`
      - Meaning: give Alice INVITE (`i`) and ACLVIEW (`A`).
    - `/msg GroupServ FLAGS !staff Alice -i`
      - Meaning: remove INVITE (`i`) from Alice.
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
    - `/msg GroupServ SET !helpers JOINFLAGS +A`
      - Meaning: new joiners can view the group member list.
    - `/msg GroupServ SET !helpers JOINFLAGS +Ai`
      - Meaning: new joiners can view and invite.
    - `/msg GroupServ SET !helpers JOINFLAGS OFF`
      - Meaning: clear group-specific join flags (falls back to `default_joinflags`).

- `SET <!group> VHOST <hostmask|OFF>`
  - Sets or clears the stored group vhost.
  - Note: this does not automatically apply to members; they must run `VHOST <!group>`.

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

Invites and DROP challenges are intentionally not persisted: they are short-lived workflow state, and dropping them on restart avoids stale/forgotten tokens lingering indefinitely.

Keeping this in memory avoids doing slow on-demand disk/DB reads for common operations (including join-time enforcement).

To keep memory usage bounded over time:
- expired invites are purged
- DROP challenges expire automatically (TTL) and are purged
- JOINFLAGS are stored internally as parsed flags (not duplicated raw strings)
