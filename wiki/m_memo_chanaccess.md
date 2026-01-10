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
	notify_founder_change = yes
	notify_successor_change = yes

	# Email (Anope mail system)
	email_access_add = no
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
