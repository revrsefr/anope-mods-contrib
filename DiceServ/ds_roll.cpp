/* ----------------------------------------------------------------------------
 * Name    : ds_roll.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The ROLL and EXROLL commands of DiceServ. See diceserv.cpp for more
 * information about DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

static ServiceReference<DiceServDataHandlerService> DiceServDataHandler("DiceServDataHandlerService", "DiceServ");

/** ROLL command
 *
 * Handles regular dice rolls.
 */
class DSRollCommand : public Command
{
public:
	DSRollCommand(Module *creator) : Command(creator, "diceserv/roll", 1, 3)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("Rolls dice (or performs math too)"));
		this->SetSyntax(_("\037dice\037 [[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		DiceServData data;
		data.rollPrefix = "Roll";

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

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		if (subcommand.empty())
		{
			this->SendSyntax(source);
			source.Reply(" ");
			source.Reply(_("Echoes a dice roll to you. If you are using this command\n"
				"while using a registered nick, it will come to you using\n"
				"the method you tell Services to use. Otherwise, it will use\n"
				"the default that Services is set to. Roll will be displayed\n"
				"as follows:\n"
				" \n"
				"<Roll [\037dice\037]: \037result\037>\n"
				" \n"
				"\037Channel\037 is an optional argument, it must be a valid\n"
				"channel and one that you are currently in. If you give an\n"
				"invalid channel or one you are not in, the command will be\n"
				"halted. If given and is valid, dice roll will be echoed to\n"
				"the channel given as a NOTICE in the following form:\n"
				" \n"
				"<Roll for \037nick\037 [\037dice\037]: \037result\037>\n"
				" \n"
				"\037Comment\037 is also an optional argument. You do not need to\n"
				"give a channel to use a comment. If given, this comment will\n"
				"be added to the end of the result.\n"
				" "));
			const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
			if (!fantasycharacters.empty())
				source.Reply(_("Additionally, if fantasy is enabled, this command can be triggered by using:\n"
					" \n"
					"!roll \037dice\037 [\037comment\037]\n"
					" \n"
					"where ! is one of the following characters: %s\n"
					" "), fantasycharacters.c_str());
			source.Reply(_(" \n"
				"For help on the dice expression syntax, see \002%s%s\002\n"
				"\002HELP ROLL EXPRESSIONS\002.\n"
				" \n"
				"Examples:\n"
				"  Roll 3d6: %s%s ROLL 3d6\n"
				"  Roll 3d6+5: %s%s ROLL 3d6+5\n"
				"  Roll 3d6+5, then double end result:\n"
				"    %s%s ROLL (3d6+5)*2\n"
				"  Roll 3d6, double the result, then add 5:\n"
				"    %s%s ROLL 3d6*2+5\n"
				"  Roll 3d6 three consecutive times:\n"
				"    %s%s ROLL 3~3d6"), "/msg ", source.service->nick.c_str(), "/msg ",
				source.service->nick.c_str(), "/msg ", source.service->nick.c_str(), "/msg ",
				source.service->nick.c_str(), "/msg ", source.service->nick.c_str(), "/msg ",
				source.service->nick.c_str());
		}
		else if (subcommand.equals_ci("EXPRESSIONS"))
			source.Reply(_("\002" "Dice expression syntax\002\n"
				" \n"
				"\037dice\037 expression must be in the form of: [\037x\037~]\037y\037\n"
				" \n"
				"(Alternatively, you can use the expression \037x\037[\037y\037], with the\n"
				"[ and ] being actual characters in this case, and this will\n"
				"be treated as if you had written \037x\037~\037y\037.)\n"
				" \n"
				"x and y can support very complex forms of expressions. In\n"
				"order to get an actual dice roll, you must use something in\n"
				"the format of: [\037z\037]d\037w\037, where z is the number of dice to\n"
				"be thrown, and w is the number of sides on each die. z is\n"
				"optional, will default to 1 if not given. Please note that\n"
				"the sides or number of dice can not be 0 or negative, and\n"
				"both can not be greater than 99999.\n"
				" \n"
				"x~ is used to determine how many consecutive sets of dice\n"
				"will be rolled. This is optional, defaults to 1 if not\n"
				"given. If you use this form, you MUST include the ~ for it\n"
				"to be recognized as you wanting to throw a dice set multiple\n"
				"times.\n"
				" \n"
				"y is normally used for the standard dice roll. You could\n"
				"also do plain math within y, if you want. You must include\n"
				"something here, but if it's not a number, it will usually\n"
				"result in a parsing error.\n"
				" \n"
				"To roll what is called a \"percentile\" die, or a 100-sided\n"
				"die, you can use %% as your roll expression, or include d%%\n"
				"within your roll expression. For the former, the expression\n"
				"will be replaced with 1d100, whereas for the latter, the\n"
				"%% in the expression will be replaced with a 100. For all\n"
				"other cases, the %% will signify modulus of the numbers\n"
				"before and after it, the modulus being the remainder that\n"
				"you'd get from dividing the first number by the second\n"
				"number.\n"
				" \n"
				"The following math operators can be used in expressions:\n"
				" \n"
				"+ - * / ^ %% (in addition to 'd' for dice rolls and\n"
				"parentheses to force order of operations.)\n"
				" \n"
				"Also note that if you use decimals in your expressions, the\n"
				"result will be returned in integer form, unless you use CALC\n"
				"or EXCALC. An additional note, implicit multiplication with\n"
				"parentheses (example: 2(3d6)) will function as it should (as\n"
				"2*(3d6)).\n"
				" \n"
				"In addition to the above math operators, certain functions\n"
				"are also recognized. For a full list, see\n"
				"\002%s%s HELP FUNCTIONS\002. The following math constants are\n"
				"also recognized and will be filled in automatically:\n"
				" \n"
				"    e              Exponential growth constant\n"
				"                   2.7182818284590452354\n"
				"    pi             Archimedes' constant\n"
				"                   3.14159265358979323846\n"
				" \n"
				"The dice roller will also recognize if you want to have a\n"
				"negative number when prefixed with a -. This will not cause\n"
				"problems even though it is also used for subtraction."), "/msg ", source.service->nick.c_str());
		return true;
	}
};

