#ifndef NULLC_REMOTE_INCLUDED
#define NULLC_REMOTE_INCLUDED

#pragma warning(disable: 4996) // disable warning C4996: 'name': This function or variable may be unsafe.

#if defined(_WIN32) || defined(_WIN64) 
	#include <process.h>
	#include <WinSock2.h>
	#pragma comment(lib, "Ws2_32.lib")
	#define NULLC_PROC_RETURN void
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <netinet/in.h>
	#include <netdb.h>

	#include <stdint.h>
	#include <setjmp.h>
	#include <signal.h>

	#include <pthread.h>

	#define SOCKET int
	#define SOCKADDR sockaddr
	#define closesocket close

	extern const char *__progname;
	static bool g_bPtrTestInstalled;
	static jmp_buf g_PtrTestJmpBuf;

	static void PtrTestHandler(int nSig)
	{
		if(g_bPtrTestInstalled)
			longjmp(g_PtrTestJmpBuf, 1);
	}

	static bool IsBadReadPtr(void* lp, unsigned int cb)
	{
		unsigned int i;
		unsigned char b1;
		bool bRet = true;
		void (* pfnPrevHandler)(int);
		g_bPtrTestInstalled = true;
		if(setjmp(g_PtrTestJmpBuf))
		{
			bRet = false;
		}else{
			pfnPrevHandler = signal(SIGSEGV, PtrTestHandler);

			for(i = 0; i < cb; i ++)
				b1 = ((unsigned char*)lp)[i];
		}
		g_bPtrTestInstalled = false;
		signal(SIGSEGV, pfnPrevHandler);

		return bRet;
	}

	static void Sleep(int length)
	{
		(void)length;
	}

	#define NULLC_PROC_RETURN void*
#endif

#define NULLC_SOCKET_ERROR (-1)

#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include "nullc_debug.h"

volatile int nullcFinished = 0;
SOCKET client;

#if defined(_WIN32) || defined(_WIN64) 
const char* nullcRemoteGetLastErrorDesc()
{
	char* msgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msgBuf), 0, NULL);
	return msgBuf;
}
void	nullcSleep()
{
	Sleep(20);
}
#else
const char* nullcRemoteGetLastErrorDesc()
{
	return "Error";
}
void	nullcSleep()
{
}
#endif

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

struct PipeDataReport
{
	unsigned int	pID;
	char			module[256];
};
struct PipeDataData
{
	unsigned int	wholeSize;
	unsigned int	dataSize;
	unsigned int	elemCount;
	char			data[512];
};
struct PipeDataDebug
{
	unsigned int	breakInst;
	bool			breakSet;
};

struct PipeData
{
	DebugCommand	cmd;
	bool			question;

	union
	{
		PipeDataReport report;
		PipeDataData data;
		PipeDataDebug debug;
	};
};

namespace Dispatcher
{
	short	serverPort = -1;
	char	*localIP = NULL;

	struct DispatchRecord
	{
		// Command
		DebugCommand	cmd;
		// Dispatcher will raise this event when 
		volatile int	*ready;
	};

	// This event should be raised after data processing is finished
	volatile int processed = 0;

	// This is the last data block
	PipeData data;

	// All records
	const unsigned int MAX_DISPATCH_CLIENTS = 128;
	DispatchRecord	records[MAX_DISPATCH_CLIENTS];
	unsigned int	recordCount;

	volatile int* DispatchRegister(DebugCommand event, volatile int* signal)
	{
		assert(recordCount < MAX_DISPATCH_CLIENTS);
		DispatchRecord &rec = records[recordCount++];
		rec.cmd = event;
		rec.ready = signal;
		return &processed;
	}

	PipeData GetData()
	{
		return data;
	}

	void DispatcherThread(void* param);
}

