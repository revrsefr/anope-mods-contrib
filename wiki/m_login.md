# m_login

Adds a `NickServ LOGIN` command.

## What it does

`NS LOGIN <nickname> <password>` combines “recover/switch nick” and “identify” behavior:

- If the nick is held/offline, it will SVSNICK you to it (when supported) and identify you.
- If the nick is in use, it will recover/collide it and force you onto it.
- With `restoreonrecover` enabled, it restores channel joins and your channel status modes from the recovered user (similar to `ns_recover`).

## Config

In `modules.conf`:

```conf
module
{
    name = "m_login"

    # When recovering a nick in use, restore channels + status modes.
    restoreonrecover = yes
}

command { service = "NickServ"; name = "LOGIN"; command = "nickserv/login"; }
```

Reload: `/msg OperServ REHASH`.
