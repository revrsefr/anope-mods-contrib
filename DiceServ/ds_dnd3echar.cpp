/* ----------------------------------------------------------------------------
 * Name    : ds_dnd3echar.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The DND3ECHAR command of DiceServ. See diceserv.cpp for more information
 * about DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include <algorithm>
#include "diceserv.h"

static ServiceReference<DiceServDataHandlerService> DiceServDataHandler("DiceServDataHandlerService", "DiceServ");

/** DND3ECHAR command
 *
 * Handles the dice rolls that make up character creation in Dungeons and Dragons 3rd Edition.
 */
class DSDnD3eCharCommand : public Command
{
	/** Find the lowest result out of the 4 6-sided dice thrown for a Dungeons and Dragons 3rd Edition character.
	 * @param result The set of results to access
	 * @param min Reference to an integer storing the lowest result
	 * @return The index of the lowest result
	 */
	static unsigned GetMinDnD(const DiceResult &result, unsigned &min)
	{
		std::vector<unsigned>::const_iterator begin = DiceServDataHandler->Results(result).begin(),
			minElement = std::min_element(begin, DiceServDataHandler->Results(result).end());
		min = *minElement;
		return minElement - begin;
	}

	/** Remove the minimum value from a result's total.
	 * @param data The data that contains the results to access
	 */
	static void DnDRollCorrect(DiceServData &data)
	{
		for (size_t i = 0; i < 6; ++i)
		{
			unsigned min;
			GetMinDnD(*static_cast<const DiceResult *>(data.opResults[i][0]), min);
			data.results[i] -= min;
		}
	}

	/** Determine the modifier of a given value for Dungeons and Dragons 3rd Edition.
	 * @param val The value to get the modifier of
	 * @return The modifier of the value
	 */
	static int DnDmod(double val)
	{
		return static_cast<int>((val - 10) / 2);
	}

	/** Calculate the sum of all the modifier values of all the rolls for Dungeons and Dragons 3rd Edition character creation.
	 * @param data The data that contains the results to access
	 * @return The sum of the modifiers
	 */
	static int DnDmodadd(const DiceServData &data)
	{
		std::vector<int> mods = std::vector<int>(6);
		std::transform(data.results.begin(), data.results.end(), mods.begin(), DnDmod);
		return std::accumulate(mods.begin(), mods.end(), 0);
	}

	/** Determine the highest roll of all the rolls for Dungeons and Dragons 3rd Edition character creation.
	 * @param data The data that contains the results to access
	 * @return The highest value
	 */
	static unsigned DnDmaxatt(const DiceServData &data)
	{
		return static_cast<unsigned>(*std::max_element(data.results.begin(), data.results.end()));
	}

public:
	DSDnD3eCharCommand(Module *creator) : Command(creator, "diceserv/dnd3echar", 0, 2)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("Rolls dice for D&D 3e character creation"));
		this->SetSyntax(_("[[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		std::vector<Anope::string> newParams = params;
		newParams.insert(newParams.begin() + (source.c ? 1 : 0), "6~4d6");

		DiceServData data;
		data.isExtended = true;
		data.rollPrefix = "D&D 3e Character roll";

		if (!DiceServDataHandler->PreParse(data, source, newParams, 1))
			return;
		if (!DiceServDataHandler->CheckMessageLengthPreProcess(data, source))
			return;

		bool cont;
		do
		{
			cont = false;

			DiceServDataHandler->Roll(data);

			if (data.errCode != DICE_ERROR_NONE)
			{
				DiceServDataHandler->HandleError(data, source);
				return;
			}

			DnDRollCorrect(data);

			if (DnDmodadd(data) <= 0 || DnDmaxatt(data) <= 13)
			{
				source.Reply(DnDmodadd(data) <= 0 ? _("D&D 3e Character roll resulted in a character that had their\n"
					"total modifiers be 0 or below, re-rolling stats again.") : _("D&D 3e Character roll resulted in a character that had a max\n"
					"score of 13 or less for all their abilities, re-rolling stats\n"
					"again."));
				cont = true;
				DiceServDataHandler->Reset(data);
			}
		} while (cont);

		Anope::string output = DiceServDataHandler->GenerateLongExOutput(data);

		size_t lastPos = 0;
		for (unsigned i = 0; i < 6; ++i)
		{
			size_t diceResult = output.find("4d6=(", lastPos);

			unsigned min, minPos = GetMinDnD(*static_cast<const DiceResult *>(data.opResults[i][0]), min);

			output = output.substr(0, diceResult + 5 + 2 * minPos) + static_cast<char>(22) + output.substr(diceResult + 5 + 2 * minPos, 1) +
				static_cast<char>(22) + output.substr(diceResult + 6 + 2 * minPos);

			lastPos = diceResult + 5;
		}

		if (!DiceServDataHandler->CheckMessageLengthPostProcess(data, source, output))
		{
			DiceServDataHandler->HandleError(data, source);
			return;
		}
		DiceServDataHandler->SendReply(data, source, output);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("This command is performs the rolls needs to create a D&D 3e\n"
			"character, which consists of 6 sets of 4d6, the lowest\n"
			"result of each set being discarded. The discarded die will\n"
			"be shown in reverse, so you can still see all 4 dice and\n"
			"which was removed. The syntax for channel and comment is the\n"
			"same as with the ROLL command (see \002%s%s HELP ROLL\002\n"
			"for more information on how to use this and ROLL).\n"
			" \n"), "/msg ", source.service->nick.c_str());
		const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
		if (!fantasycharacters.empty())
			source.Reply(_("Additionally, if fantasy is enabled, this command can be triggered by using:\n"
				" \n"
				"!dnd3echar [\037comment\037]\n"
				" \n"
				"where ! is one of the following characters: %s\n"
				" \n"), fantasycharacters.c_str());
		source.Reply(_("Example:\n"
			"  %s%s DND3ECHAR\n"
			"    {4d6=(\x16" "3\x16 5 5 6)}=16\n"
			"  (The above is basically 19 minus the lowest of 3)"), "/msg ", source.service->nick.c_str());
		return true;
	}
};

class DSDnD3eChar : public Module
{
	DSDnD3eCharCommand cmd;

public:
	DSDnD3eChar(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServDataHandler)
			throw ModuleException("No interface for DiceServ's data handler");
	}
};

MODULE_INIT(DSDnD3eChar)
