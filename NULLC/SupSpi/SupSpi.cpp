//#include "stdafx.h"
#include "SupSpi.h"

namespace supspi
{
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

	Rule	chP(char ch) throw(){ return Rule(shared_ptr<BaseP>(new ChlitP(ch))); }
	Rule	strP(char* str) throw(){ return Rule(shared_ptr<BaseP>(new StrlitP(str))); }

	Rule	operator !  (Rule a) throw(){ return Rule(shared_ptr<BaseP>(new RepeatP(a, ZERO_ONE))); }
	Rule	operator +  (Rule a) throw(){ return Rule(shared_ptr<BaseP>(new RepeatP(a, PLUS))); }
	Rule	operator *  (Rule a) throw(){ return Rule(shared_ptr<BaseP>(new RepeatP(a, ZERO_PLUS))); }

	Rule	operator |  (Rule a, Rule b) throw(){ return Rule(shared_ptr<BaseP>(new AlternativeP(a, b))); }
	Rule	operator |  (char a, Rule b) throw(){ return Rule(shared_ptr<BaseP>(new AlternativeP(chP(a), b))); }
	Rule	operator |  (Rule a, char b) throw(){ return Rule(shared_ptr<BaseP>(new AlternativeP(a, chP(b)))); }

	Rule	operator >> (Rule a, Rule b) throw(){ return Rule(shared_ptr<BaseP>(new SequenceP(a, b))); }
	Rule	operator >> (char a, Rule b) throw(){ return Rule(shared_ptr<BaseP>(new SequenceP(chP(a), b))); }
	Rule	operator >> (Rule a, char b) throw(){ return Rule(shared_ptr<BaseP>(new SequenceP(a, chP(b)))); }

	Rule	operator -  (Rule a, Rule b) throw(){ return Rule(shared_ptr<BaseP>(new ExcludeP(a, b))); }

	Rule	operator ~	(Rule a) throw(){ return Rule(shared_ptr<BaseP>(new NegateP(a))); }

	ParseResult	Parse(Rule main, char* str, Rule space)
	{
		SetAlterPolicy(ALTER_STANDART);
		SetActionPolicy(ACTION_STANDART);
		char* temp = str;
		bool res = main->Parse(&temp, space.getParser().get());
		if(res)
			SkipSpaces(&temp, space.getParser().get());
		if(!res)
			return PARSE_FAILED;
		if(res && strlen(temp))
			return PARSE_NOTFULL;
		return PARSE_OK;
	}
};
