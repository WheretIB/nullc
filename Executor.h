#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

template<typename T>
class FastVector
{
public:
	FastVector(UINT reserved = 1000){ data = new T[reserved]; max = reserved; m_size = 0; }
	~FastVector(){ delete[] data; }

	__inline void		push_back(const T& val){ data[m_size++] = val; if(m_size==max) grow(); };
	__inline void		push_back(const T* valptr, UINT count)
	{
		while(m_size+count>=max) grow();
		for(UINT i = 0; i < count; i++) data[m_size++] = valptr[i];
	};
	__inline T&			back(){ return data[m_size-1]; }
	__inline UINT		size(){ return m_size; }
	__inline void		pop_back(){ m_size--; }
	__inline void		clear(){ m_size = 0; }
	__inline T&			operator[](UINT index){ return data[index]; }
	__inline void		resize(UINT newsize){ m_size = newsize; while(m_size>=max) grow(); }
	__inline void		reserve(UINT ressize){ while(ressize >= max) grow(); }
private:
	__inline void	grow(){ T* ndata = new T[max+(max>>1)]; memcpy(ndata, data, max*sizeof(T)); delete[] data; data=ndata; max=max+(max>>1); }
	T	*data;
	UINT	max, m_size;
};


class Executor
{
public:
	Executor(CommandList* cmds, std::vector<VariableInfo>* varinfo);
	~Executor();

	UINT	Run();
	string	GetResult();
	string	GetLog();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(UINT));
private:
	ofstream			m_FileStream;
	CommandList*		m_cmds;
	ostringstream		m_ostr;
	std::vector<VariableInfo>*	m_VarInfo;

	FastVector<UINT>	genStack;
	FastVector<asmStackType>	genStackTypes;
	FastVector<char>	genParams;
	FastVector<UINT>	paramTop;
	FastVector<CallStackInfo> callStack;

	bool (*m_RunCallback)(UINT);
};