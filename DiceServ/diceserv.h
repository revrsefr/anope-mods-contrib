#pragma once

#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include "module.h"

inline Anope::string ds_stringify(double x)
{
	std::ostringstream stream;
	if (!(stream << std::setprecision(std::numeric_limits<double>::max_digits10) << x))
		return Anope::ToString(x);
	return stream.str();
}

template<typename T>
inline Anope::string ds_stringify(const T &x)
{
	return Anope::ToString(x);
}

/** Enumeration of dice error codes */
enum DiceErrorCode
{
	DICE_ERROR_NONE,
	DICE_ERROR_PARSE,
	DICE_ERROR_DIV0,
	DICE_ERROR_UNDEFINED,
	DICE_ERROR_UNACCEPTABLE_DICE,
	DICE_ERROR_UNACCEPTABLE_SIDES,
	DICE_ERROR_UNACCEPTABLE_TIMES,
	DICE_ERROR_OVERUNDERFLOW,
	DICE_ERROR_STACK
};

/** Enumeration for OperatorResult to determine its type */
enum OperatorResultType
{
	OPERATOR_RESULT_TYPE_NONE,
	OPERATOR_RESULT_TYPE_DICE,
	OPERATOR_RESULT_TYPE_FUNCTION
};

/** Base class for results of operations */
class OperatorResultBase
{
protected:
	OperatorResultType type;

public:
	OperatorResultBase(OperatorResultType t) : type(t)
	{
	}

	virtual ~OperatorResultBase()
	{
	}

	const OperatorResultType &Type() const;

	virtual double Value() const = 0;
	virtual Anope::string LongString() const = 0;
	virtual Anope::string ShortString() const = 0;
	virtual OperatorResultBase *Clone() const = 0;
};

/** Version of OperatorResult that stores the result of a set of dice rolls */
class DiceResult : public OperatorResultBase
{
	int num;
	unsigned sides;
	std::vector<unsigned> results;

public:
	DiceResult(int n = 0, unsigned s = 0);

	void AddResult(unsigned result);
	const std::vector<unsigned> &Results() const;
	const unsigned &Sides() const;
	Anope::string DiceString() const;
	unsigned Sum() const;
	double Value() const;
	Anope::string LongString() const;
	Anope::string ShortString() const;
	DiceResult *Clone() const;
};

/** Version of OperatorResult that stores the results of a function */
class FunctionResult : public OperatorResultBase
{
	Anope::string name;
	std::vector<double> arguments;
	double result;

public:
	FunctionResult(const Anope::string &Name = "", double Result = 0.0);

	void SetNameAndResult(const Anope::string &Name, double Result);
	void AddArgument(double arg);
	double Value() const;
	Anope::string LongString() const;
	Anope::string ShortString() const;
	FunctionResult *Clone() const;
};

/** Container for the list of operator results */
class OperatorResults
{
	std::vector<OperatorResultBase *> results;

public:
	OperatorResults() : results()
	{
	}

	OperatorResults(const OperatorResults &other) : results()
	{
		this->append(other);
	}

	~OperatorResults()
	{
		this->clear();
	}

	OperatorResults &operator=(const OperatorResults &other);

	void clear()
	{
		for (unsigned y = 0, count = this->results.size(); y < count; ++y)
			delete this->results[y];
		this->results.clear();
	}

	void add(const DiceResult &result);
	void add(const FunctionResult &result);
	void append(const OperatorResults &other);
	bool empty() const;

	size_t size() const
	{
		return this->results.size();
	}

	OperatorResultBase *operator[](unsigned index)
	{
		if (index >= this->results.size())
			return NULL;
		return this->results[index];
	}

	const OperatorResultBase *operator[](unsigned index) const
	{
		if (index >= this->results.size())
			return NULL;
		return this->results[index];
	}
};

class DiceServData;

class DiceServService : public Service
{
public:
	DiceServService(Module *m) : Service(m, "DiceServService", "DiceServ")
	{
	}

	static const Anope::string &Author()
	{
		static Anope::string author = "Naram Qashat";
		return author;
	}

	static const Anope::string &Version()
	{
		static Anope::string version = "3.0.4";
		return version;
	}

