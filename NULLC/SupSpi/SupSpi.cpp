#include "SupSpi.h"

namespace supspi
{
	std::vector<BaseP*>	parserList;

	Rule	nothing_P(){ return Rule(new NeverP()); }
	Rule	eps_P(){ return Rule(new EpsilonP()); }

	Rule	space_P(){ return chP(' ') | chP('\r') | chP('\n') | chP('\t'); }
	Rule	anychar_P(){ return Rule(new AnycharP()); }
	Rule	eol_P(){ return Rule(new EndOfLineP()); }
	Rule	alnum_P(){ return Rule(new AlnumP()); }
	Rule	alpha_P(){ return Rule(new AlphaP()); }
	Rule	graph_P(){ return Rule(new GraphP()); }
	Rule	digit_P(){ return Rule(new DigitP()); }

	Rule	int_P(){ return Rule(new IntNumberP(10)); }
	Rule	real_P(){ return Rule(new RealNumberP()); }

	unsigned int AllocParser(BaseP* parser){ parserList.push_back(parser); return (unsigned int)parserList.size()-1; }
	void		ReleaseParser(unsigned int ptr){ delete parserList[ptr]; }
	void		SetParser(unsigned int ptr, BaseP* parser){ parserList[ptr] = parser; }
	BaseP*		GetParser(unsigned int ptr){ return parserList[ptr]; }

	static AlternativePolicy	alerPol = ALTER_STANDART;
	static ActionPolicy			actionPol = ACTION_STANDART;

	AlternativePolicy	GetAlterPolicy(){ return alerPol; }
	void				SetAlterPolicy(AlternativePolicy pol){ alerPol = pol; }

	ActionPolicy		GetActionPolicy(){ return actionPol; }
	void				SetActionPolicy(ActionPolicy pol){ actionPol = pol; }

	void	SkipSpaces(char** str, BaseP* space)
	{
		AlternativePolicy old = GetAlterPolicy();
		SetAlterPolicy(ALTER_STANDART);
		if(space)
			while(space->Parse(str, NULL));
		SetAlterPolicy(old);
	}

	Rule	chP(char ch) throw(){ return Rule(new ChlitP(ch)); }
	Rule	strP(char* str) throw(){ return Rule(new StrlitP(str)); }

	Rule	operator !  (Rule a) throw(){ return Rule(new RepeatP(a, ZERO_ONE)); }
	Rule	operator +  (Rule a) throw(){ return Rule(new RepeatP(a, PLUS)); }
	Rule	operator *  (Rule a) throw(){ return Rule(new RepeatP(a, ZERO_PLUS)); }

	Rule	operator |  (Rule a, Rule b) throw(){ return Rule(new AlternativeP(a, b)); }
	Rule	operator |  (char a, Rule b) throw(){ return Rule(new AlternativeP(chP(a), b)); }
	Rule	operator |  (Rule a, char b) throw(){ return Rule(new AlternativeP(a, chP(b))); }

	Rule	operator >> (Rule a, Rule b) throw(){ return Rule(new SequenceP(a, b)); }
	Rule	operator >> (char a, Rule b) throw(){ return Rule(new SequenceP(chP(a), b)); }
	Rule	operator >> (Rule a, char b) throw(){ return Rule(new SequenceP(a, chP(b))); }

	Rule	operator -  (Rule a, Rule b) throw(){ return Rule(new ExcludeP(a, b)); }

	Rule	operator ~	(Rule a) throw(){ return Rule(new NegateP(a)); }

	ParseResult	Parse(Rule main, char* str, Rule space)
	{
		SetAlterPolicy(ALTER_STANDART);
		SetActionPolicy(ACTION_STANDART);
		char* temp = str;
		bool res = main->Parse(&temp, space.getParser());
		if(res)
			SkipSpaces(&temp, space.getParser());
		if(!res)
			return PARSE_FAILED;
		if(res && strlen(temp))
			return PARSE_NOTFULL;
		return PARSE_OK;
	}

	void		DeleteParsers()
	{
		for(int i = 0; i < (int)parserList.size(); i++)
		{
			if(!parserList[i])
				continue;
			delete parserList[i];
			for(int n = i + 1; n < (int)parserList.size(); n++)
			{
				if(parserList[i] == parserList[n])
					parserList[n] = NULL;
			}
			parserList[i] = NULL;
		}
	}
};
