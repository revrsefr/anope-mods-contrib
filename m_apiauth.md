# m_apiauth.cpp #
### 2024 Jean "reverse" Chevronnet ###
Module for Anope IRC Services v2.1, lets users authenticate with
credentials stored in an external API endpoint instead of the internal
Anope database.

# Configuration #
Add this configuration block in your conf/modules.conf file
```
    module
{
    name = "m_apiauth"
	  verify_ssl = "true" # false for testing
	  capath = "/etc/ssl/certs"  # Directory containing CA certificates
    cainfo = "/etc/ssl/certs/ca-certificates.crt"  # Path to CA bundle file
    api_url = "https://example.fr/accounts/irc/auth/"
    api_username_param = "username"
    api_password_param = "password"
    api_method = "POST"
    api_success_field = "success"
    api_email_field = "email"
    api_key = "vBl0Ycs1-kXEAUp4VOhPGMnDNA9vpUAjEnc9bQ9x7UxJhYtFgf"
    disable_reason = "To register, please visit: "
    disable_email_reason = "To update your email, please visit: "
	  profile_url = "https://example.fr/accounts/profile/%s/"
    register_url = "https://example.fr/accounts/register/"  
}
