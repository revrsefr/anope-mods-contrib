/* ----------------------------------------------------------------------------
 * Name    : ds_earthdawn.cpp
 * Author  : Naram Qashat (CyberBotX)
 * ----------------------------------------------------------------------------
 * Description:
 *
 * The EARTHDAWN command of DiceServ. See diceserv.cpp for more information
 * about DiceServ, including version and license.
 * ----------------------------------------------------------------------------
 */

#include "diceserv.h"

static ServiceReference<DiceServDataHandlerService> DiceServDataHandler("DiceServDataHandlerService", "DiceServ");

/* Step table for the pen & paper RPG Earthdawn 1st Edition / Classic Edition / 2nd Edition
 * Retrieved from http://arkanabar.tripod.com/steps.html
step	dice		step	dice				step	dice				step	dice
1		d4-2		26		d20d10d8d6			51		2d20d12+2d10+2d8	76		3d20+4d10+3d8d6
2		d4-1		27		d20d10+2d8			52		2d20+2d10+2d8+2d6	77		3d20+4d10+4d8
3		d4			28		d20+2d10d8			53		2d20+2d10+3d8d6		78		3d20+5d10+3d8
4		d6			29		d20d12d10d8			54		2d20+3d10+2d8d6		79		3d20d12+4d10+3d8
5		d8			30		d20d10d8+2d6		55		2d20+3d10+3d8		80		4d20+3d10+3d8d4
6		d10			31		d20d10+2d8d6		56		2d20+4d10+2d8		81		4d20+3d10+3d8d6
7		d12			32		d20+2d10d8d6		57		2d20d12+3d10+2d8	82		4d20+3d10+4d8
8		2d6			33		d20+2d10+2d8		58		3d20+2d10+2d8d4		83		4d20+4d10+3d8
9		d8d6		34		d20+3d10d8			59		3d20+2d10+2d8d6		84		4d20d12+3d10+3d8
10		d10d6		35		d20d12+2d10d8		60		3d20+2d10+3d8		85		4d20+3d10+3d8+2d6
11		d10d8		36		2d20d10d8d4			61		3d20+3d10+2d8		86		4d20+3d10+4d8d6
12		2d10		37		2d20d10d8d6			62		3d20d12+2d10+2d8	87		4d20+4d10+3d8d6
13		d12d10		38		2d20d10+2d8			63		3d20+2d10+2d8+2d6	88		4d20+4d10+4d8
14		d20d4		39		2d20+2d10d8			64		3d20+2d10+3d8d6		89		4d20+5d10+3d8
15		d20d6		40		2d20d12d10d8		65		3d20+3d10+2d8d6		90		4d20d12+4d10+3d8
16		d20d8		41		2d20d10d8+2d6		66		3d20+3d10+3d8		91		4d20+4d10+4d8d4
17		d20d10		42		2d20d10+2d8d6		67		3d20+4d10+2d8		92		4d20+4d10+4d8d6
18		d20d12		43		2d20+2d10d8d6		68		3d20d12+3d10+2d8	93		4d20+4d10+5d8
19		d20+2d6		44		2d20+2d10+2d8		69		3d20+3d10+3d8d4		94		4d20+5d10+4d8
20		d20d8d6		45		2d20+3d10d8			70		3d20+3d10+3d8d6		95		4d20d12+4d10+4d8
21		d20d10d6	46		2d20d12+2d10d8		71		3d20+3d10+4d8		96		4d20+4d10+4d8+2d6
22		d20d10d8	47		2d20+2d10+2d8d4		72		3d20+4d10+3d8		97		4d20+4d10+5d8d6
23		d20+2d10	48		2d20+2d10+2d8d6		73		3d20d12+3d10+3d8	98		4d20+5d10+4d8d6
24		d20d12d10	49		2d20+2d10+3d8		74		3d20+3d10+3d8+2d6	99		4d20+5d10+5d8
25		d20d10d8d4	50		2d20+3d10+2d8		75		3d20+3d10+4d8d6		100		4d20+6d10+4d8
 */
