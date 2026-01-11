/* GameServ: Name generator
 *
 * (C) 2019 Michael Hazell
 * Contact me at michaelhazell@hotmail.com
 *
 * You may use this module under the terms of docs/LICENSE, in the Anope source directory
 *
 * This is a port of the following code, but modified to use relevant objects and
 * functions:
 * https://github.com/atheme/atheme/blob/release/7.2/modules/gameserv/namegen_tab.h
 * https://github.com/atheme/atheme/blob/release/7.2/modules/gameserv/namegen.c
 */

#include "module.h"

Anope::string begin_sym[] = {"Del", "Ros", "Per", "Cyn", "Flar", "Ba", "Lab", "Az", "Ob", "Al", "Pas", "Nun", "Ur", "Ter", "Jar", "Est", "Car", "Houl", "In", "Kop", "Shot", "Vast", "Ael", "Skal", "Geld", "Er", "Sald", "Sar", "Binthe", "Haur", "Syn", "Kren", "Verd", "Pav", "Mart", "Yal", "Kor", "Lar", "Mel", "Claune", "For", "Rusk", "Pan", "Dir", "Chies", "Kil", "Odas", "Toc", "Plag", "Uld", "Yar", "Win", "Ry", "Harg", "Ty", "Druc", "Vald", "Keld", "Ar", "Das", "Klab", "Wearn", "Pes", "Art", "Na", "Harab", "Cal", "Ha", "Nev", "Wy", "Las", "Jes", "Kir", "Oc", "Brow", "Nist", "Var", "Lor", "Naut", "Hab", "Ard", "Vard", "Nor", "Tes", "Rol", "Ge", "Daf", "Tral", "Scaun", "Kol", "Star", "Ald", "Nap", "Loc", "Cor", "Chy", "Hy", "Har", "Char", "Bay", "Rald", "Yas", "Grave", "Dras", "Sep", "Shy", "Eth", "El", "Crac", "Hob", "Zol", "Bel", "Bar", "Zald", "Hal", "Ryle", "Obr", "Dain", "Jast", "Mold", "Bae", "As", "Ki", "Val", "Par", "Ash", "Ost", "Haith", "I", "Ca", "Vad", "Ro", "Dez", "Kuv", "Vab", "Dar", "Ral", "Mar", "Quarne", "Pulg", "Hor", "Ka", "Quer", "And", "Ny", "Ym", "Wal", "Tarl", "Vart"};
const size_t BEGIN_COUNT = 149;

Anope::string medial_sym[] = {"cath", "yd", "rik", "nal", "y", "ro", "stead", "carn", "tak", "chean", "ain", "ak", "loc", "ras", "ien", "cuth", "alen", "is", "carad", "acy", "la", "asc", "arn", "ayn", "sene", "art", "lyn", "cam", "mel", "ly", "eas", "il", "oller", "bin", "a", "astar", "tuny", "iera", "aldy", "ene", "in", "yne", "id", "aur", "era", "fyn", "om", "sume", "ir", "que", "warty", "tan", "warne", "tar", "lene", "ron", "valer", "war", "ant", "iam", "ew", "am", "ime", "varn", "orn", "quent", "geth", "aver", "car", "lin", "luth", "as", "gant", "gine", "carth", "ayne", "asty", "arik", "lir", "on", "codd", "az", "tyne", "wat", "erry", "aryne", "ert", "quin", "rath", "ame", "tin", "ter", "quine", "arth", "arne", "eth", "zane", "gar", "an", "yn", "gayn", "ax", "ast", "azer", "ont", "or", "ian", "gur", "et", "er", "im", "dar", "end", "doth", "en", "aller", "reth", "juin", "arny", "un", "len", "ere", "ane", "aryn", "dy", "gary", "arry", "arty", "ydd", "ris", "ar", "ance", "hen", "el", "air"};
const size_t MEDIAL_COUNT = 135;

