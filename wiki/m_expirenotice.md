# m_expirenotice

Send notices via email and/or MemoServ when nicks/channels are expiring soon or have expired.

Config (example from the module header):

```conf
module {
	name = "m_expirenotice"

	ns_notice_expiring = yes
	ns_notice_expired = yes
	ns_notice_time = 7d
	ns_notice_mail = yes
	ns_notice_memo = no

	cs_notice_expiring = yes
	cs_notice_expired = yes
	cs_notice_time = 3d
	cs_notice_mail = yes
	cs_notice_memo = no

	ns_expiring_subject = "Nickname expiring"
	ns_expiring_message = "Your nickname %n will expire %t.\n%N IRC Administration"
	ns_expiring_memo = "Your nickname %n will expire %t."

	ns_expired_subject = "Nickname expired"
	ns_expired_message = "Your nickname %n has expired.\n%N IRC Administration"
	ns_expired_memo = "Your nickname %n has expired."

	cs_expiring_subject = "Channel expiring"
	cs_expiring_message = "Your channel %c will expire %t.\n%N IRC Administration"
	cs_expiring_memo = "Your channel %c will expire %t."

	cs_expired_subject = "Channel expired"
	cs_expired_message = "Your channel %c has expired.\n%N IRC Administration"
	cs_expired_memo = "Your channel %c has expired."
}
```

Template variables:
- `%n` nickname
- `%c` channel
- `%t` expiry time
- `%N` network name

## HTML email formatting

This module sends whatever you put in `*_message` as the email body.

By default Anope sends plain text email. If you want HTML formatting (tables, colors, etc), set Anope mail to HTML:

```conf
mail {
	content_type = "text/html; charset=UTF-8"
}
```

Then you can use HTML in `ns_expiring_message`, `cs_expiring_message`, etc.

Note: make sure you send valid HTML. For example, `<td>...</td>` should be inside a `<table><tr>...</tr></table>`; a bare `<td>` can render inconsistently across email clients.

## Mail prerequisites / troubleshooting

Mail sending uses Anope's built-in mail system. For email notices to work you must have:

- `mail { usemail = yes }` in your main Anope config
- `mail { sendfrom = "..." }` set (required by Anope; if empty, Anope refuses to send)
- The NickCore(s) receiving mail must have a non-empty email set

If mail is enabled in this module (`ns_notice_mail` / `cs_notice_mail`) but Anope mail is not usable (e.g. `usemail` is off or `sendfrom` is missing), the module will automatically disable mail notices.

As of the latest update, failed sends are logged at debug level when `Mail::Send(...)` returns false.