	virtual void ErrorHandler(CommandSource &source, const DiceServData &data) = 0;
	virtual void Roller(DiceServData &data) = 0;
	virtual DiceResult *Dice(int num, unsigned sides) = 0;
	virtual void Ignore(Extensible *obj) = 0;
	virtual void Unignore(Extensible *obj) = 0;
	virtual bool IsIgnored(Extensible *obj) = 0;
};

class DiceServData
{
	ServiceReference<DiceServService> DiceServ;

public:
	bool isExtended, roundResults, sourceIsBot;
	Anope::string rollPrefix, dicePrefix, diceStr, timesPart, dicePart, diceSuffix, extraStr, chanStr, commentStr;
	int maxMessageLength;
	OperatorResults timesResults;
	std::vector<OperatorResults> opResults;
	std::vector<double> results;
	DiceErrorCode errCode;
	Anope::string errStr;
	unsigned errPos;
	int errNum;

	DiceServData() : DiceServ("DiceServService", "DiceServ"), isExtended(false), roundResults(true), sourceIsBot(false), rollPrefix(""), dicePrefix(""),
		diceStr(""), timesPart(""), dicePart(""), diceSuffix(""), extraStr(""), chanStr(""), commentStr(""), maxMessageLength(510), timesResults(), opResults(),
		results(), errCode(DICE_ERROR_NONE), errStr(""), errPos(0u), errNum(0)
	{
		if (!this->DiceServ)
			throw ModuleException("No interface for DiceServ");
	}

	void Reset();
	bool PreParse(CommandSource &source, const std::vector<Anope::string> &params, unsigned expectedChannelPos);
	bool CheckMessageLengthPreProcess(CommandSource &source);
	bool CheckMessageLengthPostProcess(CommandSource &source, const Anope::string &output) const;
	Anope::string GenerateLongExOutput() const;
	Anope::string GenerateShortExOutput() const;
	Anope::string GenerateNoExOutput() const;
	void StartNewOpResults();
	void AddToOpResults(const DiceResult &result);
	void AddToOpResults(const FunctionResult &result);
	void SetOpResultsAsTimesResults();
	void Roll();
	DiceResult *Dice(int num, unsigned sides);
	void HandleError(CommandSource &source);
	void SendReply(CommandSource &source, const Anope::string &output) const;
	bool HasExtended() const;
};

class DiceServDataHandlerService : public Service
{
public:
	DiceServDataHandlerService(Module *m) : Service(m, "DiceServDataHandlerService", "DiceServ")
	{
	}

	// DiceServData handlers

	virtual void Reset(DiceServData &data) = 0;
	virtual bool PreParse(DiceServData &data, CommandSource &source, const std::vector<Anope::string> &params, unsigned expectedChannelPos) = 0;
	virtual bool CheckMessageLengthPreProcess(DiceServData &data, CommandSource &source) = 0;
	virtual bool CheckMessageLengthPostProcess(const DiceServData &data, CommandSource &source, const Anope::string &output) const = 0;
	virtual Anope::string GenerateLongExOutput(const DiceServData &data) const = 0;
	virtual Anope::string GenerateShortExOutput(const DiceServData &data) const = 0;
	virtual Anope::string GenerateNoExOutput(const DiceServData &data) const = 0;
	virtual void StartNewOpResults(DiceServData &data) = 0;
	virtual void AddToOpResults(DiceServData &data, const DiceResult &result) = 0;
	virtual void AddToOpResults(DiceServData &data, const FunctionResult &result) = 0;
	virtual void SetOpResultsAsTimesResults(DiceServData &data) = 0;
	virtual void Roll(DiceServData &data) = 0;
	virtual DiceResult *Dice(DiceServData &data, int num, unsigned sides) = 0;
	virtual void HandleError(DiceServData &data, CommandSource &source) = 0;
	virtual void SendReply(const DiceServData &data, CommandSource &source, const Anope::string &output) const = 0;
	virtual bool HasExtended(const DiceServData &data) const = 0;

	// DiceResult handlers

	virtual const std::vector<unsigned> &Results(const DiceResult &result) const = 0;
	virtual const unsigned &Sides(const DiceResult &result) const = 0;
	virtual Anope::string DiceString(const DiceResult &result) const = 0;
	virtual unsigned Sum(const DiceResult &result) const = 0;
	virtual DiceResult *Clone(const DiceResult &result) const = 0;
};
