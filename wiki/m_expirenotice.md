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
