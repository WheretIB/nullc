#include "stdafx.h"
#include "SupSpi.h"

namespace supspi
{
	static AlternativePolicy	alerPol = ALTER_STANDART;
	static ActionPolicy			actionPol = ACTION_STANDART;

	AlternativePolicy	GetAlterPolicy(){ return alerPol; }
	void				SetAlterPolicy(AlternativePolicy pol){ alerPol = pol; }

	ActionPolicy		GetActionPolicy(){ return actionPol; }
	void				SetActionPolicy(ActionPolicy pol){ actionPol = pol; }

	void	SkipSpaces(char** str, shared_ptr<BaseP> space)
	{
		AlternativePolicy old = GetAlterPolicy();
		SetAlterPolicy(ALTER_STANDART);
		if(space)
			while(space->Parse(str, shared_ptr<BaseP>((BaseP*)NULL)));
		SetAlterPolicy(old);
	}

	Rule	chP(char ch){ return Rule(shared_ptr<BaseP>(new ChlitP(ch))); }
	Rule	strP(char* str){ return Rule(shared_ptr<BaseP>(new StrlitP(str))); }

	Rule	operator !  (Rule a){ return Rule(shared_ptr<BaseP>(new RepeatP(a, ZERO_ONE))); }
	Rule	operator +  (Rule a){ return Rule(shared_ptr<BaseP>(new RepeatP(a, PLUS))); }
	Rule	operator *  (Rule a){ return Rule(shared_ptr<BaseP>(new RepeatP(a, ZERO_PLUS))); }

	Rule	operator |  (Rule a, Rule b){ return Rule(shared_ptr<BaseP>(new AlternativeP(a, b))); }
	Rule	operator |  (char a, Rule b){ return Rule(shared_ptr<BaseP>(new AlternativeP(chP(a), b))); }
	Rule	operator |  (Rule a, char b){ return Rule(shared_ptr<BaseP>(new AlternativeP(a, chP(b)))); }

	Rule	operator >> (Rule a, Rule b){ return Rule(shared_ptr<BaseP>(new SequenceP(a, b))); }
	Rule	operator >> (char a, Rule b){ return Rule(shared_ptr<BaseP>(new SequenceP(chP(a), b))); }
	Rule	operator >> (Rule a, char b){ return Rule(shared_ptr<BaseP>(new SequenceP(a, chP(b)))); }

	Rule	operator -  (Rule a, Rule b){ return Rule(shared_ptr<BaseP>(new ExcludeP(a, b))); }

	Rule	operator ~	(Rule a){ return Rule(shared_ptr<BaseP>(new NegateP(a))); }

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
		if(res && strlen(temp))//(strlen(str) != (UINT)(temp-str)))
			return PARSE_NOTFULL;
		return PARSE_OK;
	}
};

namespace supgen
{
	using namespace supspi;
	ParseGen::ParseGen()
	{
		ruleList	= +(ruleName >> '=' >> ruleOne >> ';');
		ruleOne		= ruleTwo >> *(chP('|') >> ruleTwo);
		ruleTwo		= ruleInst >> *(strP(">>") >> ruleInst);
		
		ruleGroup	= '(' >> ruleOne >> ')';
		ruleInst	= (chP('!') | chP('+') | chP('*') | epsP) >>
						(ruleGroup |
						//(strP("chP") >> strP("('") >> anycharP >> strP("')")) |
						//(strP("strP") >> strP("(\"") >> *(anycharP - chP('\"')) >> strP("\")")) |
						//strP("anycharP") | strP("eolP") | strP("alphaP") | strP("alnumP") | strP("epsP") | strP("nothingP") |
						//((strP("longestD") | strP("lexemeD")) >> '[' >> ruleOne >> ']') |
						//ruleAction |
						ruleName// |
						//('\'' >> anycharP >> '\'')
						);
		ruleAction	= ruleName >> '[' >> ruleName >> ']';
		ruleName	= lexemeD[(alphaP | chP('_')) >> *(alnumP | chP('_'))];

		ruleSpace	= spaceP | ((strP("//") >> *(anycharP - eolP)) | (strP("/*") >> *(anycharP - strP("*/")) >> strP("*/")));
	}

	ParseGen::~ParseGen()
	{

	}

	void ParseGen::SetCallback(std::string name, shared_ptr<BaseCallBack> act)
	{
		callbackList[name] = act;
	}

	Rule ParseGen::CreateParser(std::string grammarDesc)
	{
		char* strTemp = (char*)grammarDesc.c_str();
		if(!Parse(ruleList, strTemp, ruleSpace))
			throw std::string("Syntax error");
		return Rule();
	}
};