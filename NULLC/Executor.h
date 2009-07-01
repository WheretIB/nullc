#pragma once
#include "stdafx.h"
#include "ParseClass.h"

#include "Bytecode.h"

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

	__forceinline void		push_back(const T& val){ data[m_size++] = val; if(m_size==max) grow(m_size); };
	__forceinline void		push_back(const T* valptr, UINT count)
	{
		if(m_size+count>=max) grow(m_size+count);
		for(UINT i = 0; i < count; i++) data[m_size++] = valptr[i];
	};
	__forceinline T&		back(){ return data[m_size-1]; }
	__forceinline UINT		size(){ return m_size; }
	__forceinline void		pop_back(){ m_size--; }
	__forceinline void		clear(){ m_size = 0; }
	__forceinline T&		operator[](UINT index){ return data[index]; }
	__forceinline void		resize(UINT newsize){ m_size = newsize; if(m_size>=max) grow(m_size); }
	__forceinline void		shrink(UINT newSize){ m_size = newSize; }
	__forceinline void		reserve(UINT ressize){ if(ressize >= max) grow(ressize); }
private:
	__inline void	grow(unsigned int newSize)
	{
		if(max+(max>>1) > newSize)
			newSize = max+(max>>1);
		else
			newSize += 32;
		//assert(max+(max>>1) >= newSize);
		T* ndata = new T[newSize];
		if(zeroNewMemory)
			memset(ndata, 0, newSize);
		memcpy(ndata, data, max*sizeof(T));
		delete[] data;
		data=ndata;
		max=newSize;
	}
	T	*data;
	UINT	max, m_size;
};


class Executor
{
public:
	Executor();
	~Executor();

	void	CleanCode();
	bool	LinkCode(const char *bytecode, int redefinitions);

	void	Run(const char* funcName = NULL) throw();

	const char*	GetResult() throw();
	const char*	GetExecError() throw();

	char*	GetVariableData();

	void	SetCallback(bool (*Func)(UINT));
private:
#ifdef NULLC_VM_LOG_INSTRUCTION_EXECUTION
	ofstream			m_FileStream;
#endif
	char		execError[512];
	char		execResult[64];

	FastVector<ExternTypeInfo*>	exTypes;
	FastVector<ExternVarInfo*>	exVariables;
	FastVector<ExternFuncInfo*>	exFunctions;
	FastVector<char>			exCode;
	unsigned int				globalVarSize;
	unsigned int				offsetToGlobalCode;

	struct ExternalFunctionInfo
	{
#if defined(_MSC_VER)
		UINT bytesToPop;
#elif defined(__CELLOS_LV2__)
		unsigned int rOffsets[8];
		unsigned int fOffsets[8];
#endif
	};
	FastVector<ExternalFunctionInfo>	exFuncInfo;
	bool CreateExternalInfo(ExternFuncInfo *fInfo, Executor::ExternalFunctionInfo& externalInfo);

	FastVector<asmStackType>	genStackTypes;

	FastVector<char, true>	genParams;
	FastVector<UINT>	paramTop;
	FastVector<char*>	fcallStack;

	UINT	*genStackBase;
	UINT	*genStackPtr;
	UINT	*genStackTop;

	OperFlag	retType;

	bool (*m_RunCallback)(UINT);
	
	bool RunExternalFunction(unsigned int funcID);
};

void PrintInstructionText(ostream* stream, CmdID cmd, UINT pos2, UINT valind, const CmdFlag cFlag, const OperFlag oFlag, UINT dw0=0, UINT dw1=0);