int SocketSend(SOCKET sck, char* source, size_t size, int timeOut)
{
	int allSize = (int)size;
	while(size)
	{
		int bytesSent;
		if((bytesSent = send(sck, source + (allSize - size), (int)size, 0)) == NULLC_SOCKET_ERROR || bytesSent == 0)
		{
			printf("send failed\n");
			return -1;
		}
		size -= bytesSent;
		if(size)
			printf("Partial send\n");
	}
	return allSize;
}
int SocketReceive(SOCKET sck, char* destination, size_t size, int timeOut)
{
	fd_set  fd = { 1, sck };
	timeval tv = { timeOut, 0 };

	int allSize = (int)size;
	while(size)
	{
		int bytesRecv;
		if((bytesRecv = recv(sck, destination + (allSize - size), (int)size, 0)) == NULLC_SOCKET_ERROR)
		{
			printf("recv failed\n");
			return -1;
		}
		if(!bytesRecv)
			return 0;
		size -= bytesRecv;
		if(size)
			printf("Partial recv\n");
	}
	return allSize;
}
int SocketIsReadable(SOCKET sck, int timeOut)
{
	fd_set  fd = { 1, sck };
	timeval tv = { timeOut, 0 };

	const int iRet = select(0, &fd, NULL, NULL, &tv);

	if(iRet == NULLC_SOCKET_ERROR)
		return -1;

	return iRet == 1;
}

void PipeSendData(SOCKET sck, PipeData &data, const char* start, unsigned int count, unsigned int whole)
{
	unsigned int left = whole;
	while(left)
	{
		data.question = false;
		data.data.dataSize = left > 512 ? 512 : left;
		data.data.wholeSize = whole;
		data.data.elemCount = count;
		memcpy(data.data.data, start + (whole - left), data.data.dataSize);
		left -= data.data.dataSize;
		int result = SocketSend(sck, (char*)&data, sizeof(data), 5);
		if(!result || result == -1)
			break;
	}
}

unsigned int csCount = 512;
unsigned int *stackFrames = NULL;

volatile int breakContinue = -1;
volatile int *breakProcessed = NULL;

unsigned int PipeDebugBreak(unsigned int instruction)
{
	if(breakContinue == -1)
	{
		// Register for event
		breakProcessed = Dispatcher::DispatchRegister(DEBUG_BREAK_CONTINUE, &breakContinue);
	}
	// Reset event
	breakContinue = 0;
	PipeData data;
	data.cmd = DEBUG_BREAK_HIT;
	data.question = false;
	data.debug.breakInst = instruction;

	SocketSend(client, (char*)&data, sizeof(data), 1000);

	unsigned int stackSize = 0;
	char *stackData = (char*)nullcGetVariableData(&stackSize);
	data.cmd = DEBUG_BREAK_STACK;
	data.question = false;
	PipeSendData(client, data, stackData, stackSize, stackSize);
	printf("DEBUG_BREAK_STACK %p (%d) %p\n", stackData, stackSize, stackData + stackSize);
	
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
	PipeSendData(client, data, (char*)stackFrames, count, count * sizeof(unsigned int));

	while(!breakContinue) nullcSleep(); breakContinue = 0;
	if(breakProcessed)
		*breakProcessed = 1;
	return NULLC_BREAK_PROCEED;
}

