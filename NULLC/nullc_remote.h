#ifndef NULLC_REMOTE_INCLUDED
#define NULLC_REMOTE_INCLUDED

#pragma warning(disable: 4996) // disable warning C4996: 'name': This function or variable may be unsafe.

#define _WIN32_WINNT 0x0501
#include <Windows.h>
#include <stdio.h>
#include "nullc_debug.h"

HANDLE nullcFinished = INVALID_HANDLE_VALUE;
HANDLE pipeThread = INVALID_HANDLE_VALUE;
HANDLE pipe = INVALID_HANDLE_VALUE;
CRITICAL_SECTION	pipeSection;

char* nullcRemoteGetLastErrorDesc()
{
	char* msgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msgBuf), 0, NULL);
	return msgBuf;
}

enum DebugCommand
{
	DEBUG_REPORT_INFO,
	DEBUG_MODULE_INFO,
	DEBUG_MODULE_NAMES,
	DEBUG_SOURCE_INFO,
	DEBUG_TYPE_INFO,
	DEBUG_VARIABLE_INFO,
	DEBUG_FUNCTION_INFO,
	DEBUG_LOCAL_INFO,
	DEBUG_TYPE_EXTRA_INFO,
	DEBUG_SYMBOL_INFO,
	DEBUG_CODE_INFO,
	DEBUG_BREAK_SET,
	DEBUG_BREAK_HIT,
	DEBUG_BREAK_CONTINUE,
	DEBUG_BREAK_STACK,
	DEBUG_BREAK_CALLSTACK,
	DEBUG_BREAK_DATA,
	DEBUG_DETACH,
};

struct PipeData
{
	DebugCommand	cmd;
	bool			question;

	union
	{
		struct Report
		{
			unsigned int	pID;
			char			module[256];
		} report;
		struct Data
		{
			unsigned int	wholeSize;
			unsigned int	dataSize;
			unsigned int	elemCount;
			char			data[512];
		} data;
		struct Debug
		{
			unsigned int	breakInst;
			bool			breakSet;
		} debug;
	};
};

void PipeSendData(HANDLE pipe, PipeData &data, char* start, unsigned int count, unsigned int whole)
{
	unsigned int left = whole;
	EnterCriticalSection(&pipeSection);
	while(left)
	{
		data.question = false;
		data.data.dataSize = left > 512 ? 512 : left;
		data.data.wholeSize = whole;
		data.data.elemCount = count;
		memcpy(data.data.data, start + (whole - left), data.data.dataSize);
		left -= data.data.dataSize;
		DWORD size;
		DWORD good = WriteFile(pipe, &data, sizeof(data), &size, NULL);
		if(!good || !size)
			break;
	}
	LeaveCriticalSection(&pipeSection);
}

unsigned int csCount = 512;
unsigned int *stackFrames = NULL;

void PipeDebugBreak(unsigned int instruction)
{
	PipeData data;
	data.cmd = DEBUG_BREAK_HIT;
	data.question = false;
	data.debug.breakInst = instruction;
	DWORD size;
	EnterCriticalSection(&pipeSection);
	DWORD good = WriteFile(pipe, &data, sizeof(data), &size, NULL);
	LeaveCriticalSection(&pipeSection);
	if(!good || !size)
		return;

	unsigned int stackSize = 0;
	char *stackData = (char*)nullcGetVariableData(&stackSize);
	data.cmd = DEBUG_BREAK_STACK;
	data.question = false;
	PipeSendData(pipe, data, stackData, stackSize, stackSize);
	printf("DEBUG_BREAK_STACK %p (%d) %p\r\n", stackData, stackSize, stackData + stackSize);
	
	if(!stackFrames)
		stackFrames = new unsigned int[csCount];
	unsigned int count = 0;
	nullcDebugBeginCallStack();
	while(int address = nullcDebugGetStackFrame())
	{
		stackFrames[count++] = address;
		if(count == csCount)
		{
			csCount *= 2;
			unsigned int *newCS = new unsigned int[csCount];
			memcpy(newCS, stackFrames, count * sizeof(unsigned int));
			delete[] stackFrames;
			stackFrames = newCS;
		}
	}
	stackFrames[count++] = 0;

	data.cmd = DEBUG_BREAK_CALLSTACK;
	data.question = false;
	PipeSendData(pipe, data, (char*)stackFrames, count, count * sizeof(unsigned int));

	while(good)
	{
		EnterCriticalSection(&pipeSection);
		good = PeekNamedPipe(pipe, NULL, 0, NULL, &size, NULL);
		if(!good)
		{
			LeaveCriticalSection(&pipeSection);
			break;
		}
		if(!size)
		{
			LeaveCriticalSection(&pipeSection);
			Sleep(128);
			continue;
		}
		good = ReadFile(pipe, &data, sizeof(data), &size, NULL);
		LeaveCriticalSection(&pipeSection);
		if(!good || !size)
			return;
		if(data.cmd == DEBUG_BREAK_CONTINUE)
			return;
		if(data.cmd == DEBUG_DETACH)
		{
			printf("DEBUG_DETACH\r\n");
			nullcDebugClearBreakpoints();
			return;
		}
		if(data.cmd == DEBUG_BREAK_DATA)
		{
			char *ptr = (char*)(intptr_t)data.data.dataSize;
			printf("DEBUG_BREAK_DATA %p (%d)\r\n", ptr, data.data.wholeSize);
			if(IsBadReadPtr(ptr, data.data.wholeSize) || !data.data.wholeSize)
				PipeSendData(pipe, data, "IsBadReadPtr!", 0, 14);
			else
				PipeSendData(pipe, data, ptr, data.data.wholeSize, data.data.wholeSize);
			Sleep(128);
			continue;
		}
		EnterCriticalSection(&pipeSection);
		good = WriteFile(pipe, &data, sizeof(data), &size, NULL);
		LeaveCriticalSection(&pipeSection);
	}
}