static Anope::string EarthdawnStepTable[] =
{
	"", /* 0, not used */
	"1d4-2", "1d4-1", "1d4", "1d6", "1d8", /* 1-5 */
	"1d10", "1d12", "2d6", "1d8+1d6", "1d10+1d6", /* 6-10 */
	"1d10+1d8", "2d10", "1d12+1d10", "1d20+1d4", "1d20+1d6", /* 11-15 */
	"1d20+1d8", "1d20+1d10", "1d20+1d12", "1d20+2d6", "1d20+1d8+1d6", /* 16-20 */
	"1d20+1d10+1d6", "1d20+1d10+1d8", "1d20+2d10", "1d20+1d12+1d10", "1d20+1d10+1d8+1d4", /* 21-25 */
	"1d20+1d10+1d8+1d6", "1d20+1d10+2d8", "1d20+2d10+1d8", "1d20+1d12+1d10+1d8", "1d20+1d10+1d8+2d6", /* 26-30 */
	"1d20+1d10+2d8+1d6", "1d20+2d10+1d8+1d6", "1d20+2d10+2d8", "1d20+3d10+1d8", "1d20+1d12+2d10+1d8", /* 31-35 */
	"2d20+1d10+1d8+1d4", "2d20+1d10+1d8+1d6", "2d20+1d10+2d8", "2d20+2d10+1d8", "2d20+1d12+1d10+1d8", /* 36-40 */
	"2d20+1d10+1d8+2d6", "2d20+1d10+2d8+1d6", "2d20+2d10+1d8+1d6", "2d20+2d10+2d8", "2d20+3d10+1d8", /* 41-45 */
	"2d20+1d12+2d10+1d8", "2d20+2d10+2d8+1d4", "2d20+2d10+2d8+1d6", "2d20+2d10+3d8", "2d20+3d10+2d8", /* 46-50 */
	"2d20+1d12+2d10+2d8", "2d20+2d10+2d8+2d6", "2d20+2d10+3d8+1d6", "2d20+3d10+2d8+1d6", "2d20+3d10+3d8", /* 51-55 */
	"2d20+4d10+2d8", "2d20+1d12+3d10+2d8", "3d20+2d10+2d8+1d4", "3d20+2d10+2d8+1d6", "3d20+2d10+3d8", /* 56-60 */
	"3d20+3d10+2d8", "3d20+1d12+2d10+2d8", "3d20+2d10+2d8+2d6", "3d20+2d10+3d8+1d6", "3d20+3d10+2d8+1d6", /* 61-65 */
	"3d20+3d10+3d8", "3d20+4d10+2d8", "3d20+1d12+3d10+2d8", "3d20+3d10+3d8+1d4", "3d20+3d10+3d8+1d6", /* 66-70 */
	"3d20+3d10+4d8", "3d20+4d10+3d8", "3d20+1d12+3d10+3d8", "3d20+3d10+3d8+2d6", "3d20+3d10+4d8+1d6", /* 71-75 */
	"3d20+4d10+3d8+1d6", "3d20+4d10+4d8", "3d20+5d10+3d8", "3d20+1d12+4d10+3d8", "4d20+3d10+3d8+1d4", /* 76-80 */
	"4d20+3d10+3d8+1d6", "4d20+3d10+4d8", "4d20+4d10+3d8", "4d20+1d12+3d10+3d8", "4d20+3d10+3d8+2d6", /* 81-85 */
	"4d20+3d10+4d8+1d6", "4d20+4d10+3d8+1d6", "4d20+4d10+4d8", "4d20+5d10+3d8", "4d20+1d12+4d10+3d8", /* 86-90 */
	"4d20+4d10+4d8+1d4", "4d20+4d10+4d8+1d6", "4d20+4d10+5d8", "4d20+5d10+4d8", "4d20+1d12+4d10+4d8", /* 91-95 */
	"4d20+4d10+4d8+2d6", "4d20+4d10+5d8+1d6", "4d20+5d10+4d8+1d6", "4d20+5d10+5d8", "4d20+6d10+4d8"/* 96-100 */
};

/** EARTHDAWN command
 *
 * Handles dice rolls for the pen & paper RPG Earthdawn.
 */