NULLC_PROC_RETURN GeneralCommandThread(void* param)
{
	volatile int ready = 0;
	// Register for events
	volatile int *processed = Dispatcher::DispatchRegister(DEBUG_REPORT_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_MODULE_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_MODULE_NAMES, &ready);
	Dispatcher::DispatchRegister(DEBUG_SOURCE_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_CODE_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_TYPE_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_VARIABLE_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_FUNCTION_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_LOCAL_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_TYPE_EXTRA_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_SYMBOL_INFO, &ready);
	Dispatcher::DispatchRegister(DEBUG_BREAK_SET, &ready);
	Dispatcher::DispatchRegister(DEBUG_BREAK_DATA, &ready);
	Dispatcher::DispatchRegister(DEBUG_DETACH, &ready);

	while(!param)
	{
		while(!ready) nullcSleep(); ready = 0;
		PipeData data = Dispatcher::GetData();
		switch(data.cmd)
		{
		case DEBUG_REPORT_INFO:
			printf("DEBUG_REPORT_INFO\n");
			data.question = false;
#ifdef __linux
			strcpy(data.report.module, /*__progname*/"test");
			data.report.pID = 10;
#else
			GetModuleFileName(NULL, data.report.module, 256);
			data.report.pID = GetCurrentProcessId();
#endif
			SocketSend(client, (char*)&data, sizeof(data), 5);
			break;
		case DEBUG_MODULE_INFO:
		{
			printf("DEBUG_MODULE_INFO\n");
			unsigned int count;
			ExternModuleInfo *modules = nullcDebugModuleInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)modules, count, count * sizeof(ExternModuleInfo));
			break;
		}
		case DEBUG_MODULE_NAMES:
		{
			printf("DEBUG_MODULE_NAMES\n");
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
			data.question = false;
			PipeSendData(client, data, names, size, size);
		}

			break;
			case DEBUG_SOURCE_INFO:
		{
			printf("DEBUG_SOURCE_INFO\n");
			unsigned int count;
			ExternModuleInfo *modules = nullcDebugModuleInfo(&count);
			char *source = nullcDebugSource();
			char *end = source + modules[count-1].sourceOffset + modules[count-1].sourceSize;
			end += strlen(end) + 1;
			data.question = false;
			PipeSendData(client, data, source, int(end-source), int(end-source));
		}
			break;
			case DEBUG_CODE_INFO:
		{
			printf("DEBUG_CODE_INFO\n");
			unsigned int count;
			NULLCCodeInfo *codeInfo = nullcDebugCodeInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)codeInfo, count, count * sizeof(NULLCCodeInfo));
		}
			break;
			case DEBUG_TYPE_INFO:
		{
			printf("DEBUG_TYPE_INFO\n");
			unsigned int count;
			ExternTypeInfo *typeInfo = nullcDebugTypeInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)typeInfo, count, count * sizeof(ExternTypeInfo));
		}
			break;
			case DEBUG_VARIABLE_INFO:
		{
			printf("DEBUG_VARIABLE_INFO\n");
			unsigned int count;
			ExternVarInfo *varInfo = nullcDebugVariableInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)varInfo, count, count * sizeof(ExternVarInfo));
		}
			break;
			case DEBUG_FUNCTION_INFO:
		{
			printf("DEBUG_FUNCTION_INFO\n");
			unsigned int count;
			ExternFuncInfo *funcInfo = nullcDebugFunctionInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)funcInfo, count, count * sizeof(ExternFuncInfo));
		}
			break;
			case DEBUG_LOCAL_INFO:
		{
			printf("DEBUG_LOCAL_INFO\n");
			unsigned int count;
			ExternLocalInfo *localInfo = nullcDebugLocalInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)localInfo, count, count * sizeof(ExternLocalInfo));
		}
			break;
			case DEBUG_TYPE_EXTRA_INFO:
		{
			printf("DEBUG_TYPE_EXTRA_INFO\n");
			unsigned int count;
			unsigned int *extraInfo = nullcDebugTypeExtraInfo(&count);
			data.question = false;
			PipeSendData(client, data, (char*)extraInfo, count, count * sizeof(unsigned int));
		}
			break;
			case DEBUG_SYMBOL_INFO:
		{
			printf("DEBUG_SYMBOL_INFO\n");
			unsigned int count;
			char *symbolInfo = nullcDebugSymbols(&count);
			data.question = false;
			PipeSendData(client, data, (char*)symbolInfo, count, count * sizeof(char));
		}
			break;
			case DEBUG_BREAK_SET:
		{
			printf("DEBUG_BREAK_SET %d %d\n", data.debug.breakInst, data.debug.breakSet);
			if(data.debug.breakSet)
				nullcDebugAddBreakpoint(data.debug.breakInst);
			else
				nullcDebugRemoveBreakpoint(data.debug.breakInst);
		}
			break;
			case DEBUG_DETACH:
		{
			printf("DEBUG_DETACH\n");
			nullcDebugClearBreakpoints();
			if(breakContinue == 0)
				breakContinue = 1;
		}
			break;
			case DEBUG_BREAK_DATA:
		{
			char *ptr = (char*)(intptr_t)data.data.dataSize;
			printf("DEBUG_BREAK_DATA %p (%d)\n", ptr, data.data.wholeSize);
			data.question = false;
			if(IsBadReadPtr(ptr, data.data.wholeSize) || !data.data.wholeSize)
				PipeSendData(client, data, "IsBadReadPtr!", 0, 14);
			else
				PipeSendData(client, data, ptr, data.data.wholeSize, data.data.wholeSize);
		}
			break;
		}
		*processed = 1;
	}