/** EXROLL command
 *
 * Handles dice rolls with extended output.
 */
class DSExrollCommand : public Command
{
public:
	DSExrollCommand(Module *creator) : Command(creator, "diceserv/exroll", 1, 3)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("Rolls dice and shows each dice roll"));
		this->SetSyntax(_("\037dice\037 [[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		DiceServData data;
		data.isExtended = true;
		data.rollPrefix = "Exroll";

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
					data.rollPrefix = "Roll";
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
			data.rollPrefix = "Roll";
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
		source.Reply(_("This command is exactly like ROLL (see \002%s%s HELP\002\n"
			"\002ROLL\002 for more information on how to use this and ROLL),\n"
			"with one slight difference. EXROLL will show what was rolled\n"
			"on each die as it is rolled.\n"
			" \n"
			"Example: Roll a 4d6: {4d6=(6 3 1 4)}=14\n"
			" \n"
			"This can be useful if you want to know exactly what each die\n"
			"said when it was rolled."), "/msg ", source.service->nick.c_str());
		const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
		if (!fantasycharacters.empty())
			source.Reply(_("Additionally, if fantasy is enabled, this command can be triggered by using:\n"
				" \n"
				"!exroll \037dice\037 [\037comment\037]\n"
				" \n"
				"where ! is one of the following characters: %s\n"
				" "), fantasycharacters.c_str());
		source.Reply(_(" \n"
			"Syntax of the dice is exactly the same as the syntax of\n"
			"ROLL."));
		return true;
	}
};

class DSRoll : public Module
{
	DSRollCommand roll_cmd;
	DSExrollCommand exroll_cmd;

public:
	DSRoll(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), roll_cmd(this), exroll_cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServDataHandler)
			throw ModuleException("No interface for DiceServ's data handler");
	}
};

MODULE_INIT(DSRoll)