class DSEarthdawnCommand : public Command
{
public:
	DSEarthdawnCommand(Module *creator) : Command(creator, "diceserv/earthdawn", 1, 3)
	{
		this->AllowUnregistered(true);
		this->RequireUser(true);
		this->SetDesc(_("Rolls dice for Earthdawn"));
		this->SetSyntax(_("\037step\037[+\037karma\037] [[\037channel\037] \037comment\037]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		DiceServData data;
		data.isExtended = true;
		data.rollPrefix = "Earthdawn roll";
		data.diceSuffix = ")";

		if (!DiceServDataHandler->PreParse(data, source, params, 1))
			return;

		if (!data.timesPart.empty())
		{
			source.Reply(_("\037step\037 for an Earthdawn roll must be a number."));
			return;
		}
		else
		{
			Anope::string stepStr;
			size_t plus = data.dicePart.find('+');
			if (plus != Anope::string::npos)
			{
				stepStr = data.dicePart.substr(0, plus);
				data.dicePart = data.dicePart.substr(plus);
				int karma = Anope::Convert<int>(data.dicePart.substr(1), 0);
				Anope::string tmp = ds_stringify(karma);
				if (data.dicePart.substr(1) != tmp)
				{
					source.Reply(_("\037karma\037 for an Earthdawn roll must be a number."));
					return;
				}
				if (karma < 0)
				{
					source.Reply(_("The karma you entered (\037%d\037) was out of range, it must be\nat least 0 or higher."), karma);
					return;
				}
			}
			else
			{
				stepStr = data.dicePart;
				data.dicePart = "";
			}
			int step = Anope::Convert<int>(stepStr, 0);
			Anope::string tmp = ds_stringify(step);
			if (stepStr != tmp)
			{
				source.Reply(_("\037step\037 for an Earthdawn roll must be a number."));
				return;
			}
			if (step < 1 || step > 100)
			{
				source.Reply(_("The step you entered (\037%d\037) was out of range, it must be\nbetween 1 and 100."), step);
				return;
			}
			if (data.dicePart.empty())
				data.diceStr = data.dicePart = EarthdawnStepTable[step];
			else
				data.diceStr = data.dicePart = std::string("(") + EarthdawnStepTable[step] + ")" + data.dicePart;
			data.dicePrefix = "Step " + ds_stringify(step) + " (";
		}

		if (!DiceServDataHandler->CheckMessageLengthPreProcess(data, source))
			return;

		DiceServDataHandler->Roll(data);

		if (data.errCode != DICE_ERROR_NONE)
		{
			DiceServDataHandler->HandleError(data, source);
			return;
		}
		Anope::string output = DiceServDataHandler->GenerateLongExOutput(data);

		size_t lastPos = 0;
		for (size_t i = 0, len = data.opResults[0].size(); i < len; ++i)
		{
			const DiceResult *result = static_cast<const DiceResult *>(data.opResults[0][i]);
			Anope::string diceStr = DiceServDataHandler->DiceString(*result);

			size_t diceResult = output.find(diceStr + "=(", lastPos);

			std::vector<unsigned>::const_iterator begin = DiceServDataHandler->Results(*result).begin(), end = DiceServDataHandler->Results(*result).end(),
				found;
			size_t tmpPos = diceResult + diceStr.length() + 2;
			while ((found = std::find(begin, end, DiceServDataHandler->Sides(*result))) != end)
			{
				std::vector<unsigned> bonuses;

				Anope::string sidesStr = ds_stringify(DiceServDataHandler->Sides(*result));
				size_t resultPos = output.find(sidesStr, tmpPos);

				DiceResult *bonusResult = NULL;
				do
				{
					if (bonusResult)
						delete bonusResult;
					bonusResult = DiceServDataHandler->Dice(data, 1, DiceServDataHandler->Sides(*result));
					bonuses.push_back(DiceServDataHandler->Sum(*bonusResult));
				} while (DiceServDataHandler->Sum(*bonusResult) == DiceServDataHandler->Sides(*result));
				delete bonusResult;

				std::ostringstream bonusStream;
				bonusStream << "Bonus[";

				bool first = true;
				for (size_t j = 0, len2 = bonuses.size(); j < len2; ++j)
				{
					if (!first)
						bonusStream << " ";
					bonusStream << ds_stringify(bonuses[j]);
					data.results[0] += bonuses[j];
					first = false;
				}

				bonusStream << "]";

				Anope::string bonusStr = bonusStream.str();

				output = output.substr(0, resultPos + sidesStr.length()) + " " + bonusStr + output.substr(resultPos + sidesStr.length());

				tmpPos = resultPos + sidesStr.length() + bonusStr.length() + 2;

				begin = found + 1;
			}

			lastPos = diceResult + diceStr.length() + 2;
		}

		size_t beforeResult = output.find("} "), afterResult = output.find(">", beforeResult);
		output = output.substr(0, beforeResult + 2) + ds_stringify(data.results[0]) + output.substr(afterResult);

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
		source.Reply(_("This command performs the rolls needed for Earthdawn.\n"
			"Earthdawn's rolling system works on the concept of a step\n"
			"table, with different rolls depending on the given step.\n"
			"Step must be an integer value and must be between 1 and 100.\n"
			"Karma is an optional modifier, and if given, must come\n"
			"right after the step and have a plus between step and karma.\n"
			"The syntax for channel and comment is the same as with the\n"
			"ROLL command (see \002%s%s HELP ROLL\002 for more\n"
			"information on how to use this and ROLL).\n"
			" \n"
			"NOTE: Unlike the ROLL and EXROLL commands, EARTHDAWN does\n"
			"not allow you to use the ~ to specify multiple throws.\n"
			" \n"), "/msg ", source.service->nick.c_str());
		const Anope::string &fantasycharacters = Config->GetModule("fantasy").Get<const Anope::string>("fantasycharacter", "!");
		if (!fantasycharacters.empty())
			source.Reply(_("Additionally, if fantasy is enabled, this command can be triggered by using:\n"
				" \n"
				"!earthdawn \037step\037[+\037karma\037] [\037comment\037]\n"
				" \n"
				"where ! is one of the following characters: %s\n"
				" \n"), fantasycharacters.c_str());
		source.Reply(_("Examples:\n"
			"  %s%s EARTHDAWN 5\n"
			"    Same as %s%s EXROLL 1d8\n"
			"  %s%s EARTHDAWN 100+6\n"
			"    Same as %s%s EXROLL (4d20+6d10+4d8)+6"), "/msg ", source.service->nick.c_str(), "/msg ",
			source.service->nick.c_str(), "/msg ", source.service->nick.c_str(), "/msg ",
		source.service->nick.c_str());
		return true;
	}
};

class DSEarthdawn : public Module
{
	DSEarthdawnCommand cmd;

public:
	DSEarthdawn(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, THIRD), cmd(this)
	{
		this->SetAuthor(DiceServService::Author());
		this->SetVersion(DiceServService::Version());

		if (!DiceServDataHandler)
			throw ModuleException("No interface for DiceServ's data handler");
	}
};

MODULE_INIT(DSEarthdawn)