Anope::string end_sym[] = {"cath", "carth", "ayne", "yd", "arik", "asty", "rik", "lir", "dor", "nal", "on", "codd", "y", "ber", "ro", "stead", "ta", "tyne", "wat", "carn", "erry", "chean", "ain", "aryne", "al", "ak", "ert", "ras", "ien", "alen", "is", "acy", "la", "ame", "ter", "arn", "quine", "ayn", "sene", "art", "arth", "lyn", "mel", "arne", "ern", "eth", "ly", "zane", "se", "gar", "eas", "ty", "an", "il", "yn", "oller", "gayn", "a", "bin", "iel", "azer", "astar", "tuny", "ont", "iera", "aldy", "ene", "ian", "or", "in", "sa", "yne", "era", "er", "fyn", "im", "dar", "sume", "doth", "en", "aller", "ir", "reth", "que", "arny", "ney", "warty", "un", "juin", "nik", "warne", "ere", "yr", "lene", "ane", "valer", "war", "ant", "iam", "ew", "dy", "aryn", "gary", "ime", "varn", "orn", "tha", "arty", "quent", "arry", "aver", "geth", "car", "lin", "ydd", "ris", "nyd", "ance", "luth", "ar", "dal", "gant", "el", "tyn", "air", "gine"};
const size_t END_COUNT = 126;

class CommandGSNamegen : public Command
{
 public:
	CommandGSNamegen(Module* creator, const Anope::string& sname = "gameserv/namegen") : Command(creator, sname, 2, 3)
	{
		this->SetDesc(_("Generate random name(s)"));
		this->SetSyntax(_("\037#channel\037 \037number_of_names\037"));
		this->AllowUnregistered(true);
	}

	void Execute(CommandSource& source, const std::vector<Anope::string>& params) override
	{
		const Anope::string& chan = params[0];
		ChannelInfo* ci = ChannelInfo::Find(chan);
		BotInfo* gs = Config->GetClient("GameServ");

		if (!ci)
		{
			source.Reply(CHAN_X_NOT_REGISTERED, chan.c_str());
			return;
		}

		if (!ci->bi)
		{
			source.Reply(BOT_NOT_ASSIGNED);
			return;
		}

		if (!ci->c || !ci->c->FindUser(ci->bi))
		{
			source.Reply(BOT_NOT_ON_CHANNEL, ci->name.c_str());
			return;
		}

		if (!gs)
		{
			source.Reply("GameServ client not found");
			return;
		}

		// In fantasy with prepend_channel=true the channel is already included.
		// Users commonly type the channel again (e.g. "!NAMEGEN #chan 2"), so accept it.
		Anope::string amount_str = params.back();
		char* endptr = NULL;
		const unsigned long amount_ul = std::strtoul(amount_str.c_str(), &endptr, 10);
		const bool amount_valid = endptr != NULL && *endptr == '\0' && amount_ul > 0;
		if (!amount_valid)
		{
			IRCD->SendPrivmsg(gs, ci->name, "Please tell me how many names to generate (1-20). Try: !NAMEGEN 2");
			this->OnHelp(source, "");
			return;
		}
		size_t amount = static_cast<size_t>(amount_ul);

		// We do no more than 20 names
		if (amount > 20)
		{
			IRCD->SendPrivmsg(gs, ci->name, "Generating a maximum of \00220\002 names.");
			amount = 20;
		}

		Anope::string finished_names;
		for (size_t i = 0; i < amount; ++i)
		{
			Anope::string namebuf;
			size_t medial_iter;

			namebuf += begin_sym[rand() % BEGIN_COUNT];

			for (medial_iter = rand() % 3; medial_iter > 0; --medial_iter)
			{
				namebuf += medial_sym[rand() % MEDIAL_COUNT];
			}

			namebuf += end_sym[rand() % END_COUNT];

			// Add generated name to list of names
			finished_names += namebuf;

			// Add a comma if there will be more, otherwise a period
			finished_names += (i + 1 < amount ? ", " : ".");
		}

		// Deliver the response
		IRCD->SendPrivmsg(gs, ci->name, Anope::Format("Some names to ponder: %s", finished_names.c_str()));
	}

	bool OnHelp(CommandSource& source, const Anope::string& subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Generate random name(s)"));
		return true;
	}
};

class GSNamegen : public Module
{
	CommandGSNamegen commandgsnamegen;

 public:
	GSNamegen(const Anope::string& modname, const Anope::string& creator) : Module(modname, creator, THIRD),
		commandgsnamegen(this)
	{
		this->SetAuthor("Techman");
		this->SetVersion("0.1");

		if (!ModuleManager::FindModule("gameserv"))
		{
			throw ModuleException("This module requires the GameServ core module to be loaded in order to function.");
		}
	}
};

MODULE_INIT(GSNamegen)
