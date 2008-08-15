#include "stdafx.h"
#pragma once
using boost::shared_ptr;

namespace supspi
{
	typedef void (*callBack)(char const*, char const*);

	template<typename T>
	struct AssignVar
	{
		AssignVar(): my_ref(NULL){ }
		AssignVar(T& ref, T val): my_ref(&ref){ my_val = val; }

		void operator()(char const*s, char const*e)
		{
			if(my_ref) (*my_ref) = my_val;
		}

	private:
		T*	my_ref;
		T	my_val;
	};
	template<typename T>
	struct IncVar
	{
		IncVar(): my_ref(NULL){ }
		IncVar(T& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e){ if(my_ref) (*my_ref)++; }
	private:
		T*	my_ref;
	};
	template<typename T>
	struct ArrBackInc
	{
		ArrBackInc(): my_ref(NULL){ }
		ArrBackInc(T& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e){ if(my_ref) (*my_ref).back()++; }
	private:
		T*	my_ref;
	};
	template<typename T, typename V>
	struct PushBackVal
	{
		PushBackVal(): my_ref(NULL){ }
		PushBackVal(T& ref, V val): my_ref(&ref), my_val(val){ }

		void operator()(char const*s, char const*e){ if(my_ref) (*my_ref).push_back(my_val); }
	private:
		T*	my_ref;
		V	my_val;
	};
	struct StrToInt
	{
		StrToInt(): my_ref(NULL){ }
		StrToInt(UINT& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e){ if(my_ref) (*my_ref) = atoi(s); }
	private:
		UINT*	my_ref;
	};
	
