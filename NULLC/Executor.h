#pragma once
#include "stdafx.h"
#include "ParseCommand.h"
#include "ParseClass.h"

template<typename T, bool zeroNewMemory = false>
class FastVector
{
public:
	FastVector(UINT reserved = 1000)
	{
		data = new T[reserved];
		if(zeroNewMemory)
			memset(data, 0, reserved);
		max = reserved;
		m_size = 0;
	}
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
	__inline void		shrink(UINT newSize){ m_size = newSize; }
	__inline void		reserve(UINT ressize){ while(ressize >= max) grow(); }
private:
	__inline void	grow()
	{
		T* ndata = new T[max+(max>>1)];
		if(zeroNewMemory)
			memset(ndata, 0, max+(max>>1));
		memcpy(ndata, data, max*sizeof(T));
		delete[] data;
		data=ndata;
		max=max+(max>>1);
	}
	T	*data;
	UINT	max, m_size;
};


class Executor
{
public:
	Executor();
	~Executor();

	UINT	Run(const char* funcName = NULL) throw();
	string	GetResult() throw();

	const char*	GetExecError();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(UINT));
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	ofstream			m_FileStream;
#endif
	char		execError[256];

	FastVector<asmStackType>	genStackTypes;

	FastVector<char, true>	genParams;
	FastVector<UINT>	paramTop;
	FastVector<char*>	fcallStack;

	UINT	*genStackBase;
	UINT	*genStackPtr;
	UINT	*genStackTop;

	OperFlag	retType;

	bool (*m_RunCallback)(UINT);
};

void PrintInstructionText(ostream* stream, CmdID cmd, UINT pos2, UINT valind, const CmdFlag cFlag, const OperFlag oFlag, UINT dw0=0, UINT dw1=0);
