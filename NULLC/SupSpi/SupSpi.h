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
	
	//Our base parser
	class BaseP
	{
	public:
		BaseP(){ }
		virtual			~BaseP(){ }

		virtual bool	Parse(char** str, BaseP* space) = 0;
	protected:
	};

	//Rule is a wrapper over shared_ptr<BaseP>
	class Rule
	{
	public:
		Rule(){ m_ptr.reset(new shared_ptr<BaseP>()); }
		Rule(shared_ptr<BaseP> a){ m_ptr.reset(new shared_ptr<BaseP>()); *m_ptr=a; }
		
		Rule&	operator =(const Rule& a)
		{
			*m_ptr=*a.m_ptr;
			return *this;
		}
		
		BaseP*	operator ->(){ return m_ptr->get(); }
		shared_ptr<BaseP> getParser(){ return *m_ptr; };

		void set(const Rule& r){ m_ptr = r.m_ptr; }

		template<typename ActionT>
		Rule	operator [](ActionT act);

		void detach(){ m_ptr->reset(); /*m_ptr.reset();*/ }
	private:
		shared_ptr<shared_ptr<BaseP> >	m_ptr;
	};

	void	SkipSpaces(char** str, BaseP* space);

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
		ActionP(Rule a, ActionT act): m_act(act) { m_a.set(a); }
		~ActionP(){ /*m_a.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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
		~NoSpaceP(){ /*m_sub.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
		{
			(void)space;
			return m_sub->Parse(str, NULL);
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
		~LongestP(){ /*m_altp.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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
		StrlitP(char* str){ m_str = str; m_len = (unsigned int)strlen(str); }
		virtual ~StrlitP(){ }

		virtual bool	Parse(char** str, BaseP* space)
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
		unsigned int	m_len;
	};

	class IntNumberP: public BaseP
	{
	public:
		IntNumberP(int base){ m_base = base; }
		~IntNumberP(){}

		virtual bool	Parse(char** str, BaseP* space)
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

		virtual bool	Parse(char** str, BaseP* space)
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
	const unsigned int ZERO_ONE = 1;
	const unsigned int PLUS = 2;
	const unsigned int ZERO_PLUS = 3;
	class RepeatP: public BaseP
	{
	public:
		RepeatP(Rule a, unsigned int cnt){ m_a.set(a); m_cnt = cnt; }
		virtual ~RepeatP(){ /*m_a.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
		{
			char* curr = *str;
			unsigned int iter = 0;
			for(;;)
			{
				//SkipSpaces(str, space);
				if(!m_a->Parse(str, space))
				{
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
		AlternativeP(Rule a, Rule b){ m_a.set(a); m_b.set(b); }
		virtual ~AlternativeP(){ /*m_a.detach(); m_b.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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
			return true;
		}
	protected:
		Rule	m_a, m_b;
	};

	class SequenceP: public BaseP
	{
	public:
		SequenceP(const Rule& a, const Rule& b){ m_a.set(a); m_b.set(b); }
		virtual ~SequenceP(){ /*m_a.detach(); m_b.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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
		~ExcludeP(){ /*m_a.detach(); m_b.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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
		virtual ~NegateP(){ /*m_a.detach();*/ }

		virtual bool	Parse(char** str, BaseP* space)
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
	Rule	operator !  (Rule a) throw();
	Rule	operator +  (Rule a) throw();
	Rule	operator *  (Rule a) throw();

	Rule	operator |  (Rule a, Rule b) throw();
	Rule	operator |  (char a, Rule b) throw();
	Rule	operator |  (Rule a, char b) throw();

	Rule	operator >> (Rule a, Rule b) throw();
	Rule	operator >> (char a, Rule b) throw();
	Rule	operator >> (Rule a, char b) throw();

	Rule	operator -  (Rule a, Rule b) throw();

	Rule	operator ~	(Rule a) throw();

	//Parser creation
	Rule	chP(char ch) throw();
	Rule	strP(char* str) throw();

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
	enum ParseResult{ PARSE_FAILED, PARSE_OK, PARSE_NOTFULL, };
	ParseResult	Parse(Rule main, char* str, Rule space);
};
