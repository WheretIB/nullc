#include "../stdafx.h"
#pragma once

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
			(void)s; (void)e;	// C4100
			if(my_ref)
				(*my_ref) = my_val;
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

		void operator()(char const*s, char const*e)
		{
			(void)s; (void)e;	// C4100
			if(my_ref)
				(*my_ref)++;
		}
	private:
		T*	my_ref;
	};
	template<typename T>
	struct ArrBackInc
	{
		ArrBackInc(): my_ref(NULL){ }
		ArrBackInc(T& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e)
		{
			(void)s; (void)e;	// C4100
			if(my_ref)
				(*my_ref).back()++;
		}
	private:
		T*	my_ref;
	};
	template<typename T, typename V>
	struct PushBackVal
	{
		PushBackVal(): my_ref(NULL){ }
		PushBackVal(T& ref, V val): my_ref(&ref), my_val(val){ }

		void operator()(char const*s, char const*e)
		{
			(void)s; (void)e;	// C4100
			if(my_ref)
				(*my_ref).push_back(my_val);
		}
	private:
		T*	my_ref;
		V	my_val;
	};
	template<typename T>
	struct PopBack
	{
		PopBack(): my_ref(NULL){ }
		PopBack(T& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e)
		{
			(void)s; (void)e;	// C4100
			if(my_ref)
				(*my_ref).pop_back();
		}
	private:
		T*	my_ref;
	};
	struct StrToInt
	{
		StrToInt(): my_ref(NULL){ }
		StrToInt(unsigned int& ref): my_ref(&ref){ }

		void operator()(char const*s, char const*e)
		{
			(void)e;	// C4100
			if(my_ref)
				(*my_ref) = atoi(s);
		}
	private:
		unsigned int*	my_ref;
	};

	class BaseP;
	extern std::vector<BaseP*>	uniqueParserList;

	typedef void (*SpaceRule)(char**);

	//Our base parser
	class BaseP
	{
	public:
		BaseP(){ uniqueParserList.push_back(this); };
		virtual			~BaseP(){ }

		virtual bool	Parse(char** str, SpaceRule space) = 0;

		static bool continueParse;
	protected:
	};

	extern std::vector<BaseP*>	parserList;

	unsigned int AllocParser(BaseP* parser);
	void		SetParser(unsigned int ptr, BaseP* parser);
	BaseP*		GetParser(unsigned int ptr);

	//Rule is a wrapper over BaseP
	class Rule
	{
	public:
		Rule(){ myParser = AllocParser(NULL); }
		explicit Rule(BaseP* parser){ myParser = AllocParser(parser); }
		explicit Rule(unsigned int ptr){ myParser = ptr; }
		
		Rule&	operator =(const Rule& r)
		{
			SetParser(myParser, GetParser(r.myParser));
			return *this;
		}
		
		BaseP*	operator ->() const{ assert(GetParser(myParser) != NULL); return GetParser(myParser); }
		BaseP*	getParser() const{ return GetParser(myParser); };
		unsigned int	getPtr() const{ return myParser; }

		template<typename ActionT>
		Rule	operator [](ActionT act);
	private:
		unsigned int	myParser;
	};

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
		ActionP(Rule a, ActionT act): m_a(a.getPtr()), m_act(act){ }
		~ActionP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			if(space)
				space(str);
			char* start = *str;
			if(!m_a->Parse(str, space))
				return false;
			if(GetActionPolicy() == ACTION_STANDART)
				m_act(start,*str);
			return continueParse;
		}
	private:
		ActionT				m_act;
		Rule	m_a;
	};
	template<typename ActionT>
	Rule	Rule::operator [](ActionT act){ return Rule(new ActionP<ActionT>(*this, act)); }

	//Policies

	//Rule inside this policy won't skip any spaces
	class NoSpaceP: public BaseP
	{
	public:
		NoSpaceP(Rule a): m_sub(a.getPtr()){  }
		~NoSpaceP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			(void)space;
			//if(space)
			//	space(str);	// Skip spaces before lexeme
			return m_sub->Parse(str, NULL);
		}
	private:
		Rule	m_sub;
	};
	//helper will help to use syntax like lexemeD[rule]
	struct NoSpaceHelper
	{
		Rule	operator[](Rule a){ return Rule(new NoSpaceP(a)); }
	};

	//Longest parses both rules and applies the one, that have parsed longest string
	class LongestP: public BaseP
	{
	public:
		LongestP(Rule a): m_altp(a.getPtr()){ }
		~LongestP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			AlternativePolicy old = GetAlterPolicy();
			SetAlterPolicy(ALTER_LONGEST);
			bool ret = m_altp->Parse(str, space);
			SetAlterPolicy(old);
			return ret && continueParse;
		}
	private:
		Rule	m_altp;
	};
	struct LongestHelper
	{
		Rule	operator[](Rule altp){ return Rule(new LongestP(altp)); }
	};

	static inline bool isDigit(char data)
	{
		return (unsigned char)(data - '0') < 10;
	}

	static inline bool isAlpha(char data)
	{
		return (unsigned int)(data - 'A') < 26 || (unsigned int)(data - 'a') < 26;
	}

	//epsilon and nothing
	class EpsilonP: public BaseP
	{
	public:
		EpsilonP(){ }
		virtual ~EpsilonP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			(void)str;
			(void)space;
			return true;
		}
	protected:
	};

	class NeverP: public BaseP
	{
	public:
		NeverP(){ }
		virtual ~NeverP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			(void)str;
			(void)space;
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
			if(!isDigit(*str[0])){
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
			if(!(isAlpha(*str[0]) || isDigit(*str[0]))){
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
			if(!isAlpha(*str[0])){
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
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
		StrlitP(char* str){ m_str = str; m_len = (unsigned int)strlen(str); }
		virtual ~StrlitP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
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
		unsigned int	m_len;
	};

	class IntNumberP: public BaseP
	{
	public:
		IntNumberP(int base){ m_base = base; }
		~IntNumberP(){}

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
			while(isDigit(*str[0]))
				(*str)++;
			if(curr == *str)
				return false;	//no characters were parsed
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

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(space)
				space(str);
			if(*str[0] == '-' || *str[0] == '+')
				(*str)++;
			while(isDigit(*str[0]))
				(*str)++;
			if(curr == *str && *str[0] != '.')
				return false;	//no characters were parsed
			if(*str[0] == '.'){
				(*str)++;
				while(isDigit(*str[0]))
					(*str)++;
			}
			if(*str[0] == 'e'){
				(*str)++;
				if(*str[0] == '-')
					(*str)++;
				while(isDigit(*str[0]))
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
	const unsigned int ZERO_ONE = 1;
	const unsigned int PLUS = 2;
	const unsigned int ZERO_PLUS = 3;
	class RepeatP: public BaseP
	{
	public:
		RepeatP(Rule a, unsigned int cnt): m_a(a.getPtr()){ m_cnt = cnt; }
		virtual ~RepeatP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			unsigned int iter = 0;
			for(;;)
			{
				//SkipSpaces(str, space);
				if(!m_a->Parse(str, space))
				{
					if(!continueParse)
						return false;
					if((m_cnt == ZERO_ONE || m_cnt == ZERO_PLUS) && iter == 0)
					{
						(*str) = curr;
						return true;
					}
					if(m_cnt == PLUS && iter == 0)
					{
						(*str) = curr;
						return false;
					}
					if(iter != 0)
					{
						return true;
					}
				}else{
					if(!continueParse)
						return false;
					iter++;
					curr = *str;
					if(m_cnt == ZERO_ONE)
						return true;
				}
			}
		}
	protected:
		Rule	m_a;
		unsigned int				m_cnt;
	};

	//Binary operators
	class AlternativeP: public BaseP
	{
	public:
		AlternativeP(Rule a, Rule b): m_a(a.getPtr()), m_b(b.getPtr()){ }
		virtual ~AlternativeP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			//static int issued = 0;
			//issued++;
			char* curr = *str;
			
			if(GetAlterPolicy() == ALTER_STANDART)
			{
				if(!m_a->Parse(str, space))
				{
					if(!continueParse)
						return false;
					if(!m_b->Parse(str, space))
					{
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

				if(oldAlter == ALTER_LONGEST)
				{
					if((unsigned int)(temp1-(*str)) >= (unsigned int)(temp2-(*str)))
						m_a->Parse(str, space);
					else
						m_b->Parse(str, space);
				}else{
					if((unsigned int)(temp1-(*str)) <= (unsigned int)(temp2-(*str)))
						m_a->Parse(str, space);
					else
						m_b->Parse(str, space);
				}
				SetAlterPolicy(oldAlter);
			}
			return continueParse;
		}
	protected:
		Rule	m_a, m_b;
	};

	class SequenceP: public BaseP
	{
	public:
		SequenceP(const Rule& a, const Rule& b): m_a(a.getPtr()), m_b(b.getPtr()){ }
		virtual ~SequenceP(){  }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			//static int issued = 0;
			//issued++;
			char* curr = *str;
			if(!m_a->Parse(str, space))
			{
				(*str) = curr;
				return false;
			}
			if(!continueParse)
				return false;
			if(!m_b->Parse(str, space))
			{
				(*str) = curr;
				return false;
			}
			return continueParse;
		}
	protected:
		Rule	m_a, m_b;
	};

	class ExcludeP: public BaseP
	{
	public:
		ExcludeP(Rule a, Rule b): m_a(a.getPtr()), m_b(b.getPtr()){ }
		~ExcludeP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			//SkipSpaces(str, space);
			char* copy = *str;
			if(!m_a->Parse(str, space))
			{
				(*str) = curr;
				return false;
			}
			if(!continueParse)
				return false;
			if(m_b->Parse(&copy, space))
			{
				(*str) = curr;
				return false;
			}
			return continueParse;
		}
	private:
		Rule	m_a, m_b;
	};

	class NegateP: public BaseP
	{
	public:
		NegateP(Rule a): m_a(a.getPtr()){ }
		virtual ~NegateP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			if(!m_a->Parse(str, space))
			{
				(*str) = curr;
				return continueParse;
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

	Rule	nothing_P();
	Rule	eps_P();

	Rule	space_P();
	Rule	anychar_P();
	Rule	eol_P();
	Rule	alnum_P();
	Rule	alpha_P();
	Rule	graph_P();
	Rule	digit_P();

	Rule	int_P();
	Rule	real_P();

#define nothingP nothing_P()
#define epsP eps_P()

#define spaceP space_P()
#define anycharP anychar_P()
#define eolP eol_P()
#define alnumP alnum_P()
#define alphaP alpha_P()
#define graphP graph_P()
#define digitP digit_P()

#define intP int_P()
#define realP real_P()

	//Static policies
	static NoSpaceHelper	lexemeD;
	static LongestHelper	longestD;

	//Main function
	enum ParseResult{ PARSE_FAILED, PARSE_OK, PARSE_NOTFULL, PARSE_ABORTED };
	ParseResult	Parse(const Rule& main, char* str, SpaceRule space, bool skipAction=false);
	void		Abort();

	void		DeleteParsers();
};
