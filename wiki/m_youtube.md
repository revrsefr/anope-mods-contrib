# m_youtube

BotServ command that detects YouTube links and replies with metadata (title/duration/views).

Extra deps:
- libcurl
- rapidjson

Important note:

- Configuration in modules.conf:

module
{
	name = "m_youtube"

	/* YouTube Data API v3 key (preferred name). */
	youtube_api_key = "API-KEY"

	/* Also accepted as an alias. */
	#api_key = "API-KEY"

	/* Optional: customize output (use IRC color codes if desired). */
	prefix = "\x02\x0301,00You\x0300,04Tube\x0F\x02"
	duration_text = "Durée : "
	seen_text = "Vues : "
	times_text = " fois."

	/* Optional: full template (overrides the *_text settings).
	 * Available tokens: {title}, {duration}, {views}
	 */
	#response_format = "{title} — Durée : {duration} — Vues : {views} fois."
}