DWORD WINAPI PipeThread(void* param)
{
	nullcDebugSetBreakFunction(PipeDebugBreak);

	if(!InitializeCriticalSectionAndSpinCount(&pipeSection, 0x80000400))
	{
		MessageBox(NULL, "Failed to create critical section for remote debugging", "Error", MB_OK);
		ExitThread(1);
	}

	// Create pipe with a free name
	unsigned int ID = 0;
	char pipeName[64];
	while(ID < 16)
	{
		sprintf(pipeName, "\\\\.\\pipe\\NULLC%d", ID);
		pipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024, 1024, 512/*NMPWAIT_USE_DEFAULT_WAIT*/, NULL);
		if(pipe != INVALID_HANDLE_VALUE)
			break;
		ID++;
	}
	if(pipe == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "All 16 debug pipes are being used", "Error", MB_OK);
		ExitThread(1);
	}
	while(1)
	{
		// Wait for connection
		if(!ConnectNamedPipe(pipe, NULL))
		{
			CloseHandle(pipe);
			printf("%s", nullcRemoteGetLastErrorDesc());
			ExitThread(1);
		}
		char buf[1024];
		while(true)
		{
			if(WaitForSingleObject(nullcFinished, 0) != WAIT_TIMEOUT)
			{
				FlushFileBuffers(pipe);
				DisconnectNamedPipe(pipe);
				CloseHandle(pipe);
				DeleteCriticalSection(&pipeSection);
				CloseHandle(nullcFinished);
				ExitThread(0);
			}
			PipeData data;
			DWORD size = 0, good = 0;
			EnterCriticalSection(&pipeSection);
			good = PeekNamedPipe(pipe, NULL, 0, NULL, &size, NULL);
			if(!good)
			{
				LeaveCriticalSection(&pipeSection);
				break;
			}
			if(!size)
			{
				LeaveCriticalSection(&pipeSection);
				Sleep(64);
				continue;
			}
			good = ReadFile(pipe, &data, sizeof(data), &size, NULL);
			LeaveCriticalSection(&pipeSection);
			if(!good || !size)
				break;
			switch(data.cmd)
			{
			case DEBUG_REPORT_INFO:
				printf("DEBUG_REPORT_INFO\r\n", buf);
				data.question = false;
				GetModuleFileName(NULL, data.report.module, 256);
				data.report.pID = GetCurrentProcessId();
				good = WriteFile(pipe, &data, sizeof(data), &size, NULL);
				break;
			case DEBUG_MODULE_INFO:
			{
				printf("DEBUG_MODULE_INFO\r\n");
				unsigned int count;
				ExternModuleInfo *modules = nullcDebugModuleInfo(&count);
				PipeSendData(pipe, data, (char*)modules, count, count * sizeof(ExternModuleInfo));
				break;
			}
			case DEBUG_MODULE_NAMES:
			{
				printf("DEBUG_MODULE_NAMES\r\n");
				unsigned int count;
				ExternModuleInfo *modules = nullcDebugModuleInfo(&count);
				char *symbols = nullcDebugSymbols(NULL);
				unsigned int size = 0;
				for(unsigned int i = 0; i < count; i++)
					size += (int)strlen(symbols + modules[i].nameOffset) + 1;
				char *names = new char[size], *pos = names;
				for(unsigned int i = 0; i < count; i++)
				{
					memcpy(pos, symbols + modules[i].nameOffset, strlen(symbols + modules[i].nameOffset) + 1);
					pos += strlen(symbols + modules[i].nameOffset) + 1;
				}
				PipeSendData(pipe, data, names, size, size);
			}
				break;
				case DEBUG_SOURCE_INFO:
			{
				printf("DEBUG_SOURCE_INFO\r\n");
				unsigned int count;
				ExternModuleInfo *modules = nullcDebugModuleInfo(&count);
				char *source = nullcDebugSource();
				char *end = source + modules[count-1].sourceOffset + modules[count-1].sourceSize;
				end += strlen(end) + 1;
				PipeSendData(pipe, data, source, int(end-source), int(end-source));
			}
				break;
				case DEBUG_CODE_INFO:
			{
				printf("DEBUG_CODE_INFO\r\n");
				unsigned int count;
				NULLCCodeInfo *codeInfo = nullcDebugCodeInfo(&count);
				PipeSendData(pipe, data, (char*)codeInfo, count, count * sizeof(NULLCCodeInfo));
			}
				break;
				case DEBUG_TYPE_INFO:
			{
				printf("DEBUG_TYPE_INFO\r\n");
				unsigned int count;
				ExternTypeInfo *typeInfo = nullcDebugTypeInfo(&count);
				PipeSendData(pipe, data, (char*)typeInfo, count, count * sizeof(ExternTypeInfo));
			}
				break;
				case DEBUG_VARIABLE_INFO:
			{
				printf("DEBUG_VARIABLE_INFO\r\n");
				unsigned int count;
				ExternVarInfo *varInfo = nullcDebugVariableInfo(&count);
				PipeSendData(pipe, data, (char*)varInfo, count, count * sizeof(ExternVarInfo));
			}
				break;
				case DEBUG_FUNCTION_INFO:
			{
				printf("DEBUG_FUNCTION_INFO\r\n");
				unsigned int count;
				ExternFuncInfo *funcInfo = nullcDebugFunctionInfo(&count);
				PipeSendData(pipe, data, (char*)funcInfo, count, count * sizeof(ExternFuncInfo));
			}
				break;
				case DEBUG_LOCAL_INFO:
			{
				printf("DEBUG_LOCAL_INFO\r\n");
				unsigned int count;
				ExternLocalInfo *localInfo = nullcDebugLocalInfo(&count);
				PipeSendData(pipe, data, (char*)localInfo, count, count * sizeof(ExternLocalInfo));
			}
				break;
				case DEBUG_TYPE_EXTRA_INFO:
			{
				printf("DEBUG_TYPE_EXTRA_INFO\r\n");
				unsigned int count;
				unsigned int *extraInfo = nullcDebugTypeExtraInfo(&count);
				PipeSendData(pipe, data, (char*)extraInfo, count, count * sizeof(unsigned int));
			}
				break;
				case DEBUG_SYMBOL_INFO:
			{
				printf("DEBUG_SYMBOL_INFO\r\n");
				unsigned int count;
				char *symbolInfo = nullcDebugSymbols(&count);
				PipeSendData(pipe, data, (char*)symbolInfo, count, count * sizeof(char));
			}
				break;
				case DEBUG_BREAK_SET:
			{
				printf("DEBUG_BREAK_SET %d %d\r\n", data.debug.breakInst, data.debug.breakSet);
				if(data.debug.breakSet)
					nullcDebugAddBreakpoint(data.debug.breakInst);
				else
					nullcDebugRemoveBreakpoint(data.debug.breakInst);
			}
				break;
				case DEBUG_DETACH:
			{
				printf("DEBUG_DETACH\r\n");
				nullcDebugClearBreakpoints();
			}
				break;
				case DEBUG_BREAK_DATA:
			{
				char *ptr = (char*)(intptr_t)data.data.dataSize;
				printf("DEBUG_BREAK_DATA %p (%d)\r\n", ptr, data.data.wholeSize);
				if(IsBadReadPtr(ptr, data.data.wholeSize) || !data.data.wholeSize)
					PipeSendData(pipe, data, "IsBadReadPtr!", 0, 14);
				else
					PipeSendData(pipe, data, ptr, data.data.wholeSize, data.data.wholeSize);
			}
				break;
				case DEBUG_BREAK_CONTINUE:
			{
				printf("DEBUG_BREAK_CONTINUE\r\n");
				EnterCriticalSection(&pipeSection);
				good = WriteFile(pipe, &data, sizeof(data), &size, NULL);
				LeaveCriticalSection(&pipeSection);
				Sleep(128);
			}
				break;
			}
			Sleep(64);
		}
		FlushFileBuffers(pipe);
		DisconnectNamedPipe(pipe);
	}
	ExitThread(1);
}

HANDLE nullcEnableRemoteDebugging()
{
	pipeThread = CreateThread(NULL, 1024 * 1024, PipeThread, NULL, NULL, 0);
	return nullcFinished = CreateEvent(NULL, false, false, "NULLC Finished execution Event");
}

#endif