	//Our base parser
	class BaseP
	{
	public:
		BaseP(){ }
		virtual			~BaseP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space) = 0;
	protected:
	};

	//Rule is a wrapper over boost::shared_ptr<BaseP>
	class Rule
	{
	public:
		Rule(){ m_ptr.reset(new shared_ptr<BaseP>()); }
		Rule(shared_ptr<BaseP> a){ m_ptr.reset(new shared_ptr<BaseP>()); *m_ptr=a; }
		~Rule(){}
		
		Rule&	operator =(const Rule& a)
		{
			*m_ptr=*a.m_ptr;
			return *this;
		}
		
		BaseP*	operator ->(){ return m_ptr->get(); }
		shared_ptr<BaseP> getParser(){ return *m_ptr; };

		void set(const Rule& r) {m_ptr = r.m_ptr;}

		template<typename ActionT>
		Rule	operator [](ActionT act);
	private:
		shared_ptr<shared_ptr<BaseP> >	m_ptr;
	};

	void	SkipSpaces(char** str, shared_ptr<BaseP> space);

	//AlternativeP can act differently, depending on state of this policy
	enum AlternativePolicy{ ALTER_STANDART, ALTER_LONGEST, ALTER_SHORTEST, };
	//static AlternativePolicy alternativePol = ALTER_STANDART;
	AlternativePolicy	GetAlterPolicy();
	void				SetAlterPolicy(AlternativePolicy);

	//ActionP can act differently, depending on state of this policy
	enum ActionPolicy{ ACTION_NONE, ACTION_STANDART, };
	//static ActionPolicy actionPol = ACTION_STANDART;
	ActionPolicy	GetActionPolicy();
	void			SetActionPolicy(ActionPolicy);

	//Semantic action class
	template<typename ActionT>
	class ActionP: public BaseP
	{
	public:
		ActionP(Rule a, ActionT act){ m_a.set(a); m_act = act; }
		~ActionP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			SkipSpaces(str, space);
			char* start = *str;
			if(!m_a->Parse(str, space))
				return false;
			if(GetActionPolicy() == ACTION_STANDART)
				m_act(start,*str);
			return true;
		}
	private:
		ActionT				m_act;
		Rule	m_a;
	};
	template<typename ActionT>
	Rule	Rule::operator [](ActionT act){ return Rule(shared_ptr<BaseP>(new ActionP<ActionT>(*this, act))); }
	
	//Policies

	//Rule inside this policy won't skip any spaces
	class NoSpaceP: public BaseP
	{
	public:
		NoSpaceP(Rule a){ m_sub.set(a); }
		~NoSpaceP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			return m_sub->Parse(str, shared_ptr<BaseP>((BaseP*)NULL));
		}
	private:
		Rule	m_sub;
	};
	//helper will help to use syntax like lexemeD[rule]
	struct NoSpaceHelper
	{
		Rule	operator[](Rule a){ return Rule(shared_ptr<BaseP>(new NoSpaceP(a))); }
	};

	//Longest parses both rules and apply's the one, that have parsed longest string
	class LongestP: public BaseP
	{
	public:
		LongestP(Rule a){ m_altp.set(a); }
		~LongestP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			AlternativePolicy old = GetAlterPolicy();
			SetAlterPolicy(ALTER_LONGEST);
			bool ret = m_altp->Parse(str, space);
			SetAlterPolicy(old);
			return ret;
		}
	private:
		Rule	m_altp;
	};
	struct LongestHelper
	{
		Rule	operator[](Rule altp){ return Rule(shared_ptr<BaseP>(new LongestP(altp))); }
	};

	//epsilon and nothing
	class EpsilonP: public BaseP
	{
	public:
		EpsilonP(){ }
		virtual ~EpsilonP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			return true;
		}
	protected:
	};

	class NeverP: public BaseP
	{
	public:
		NeverP(){ }
		virtual ~NeverP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			return false;
		}
	protected:
	};

	//basic parsers
	class ChlitP: public BaseP
	{
	public:
		ChlitP(char ch){ m_ch = ch; }
		virtual ~ChlitP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(*str[0] != m_ch){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
		char	m_ch;
	};

	class AnycharP: public BaseP
	{
	public:
		AnycharP(){ }
		virtual ~AnycharP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(*str[0] == NULL){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	class EndOfLineP: public BaseP
	{
	public:
		EndOfLineP(){ }
		virtual ~EndOfLineP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(*str[0] != '\n'){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	class DigitP: public BaseP
	{
	public:
		DigitP(){ }
		virtual ~DigitP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(!isdigit(*str[0])){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	class AlnumP: public BaseP
	{
	public:
		AlnumP(){ }
		virtual ~AlnumP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(!isalnum(*str[0])){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	class AlphaP: public BaseP
	{
	public:
		AlphaP(){ }
		virtual ~AlphaP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(!isalpha(*str[0])){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	class GraphP: public BaseP
	{
	public:
		GraphP(){ }
		virtual ~GraphP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(!isgraph(*str[0])){
				(*str) = curr;
				return false;
			}else{
				(*str)++;
				return true;
			}
		}
	protected:
	};
	
	class StrlitP: public BaseP
	{
	public:
		StrlitP(char* str){ m_str = str; m_len = (UINT)strlen(str); }
		virtual ~StrlitP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(memcmp(*str, m_str, m_len) != 0){
				(*str) = curr;
				return false;
			}else{
				(*str) += m_len;
				return true;
			}
		}
	protected:
		char*	m_str;
		UINT	m_len;
	};

	class IntNumberP: public BaseP
	{
	public:
		IntNumberP(int base){ m_base = base; }
		~IntNumberP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			while(isdigit(*str[0]))
				(*str)++;
			if(curr == *str)
				return false;	//no characters were parsed...
			return true;
		}
	private:
		int	m_base;
	};

	class RealNumberP: public BaseP
	{
	public:
		RealNumberP(){}
		~RealNumberP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			SkipSpaces(str, space);
			if(*str[0] == '-' || *str[0] == '+')
				(*str)++;
			while(isdigit(*str[0]))
				(*str)++;
			if(curr == *str && *str[0] != '.')
				return false;	//no characters were parsed...
			if(*str[0] == '.'){
				(*str)++;
				while(isdigit(*str[0]))
					(*str)++;
			}
			if(*str[0] == 'e'){
				(*str)++;
				if(*str[0] == '-')
					(*str)++;
				while(isdigit(*str[0]))
					(*str)++;
			}
			if(curr[0] == '.' && (*str)-curr == 1)
			{
				(*str) = curr;
				return false;
			}
			return true;
		}
	private:
		int	m_base;
	};
	
	//Unary operators
	const UINT ZERO_ONE = 1;
	const UINT PLUS = 2;
	const UINT ZERO_PLUS = 3;
	class RepeatP: public BaseP
	{
	public:
		RepeatP(Rule a, UINT cnt){ m_a.set(a); m_cnt = cnt; }
		virtual ~RepeatP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			UINT iter = 0;
			while(true)
			{
				//SkipSpaces(str, space);
				if(!m_a->Parse(str, space)){
					if((m_cnt == ZERO_ONE || m_cnt == ZERO_PLUS) && iter == 0){
						(*str) = curr;
						return true;
					}
					if(m_cnt == PLUS && iter == 0){
						(*str) = curr;
						return false;
					}
					if(iter != 0){
						return true;
					}
				}else{
					iter++;
					curr = *str;
					if(m_cnt == ZERO_ONE)
						return true;
				}
			}
			
			return false;
		}
	protected:
		Rule	m_a;
		UINT				m_cnt;
	};

	//Binary operators
	class AlternativeP: public BaseP
	{
	public:
		AlternativeP(Rule a, Rule b){ m_a.set(a); m_b.set(b); }
		virtual ~AlternativeP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			
			if(GetAlterPolicy() == ALTER_STANDART){
				if(!m_a->Parse(str, space)){
					if(!m_b->Parse(str, space)){
						(*str) = curr;
						return false;
					}
				}
			}else if(GetAlterPolicy() == ALTER_LONGEST || GetAlterPolicy() == ALTER_SHORTEST){
				ActionPolicy oldAction = GetActionPolicy();
				SetActionPolicy(ACTION_NONE);
				AlternativePolicy oldAlter = GetAlterPolicy();
				SetAlterPolicy(ALTER_STANDART);
				char *temp1 = *str, *temp2 = *str;
				bool agood = m_a->Parse(&temp1, space);
				bool bgood = m_b->Parse(&temp2, space);
				SetActionPolicy(oldAction);
				
				if(!agood && !bgood){
					SetAlterPolicy(oldAlter);
					return false;
				}

				if(oldAlter == ALTER_LONGEST){
					if((UINT)(temp1-(*str)) >= (UINT)(temp2-(*str)))
						m_a->Parse(str, space);
					else
						m_b->Parse(str, space);
				}else{
					if((UINT)(temp1-(*str)) <= (UINT)(temp2-(*str)))
						m_a->Parse(str, space);
					else
						m_b->Parse(str, space);
				}
				SetAlterPolicy(oldAlter);
			}
			return true;
		}
	protected:
		Rule	m_a, m_b;
	};

	class SequenceP: public BaseP
	{
	public:
		SequenceP(Rule a, Rule b){ m_a.set(a); m_b.set(b); }
		virtual ~SequenceP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			//SkipSpaces(str, space);
			if(!m_a->Parse(str, space)){
				(*str) = curr;
				return false;
			}
			//SkipSpaces(str, space);
			if(!m_b->Parse(str, space)){
				(*str) = curr;
				return false;
			}
			return true;
		}
	protected:
		Rule	m_a, m_b;
	};

	class ExcludeP: public BaseP
	{
	public:
		ExcludeP(Rule a, Rule b){ m_a.set(a); m_b.set(b); }
		~ExcludeP(){}

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			//SkipSpaces(str, space);
			char* copy = *str;
			if(!m_a->Parse(str, space)){
				(*str) = curr;
				return false;
			}
			if(m_b->Parse(&copy, space)){
				(*str) = curr;
				return false;
			}
			return true;
		}
	private:
		Rule	m_a, m_b;
	};

	class NegateP: public BaseP
	{
	public:
		NegateP(Rule a){ m_a.set(a); }
		virtual ~NegateP(){ }

		virtual bool	Parse(char** str, shared_ptr<BaseP> space)
		{
			char* curr = *str;
			if(!m_a->Parse(str, space)){
				(*str) = curr;
				return true;
			}
			return false;
		}
	protected:
		Rule	m_a;
	};

	//Operators
	Rule	operator !  (Rule a);
	Rule	operator +  (Rule a);
	Rule	operator *  (Rule a);

	Rule	operator |  (Rule a, Rule b);
	Rule	operator |  (char a, Rule b);
	Rule	operator |  (Rule a, char b);

	Rule	operator >> (Rule a, Rule b);
	Rule	operator >> (char a, Rule b);
	Rule	operator >> (Rule a, char b);

	Rule	operator -  (Rule a, Rule b);

	Rule	operator ~	(Rule a);

	//Parser creation
	Rule	chP(char ch);
	Rule	strP(char* str);

	//Static parsers
	static Rule nothingP = Rule(shared_ptr<BaseP>(new NeverP()));
	static Rule epsP = Rule(shared_ptr<BaseP>(new EpsilonP()));

	static Rule	spaceP = chP(' ') | chP('\r') | chP('\n') | chP('\t');
	static Rule	anycharP = Rule(shared_ptr<BaseP>(new AnycharP()));
	static Rule	eolP = Rule(shared_ptr<BaseP>(new EndOfLineP()));
	static Rule	alnumP = Rule(shared_ptr<BaseP>(new AlnumP()));
	static Rule	alphaP = Rule(shared_ptr<BaseP>(new AlphaP()));
	static Rule	graphP = Rule(shared_ptr<BaseP>(new GraphP()));
	static Rule	digitP = Rule(shared_ptr<BaseP>(new DigitP()));

	static Rule intP = Rule(shared_ptr<BaseP>(new IntNumberP(10)));
	static Rule realP = Rule(shared_ptr<BaseP>(new RealNumberP()));

	//Static policies
	static NoSpaceHelper	lexemeD;
	static LongestHelper	longestD;

	//Main function
	bool	Parse(Rule main, char* str, Rule space);
};

namespace supgen
{
	//Parser generator from description string
	class BaseCallBack
	{
	public:
		BaseCallBack(){}
		virtual ~BaseCallBack(){}

		virtual void operator() (){}
	};

	template<typename ActionT>
	class DerivedCallBack
	{
	public:
		DerivedCallBack(ActionT act){ m_act = act; }
		virtual ~DerivedCallBack(){}

		virtual void operator() (){ m_act(); }
	private:
		ActionT	m_act;
	};

	class ParseGen
	{
	public:
		ParseGen();
		~ParseGen();

		void	SetCallback(std::string name, shared_ptr<BaseCallBack> act);
		
		supspi::Rule	CreateParser(std::string grammarDesc);
	private:
		supspi::Rule	ruleList, ruleOne, ruleTwo, ruleGroup, ruleInst, ruleAction, ruleName, ruleSpace;

		std::map<std::string, supspi::Rule >				parserList;
		std::map<std::string, shared_ptr<BaseCallBack> >		callbackList;

		typedef std::map<std::string, supspi::Rule >::iterator			plIt;
		typedef std::map<std::string, shared_ptr<BaseCallBack> >::iterator	cblIt;
	};

};