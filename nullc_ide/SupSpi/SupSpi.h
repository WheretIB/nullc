#include "../stdafx.h"
#pragma once

namespace supspi
{
	template<int chunkSize>
	class ChunkedStackPool
	{
	public:
		ChunkedStackPool()
		{
			curr = first = &one;
			first->next = NULL;
			size = 0;
		}
		~ChunkedStackPool()
		{
			first = first->next;
			while(first)
			{
				StackChunk *next = first->next;
				delete first;
				first = next;
			}
			curr = first = &one;
			first->next = NULL;
			size = 0;
		}

		void	Clear()
		{
			curr = first;
			size = 0;
		}
		void	ClearTo(unsigned int bytes)
		{
			curr = first;
			while(bytes > chunkSize)
			{
				assert(curr->next != NULL);
				curr = curr->next;
				bytes -= chunkSize;
			}
			size = bytes;
		}
		void*	Allocate(unsigned int bytes)
		{
			assert(bytes < chunkSize);
			if(size + bytes < chunkSize)
			{
				size += bytes;
				return curr->data + size - bytes;
			}
			if(curr->next)
			{
				curr = curr->next;
				size = bytes;
				return curr->data;
			}
			curr->next = new StackChunk;
			curr = curr->next;
			curr->next = NULL;
			size = bytes;
			return curr->data;
		}
		unsigned int GetSize()
		{
			unsigned int wholeSize = 0;
			StackChunk *temp = first;
			while(temp != curr)
			{
				wholeSize += chunkSize;
				temp = temp->next;
			}
			return wholeSize + size;
		}
	private:
		struct StackChunk
		{
			StackChunk *next;
			char		data[chunkSize];
		};
		StackChunk	*first, *curr;
		StackChunk	one;
		unsigned int size;
	};

#ifndef _MSC_VER
#undef __forceinline
#define __forceinline inline
#endif

	template<typename T>
	class FastVector
	{
	public:
		FastVector()
		{
			data = &one;
			memset(data, 0, sizeof(T));
			max = 1;
			count = 0;
		}
		~FastVector()
		{
			if(data != &one)
				delete[] data;
		}

		__forceinline void		push_back(const T& val)
		{
			assert(data);
			data[count++] = val;
			if(count == max)
				grow(count);
		};
		__forceinline unsigned int		size()
		{
			return count;
		}
		__forceinline T&		operator[](unsigned int index)
		{
			assert(index < count);
			return data[index];
		}
		__forceinline void		resize(unsigned int newSize)
		{
			if(newSize >= max)
				grow(newSize);
			count = newSize;
		}
		__inline void	grow(unsigned int newSize)
		{
			if(max + (max >> 1) > newSize)
				newSize = max + (max >> 1);
			else
				newSize += 32;
			T* newData;
			newData = new T[newSize];
			assert(newData);
			memset(newData, 0, newSize * sizeof(T));
			memcpy(newData, data, max * sizeof(T));
			if(data != &one)
				delete[] data;
			data = newData;
			max = newSize;
		}
		T	*data;
		T	one;
		unsigned int	max, count;
	private:
		// Disable assignment and copy constructor
		void operator =(FastVector &r);
		FastVector(FastVector &r);
	};

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
	extern FastVector<BaseP*>	uniqueParserList;

	typedef void (*SpaceRule)(char**);

	//Our base parser
	class BaseP
	{
	public:
		BaseP()
		{
			uniqueParserList.push_back(this);
		};
		virtual			~BaseP(){ }

		virtual bool	Parse(char** str, SpaceRule space) = 0;

		static	void	DeleteParsers()
		{
			parserPool.Clear();
		}

		static bool continueParse;

		void*		operator new(size_t size)
		{
			return parserPool.Allocate((unsigned int)size);
		}
		void		operator delete(void *ptr, size_t size)
		{
			(void)ptr; (void)size;
			assert(!"Cannot delete parser");
		}
	protected:
		static ChunkedStackPool<4092>	parserPool;
	};

	extern FastVector<BaseP*>	parserList;

	unsigned int AllocParser(BaseP* parser);

	//Rule is a wrapper over BaseP
	class Rule
	{
	public:
		Rule()
		{
			myParser = AllocParser(NULL);
		}
		explicit Rule(BaseP* parser)
		{
			myParser = AllocParser(parser);
		}
		explicit Rule(unsigned int ptr)
		{
			myParser = ptr;
		}
		
		Rule&	operator =(const Rule& r)
		{
			parserList.data[myParser] = parserList.data[r.myParser];
			return *this;
		}
		void	forceSet(unsigned int ptr)
		{
			myParser = ptr;
		}
		
		BaseP*	operator ->() const
		{
			assert(parserList.data[myParser] != NULL);
			return parserList.data[myParser];
		}
		BaseP*	getParser() const
		{
			return parserList.data[myParser];
		};
		unsigned int	getPtr() const
		{
			return myParser;
		}

		template<typename ActionT>
		Rule	operator [](ActionT act);