#ifdef __linux
	return NULL;
#endif
}

NULLC_PROC_RETURN DispatcherThread(void* param)
{
	nullcDebugSetBreakFunction(PipeDebugBreak);

	// Create a listening socket
	SOCKET sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	printf("%s\n", Dispatcher::localIP);

	sockaddr_in saServer;
	// Set up the sockaddr structure
	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = inet_addr(Dispatcher::localIP);
	saServer.sin_port = htons(Dispatcher::serverPort == -1 ? 7590 : Dispatcher::serverPort);

	int shift = 0;
	while(shift < 8)
	{
		// Bind the listening socket using the information in the sockaddr structure
		if(bind(sck, (SOCKADDR*)&saServer, sizeof(saServer)))
		{
			printf("%s\n", nullcRemoteGetLastErrorDesc());
			if(Dispatcher::serverPort == -1)
			{
				shift++;
				saServer.sin_port = htons(7590 + shift);
				continue;
			}
		}
		break;
	}
	while(1)
	{
		// Wait for connection
		if(listen(sck, 1))
			printf("%s\n", nullcRemoteGetLastErrorDesc());
		client = accept(sck, NULL, NULL);
		while(true)
		{
			if(nullcFinished)
			{
				closesocket(sck);
				nullcFinished = 0;
#ifdef __linux
				return NULL;
#else
				return;
#endif
			}
			PipeData data;
			timeval tv = { 1, 0 };

			fd_set	fdSet;
			FD_ZERO(&fdSet);
			FD_SET(client, &fdSet);

			int active = select(0, &fdSet, NULL, NULL, &tv);
			if(active == NULLC_SOCKET_ERROR)
			{
				printf("select NULLC_SOCKET_ERROR: %s\n", nullcRemoteGetLastErrorDesc());
				break;
			}
			if(!FD_ISSET(client, &fdSet))
				continue;

			int result = SocketReceive(client, (char*)&data, (int)sizeof(data), 5);
			if(result == 0 || result == -1)
			{
				nullcDebugClearBreakpoints();
				if(breakContinue == 0)
					breakContinue = 1;
				printf("Client disconnected\n");
				break;
			}
			Dispatcher::data = data;
			bool foundTarget = false;
			for(unsigned int i = 0; i < Dispatcher::recordCount; i++)
			{
				if(Dispatcher::records[i].cmd == data.cmd)
				{
					foundTarget = true;
					Dispatcher::processed = 0;
					*Dispatcher::records[i].ready = 1;
					while(!Dispatcher::processed) Sleep(5);
					Dispatcher::processed = 0;
				}
			}
			if(!foundTarget)
				printf("There is no receiver for the event %d\n", data.cmd);
		}
	}
#ifdef __linux
	return NULL;
#endif
}

volatile int* nullcEnableRemoteDebugging(const char *serverAddress, short serverPort)
{
#ifndef __linux
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif
	hostent* localHost;

	// Get the local host information
	localHost = gethostbyname(serverAddress);
	Dispatcher::localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);
	Dispatcher::serverPort = serverPort;

	Dispatcher::processed = 0;
#ifdef _WIN32
	_beginthread(DispatcherThread, 1024 * 1024, NULL);
	_beginthread(GeneralCommandThread, 1024 * 1024, NULL);
#else
	pthread_t dispThread, generalThread;
	pthread_create(&dispThread, NULL, DispatcherThread, NULL);
	pthread_create(&generalThread, NULL, GeneralCommandThread, NULL);
#endif
	nullcFinished = 0;
	return &nullcFinished;
}

#endif
