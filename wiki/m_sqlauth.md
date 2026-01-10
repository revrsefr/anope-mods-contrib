# m_sqlauth

Authenticate users against an SQL database (and auto-create/update Anope accounts).

Config keys (from code):
- `engine` (required) — SQL engine name (e.g. `mysql/main`)
- `password_field` (default: `password`)
- `email_field` (default: `email`)
- `username_field` (default: `username`)
- `table_name` (default: `users`)
- `query` (optional) — custom SQL query
- `disable_reason` (optional) — message shown on `NickServ REGISTER` / `GROUP`
- `disable_email_reason` (optional) — message shown on `NickServ SET EMAIL`
- `kill_message` (default: `Error: Too many failed login attempts. Please try again later. ID:`)
- `max_attempts` (default: `5`)

Config (recommended explicit `query`):

```conf
module {
	name = "m_sqlauth"
	engine = "mysql/main"

	# Custom query (the module substitutes @a@ with the account name).
	query = "SELECT `password`, `email` FROM `users` WHERE `username` = @a@"

	# Optional: block local register/email commands and redirect users elsewhere.
	disable_reason = "To register a new account navigate to https://example.net/register/"
	disable_email_reason = "To change your email navigate to https://example.net/profile/"

	kill_message = "Error: Too many failed login attempts. Please try again later. ID:"
	max_attempts = 5
}
```