		unsigned int	myParser;
	};
	class AltRule : public Rule
	{
	public:
		AltRule(): Rule(){}
		explicit AltRule(BaseP* parser): Rule(parser){}
		explicit AltRule(unsigned int ptr): Rule(ptr){}
	};
	class SeqRule : public Rule
	{
	public:
		SeqRule(): Rule(){}
		explicit SeqRule(BaseP* parser): Rule(parser){}
		explicit SeqRule(unsigned int ptr): Rule(ptr){}
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
		ActionP(Rule a, ActionT act): m_a(a.getPtr()), m_act(act)
		{
		}
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
		NoSpaceP(Rule a): m_sub(a.getPtr())
		{
		}
		~NoSpaceP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			(void)space;
			if(space)
				space(str);	// Skip spaces before lexeme
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
		LongestP(Rule a): m_altp(a.getPtr())
		{
		}
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

	static inline bool isPunctuation(char data)
	{
		return (unsigned int)(data - '!') < 15 || (unsigned int)(data - ':') < 7 || (unsigned int)(data - '[') < 6 || (unsigned int)(data - '{') < 4;
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
			if(!(isAlpha(*str[0]) || isDigit(*str[0]) || isPunctuation(*str[0]))){
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
			if(memcmp(*str, m_str, m_len) != 0)
			{
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
		RepeatP(Rule a, unsigned int cnt): m_a(a.getPtr())
		{
			m_cnt = cnt;
		}
		virtual ~RepeatP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			unsigned int iter = 0;
			for(;;)
			{
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
		AlternativeP(Rule a, Rule b)
		{
			arr[0].forceSet(a.getPtr());
			arr[1].forceSet(b.getPtr());
			count = 2;
		}
		virtual ~AlternativeP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
			
			if(GetAlterPolicy() == ALTER_STANDART)
			{
				bool parsed = false;
				for(unsigned i = 0; i < count; i++)
				{
					if(arr[i]->Parse(str, space))
					{
						parsed = true;
						break;
					}
					if(!continueParse)
						return false;
				}
				if(!parsed)
				{
					(*str) = curr;
					return false;
				}
			}else if(GetAlterPolicy() == ALTER_LONGEST || GetAlterPolicy() == ALTER_SHORTEST){
				assert(count == 2);
				ActionPolicy oldAction = GetActionPolicy();
				SetActionPolicy(ACTION_NONE);
				AlternativePolicy oldAlter = GetAlterPolicy();
				SetAlterPolicy(ALTER_STANDART);
				char *temp1 = *str, *temp2 = *str;
				bool agood = arr[0]->Parse(&temp1, space);
				bool bgood = arr[1]->Parse(&temp2, space);
				SetActionPolicy(oldAction);
				
				if(!agood && !bgood){
					SetAlterPolicy(oldAlter);
					return false;
				}

				if(oldAlter == ALTER_LONGEST)
				{
					if((unsigned int)((char*)temp1-(char*)(*str)) >= (unsigned int)((char*)temp2-(char*)(*str)))
						arr[0]->Parse(str, space);
					else
						arr[1]->Parse(str, space);
				}else{
					if((unsigned int)((char*)temp1-(char*)(*str)) <= (unsigned int)((char*)temp2-(char*)(*str)))
						arr[0]->Parse(str, space);
					else
						arr[1]->Parse(str, space);
				}
				SetAlterPolicy(oldAlter);
			}
			return continueParse;
		}
		void AddAlternative(Rule x)
		{
			assert(count != max);
			arr[count++].forceSet(x.getPtr());
		}
	protected:
		static const unsigned max = 36;
		Rule	arr[max];
		unsigned	count;
	};

	class SequenceP: public BaseP
	{
	public:
		SequenceP(const Rule& a, const Rule& b)
		{
			arr[0].forceSet(a.getPtr());
			arr[1].forceSet(b.getPtr());
			count = 2;
		}
		virtual ~SequenceP(){  }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;

			for(unsigned i = 0; i < count; i++)
			{
				if(!arr[i]->Parse(str, space))
				{
					(*str) = curr;
					return false;
				}
				if(!continueParse)
					return false;
			}
			return continueParse;
		}
		void AddSequence(Rule x)
		{
			assert(count != max);
			arr[count++].forceSet(x.getPtr());
		}
	protected:
		static const unsigned max = 8;
		Rule	arr[max];
		unsigned	count;
	};

	class ExcludeP: public BaseP
	{
	public:
		ExcludeP(Rule a, Rule b): m_a(a.getPtr()), m_b(b.getPtr())
		{
		}
		~ExcludeP(){ }

		virtual bool	Parse(char** str, SpaceRule space)
		{
			char* curr = *str;
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
		NegateP(Rule a): m_a(a.getPtr())
		{
		}
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

	AltRule	operator |  (Rule a, Rule b);
	AltRule	operator |  (char a, Rule b);
	AltRule	operator |  (Rule a, char b);

	AltRule	operator |  (AltRule a, Rule b);

	SeqRule	operator >> (Rule a, Rule b);
	SeqRule	operator >> (char a, Rule b);
	SeqRule	operator >> (Rule a, char b);

	SeqRule	operator >> (SeqRule a, Rule b);

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
