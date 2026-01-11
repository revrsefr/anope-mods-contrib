/* ----------------------------------------------------------------------------
 * Name    : ds_calc.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The CALC and EXCALC commands of DiceServ. See diceserv.cpp for more
 * information about DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

static ServiceReference<DiceServDataHandlerService> DiceServDataHandler("DiceServDataHandlerService", "DiceServ");

/** CALC command
 *
 * Handles regular dice rolls, sans rounding, resulting in more of a calculation.
 */
class DSCalcCommand : public Command
{
public:
	DSCalcCommand(Module *creator) : Command(creator, "diceserv/calc", 1, 3)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("ROLL without rounding, for calculations"));
		this->SetSyntax(_("\037dice\037 [[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		DiceServData data;
		data.roundResults = false;
		data.rollPrefix = "Calc";

		if (!DiceServDataHandler->PreParse(data, source, params, 1))
			return;
		if (!DiceServDataHandler->CheckMessageLengthPreProcess(data, source))
			return;

		DiceServDataHandler->Roll(data);

		if (data.errCode != DICE_ERROR_NONE)
		{
			DiceServDataHandler->HandleError(data, source);
			return;
		}
		Anope::string output = DiceServDataHandler->GenerateNoExOutput(data);
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
		source.Reply(_("This command is identical to ROLL (see \002%s%s\002\n"
			"\002HELP ROLL\002 for more information on how to use this and\n"
			"ROLL), except the results are not rounded off and are\n"
			"displayed as is."), "/msg ", source.service->nick.c_str());
		const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
		if (!fantasycharacters.empty())
			source.Reply(_(" \n"
				"Additionally, if fantasy is enabled, this command can be triggered by using:\n"
				" \n"
				"!calc \037dice\037 [\037comment\037]\n"
				" \n"
				"where ! is one of the following characters: %s"), fantasycharacters.c_str());
		return true;
	}
};

/** EXCALC command
 *
 * Handles dice rolls with extended output, sans rounding, resulting in more of a calculation.
 */
class DSExcalcCommand : public Command
{
public:
	DSExcalcCommand(Module *creator) : Command(creator, "diceserv/excalc", 1, 3)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("EXROLL without rounding, for calculations"));
		this->SetSyntax(_("\037dice\037 [[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		DiceServData data;
		data.roundResults = false;
		data.isExtended = true;
		data.rollPrefix = "Excalc";

		if (!DiceServDataHandler->PreParse(data, source, params, 1))
			return;
		if (!DiceServDataHandler->CheckMessageLengthPreProcess(data, source))
			return;

		DiceServDataHandler->Roll(data);

		if (data.errCode != DICE_ERROR_NONE)
		{
			DiceServDataHandler->HandleError(data, source);
			return;
		}
		Anope::string output;
		if (DiceServDataHandler->HasExtended(data))
		{
			output = DiceServDataHandler->GenerateLongExOutput(data);
			if (!DiceServDataHandler->CheckMessageLengthPostProcess(data, source, output))
			{
				output = DiceServDataHandler->GenerateShortExOutput(data);
				if (!DiceServDataHandler->CheckMessageLengthPostProcess(data, source, output))
				{
					data.rollPrefix = "Calc";
					output = DiceServDataHandler->GenerateNoExOutput(data);
					if (!DiceServDataHandler->CheckMessageLengthPostProcess(data, source, output))
					{
						DiceServDataHandler->HandleError(data, source);
						return;
					}
				}
			}
		}
		else
		{
			data.rollPrefix = "Calc";
			output = DiceServDataHandler->GenerateNoExOutput(data);
			if (!DiceServDataHandler->CheckMessageLengthPostProcess(data, source, output))
			{
				DiceServDataHandler->HandleError(data, source);
				return;
			}
		}
		DiceServDataHandler->SendReply(data, source, output);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("This command is identical to EXROLL (see \002/%s%s\002\n"
			"\002HELP EXROLL\002 for more information on how to use this and\n"
			"EXROLL), except the results are not rounded off and are\n"
			"displayed as is."), "msg ", source.service->nick.c_str());
		const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
		if (!fantasycharacters.empty())
			source.Reply(_(" \n"
				"Additionally, if fantasy is enabled, this command can be triggered by using:\n"
				" \n"
				"!excalc \037dice\037 [\037comment\037]\n"
				" \n"
				"where ! is one of the following characters: %s"), fantasycharacters.c_str());
		return true;
	}
};

class DSCalc : public Module
{
	DSCalcCommand calc_cmd;
	DSExcalcCommand excalc_cmd;

public:
	DSCalc(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), calc_cmd(this), excalc_cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServDataHandler)
			throw ModuleException("No interface for DiceServ's data handler");
	}
};

MODULE_INIT(DSCalc)
