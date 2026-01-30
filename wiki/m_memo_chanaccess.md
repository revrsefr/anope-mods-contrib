# m_memo_chanaccess

Notify users when they’re added to a channel access list, or when founder/successor changes.
Supports MemoServ memos, email (Anope mail system), and online notices.
Also supports mask entries (e.g. `nick!ident@host` or `*!*@*`).

Config:

```conf
module {
	name = "m_memo_chanaccess"

	# MemoServ (memos)
	notify_access_add = yes
	notify_access_del = yes
	notify_founder_change = yes
	notify_successor_change = yes

	# Email (Anope mail system)
	email_access_add = no
	email_access_del = no
	email_founder_change = no
	email_successor_change = no

	# If a mask (non-account) is added, reply to the command source.
	notice_unregistered_access_add = no

	# If a mask (including *!*@*) is added and the channel currently exists,
	# notify matching online users in the channel.
	notice_mask_access_add = no

	# If the added account is currently online/identified, send them a notice too.
	notice_online_access_add = no

	# If "no", don’t notify when you change your own access/founder/successor.
	notify_self = no

	# Optional: force memo sender nick (otherwise uses WhoSends()/service/ChanServ).
	sender = "ChanServ"
}
```

Mail templates (add to anope.conf, inside the mail { } block):

```conf
mail
{
	# Enable HTML formatting for this module’s emails.
	content_type = "text/html; charset=UTF-8"

	# Templates used by m_memo_chanaccess
	chanaccess_access_subject = "Access update for {channel}"
	chanaccess_access_message = "<p>Hello {target},</p>

			<p>{actor} added you to the access list for <strong>{channel}</strong> (access: <strong>{access}</strong>).</p>

			<p><strong>Mask:</strong> <code>{mask}</code><br>
			<strong>Time:</strong> {timestamp}<br>
			<strong>Network:</strong> {network}</p>"

	chanaccess_access_del_subject = "Access removed for {channel}"
	chanaccess_access_del_message = "<p>Hello {target},</p>

			<p>{actor} removed you from the access list for <strong>{channel}</strong> (access: <strong>{access}</strong>).</p>

			<p><strong>Mask:</strong> <code>{mask}</code><br>
			<strong>Time:</strong> {timestamp}<br>
			<strong>Network:</strong> {network}</p>"

	chanaccess_founder_subject = "Founder change for {channel}"
	chanaccess_founder_message = "<p>Hello {target},</p>

			<p>{actor} has set you as founder of <strong>{channel}</strong>.</p>

			<p><strong>Time:</strong> {timestamp}<br>
			<strong>Network:</strong> {network}</p>"

	chanaccess_successor_subject = "Successor change for {channel}"
	chanaccess_successor_message = "<p>Hello {target},</p>

			<p>{actor} has set you as successor of <strong>{channel}</strong>.</p>

			<p><strong>Time:</strong> {timestamp}<br>
			<strong>Network:</strong> {network}</p>"
}
```

Available tokens for templates:

- {channel} channel name
- {actor} nickname of the actor
- {target} recipient account
- {access} access string (access add only)
- {network} network name
- {account} recipient account
- {mask} access mask (access add only)
- {timestamp} current timestamp
