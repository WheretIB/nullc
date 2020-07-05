#include "stdafx.h"
#include "resource.h"

#ifdef _WIN64
	#define WND_PTR_TYPE LONG_PTR
#else
	#define WND_PTR_TYPE LONG
#endif

#pragma warning(disable: 4127)

#define _WIN32_WINNT 0x0501
#include <windows.h>

#include "commctrl.h"
#pragma comment(lib, "comctl32.lib")
#include <windowsx.h>

#pragma warning(disable: 4201)
#include <MMSystem.h>
#pragma comment(lib, "Winmm.lib")
#pragma warning(default: 4201)

#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

#include "../NULLC/nullc.h"
#include "../NULLC/nullbind.h"
#include "../NULLC/nullc_debug.h"
#include "../NULLC/StrAlgo.h"

#include "Colorer.h"

#include "GUI/RichTextarea.h"
#include "GUI/TabbedFiles.h"

// NULLC modules
#include "../NULLC/includes/file.h"
#include "../NULLC/includes/io.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/string.h"
#include "../NULLC/includes/vector.h"
#include "../NULLC/includes/random.h"
#include "../NULLC/includes/time.h"
#include "../NULLC/includes/gc.h"
#include "../NULLC/includes/memory.h"

#include "../NULLC/includes/window.h"

#include "../NULLC/includes/canvas.h"

#include "../NULLC/includes/pugi.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
char szTitle[MAX_LOADSTRING];
char szWindowClass[MAX_LOADSTRING];

WORD				MyRegisterClass(HINSTANCE hInstance);
bool				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, unsigned int, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, unsigned int, WPARAM, LPARAM);

// Window handles
HWND hWnd;			// Main window
HWND hButtonCalc;	// Run/Abort button
HWND hContinue;		// Button that continues an interrupted execution
HWND hShowTemporaries;	// Show temporary variables
HWND hExecutionType; // Target selection
HWND hTabs;	
HWND hNewTab, hNewFilename, hNewFile;
HWND hResult;		// label with execution result

HWND hDebugTabs;
HWND hCode;			// disabled text area for error messages and other information
HWND hVars;			// disabled text area that shows values of all variables in global scope
HWND hWatch;
HWND hStatus;		// Main window status bar

HWND hAttachPanel, hAttachList, hAttachDo, hAttachAdd, hAttachAddName, hAttachBack, hAttachTabs;
bool stateAttach = false, stateRemote = false;
CRITICAL_SECTION	pipeSection;

HWND mainCodeWnd = NULL;

unsigned int areaWidth = 400, areaHeight = 300;

HFONT	fontMonospace, fontDefault;

Colorer*	colorer;

struct Breakpoint
{
	HWND	tab;
	unsigned int	line;
	bool	valid;
};

std::vector<HWND>	richEdits;
std::vector<HWND>	attachedEdits;

const unsigned int INIT_BUFFER_SIZE = 4096;
char	initError[INIT_BUFFER_SIZE];

// for text update
bool needTextUpdate;
DWORD lastUpdate;

std::vector<unsigned>	baseModules;

//////////////////////////////////////////////////////////////////////////
// Remote debugging
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
HANDLE	pipeThread = INVALID_HANDLE_VALUE;

namespace RemoteData
{
	ExternModuleInfo	*modules = NULL;
	unsigned int		moduleCount = 0;
	char				*moduleNames = NULL;

	unsigned int		infoSize = 0;
	ExternSourceInfo	*sourceInfo = NULL;

	char				*sourceCode = NULL;

	unsigned int		varCount = 0;
	ExternVarInfo		*vars = NULL;

	unsigned int		typeCount = 0;
	ExternTypeInfo		*types = NULL;

	unsigned int		funcCount = 0;
	ExternFuncInfo		*functions = NULL;

	unsigned int		localCount = 0;
	ExternLocalInfo		*locals = NULL;

	ExternMemberInfo	*typeExtra = NULL;
	char				*symbols = NULL;

	char				*stackData = NULL;
	unsigned int		stackSize = 0;

	unsigned int		callStackFrames = 0;
	unsigned int		*callStack = NULL;

	hostent			*localHost;
	char			*localIP;
	SOCKET			sck;

	int SocketSend(SOCKET sck, char* source, size_t size, int timeOut)
	{
		TIMEVAL tv = { timeOut, 0 };
		fd_set	fdSet;
		FD_ZERO(&fdSet);
		FD_SET(sck, &fdSet);

		int active = select(0, NULL, &fdSet, NULL, &tv);
		if(active == SOCKET_ERROR)
		{
			printf("select failed\n");
			return -1;
		}
		if(!FD_ISSET(sck, &fdSet))
		{
			printf("!FD_ISSET\n");
			return 0;
		}
		
		int allSize = (int)size;
		while(size)
		{
			int bytesSent;
			if((bytesSent = send(sck, source + (allSize - size), (int)size, 0)) == SOCKET_ERROR || bytesSent == 0)
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
		TIMEVAL tv = { timeOut, 0 };
		fd_set	fdSet;
		FD_ZERO(&fdSet);
		FD_SET(sck, &fdSet);

		int active = select(0, &fdSet, NULL, NULL, &tv);
		if(active == SOCKET_ERROR)
		{
			printf("SocketReceive: select failed\n");
			return -1;
		}
		if(!FD_ISSET(sck, &fdSet))
		{
			printf("SocketReceive: select timeout\n");
			return 0;
		}

		int allSize = (int)size;
		while(size)
		{
			int bytesRecv;
			if((bytesRecv = recv(sck, destination + (allSize - size), (int)size, 0)) == SOCKET_ERROR || bytesRecv == 0)
			{
				printf("recv failed\n");
				return -1;
			}
			size -= bytesRecv;
			if(size)
				printf("Partial recv\n");
		}
		return allSize;
	}
}

char* GetLastErrorDesc()
{
	char* msgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msgBuf), 0, NULL);
	return msgBuf;
}

bool PipeSendRequest(PipeData &data)
{
	int result = RemoteData::SocketSend(RemoteData::sck, (char*)&data, sizeof(data), 5);
	if(!result || result == -1)
	{
		if(!result)
			MessageBox(hWnd, "Failed to send data through socket (select failed)", "ERROR", MB_OK);
		else
			MessageBox(hWnd, GetLastErrorDesc(), "Error", MB_OK);
		return false;
	}
	return true;
}

char* PipeReceiveResponce(PipeData &data)
{
	unsigned int recvSize = 0;
	DebugCommand cmd = data.cmd;
	char *rawData = NULL;
	do
	{
		int result = RemoteData::SocketReceive(RemoteData::sck, (char*)&data, sizeof(data), 5);
		if(!result || result == -1 || result != sizeof(data) || data.cmd != cmd || data.question)
		{
			delete[] rawData;
			if(!result)
				MessageBox(hWnd, "Failed to receive data through socket (select failed)", "ERROR", MB_OK);
			else if(result == -1)
				MessageBox(hWnd, GetLastErrorDesc(), "Error", MB_OK);
			else
				printf("%d != %d || %d != %d || %d\n", result, int(sizeof(data)), data.cmd, cmd, data.question);
			return NULL;
		}
		if(!recvSize)
			rawData = new char[data.data.wholeSize];
		memcpy(rawData + recvSize, data.data.data, data.data.dataSize);
		recvSize += data.data.dataSize;
	}while(recvSize < data.data.wholeSize);
	return rawData;
}

//////////////////////////////////////////////////////////////////////////

enum OverlayType
{
	OVERLAY_NONE,
	OVERLAY_CALLED,
	OVERLAY_STOP,
	OVERLAY_CURRENT,
	OVERLAY_BREAKPOINT,
	OVERLAY_BREAKPOINT_INVALID,
};
enum ExtraType
{
	EXTRA_NONE,
	EXTRA_BREAKPOINT,
	EXTRA_BREAKPOINT_INVALID,
};

void FillArrayVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update = false);
void FillComplexVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update = false);
void FillVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update = false);

double myGetPreciseTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return temp*1000.0;
}

const char* GetLastNullcErrorWindows()
{
	const unsigned formattedSize = 8192;
	static char formatted[formattedSize];

	const char *src = nullcGetLastError();
	char *dst = formatted;

	while(*src)
	{
		if(*src == '\n')
		{
			if(dst < formatted + formattedSize - 1)
				*dst++ = '\r';

			if(dst < formatted + formattedSize - 1)
				*dst++ = '\n';
		}
		else
		{
			if(dst < formatted + formattedSize - 1)
				*dst++ = *src;
		}

		src++;
	}

	*dst = 0;

	return formatted;
}

HANDLE breakResponse = NULL;
unsigned int	breakCommand = NULLC_BREAK_PROCEED, lastBreakCommand = NULLC_BREAK_PROCEED;
unsigned int	lastCodeLine = ~0u;
unsigned int	lastInstruction = ~0u;

void IDEDebugBreak()
{
	SendMessage(hWnd, WM_USER + 2, 0, 0);
	WaitForSingleObject(breakResponse, INFINITE);
}

unsigned IDEDebugBreakEx(void *context, unsigned instruction)
{
	(void)context;

	lastInstruction = instruction;
	lastBreakCommand = breakCommand;
	breakCommand = NULLC_BREAK_PROCEED;

	unsigned int codeLine = ~0u;

	unsigned int infoSize = stateRemote ? RemoteData::infoSize : 0;
	ExternSourceInfo *sourceInfo = stateRemote ? RemoteData::sourceInfo : nullcDebugSourceInfo(&infoSize);

	unsigned int moduleSize = stateRemote ? RemoteData::moduleCount : 0;
	ExternModuleInfo *modules = stateRemote ? RemoteData::modules : nullcDebugModuleInfo(&moduleSize);

	nullcDebugBeginCallStack();
	int address = -1;
	while(int nextAddress = nullcDebugGetStackFrame())
		address = nextAddress;
	if(address != -1)
	{
		unsigned int line = 0;
		unsigned int i = address - 1;
		while((line < infoSize - 1) && (i >= sourceInfo[line + 1].instruction))
			line++;
		if(sourceInfo[line].sourceOffset >= modules[moduleSize-1].sourceOffset + modules[moduleSize-1].sourceSize)
		{
			const char *source = RichTextarea::GetAreaText(mainCodeWnd);
			codeLine = 0;
			const char *curr = source, *end = source + sourceInfo[line].sourceOffset - modules[moduleSize-1].sourceOffset - modules[moduleSize-1].sourceSize;
			while(const char *next = strchr(curr, '\n'))
			{
				if(next > end)
					break;
				curr = next + 1;
				codeLine++;
			}
		}else{
			const char *fullSource = nullcDebugSource();
			// Find module, where execution stopped
			unsigned int module = 0;
			for(module = 0; module < moduleSize; module++)
			{
				if(sourceInfo[line].sourceOffset >= modules[module].sourceOffset && sourceInfo[line].sourceOffset < modules[module].sourceOffset + modules[module].sourceSize)
					break;
			}
			codeLine = 0;
			const char *curr = fullSource + modules[module].sourceOffset, *end = fullSource + sourceInfo[line].sourceOffset;
			while(const char *next = strchr(curr, '\n'))
			{
				if(next > end)
					break;
				curr = next + 1;
				codeLine++;
			}
		}
	}
	if(codeLine == lastCodeLine && lastBreakCommand != NULLC_BREAK_PROCEED)
	{
		breakCommand = lastBreakCommand;
		return lastBreakCommand;
	}

	SendMessage(hWnd, WM_USER + 2, 0, 0);
	WaitForSingleObject(breakResponse, INFINITE);
	return breakCommand;
}

#include <io.h>

int APIENTRY WinMain(HINSTANCE	hInstance,
					HINSTANCE	hPrevInstance,
					LPTSTR		lpCmdLine,
					int			nCmdShow)
{
	(void)lpCmdLine;
	(void)hPrevInstance;

	MSG msg;

	needTextUpdate = true;
	lastUpdate = GetTickCount();

#ifdef _DEBUG
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	freopen("CONIN$", "r", stdin);

	_dup2(_dup(_fileno(stdin)), 0);
	_dup2(_dup(_fileno(stdout)), 1);
	_dup2(_dup(_fileno(stderr)), 2);
#endif

	nullcInit();
	nullcAddImportPath("Modules/");
	nullcAddImportPath("../Modules/");

#ifdef _DEBUG
	nullcSetEnableTimeTrace(1);
#endif

	nullcSetEnableExternalDebugger(1);

	char modulePath[MAX_PATH];
	GetModuleFileName(NULL, modulePath, MAX_PATH);

	memset(initError, 0, INIT_BUFFER_SIZE);

	// in possible, load precompiled modules from nullclib.ncm
	FILE *modulePack = fopen(sizeof(void*) == sizeof(int) ? "nullclib.ncm" : "nullclib_x64.ncm", "rb");
	if(!modulePack)
	{
		strcat(initError, "WARNING: Failed to open precompiled module file ");
		strcat(initError, sizeof(void*) == sizeof(int) ? "nullclib.ncm\r\n" : "nullclib_x64.ncm\r\n");
	}else{
		fseek(modulePack, 0, SEEK_END);
		unsigned int fileSize = ftell(modulePack);
		fseek(modulePack, 0, SEEK_SET);
		char *fileContent = new char[fileSize];
		fread(fileContent, 1, fileSize, modulePack);
		fclose(modulePack);

		char *filePos = fileContent;
		while((unsigned int)(filePos - fileContent) < fileSize)
		{
			char *moduleName = filePos;
			filePos += strlen(moduleName) + 1;
			char *binaryCode = filePos;
			filePos += *(unsigned int*)binaryCode;
			nullcLoadModuleByBinary(moduleName, binaryCode);
		}

		delete[] fileContent;
	}

	if(!nullcInitTypeinfoModule())
		strcat(initError, "ERROR: Failed to init std.typeinfo module\r\n");
	if(!nullcInitDynamicModule())
		strcat(initError, "ERROR: Failed to init std.dynamic module\r\n");

	if(!nullcInitFileModule())
		strcat(initError, "ERROR: Failed to init std.file module\r\n");
	if(!nullcInitIOModule())
		strcat(initError, "ERROR: Failed to init std.io module\r\n");
	if(!nullcInitMathModule())
		strcat(initError, "ERROR: Failed to init std.math module\r\n");
	if(!nullcInitStringModule())
		strcat(initError, "ERROR: Failed to init std.string module\r\n");

	if(!nullcInitCanvasModule())
		strcat(initError, "ERROR: Failed to init img.canvas module\r\n");
	if(!nullcInitWindowModule())
		strcat(initError, "ERROR: Failed to init win.window module\r\n");

	if(!nullcInitVectorModule())
		strcat(initError, "ERROR: Failed to init old.vector module\r\n");
	if(!nullcInitRandomModule())
		strcat(initError, "ERROR: Failed to init std.random module\r\n");
	if(!nullcInitTimeModule())
		strcat(initError, "ERROR: Failed to init std.time module\r\n");
	if(!nullcInitGCModule())
		strcat(initError, "ERROR: Failed to init std.gc module\r\n");
	if(!nullcInitMemoryModule())
		strcat(initError, "ERROR: Failed to init std.memory module\r\n");

	if(!nullcInitPugiXMLModule())
		strcat(initError, "ERROR: Failed to init ext.pugixml module\r\n");

	nullcLoadModuleBySource("ide.debug", "void _debugBreak();");
	nullcBindModuleFunctionHelper("ide.debug", IDEDebugBreak, "_debugBreak", 0);

	// Save a list of base modules
	while(const char* moduleName = nullcEnumerateModules((unsigned)baseModules.size()))
		baseModules.push_back(NULLC::GetStringHash(moduleName));

	colorer = NULL;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_NULLC_IDE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	lastUpdate = 0;
	// Perform application initialization:
	if(!InitInstance(hInstance, nCmdShow)) 
		return 0;

	if(!nullcDebugSetBreakFunction(NULL, IDEDebugBreakEx))
		strcat(initError, GetLastNullcErrorWindows());

	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);

	// Get the local host information
	RemoteData::localHost = gethostbyname("localhost");
	RemoteData::localIP = inet_ntoa(*(struct in_addr *)*RemoteData::localHost->h_addr_list);

	if(!InitializeCriticalSectionAndSpinCount(&pipeSection, 0x80000400))
		strcat(initError, "Failed to create critical section for remote debugging");

	HACCEL hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDR_SHORTCUTS);

	// Main message loop:
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!TranslateAccelerator(hWnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	delete colorer;
	DeleteCriticalSection(&pipeSection);

	nullcTerminate();

	return (int) msg.wParam;
}

WORD MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_NULLC_IDE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= (LPCTSTR)IDC_NULLC_IDE;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

void AddTabWithFile(const char* filename, HINSTANCE hInstance)
{
	char buf[1024];
	char *file = NULL;
	GetFullPathName(filename, 1024, buf, &file);

	FILE *startText = fopen(buf, "rb");
	char *fileContent = NULL;
	if(!startText)
		return;

	richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, hInstance, NULL));
	TabbedFiles::AddTab(hTabs, buf, richEdits.back());
	ShowWindow(richEdits.back(), SW_HIDE);
	
	fseek(startText, 0, SEEK_END);
	unsigned int textSize = ftell(startText);
	fseek(startText, 0, SEEK_SET);
	fileContent = new char[textSize+1];
	fread(fileContent, 1, textSize, startText);
	fileContent[textSize] = 0;
	fclose(startText);

	RichTextarea::SetAreaText(richEdits.back(), fileContent ? fileContent : "");
	delete[] fileContent;

	RichTextarea::BeginStyleUpdate(richEdits.back());
	colorer->ColorText(richEdits.back(), (char*)RichTextarea::GetAreaText(richEdits.back()), RichTextarea::SetStyleToSelection);
	RichTextarea::EndStyleUpdate(richEdits.back());
	RichTextarea::UpdateArea(richEdits.back());
	RichTextarea::ResetUpdate(richEdits.back());
}

bool SaveFileFromTab(const char *file, const char *data, HWND editWnd)
{
	FILE *fSave = fopen(file, "wb");
	if(!fSave)
	{
		if(file[0] != '?')
			MessageBox(hWnd, "File cannot be saved", "Warning", MB_OK);
		return false;
	}else{
		fwrite(data, 1, strlen(data), fSave);
		fclose(fSave);
	}
	RichTextarea::ValidateHistory(editWnd);
	return true;
}

void CloseTabWithFile(TabbedFiles::TabInfo &info)
{
	if(info.dirty && info.last[0] != '?' && MessageBox(hWnd, "File was changed. Save changes?", "Warning", MB_YESNO) == IDYES)
	{
		SaveFileFromTab(info.name, RichTextarea::GetAreaText(info.window), info.window);
	}
	DestroyWindow(info.window);
	for(unsigned int i = 0; i < richEdits.size(); i++)
	{
		if(richEdits[i] == info.window)
		{
			richEdits.erase(richEdits.begin() + i);
			break;
		}
	}
}

void TextareaToggleBreakpoint(HWND area, RichTextarea::LineIterator it)
{
	(void)it;
	(void)area;
	SendMessage(hWnd, WM_COMMAND, ID_TOGGLE_BREAK, 0);
}

bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 100, 100, 900, 450, NULL, NULL, hInstance, NULL);
	if(!hWnd)
		return 0;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	fontMonospace = CreateFont(-9 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Courier New");
	fontDefault = CreateFont(-10 * GetDeviceCaps(hdc, LOGPIXELSY) / 72, 0, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Arial");
	EndPaint(hWnd, &ps);

	hButtonCalc = CreateWindow("BUTTON", "Run", WS_VISIBLE | WS_CHILD, 5, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	if(!hButtonCalc)
		return 0;
	SendMessage(hButtonCalc, WM_SETFONT, (WPARAM)fontDefault, 0);

	hContinue = CreateWindow("BUTTON", "Continue", WS_CHILD, 110, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	if(!hContinue)
		return 0;
	SendMessage(hContinue, WM_SETFONT, (WPARAM)fontDefault, 0);

	hExecutionType = CreateWindow("COMBOBOX", "TEST", WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD, 800-140, 185, 130, 300, hWnd, NULL, hInstance, NULL);
	if(!hExecutionType)
		return 0;
	SendMessage(hExecutionType, WM_SETFONT, (WPARAM)fontDefault, 0);

	ComboBox_AddString(hExecutionType, "VM");
	ComboBox_AddString(hExecutionType, "JiT");

#if defined(NULLC_LLVM_SUPPORT)
	ComboBox_AddString(hExecutionType, "LLVM");
#else
	ComboBox_AddString(hExecutionType, "LLVM (Not Available)");
#endif

	ComboBox_AddString(hExecutionType, "Expression Eval");
	ComboBox_AddString(hExecutionType, "Instruction Eval");

	ComboBox_SetCurSel(hExecutionType, 0);

	hShowTemporaries = CreateWindow("BUTTON", "Show temps", WS_VISIBLE | BS_AUTOCHECKBOX | WS_CHILD, 800-280, 185, 130, 30, hWnd, NULL, hInstance, NULL);
	if(!hShowTemporaries)
		return 0;
	SendMessage(hShowTemporaries, WM_SETFONT, (WPARAM)fontDefault, 0);

	INITCOMMONCONTROLSEX commControlTypes;
	commControlTypes.dwSize = sizeof(INITCOMMONCONTROLSEX);
	commControlTypes.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES;
	int commControlsAvailable = InitCommonControlsEx(&commControlTypes);
	if(!commControlsAvailable)
		return 0;

	hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE, "Ready", hWnd, 0);

	TabbedFiles::RegisterTabbedFiles("NULLCTABS", hInstance);
	RichTextarea::RegisterTextarea("NULLCTEXT", hInstance);

	colorer = new Colorer();

	hTabs = CreateWindow("NULLCTABS", "tabs", WS_VISIBLE | WS_CHILD, 5, 4, 800, 20, hWnd, 0, hInstance, 0);
	if(!hTabs)
		return 0;

	hAttachTabs = CreateWindow("NULLCTABS", "tabs", WS_CHILD, 5, 4, 800, 20, hWnd, 0, hInstance, 0);
	if(!hAttachTabs)
		return 0;

	hDebugTabs = CreateWindow("NULLCTABS", "tabs", WS_VISIBLE | WS_CHILD, 5, 225, 800, 20, hWnd, 0, hInstance, 0);
	if(!hDebugTabs)
		return 0;

	// Load tab information
	FILE *tabInfo = fopen("nullc_tab.cfg", "rb");
	if(!tabInfo)
	{
		AddTabWithFile("main.nc", hInstance);
		if(richEdits.empty())
		{
			richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, hInstance, NULL));
			TabbedFiles::AddTab(hTabs, "main.nc", richEdits.back());
			ShowWindow(richEdits.back(), SW_HIDE);
			RichTextarea::SetAreaText(richEdits.back(), "");
		}
	}else{
		fseek(tabInfo, 0, SEEK_END);
		unsigned int textSize = ftell(tabInfo);
		fseek(tabInfo, 0, SEEK_SET);
		char *fileContent = new char[textSize+1];
		fread(fileContent, 1, textSize, tabInfo);
		fileContent[textSize] = 0;
		fclose(tabInfo);

		char *start = fileContent;
		while(char *end = strchr(start, '\r'))
		{
			*end = 0;
			AddTabWithFile(start, hInstance);
			start = end + 2;
		}
		delete[] fileContent;
	}
	FILE *undoStorage = fopen("nullc_undo.bin", "rb");
	if(undoStorage)
	{
		int ptrSize = 0;
		fread(&ptrSize, sizeof(int), 1, undoStorage);
		if(ptrSize == sizeof(void*))
		{
			unsigned nameLength = 0;
			while(fread(&nameLength, sizeof(unsigned), 1, undoStorage))
			{
				char buf[1024];
				assert(nameLength < 1024);
				fread(buf, 1, nameLength, undoStorage);
				
				bool loaded = false;
				// Find window
				for(unsigned i = 0; i < richEdits.size(); i++)
				{
					if(strcmp(TabbedFiles::GetTabInfo(hTabs, i).name, buf) != 0)
						continue;
					RichTextarea::LoadHistory(TabbedFiles::GetTabInfo(hTabs, i).window, undoStorage);
					loaded = true;
				}
				if(!loaded)
					RichTextarea::LoadHistory(NULL, undoStorage);
			}
		}
		fclose(undoStorage);
	}

	TabbedFiles::SetOnCloseTab(hTabs, CloseTabWithFile);
	TabbedFiles::SetNewTabWindow(hTabs, hNewTab = CreateWindow("STATIC", "", WS_CHILD | SS_GRAYFRAME, 5, 25, 780, 175, hWnd, NULL, hInstance, NULL));
	HWND createPanel = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD, 5, 5, 190, 60, hNewTab, NULL, hInstance, NULL);
	/*HWND panel0 = */CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDFRAME, 0, 0, 190, 60, createPanel, NULL, hInstance, NULL);
	HWND panel1 = CreateWindow("STATIC", "File name: ", WS_VISIBLE | WS_CHILD, 5, 5, 100, 20, createPanel, NULL, hInstance, NULL);
	hNewFilename = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD, 80, 5, 100, 20, createPanel, NULL, hInstance, NULL);
	hNewFile = CreateWindow("BUTTON", "Create", WS_VISIBLE | WS_CHILD, 5, 30, 100, 25, createPanel, NULL, hInstance, NULL);

	SetWindowLongPtr(createPanel, GWLP_WNDPROC, (WND_PTR_TYPE)(intptr_t)WndProc);

	SendMessage(panel1, WM_SETFONT, (WPARAM)fontDefault, 0);
	SendMessage(hNewFilename, WM_SETFONT, (WPARAM)fontDefault, 0);
	SendMessage(hNewFile, WM_SETFONT, (WPARAM)fontDefault, 0);

	UpdateWindow(hTabs);

	if(!richEdits.empty())
	{
		ShowWindow(richEdits[0], SW_SHOW);
		SetFocus(richEdits[0]);
		RichTextarea::ScrollToLine(richEdits[0], 0);
	}else{
		ShowWindow(hNewTab, SW_SHOW);
		SetFocus(hNewTab);
	}

	RichTextarea::SetStatusBar(hStatus, 900);

	RichTextarea::SetTextStyle(   COLOR_CODE,    0,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(  COLOR_RWORD,    0,   0, 255, false, false, false);
	RichTextarea::SetTextStyle(    COLOR_VAR,  128, 128, 128, false, false, false);
	RichTextarea::SetTextStyle( COLOR_VARDEF,   50,  50,  50, false, false, false);
	RichTextarea::SetTextStyle(   COLOR_FUNC,  136,   0,   0, false,  true, false);
	RichTextarea::SetTextStyle(   COLOR_TEXT,    0,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(   COLOR_BOLD,    0,   0,   0,  true, false, false);
	RichTextarea::SetTextStyle(   COLOR_CHAR,  136,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(   COLOR_REAL,    0, 150,   0, false, false, false);
	RichTextarea::SetTextStyle(    COLOR_INT,    0, 150,   0, false,  true, false);
	RichTextarea::SetTextStyle(    COLOR_ERR, 255,   0,   0, false, false,  true);
	RichTextarea::SetTextStyle(COLOR_COMMENT, 255,   0, 255, false, false, false);

	RichTextarea::SetLineStyle(OVERLAY_CALLED, LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CALL)), "This code has called into another function");
	RichTextarea::SetLineStyle(OVERLAY_STOP, LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LASTCALL)), "Code execution stopped at this point");
	RichTextarea::SetLineStyle(OVERLAY_CURRENT, LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CURR)), "Code execution is currently at this point");
	RichTextarea::SetLineStyle(OVERLAY_BREAKPOINT, LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BREAK)), "Breakpoint");
	RichTextarea::SetLineStyle(OVERLAY_BREAKPOINT_INVALID, LoadBitmap(hInst, MAKEINTRESOURCE(IDB_UNREACHABLE)), "Breakpoint is never reached");

	RichTextarea::SetTooltipClickCallback(TextareaToggleBreakpoint);

	unsigned int width = (800 - 25) / 4;

	hCode = CreateWindow("EDIT", initError, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		5, 225, width*2, 165, hWnd, NULL, hInstance, NULL);
	if(!hCode)
		return 0;
	SendMessage(hCode, WM_SETFONT, (WPARAM)fontMonospace, 0);

	hVars = CreateWindow(WC_TREEVIEW, "", WS_CHILD | WS_BORDER | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS,
		3*width+15, 225, width, 165, hWnd, NULL, hInstance, NULL);
	if(!hVars)
		return 0;

	hWatch = CreateWindow(WC_TREEVIEW, "", WS_CHILD | WS_BORDER | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS,
		3*width+15, 225, width, 165, hWnd, NULL, hInstance, NULL);
	if(!hWatch)
		return 0;

	TabbedFiles::AddTab(hDebugTabs, "Output", hCode);
	TabbedFiles::AddTab(hDebugTabs, "Variable info", hVars);
	TabbedFiles::AddTab(hDebugTabs, "Watch", hWatch);

	hResult = CreateWindow("STATIC", "The result will be here", WS_CHILD, 110, 185, 300, 30, hWnd, NULL, hInstance, NULL);
	if(!hResult)
		return 0;
	ShowWindow(hResult, nCmdShow);
	UpdateWindow(hResult);
	SendMessage(hResult, WM_SETFONT, (WPARAM)fontDefault, 0);

	PostMessage(hWnd, WM_SIZE, 0, (394 << 16) + (900 - 16));

	hAttachPanel = CreateWindow("STATIC", "", WS_CHILD | SS_ETCHEDFRAME, 5, 5, 190, 60, hWnd, NULL, hInstance, NULL);
	hAttachList = CreateWindow(WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | LVS_REPORT, 10, 10, 500, 400, hAttachPanel, NULL, hInstance, NULL);

	hAttachDo = CreateWindow("BUTTON", "Attach to process", WS_CHILD, 5, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	SendMessage(hAttachDo, WM_SETFONT, (WPARAM)fontDefault, 0);

	hAttachAdd = CreateWindow("BUTTON", "Add address", WS_CHILD, 5, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	SendMessage(hAttachAdd, WM_SETFONT, (WPARAM)fontDefault, 0);

	hAttachAddName = CreateWindow("EDIT", "127.0.0.1", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 5, 225, width*2, 165, hWnd, NULL, hInstance, NULL);
	SendMessage(hAttachAddName, WM_SETFONT, (WPARAM)fontMonospace, 0);

	hAttachBack = CreateWindow("BUTTON", "Cancel", WS_CHILD, 110, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	SendMessage(hAttachBack, WM_SETFONT, (WPARAM)fontDefault, 0);

	ListView_SetExtendedListViewStyle(hAttachList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);// | LVS_SHOWSELALWAYS);

	LVCOLUMN lvColumn;
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.iSubItem = 0;
	lvColumn.pszText = "Pipe";
	lvColumn.cx = 200;
	lvColumn.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hAttachList, 0, &lvColumn);

	lvColumn.iSubItem = 1;
	lvColumn.pszText = "Application";
	lvColumn.cx = 400;
	lvColumn.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hAttachList, 1, &lvColumn);

	lvColumn.iSubItem = 2;
	lvColumn.pszText = "PID";
	lvColumn.cx = 50;
	lvColumn.fmt = LVCFMT_LEFT;
	ListView_InsertColumn(hAttachList, 2, &lvColumn);

	for(unsigned int i = 0; i < 16; i++)
	{
		LVITEM lvItem;
		lvItem.mask = LVIF_TEXT | LVIF_STATE;
		lvItem.state = 0;
		lvItem.stateMask = 0;

		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.pszText = "";
		ListView_InsertItem(hAttachList, &lvItem);
	}

	SetTimer(hWnd, 1, 100, 0);

	return TRUE;
}

// zero-terminated safe sprintf
int	safeprintf(char* dst, size_t size, const char* src, ...)
{
	if(size == 0)
		return 0;

	va_list args;
	va_start(args, src);

	int result = vsnprintf(dst, size, src, args);
	dst[size-1] = '\0';

	va_end(args);

	return result;
}

ExternVarInfo	*codeVars = NULL;
unsigned int	codeTypeCount = 0;
ExternTypeInfo	*codeTypes = NULL;
ExternFuncInfo	*codeFunctions = NULL;
ExternLocalInfo	*codeLocals = NULL;
ExternMemberInfo*codeTypeExtra = NULL;
char			*codeSymbols = NULL;

struct TreeItemExtra
{
	TreeItemExtra():address(NULL), type(NULL), item(NULL), expandable(false), name(NULL){}
	TreeItemExtra(void* a, const ExternTypeInfo* t, HTREEITEM i, bool e, const char *n = NULL):address(a), type(t), item(i), expandable(e), name(n){}

	void			*address;
	const ExternTypeInfo	*type;
	HTREEITEM		item;
	bool			expandable;
	const char		*name;
};
std::vector<TreeItemExtra>	tiExtra;
std::vector<TreeItemExtra>	tiWatch;
std::vector<char*> externalBlocks;

const char* GetBasicVariableInfo(const ExternTypeInfo& type, char* ptr)
{
	static char val[256];

	switch(type.type)
	{
	case ExternTypeInfo::TYPE_CHAR:
		if(codeSymbols[type.offsetToName] == 'b')
		{
			safeprintf(val, 256, *(unsigned char*)ptr ? "true" : "false");
		}else{
			if(strcmp(codeSymbols + type.offsetToName, "uchar") == 0)
				safeprintf(val, 256, "'%c' (%u)", *(unsigned char*)ptr, (int)*(unsigned char*)ptr);
			else if(*(unsigned char*)ptr)
				safeprintf(val, 256, "'%c' (%d)", *(unsigned char*)ptr, (int)*(char*)ptr);
			else
				safeprintf(val, 256, "0");
		}
		break;
	case ExternTypeInfo::TYPE_SHORT:
		if(strcmp(codeSymbols + type.offsetToName, "ushort") == 0)
			safeprintf(val, 256, "%u", *(unsigned short*)ptr);
		else
			safeprintf(val, 256, "%d", *(short*)ptr);
		break;
	case ExternTypeInfo::TYPE_INT:
		if(type.subCat == ExternTypeInfo::CAT_CLASS)
		{
			const char *memberName = codeSymbols + type.offsetToName + (unsigned int)strlen(codeSymbols + type.offsetToName) + 1;

			for(unsigned i = 0; i < type.constantCount && i < *(unsigned*)ptr; i++)
				memberName += (unsigned)strlen(memberName) + 1;

			safeprintf(val, 256, "%s (%d)", memberName, *(int*)ptr);
		}
		else
		{
			safeprintf(val, 256, type.subType == 0 ? (strcmp(codeSymbols + type.offsetToName, "uint") == 0 ? "%u" : "%d") : "0x%x", *(int*)ptr);
		}
		break;
	case ExternTypeInfo::TYPE_LONG:
		safeprintf(val, 256, type.subType == 0 ? (strcmp(codeSymbols + type.offsetToName, "ulong") == 0 ? "%llu" : "%lld") : "0x%llx", *(long long*)ptr);
		break;
	case ExternTypeInfo::TYPE_FLOAT:
		safeprintf(val, 256, "%f", *(float*)ptr);
		break;
	case ExternTypeInfo::TYPE_DOUBLE:
		safeprintf(val, 256, "%f", *(double*)ptr);
		break;
	default:
		safeprintf(val, 256, "not basic type");
	}
	return val;
}

void FillArrayVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	HTREEITEM child = update ? TreeView_GetChild(hWatch, parent) : NULL;

	char name[256];
	HTREEITEM lastItem;

	ExternTypeInfo	&subType = codeTypes[type.subType];
	unsigned int arrSize = (type.arrSize == ~0u) ? *(unsigned int*)(ptr + 4) : type.arrSize;
	if(type.arrSize == ~0u)
	{
		arrSize = *(unsigned int*)(ptr + 4);
		TVITEM item;
		item.mask = TVIF_PARAM;
		item.lParam = tiExtra.size();
		item.hItem = parent;
		tiExtra.push_back(TreeItemExtra((void*)ptr, &type, parent, true));
		TreeView_SetItem(hVars, &item);
		return;
	}
	if(type.defaultAlign == 0xff)
	{
		safeprintf(name, 256, "base: %p", ptr);
		helpInsert.item.mask = TVIF_TEXT;
		helpInsert.item.pszText = name;
		HTREEITEM lastItem;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);
	}
	for(unsigned int i = 0; i < arrSize; i++, ptr += subType.size)
	{
		if(i > 100)
			break;
		char *it = name;
		memset(name, 0, 256);

		it += safeprintf(it, 256 - int(it - name), "[%d]: ", i);

		bool simpleType = subType.subCat == ExternTypeInfo::CAT_NONE || (subType.subCat == ExternTypeInfo::CAT_CLASS && subType.type != ExternTypeInfo::TYPE_COMPLEX);
		bool pointerType = subType.subCat == ExternTypeInfo::CAT_POINTER;

		if(&subType == &codeTypes[NULLC_TYPE_TYPEID])
			it += safeprintf(it, 256 - int(it - name), " = %s", *(unsigned*)(ptr) < codeTypeCount ? codeSymbols + codeTypes[*(int*)(ptr)].offsetToName : "invalid: out of range");
		else if(simpleType || pointerType)
			it += safeprintf(it, 256 - int(it - name), " %s", GetBasicVariableInfo(subType, ptr));

		helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
		helpInsert.item.pszText = name;
		helpInsert.item.cChildren = pointerType ? I_CHILDRENCALLBACK : (simpleType ? 0 : 1);
		helpInsert.item.lParam = pointerType ? tiExtra.size() : 0;
		if(pointerType)
			tiExtra.push_back(TreeItemExtra());

		HTREEITEM lastItem;
		if(update)
		{
			helpInsert.item.hItem = child;
			TreeView_SetItem(hWatch, &helpInsert.item);
			lastItem = child;
		}else{
			lastItem = TreeView_InsertItem(hVars, &helpInsert);
			if(pointerType)
				tiExtra.back() = TreeItemExtra((void*)ptr, &subType, lastItem, true);
		}

		FillVariableInfo(subType, ptr, lastItem);
		if(update)
			child = TreeView_GetNextSibling(hWatch, child);
	}
	if(arrSize > 100)
	{
		safeprintf(name, 256, "[101]-[%d]...", arrSize);
		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);
	}
}

void FillComplexVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	HTREEITEM child = update ? TreeView_GetChild(hWatch, parent) : NULL;

	char name[256];

	if(type.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE)
	{
		char *it = name;
		memset(name, 0, 256);

		ExternTypeInfo &memberType = codeTypes[NULLC_TYPE_TYPEID];

		it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + memberType.offsetToName, "$typeid");
		it += safeprintf(it, 256 - int(it - name), " = %s", *(unsigned*)(ptr) < codeTypeCount ? codeSymbols + codeTypes[*(int*)(ptr)].offsetToName : "invalid: out of range");

		helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
		helpInsert.item.pszText = name;
		helpInsert.item.cChildren = 0;
		helpInsert.item.lParam = 0;

		HTREEITEM lastItem;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);
	}

	const char *memberName = codeSymbols + type.offsetToName + (unsigned int)strlen(codeSymbols + type.offsetToName) + 1;
	for(unsigned int i = 0; i < type.memberCount; i++)
	{
		char *it = name;
		memset(name, 0, 256);

		ExternTypeInfo &memberType = codeTypes[codeTypeExtra[type.memberOffset + i].type];

		unsigned localOffset = codeTypeExtra[type.memberOffset + i].offset;

		it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + memberType.offsetToName, memberName);

		bool simpleType = memberType.subCat == ExternTypeInfo::CAT_NONE || (memberType.subCat == ExternTypeInfo::CAT_CLASS && memberType.type != ExternTypeInfo::TYPE_COMPLEX);
		bool pointerType = memberType.subCat == ExternTypeInfo::CAT_POINTER;

		if(&memberType == &codeTypes[NULLC_TYPE_TYPEID])
			it += safeprintf(it, 256 - int(it - name), " = %s", *(unsigned*)(ptr + localOffset) < codeTypeCount ? codeSymbols + codeTypes[*(int*)(ptr + localOffset)].offsetToName : "invalid: out of range");
		else if(simpleType || pointerType)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(memberType, ptr + localOffset));

		helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
		helpInsert.item.pszText = name;
		helpInsert.item.cChildren = pointerType ? I_CHILDRENCALLBACK : (simpleType ? 0 : 1);
		helpInsert.item.lParam = pointerType ? tiExtra.size() : 0;
		if(pointerType)
			tiExtra.push_back(TreeItemExtra());
	
		HTREEITEM lastItem;
		if(update)
		{
			helpInsert.item.hItem = child;
			TreeView_SetItem(hWatch, &helpInsert.item);
			lastItem = child;
		}else{
			lastItem = TreeView_InsertItem(hVars, &helpInsert);
			if(pointerType)
				tiExtra.back() = TreeItemExtra((void*)(ptr + localOffset), &memberType, lastItem, true);
		}

		FillVariableInfo(memberType, ptr + localOffset, lastItem);

		memberName += (unsigned int)strlen(memberName) + 1;

		if(update)
			child = TreeView_GetNextSibling(hWatch, child);
	}
}

void FillAutoInfo(char* ptr, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;
	char name[256];

	safeprintf(name, 256, "typeid type = %d (%s)", *(int*)ptr, codeSymbols + codeTypes[*(int*)(ptr)].offsetToName);
	helpInsert.item.pszText = name;
	TreeView_InsertItem(hVars, &helpInsert);

	safeprintf(name, 256, "%s ref ptr = 0x%x", codeSymbols + codeTypes[*(int*)(ptr)].offsetToName, *(int*)(ptr + 4));

	// Find parent type
	ExternTypeInfo *parentType = NULL;
	for(unsigned int i = 0; i < codeTypeCount && !parentType; i++)
	{
		if(codeTypes[i].subCat == ExternTypeInfo::CAT_POINTER && codeTypes[i].subType == *(unsigned int*)(ptr))
			parentType = &codeTypes[i];
	}

	helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
	helpInsert.item.pszText = name;
	helpInsert.item.cChildren = I_CHILDRENCALLBACK;
	helpInsert.item.lParam = tiExtra.size();
	tiExtra.push_back(TreeItemExtra());
	HTREEITEM lastItem = TreeView_InsertItem(hVars, &helpInsert);
	tiExtra.back() = TreeItemExtra((void*)(ptr + 4), parentType, lastItem, true);
}

void FillAutoArrayInfo(char* ptr, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;
	char name[256];

	NULLCAutoArray *arr = (NULLCAutoArray*)ptr;

	safeprintf(name, 256, "typeid type = %d (%s)", arr->typeID, codeSymbols + codeTypes[arr->typeID].offsetToName);
	helpInsert.item.pszText = name;
	TreeView_InsertItem(hVars, &helpInsert);

	safeprintf(name, 256, "%s[] data = 0x%x", codeSymbols + codeTypes[arr->typeID].offsetToName, arr->ptr);

	if(!arr->ptr)
		return;
	ExternTypeInfo parentType;
	memset(&parentType, 0, sizeof(ExternTypeInfo));
	parentType.arrSize = arr->len;
	parentType.subType = arr->typeID;

	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.pszText = name;
	HTREEITEM lastItem = TreeView_InsertItem(hVars, &helpInsert);
	FillArrayVariableInfo(parentType, arr->ptr, lastItem);
}

void InsertUnavailableInfo(HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;
	helpInsert.item.pszText = "Cannot be evaluated";
	TreeView_InsertItem(hVars, &helpInsert);
}

void FillFunctionPointerInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;
	char name[256];

	ExternFuncInfo	&func = codeFunctions[*(int*)(ptr + NULLC_PTR_SIZE)];
	ExternTypeInfo	&returnType = codeTypes[codeTypeExtra[type.memberOffset].type];

	char *it = name;
	it += safeprintf(it, 256 - int(it - name), "function %d %s %s(", *(int*)(ptr + NULLC_PTR_SIZE), codeSymbols + returnType.offsetToName, codeSymbols + func.offsetToName);
	for(unsigned int arg = 0; arg < func.paramCount; arg++)
	{
		ExternLocalInfo &lInfo = codeLocals[func.offsetToFirstLocal + arg];
		it += safeprintf(it, 256 - int(it - name), "%s %s%s", codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName, arg == func.paramCount - 1 ? "" : ", ");
	}
	it += safeprintf(it, 256 - int(it - name), ")");

	helpInsert.item.pszText = name;
	TreeView_InsertItem(hVars, &helpInsert);

	safeprintf(name, 256, "%s context = 0x%x", func.contextType == ~0u ? "void ref" : codeSymbols + codeTypes[func.contextType].offsetToName, *(int*)(ptr));
	helpInsert.item.pszText = name;
	HTREEITEM contextList = TreeView_InsertItem(hVars, &helpInsert);

	TVINSERTSTRUCT nextInsert;
	nextInsert.hParent = contextList;
	nextInsert.hInsertAfter = TVI_LAST;
	nextInsert.item.mask = TVIF_TEXT;
	nextInsert.item.cchTextMax = 0;

	ExternFuncInfo::Upvalue *upvalue = *(ExternFuncInfo::Upvalue**)(ptr);
	if(func.externCount && (!upvalue || !upvalue->ptr))
	{
		InsertUnavailableInfo(contextList);
		return;
	}
	if(func.contextType != ~0u)
		FillVariableInfo(codeTypes[codeTypes[func.contextType].subType], (char*)upvalue, contextList, false);
}

void FillVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent, bool update)
{
	if(&type == &codeTypes[NULLC_TYPE_TYPEID])
		return;

	if(&type == &codeTypes[NULLC_TYPE_AUTO_REF])
	{
		FillAutoInfo(ptr, parent);
		return;
	}

	if(&type == &codeTypes[NULLC_TYPE_AUTO_ARRAY])
	{
		FillAutoArrayInfo(ptr, parent);
		return;
	}

	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_FUNCTION:
		FillFunctionPointerInfo(type, ptr, parent);
		break;
	case ExternTypeInfo::CAT_CLASS:
		FillComplexVariableInfo(type, ptr, parent, update);
		break;
	case ExternTypeInfo::CAT_ARRAY:
		FillArrayVariableInfo(type, ptr, parent, update);
		break;
	}
}

unsigned int FillVariableInfoTree(bool lastIsCurrent = false)
{
	TreeView_DeleteAllItems(hVars);

	bool showTemps = !!Button_GetCheck(hShowTemporaries);

	tiExtra.clear();
	tiExtra.push_back(TreeItemExtra(NULL, NULL, 0, false));

	TVINSERTSTRUCT	helpInsert;
	helpInsert.hParent = NULL;
	helpInsert.hInsertAfter = TVI_ROOT;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	unsigned int	dataCount = ~0u;
	char	*data = stateRemote ? RemoteData::stackData : (char*)nullcGetVariableData(&dataCount);

	unsigned int	variableCount	= stateRemote ? RemoteData::varCount : 0;
	unsigned int	functionCount	= stateRemote ? RemoteData::funcCount : 0;
	codeTypeCount = stateRemote ? RemoteData::typeCount : 0;
	codeVars		= stateRemote ? RemoteData::vars : nullcDebugVariableInfo(&variableCount);
	codeTypes		= stateRemote ? RemoteData::types : nullcDebugTypeInfo(&codeTypeCount);
	codeFunctions	= stateRemote ? RemoteData::functions : nullcDebugFunctionInfo(&functionCount);
	codeLocals		= stateRemote ? RemoteData::locals : nullcDebugLocalInfo(NULL);
	codeTypeExtra	= stateRemote ? RemoteData::typeExtra : nullcDebugTypeExtraInfo(NULL);
	codeSymbols		= stateRemote ? RemoteData::symbols : nullcDebugSymbols(NULL);

	char name[256];
	unsigned int offset = 0;

	for(unsigned int i = 0; i < variableCount; i++)
	{
		ExternTypeInfo &type = codeTypes[codeVars[i].type];

		char *it = name;
		memset(name, 0, 256);
		it += safeprintf(it, 256 - int(it - name), "0x%x: %s %s", data + codeVars[i].offset, codeSymbols + type.offsetToName, codeSymbols + codeVars[i].offsetToName);

		bool simpleType = type.subCat == ExternTypeInfo::CAT_NONE || (type.subCat == ExternTypeInfo::CAT_CLASS && type.type != ExternTypeInfo::TYPE_COMPLEX);
		bool pointerType = type.subCat == ExternTypeInfo::CAT_POINTER;

		if(&type == &codeTypes[NULLC_TYPE_TYPEID])
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(data + codeVars[i].offset)].offsetToName);
		else if(simpleType || pointerType)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(type, data + codeVars[i].offset));

		if(showTemps || (strstr(name, "$temp") == 0 && strstr(name, "$vtbl") == 0))
		{
			helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
			helpInsert.item.pszText = name;
			helpInsert.item.cChildren = pointerType ? I_CHILDRENCALLBACK : (simpleType ? 0 : 1);
			helpInsert.item.lParam = tiExtra.size();

			tiExtra.push_back(TreeItemExtra());
			HTREEITEM lastItem = TreeView_InsertItem(hVars, &helpInsert);
			tiExtra.back() = TreeItemExtra(data + codeVars[i].offset, &type, lastItem, pointerType, codeSymbols + codeVars[i].offsetToName);

			FillVariableInfo(type, data + codeVars[i].offset, lastItem);
		}

		if(codeVars[i].offset + type.size > offset)
			offset = codeVars[i].offset + type.size;
	}

	unsigned int codeLine = ~0u;
	HWND lastWindow = mainCodeWnd;
	unsigned int retLine = ~0u;

	unsigned int infoSize = stateRemote ? RemoteData::infoSize : 0;
	ExternSourceInfo *sourceInfo = stateRemote ? RemoteData::sourceInfo : nullcDebugSourceInfo(&infoSize);

	unsigned int moduleSize = stateRemote ? RemoteData::moduleCount : 0;
	ExternModuleInfo *modules = stateRemote ? RemoteData::modules : nullcDebugModuleInfo(&moduleSize);

	if(!mainCodeWnd && stateRemote)
		mainCodeWnd = TabbedFiles::GetTabInfo(hAttachTabs, TabbedFiles::GetCurrentTab(hAttachTabs)).window;

	unsigned int id = ~0u;
	const char *source = RichTextarea::GetAreaText(mainCodeWnd);
	const char *fullSource = stateRemote ? RemoteData::sourceCode : nullcDebugSource();

	unsigned int csPos = 0;
	if(!stateRemote)
		nullcDebugBeginCallStack();
	while(int address = stateRemote ? RemoteData::callStack[csPos++] : nullcDebugGetStackFrame())
	{
		// Find corresponding function
		ExternFuncInfo *func = nullcDebugConvertAddressToFunction(address - 1, codeFunctions, functionCount);

		if(address != -1)
		{
			unsigned int line = 0;
			unsigned int i = address - 1;
			while((line < infoSize - 1) && (i >= sourceInfo[line + 1].instruction))
				line++;
			if(!moduleSize || (sourceInfo[line].sourceOffset >= modules[moduleSize-1].sourceOffset + modules[moduleSize-1].sourceSize))
			{
				codeLine = 0;
				const char *curr = source, *end = source + sourceInfo[line].sourceOffset - (moduleSize ? modules[moduleSize-1].sourceOffset + modules[moduleSize-1].sourceSize : 0);
				while(const char *next = strchr(curr, '\n'))
				{
					if(next > end)
						break;
					curr = next + 1;
					codeLine++;
				}
				RichTextarea::SetStyleToLine(mainCodeWnd, codeLine, OVERLAY_CALLED);
				lastWindow = mainCodeWnd;
				retLine = codeLine;
				for(unsigned int t = 0; t < (stateRemote ? attachedEdits : richEdits).size(); t++)
				{
					if(TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, t).window == mainCodeWnd)
					{
						id = t;
						break;
					}
				}
			}else{
				// Find module, where execution stopped
				unsigned int module = 0;
				for(module = 0; module < moduleSize; module++)
				{
					if(sourceInfo[line].sourceOffset >= modules[module].sourceOffset && sourceInfo[line].sourceOffset < modules[module].sourceOffset + modules[module].sourceSize)
						break;
				}
				// Create module name (with prefix so that file couldn't be saved)
				char moduleName[256];
				safeprintf(moduleName, 256, "?%s", codeSymbols + modules[module].nameOffset);
				// Find tab
				unsigned int targetTab = ~0u;
				for(unsigned int t = 0; t < (stateRemote ? attachedEdits : richEdits).size() && targetTab == ~0u; t++)
				{
					const char *tabName = TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, t).last;
					if(strcmp(tabName, strrchr(moduleName, '/') ? strrchr(moduleName, '/') + 1 : moduleName + 1) == 0)
						targetTab = t;
					if(strcmp(tabName, moduleName) == 0)
						targetTab = t;
				}
				// If tab not found, open it
				if(targetTab == ~0u)
				{
					richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, NULL, NULL));
					TabbedFiles::AddTab(stateRemote ? hAttachTabs : hTabs, moduleName, (stateRemote ? attachedEdits : richEdits).back());
					ShowWindow((stateRemote ? attachedEdits : richEdits).back(), SW_HIDE);
					RichTextarea::SetAreaText((stateRemote ? attachedEdits : richEdits).back(), fullSource + modules[module].sourceOffset);
					targetTab = (unsigned int)(stateRemote ? attachedEdits : richEdits).size()-1;
				}
				id = targetTab;
				codeLine = 0;
				const char *curr = fullSource + modules[module].sourceOffset, *end = fullSource + sourceInfo[line].sourceOffset;
				while(const char *next = strchr(curr, '\n'))
				{
					if(next > end)
						break;
					curr = next + 1;
					codeLine++;
				}
				HWND updateWnd = TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, targetTab).window;
				RichTextarea::SetStyleToLine(updateWnd, codeLine, OVERLAY_CALLED);
				RichTextarea::ScrollToLine(updateWnd, codeLine);
				RichTextarea::UpdateArea(updateWnd);
				printf("Breakpoint is in the %s module (%d) %d, line %d\n", codeSymbols + modules[module].nameOffset, module, targetTab, codeLine);
				lastWindow = updateWnd;
				retLine = codeLine;
			}
		}

		// If we are not in global scope
		if(func)
		{
			ExternFuncInfo	&function = *func;

			// Align offset to the first variable (by 16 byte boundary)
			int alignOffset = (offset % 16 != 0) ? (16 - (offset % 16)) : 0;
			offset += alignOffset;

			char *it = name;
			it += safeprintf(it, 256 - int(it - name), "0x%x: function %s(", data + offset, codeSymbols + function.offsetToName);
			for(unsigned int arg = 0; arg < function.paramCount; arg++)
			{
				ExternLocalInfo &lInfo = codeLocals[function.offsetToFirstLocal + arg];
				it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);
				if(arg != function.paramCount - 1)
					it += safeprintf(it, 256 - int(it - name), ", ");
			}
			it += safeprintf(it, 256 - int(it - name), ")");

			helpInsert.item.mask = TVIF_TEXT;
			helpInsert.item.pszText = name;
			HTREEITEM lastItem = TreeView_InsertItem(hVars, &helpInsert);

			unsigned int offsetToNextFrame = function.bytesToPop;
			// Check every function local
			for(unsigned int i = 0; i < function.localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = codeLocals[function.offsetToFirstLocal + i];
				if(function.funcCat == ExternFuncInfo::COROUTINE && lInfo.offset >= function.bytesToPop)
					break;

				char *it = name;
				it += safeprintf(it, 256, "0x%x: %s %s", data + offset + lInfo.offset, codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);

				bool simpleType = codeTypes[lInfo.type].subCat == ExternTypeInfo::CAT_NONE || (codeTypes[lInfo.type].subCat == ExternTypeInfo::CAT_CLASS && codeTypes[lInfo.type].type != ExternTypeInfo::TYPE_COMPLEX);
				bool pointerType = codeTypes[lInfo.type].subCat == ExternTypeInfo::CAT_POINTER;

				if(simpleType || pointerType)
					it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(codeTypes[lInfo.type], data + offset + lInfo.offset));
				else if(lInfo.type == 8)	// for typeid
					it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(data + offset + lInfo.offset)].offsetToName);

				if(showTemps || (strstr(codeSymbols + lInfo.offsetToName, "$temp") == 0 && strstr(codeSymbols + lInfo.offsetToName, "$vtbl") == 0))
				{
					TVINSERTSTRUCT localInfo;
					localInfo.hParent = lastItem;
					localInfo.hInsertAfter = TVI_LAST;
					localInfo.item.cchTextMax = 0;
					localInfo.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
					localInfo.item.pszText = name;
					localInfo.item.cChildren = pointerType ? I_CHILDRENCALLBACK : (simpleType ? 0 : 1);
					localInfo.item.lParam = tiExtra.size();
				
					tiExtra.push_back(TreeItemExtra());
					HTREEITEM thisItem = TreeView_InsertItem(hVars, &localInfo);
					tiExtra.back() = TreeItemExtra((void*)(data + offset + lInfo.offset), &codeTypes[lInfo.type], thisItem, pointerType, codeSymbols + lInfo.offsetToName);

					if(offset + lInfo.offset + lInfo.size > dataCount)
						InsertUnavailableInfo(thisItem);
					else
						FillVariableInfo(codeTypes[lInfo.type], data + offset + lInfo.offset, thisItem);
				}

				if(lInfo.offset + lInfo.size > offsetToNextFrame)
					offsetToNextFrame = lInfo.offset + lInfo.size;
			}
			if(function.parentType != ~0u)
			{
				char *ptr = (char*)(data + offset + function.bytesToPop - NULLC_PTR_SIZE);

				char *it = name;
				it += safeprintf(it, 256, "0x%x: %s %s = %p", ptr, "$this", codeSymbols + codeTypes[function.parentType].offsetToName, *(char**)ptr);

				TVINSERTSTRUCT localInfo;
				localInfo.hParent = lastItem;
				localInfo.hInsertAfter = TVI_LAST;
				localInfo.item.cchTextMax = 0;
				localInfo.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
				localInfo.item.pszText = name;
				localInfo.item.cChildren = codeTypes[function.parentType].subCat == ExternTypeInfo::CAT_POINTER ? I_CHILDRENCALLBACK : (codeTypes[function.parentType].subCat == ExternTypeInfo::CAT_NONE ? 0 : 1);
				localInfo.item.lParam = tiExtra.size();
				
				tiExtra.push_back(TreeItemExtra());
				HTREEITEM thisItem = TreeView_InsertItem(hVars, &localInfo);
				tiExtra.back() = TreeItemExtra(*(char**)ptr, &codeTypes[function.parentType], thisItem, true, "$this");

				if(offset + function.bytesToPop > dataCount)
					InsertUnavailableInfo(thisItem);
			}
			if(function.contextType != ~0u)
			{
				char *ptr = (char*)(data + offset + function.bytesToPop - NULLC_PTR_SIZE);

				char *it = name;
				it += safeprintf(it, 256, "0x%x: %s %s = %p", ptr, "$context", codeSymbols + codeTypes[function.contextType].offsetToName, *(char**)ptr);

				TVINSERTSTRUCT localInfo;
				localInfo.hParent = lastItem;
				localInfo.hInsertAfter = TVI_LAST;
				localInfo.item.cchTextMax = 0;
				localInfo.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
				localInfo.item.pszText = name;
				localInfo.item.cChildren = codeTypes[function.contextType].subCat == ExternTypeInfo::CAT_POINTER ? I_CHILDRENCALLBACK : (codeTypes[function.contextType].subCat == ExternTypeInfo::CAT_NONE ? 0 : 1);
				localInfo.item.lParam = tiExtra.size();
				
				tiExtra.push_back(TreeItemExtra());
				HTREEITEM thisItem = TreeView_InsertItem(hVars, &localInfo);
				tiExtra.back() = TreeItemExtra((void*)ptr, &codeTypes[function.contextType], thisItem, true, "$context");

				if(offset + function.bytesToPop > dataCount)
					InsertUnavailableInfo(thisItem);
			}
			offset += offsetToNextFrame;
		}
	}

	if(id != ~0u)
		TabbedFiles::SetCurrentTab(stateRemote ? hAttachTabs : hTabs, id);
	if(codeLine != ~0u)
	{
		RichTextarea::SetStyleToLine(lastWindow, codeLine, lastIsCurrent ? OVERLAY_CURRENT : OVERLAY_STOP);
		RichTextarea::ScrollToLine(lastWindow, codeLine);
	}
	RichTextarea::UpdateArea(lastWindow);
	return retLine;
}

void UpdateWatchedVariables()
{
	char name[256];
	HTREEITEM elem = TreeView_GetRoot(hWatch);
	while(elem)
	{
		TVITEMEX item;
		memset(&item, 0, sizeof(item));
		item.mask = TVIF_HANDLE | TVIF_PARAM;
		item.hItem = elem;
		TreeView_GetItem(hVars, &item);

		item.mask = TVIF_TEXT;
		item.hItem = elem;
		item.cchTextMax = 0;
		
		TreeItemExtra extra = tiWatch[item.lParam];
		const ExternTypeInfo &type = *extra.type;

		char *it = name;
		memset(name, 0, 256);
		it += safeprintf(it, 256 - int(it - name), "0x%x: %s %s", extra.address, codeSymbols + type.offsetToName, extra.name);

		bool simpleType = type.subCat == ExternTypeInfo::CAT_NONE || (type.subCat == ExternTypeInfo::CAT_CLASS && type.type != ExternTypeInfo::TYPE_COMPLEX);
		bool pointerType = type.subCat == ExternTypeInfo::CAT_POINTER;

		if(&type == &codeTypes[NULLC_TYPE_TYPEID])
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(extra.address)].offsetToName);
		else if(simpleType || pointerType)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(type, (char*)extra.address));

		item.pszText = name;
		TreeView_SetItem(hWatch, &item);
		FillVariableInfo(type, (char*)extra.address, elem, true);
		elem = TreeView_GetNextSibling(hWatch, elem);
	}
}

struct RunResult
{
	bool	finished;
	nullres	result;
	double	time;
	HWND	wnd;
} runRes;
HANDLE calcThread = INVALID_HANDLE_VALUE;

DWORD WINAPI CalcThread(void* param)
{
	RunResult &rres = *(RunResult*)param;
	rres.finished = false;
	rres.result = false;
	double time = myGetPreciseTime();
	nullres goodRun = nullcRunFunction(NULL);
	rres.time = myGetPreciseTime() - time;
	rres.result = goodRun;
	if(goodRun)
	{
		const char *val = nullcGetResult();

		SetWindowText(hResult, "Finalizing...");
		nullcFinalize();

		char	result[1024];
		_snprintf(result, 1024, "The answer is: %s [in %f]", val, runRes.time);
		result[1023] = '\0';
		SetWindowText(hResult, result);
	}
	rres.finished = true;
	SendMessage(rres.wnd, WM_USER + 1, 0, 0);
	ExitThread(goodRun);
}

void RefreshBreakpoints()
{
	unsigned int id = TabbedFiles::GetCurrentTab(stateRemote ? hAttachTabs : hTabs);
	HWND wnd = TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, id).window;
	for(unsigned int i = 0; i < (stateRemote ? attachedEdits.size() : richEdits.size()); i++)
	{
		RichTextarea::LineIterator it = RichTextarea::GetFirstLine(stateRemote ? attachedEdits[i] : richEdits[i]);
		while(it.line)
		{
			if(it.GetExtra() == 1)
				it.SetStyle(OVERLAY_BREAKPOINT);
			if(it.GetExtra() == 2)
				it.SetStyle(OVERLAY_BREAKPOINT_INVALID);
			it.GoForward();
		}
	}
	RichTextarea::UpdateArea(wnd);
	RichTextarea::ResetUpdate(wnd);
}

FastVector<unsigned>	breakPos;
FastVector<unsigned>	byteCodePos;

unsigned int ConvertPositionToInstruction(unsigned int relPos, unsigned int infoSize, ExternSourceInfo* sourceInfo, unsigned int &sourceOffset)
{
	breakPos.clear();
	byteCodePos.clear();
	// Find instruction...
	unsigned int lastDistance = ~0u;
	for(unsigned int infoID = 0; infoID < infoSize; infoID++)
	{
		if(sourceInfo[infoID].sourceOffset >= relPos && (unsigned int)(sourceInfo[infoID].sourceOffset - relPos) <= lastDistance)
		{
			breakPos.push_back(infoID);
			lastDistance = (unsigned int)(sourceInfo[infoID].sourceOffset - relPos);
		}
	}
	// Filter results so that only the best matches remain
	for(unsigned int i = 0; i < breakPos.size(); i++)
	{
		if((unsigned int)(sourceInfo[breakPos[i]].sourceOffset - relPos) <= lastDistance)
		{
			byteCodePos.push_back(sourceInfo[breakPos[i]].instruction);
			sourceOffset = sourceInfo[breakPos[i]].sourceOffset;
		}
	}
	return byteCodePos.size();
}

unsigned int ConvertLineToInstruction(const char *source, unsigned int line, const char* fullSource, unsigned int infoSize, ExternSourceInfo* sourceInfo, unsigned int moduleSize, ExternModuleInfo *modules)
{
	// Find source code position for this line
	unsigned int origLine = line;
	const char *pos = source;
	while(line-- && NULL != (pos = strchr(pos, '\n')))
		pos++;
	if(pos)
	{
		// Get relative position
		unsigned int relPos = (unsigned int)(pos - source);
		// Find module, where this source code is contained
		unsigned int shiftToLastModule = modules[moduleSize-1].sourceOffset + modules[moduleSize-1].sourceSize;
		for(unsigned int module = 0; module < moduleSize; module++)
		{
			if(strcmp(source, fullSource + modules[module].sourceOffset) == 0)
			{
				relPos += modules[module].sourceOffset;
				shiftToLastModule = 0;
				break;
			}
		}
		// Move position to start of main module
		if(shiftToLastModule && strcmp(source, fullSource + modules[moduleSize-1].sourceOffset + modules[moduleSize-1].sourceSize) == 0)
			relPos += shiftToLastModule;
		unsigned int offset = ~0u;
		unsigned int matches = ConvertPositionToInstruction(relPos, infoSize, sourceInfo, offset);
		if(offset != ~0u)
		{
			const char *pos = fullSource + offset;
			const char *start = pos;
			while(*start && start > fullSource)
				start--;
			start++;
			unsigned int realLine = 0;
			while(++realLine && NULL != (start = strchr(start, '\n')) && start < pos)
				start++;
			realLine--;
			if(realLine != origLine)
				return 0;
		}
		return matches;
	}
	return 0;
}

std::vector<std::string>	activeDependencies;

void RegisterDependency(const char *fileName)
{
	activeDependencies.push_back(fileName);
}

bool CheckFile(const char *name)
{
	if(FILE *file = fopen(name, "r"))
	{
		fclose(file);
		return true;
	}

	return false;
}

std::string ResolveSourceFile(const char* name)
{
	char tmp[256];
	sprintf(tmp, "%s", name);

	if(CheckFile(tmp))
		return tmp;

	sprintf(tmp, "translation/%s", name);

	if(CheckFile(tmp))
		return tmp;

	sprintf(tmp, "../NULLC/translation/%s", name);

	if(CheckFile(tmp))
		return tmp;

	return "";
}

void IdeSetBreakpoints()
{
	nullcDebugClearBreakpoints();
	unsigned int infoSize = 0;
	ExternSourceInfo *sourceInfo = nullcDebugSourceInfo(&infoSize);

	const char *fullSource = nullcDebugSource();

	unsigned int moduleSize = 0;
	ExternModuleInfo *modules = nullcDebugModuleInfo(&moduleSize);
	// Set all breakpoints
	for(unsigned int i = 0; i < richEdits.size(); i++)
	{
		RichTextarea::LineIterator it = RichTextarea::GetFirstLine(richEdits[i]);
		while(it.line)
		{
			if(it.GetExtra())
			{
				unsigned int matches = ConvertLineToInstruction(RichTextarea::GetCachedAreaText(richEdits[i]), it.number, fullSource, infoSize, sourceInfo, moduleSize, modules);
				for(unsigned k = 0; k < matches; k++)
				{
					it.SetExtra(EXTRA_BREAKPOINT);
					nullcDebugAddBreakpoint(byteCodePos[k]);
				}
				if(!matches)
				{
					it.SetExtra(EXTRA_BREAKPOINT_INVALID);
					printf("Failed to add breakpoint at line %d\r\n", it.number);
				}
			}
			it.GoForward();
		}
	}
}

TabbedFiles::TabInfo* IdePrepareActiveSourceForBuild()
{
	unsigned id = TabbedFiles::GetCurrentTab(hTabs);

	if(id == richEdits.size())
		return NULL;

	TabbedFiles::TabInfo &mainTabInfo = TabbedFiles::GetTabInfo(hTabs, id);

	RichTextarea::ResetLineStyle(mainTabInfo.window);

	for(unsigned int i = 0; i < richEdits.size(); i++)
	{
		if(!TabbedFiles::GetTabInfo(hTabs, i).dirty)
			continue;

		if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, i).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, i).window), TabbedFiles::GetTabInfo(hTabs, i).window))
		{
			TabbedFiles::GetTabInfo(hTabs, i).dirty = false;
			RichTextarea::ResetUpdate(TabbedFiles::GetTabInfo(hTabs, i).window);
			InvalidateRect(hTabs, NULL, true);
		}
	}

	// Remove all non-base modules
	id = 0;
	while(const char* moduleName = nullcEnumerateModules(id))
	{
		if(std::find(baseModules.begin(), baseModules.end(), NULLC::GetStringHash(moduleName)) == baseModules.end())
		{
			nullcRemoveModule(moduleName);
		}
		else
		{
			id++;
		}
	}

	return &mainTabInfo;
}

void IdeRun(bool debug)
{
	if(!runRes.finished)
	{
		TerminateThread(calcThread, 0);
		ShowWindow(hContinue, SW_HIDE);
		SetWindowText(hButtonCalc, "Run");
		runRes.finished = true;
		return;
	}

#ifndef _DEBUG
	FreeConsole();
#endif

	SetWindowText(hCode, "");
	SetWindowText(hResult, "");

	nullcSetExecutorStackSize(8 * 1024 * 1024);

	if(TabbedFiles::TabInfo *activeTab = IdePrepareActiveSourceForBuild())
	{
		mainCodeWnd = activeTab->window;
		const char *source = RichTextarea::GetAreaText(activeTab->window);

		nullcClearImportPaths();

		nullcAddImportPath("Modules/");
		nullcAddImportPath("../Modules/");

		if(const char *pos = strrchr(activeTab->name, '\\'))
		{
			char path[512];
			NULLC::SafeSprintf(path, 1024, "%.*s", unsigned(pos - activeTab->name) + 1, activeTab->name);

			for(int i = 0; i < strlen(path); i++)
				path[i] = path[i] == '\\' ? '/' : path[i];

			nullcAddImportPath(path);
		}

		nullres good = false;

		if(ComboBox_GetCurSel(hExecutionType) == 0)
		{
			nullcSetExecutor(NULLC_REG_VM);

			good = nullcBuildWithModuleName(source, activeTab->last);
		}
		else if(ComboBox_GetCurSel(hExecutionType) == 1)
		{
			nullcSetExecutor(NULLC_X86);

			good = nullcBuildWithModuleName(source, activeTab->last);
		}
		else if(ComboBox_GetCurSel(hExecutionType) == 2)
		{
			nullcSetExecutor(NULLC_LLVM);

			good = nullcBuildWithModuleName(source, activeTab->last);
		}
		else if(ComboBox_GetCurSel(hExecutionType) == 3)
		{
			good = nullcCompile(source);

			if(good)
			{
				double time = myGetPreciseTime();

				RunResult &rres = runRes;
				rres.finished = false;

				char val[1024];
				nullres goodRun = nullcTestEvaluateExpressionTree(val, 1024);

				rres.time = myGetPreciseTime() - time;
				rres.result = goodRun;
				rres.finished = true;

				if(goodRun)
				{
					char result[1024];
					_snprintf(result, 1024, "The answer is: %s [in %f]", val, runRes.time);
					result[1023] = '\0';
					SetWindowText(hResult, result);
				}

				SendMessage(rres.wnd, WM_USER + 1, 0, 0);

				return;
			}
		}
		else if(ComboBox_GetCurSel(hExecutionType) == 4)
		{
			good = nullcCompile(source);

			if(good)
			{
				double time = myGetPreciseTime();

				RunResult &rres = runRes;
				rres.finished = false;

				char val[1024];
				nullres goodRun = nullcTestEvaluateInstructionTree(val, 1024);

				rres.time = myGetPreciseTime() - time;
				rres.result = goodRun;
				rres.finished = true;

				if(goodRun)
				{
					char result[1024];
					_snprintf(result, 1024, "The answer is: %s [in %f]", val, runRes.time);
					result[1023] = '\0';
					SetWindowText(hResult, result);
				}

				SendMessage(rres.wnd, WM_USER + 1, 0, 0);

				return;
			}
		}

		if(!good)
		{
			SetWindowText(hCode, GetLastNullcErrorWindows());

			TabbedFiles::SetCurrentTab(hDebugTabs, 0);
		}
		else
		{
			if(debug)
			{
				// Cache all source code in linear form
				for(unsigned int i = 0; i < richEdits.size(); i++)
					RichTextarea::GetAreaText(richEdits[i]);

				IdeSetBreakpoints();
			}

			SetWindowText(hButtonCalc, "Abort");
			calcThread = CreateThread(NULL, 1024*1024, CalcThread, &runRes, NULL, 0);
		}
	}
}

DWORD WINAPI PipeThread(void* param)
{
	PipeData data;

	while(!param)
	{
		int result = RemoteData::SocketReceive(RemoteData::sck, (char*)&data, sizeof(data), 5);
		if(!result || result == -1 || result != sizeof(data) || data.cmd != DEBUG_BREAK_HIT)
		{
			continue;
		}
		ShowWindow(hContinue, SW_SHOW);

		for(unsigned int i = 0; i < attachedEdits.size(); i++)
		{
			RichTextarea::ResetLineStyle(attachedEdits[i]);
			RichTextarea::UpdateArea(attachedEdits[i]);
			RichTextarea::ResetUpdate(attachedEdits[i]);
		}
		RefreshBreakpoints();

		data.cmd = DEBUG_BREAK_STACK;
		delete[] RemoteData::stackData;
		RemoteData::stackData = PipeReceiveResponce(data);
		RemoteData::stackSize = data.data.elemCount;

		data.cmd = DEBUG_BREAK_CALLSTACK;
		RemoteData::callStack = (unsigned int*)PipeReceiveResponce(data);
		RemoteData::callStackFrames = data.data.elemCount;

		if(RemoteData::stackData && RemoteData::callStack)
			FillVariableInfoTree(true);
		TabbedFiles::SetCurrentTab(hDebugTabs, 1);

		delete[] RemoteData::callStack;
		RemoteData::callStack = NULL;

		breakCommand = NULLC_BREAK_PROCEED;
		WaitForSingleObject(breakResponse, INFINITE);

		data.cmd = DEBUG_BREAK_CONTINUE;
		data.question = false;
		data.debug.breakInst = breakCommand;
		if(!PipeSendRequest(data))
			break;
	}
	
	ExitThread(0);
}

unsigned int PipeReuqestData(DebugCommand cmd, void **ptr)
{
	PipeData data;
	data.cmd = cmd;
	data.question = true;
	if(!PipeSendRequest(data))
	{
		MessageBox(hWnd, "Failed to send request through pipe", "Error", MB_OK);
		return 0;
	}
	*ptr = (ExternModuleInfo*)PipeReceiveResponce(data);
	if(!*ptr)
	{
		MessageBox(hWnd, "Failed to receive response through pipe", "Error", MB_OK);
		return 0;
	}
	return data.data.elemCount;
}

void PipeInit()
{
	using namespace RemoteData;

	// Start by retrieving source and module information
	moduleCount = PipeReuqestData(DEBUG_MODULE_INFO, (void**)&modules);
	PipeReuqestData(DEBUG_MODULE_NAMES, (void**)&moduleNames);
	char *moduleNamesTmp = moduleNames;
	PipeReuqestData(DEBUG_SOURCE_INFO, (void**)&sourceCode);
	infoSize = PipeReuqestData(DEBUG_CODE_INFO, (void**)&sourceInfo);

	typeCount = PipeReuqestData(DEBUG_TYPE_INFO, (void**)&types);
	funcCount = PipeReuqestData(DEBUG_FUNCTION_INFO, (void**)&functions);
	varCount = PipeReuqestData(DEBUG_VARIABLE_INFO, (void**)&vars);
	localCount = PipeReuqestData(DEBUG_LOCAL_INFO, (void**)&locals);
	PipeReuqestData(DEBUG_TYPE_EXTRA_INFO, (void**)&typeExtra);
	PipeReuqestData(DEBUG_SYMBOL_INFO, (void**)&symbols);

	if(!modules || !moduleNames || !sourceCode || !sourceInfo || !types || !functions || !vars || !locals || !typeExtra || !symbols)
		return;

	char message[1024], *pos = message;
	message[0] = 0;

	for(unsigned int i = 0; i < attachedEdits.size(); i++)
		DestroyWindow(attachedEdits[i]);
	attachedEdits.clear();
	for(unsigned int i = 0; i < moduleCount; i++)
	{
		const char *moduleName = moduleNamesTmp;
		moduleNamesTmp += strlen(moduleNamesTmp) + 1;
		pos += safeprintf(pos, 1024 - (pos - message), "Received information about module '%s'\r\n", moduleName);

		attachedEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, hInst, NULL));
		TabbedFiles::AddTab(hAttachTabs, moduleName, attachedEdits.back());
		RichTextarea::SetAreaText(attachedEdits.back(), sourceCode + modules[i].sourceOffset);
		UpdateWindow(attachedEdits.back());
	}
	attachedEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, hInst, NULL));
	TabbedFiles::AddTab(hAttachTabs, "__last.nc", attachedEdits.back());
	RichTextarea::SetAreaText(attachedEdits.back(), sourceCode + modules[moduleCount-1].sourceOffset + modules[moduleCount-1].sourceSize);
	UpdateWindow(attachedEdits.back());

	SetWindowText(hCode, message);
	TabbedFiles::SetCurrentTab(hDebugTabs, 0);

	TabbedFiles::SetCurrentTab(hAttachTabs, moduleCount);
	ShowWindow(attachedEdits.back(), SW_SHOW);

	pipeThread = CreateThread(NULL, 1024*1024, PipeThread, NULL, NULL, 0);
}

void ContinueAfterBreak()
{
	HWND wnd = TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, TabbedFiles::GetCurrentTab(stateRemote ? hAttachTabs : hTabs)).window;
	RichTextarea::ResetLineStyle(wnd);
	RefreshBreakpoints();
	RichTextarea::UpdateArea(wnd);
	RichTextarea::ResetUpdate(wnd);
	ShowWindow(hContinue, SW_HIDE);
	SetEvent(breakResponse);
}

namespace Broadcast
{
	int		broadcastWorkers[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct	BroadcastResult
	{
		char	address[512];
		char	message[1024];
		char	pid[16];
	} itemResult[16];
	bool	itemAvailable[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	HANDLE	broadcastThreads[16];

	char	nextAddress[8][128];
	unsigned int	nextCount = 0;
}

DWORD WINAPI	BroadcastThread(void *param)
{
	using namespace Broadcast;

	int ID = (*(int*)param) - 1;

	sockaddr_in	saServer;

	if(ID >= 8)
	{
		unsigned short port = 7590;
		char *portStr = NULL;
		if(NULL != (portStr = strchr(Broadcast::nextAddress[ID - 8], ':')))
		{
			*portStr = 0;
			port = (unsigned short)atoi(portStr + 1);
		}

		hostent *localHost = gethostbyname(Broadcast::nextAddress[ID - 8]);
		char	*localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);

		saServer.sin_family = AF_INET;
		saServer.sin_addr.s_addr = inet_addr(localIP);
		saServer.sin_port = htons((u_short)(port));
		safeprintf(itemResult[ID].address, 512, "%s:%d", Broadcast::nextAddress[ID-8], port);

		if(portStr)
			*portStr = ':';
	}else{
		saServer.sin_family = AF_INET;
		saServer.sin_addr.s_addr = inet_addr(RemoteData::localIP);
		saServer.sin_port = htons((u_short)(7590 + ID));
		safeprintf(itemResult[ID].address, 512, "localhost:%d", 7590 + ID);
	}

	SOCKET sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(connect(sck, (SOCKADDR*)&saServer, sizeof(saServer)))
	{
		closesocket(sck);

		safeprintf(itemResult[ID].message, 1024, "%s", GetLastErrorDesc());
		safeprintf(itemResult[ID].pid, 16, "-");
		EnterCriticalSection(&pipeSection);
		broadcastWorkers[ID] = 0;
		itemAvailable[ID] = false;
		LeaveCriticalSection(&pipeSection);
	}else{
		PipeData data;
		data.cmd = DEBUG_REPORT_INFO;
		data.question = true;

		int result = RemoteData::SocketSend(sck, (char*)&data, sizeof(data), 2);
		if(!result || result == -1)
		{
			safeprintf(itemResult[ID].message, 1024, "Failed to request information to socket '%s'\r\n", GetLastErrorDesc());
			safeprintf(itemResult[ID].pid, 16, "-");
			EnterCriticalSection(&pipeSection);
			broadcastWorkers[ID] = 0;
			itemAvailable[ID] = false;
			LeaveCriticalSection(&pipeSection);
			closesocket(sck);
			return 0;
		}

		result = RemoteData::SocketReceive(sck, (char*)&data, sizeof(data), 5);
		if(!result || result == -1)
		{
			safeprintf(itemResult[ID].message, 1024, "Failed to receive information '%s'\r\n", GetLastErrorDesc());
			safeprintf(itemResult[ID].pid, 16, "-");
			EnterCriticalSection(&pipeSection);
			broadcastWorkers[ID] = 0;
			itemAvailable[ID] = false;
			LeaveCriticalSection(&pipeSection);
			closesocket(sck);
			return 0;
		}
		if(data.cmd == DEBUG_REPORT_INFO && !data.question)
		{
			safeprintf(itemResult[ID].message, 1024, "%s", data.report.module);
			safeprintf(itemResult[ID].pid, 16, "%d", data.report.pID);
			EnterCriticalSection(&pipeSection);
			broadcastWorkers[ID] = 0;
			itemAvailable[ID] = true;
			LeaveCriticalSection(&pipeSection);
		}
		closesocket(sck);
	}

	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	char fileName[512];
	fileName[0] = 0;
	OPENFILENAME openData = { sizeof(OPENFILENAME), hWnd, NULL, "NULLC Files\0*.nc\0All Files\0*.*\0\0", NULL, 0, 0, fileName, 512,
		NULL, 0, NULL, NULL, OFN_ALLOWMULTISELECT | OFN_EXPLORER, 0, 0, 0, 0, 0, 0, NULL, 0, 0 };

	char	result[1024];

	__try
	{
		switch(message) 
		{
		case WM_CREATE:
			runRes.finished = true;
			runRes.wnd = hWnd;

			breakResponse = CreateEvent(NULL, false, false, "NULLC Debug Break Continue Event");
			break;
		case WM_DESTROY:
			if(hWnd == ::hWnd)
			{
				if(!runRes.finished)
				{
					TerminateThread(calcThread, 0);
					runRes.finished = true;
				}

				FILE *tabInfo = fopen("nullc_tab.cfg", "wb");
				FILE *undoStorage = fopen("nullc_undo.bin", "wb");
				int ptrSize = sizeof(void*);
				fwrite(&ptrSize, sizeof(int), 1, undoStorage);
				for(unsigned int i = 0; i < richEdits.size(); i++)
				{
					char buf[1024];
					safeprintf(buf, 1024, "File '%s' was changed.\r\nSave changes?", TabbedFiles::GetTabInfo(hTabs, i).name);
					if(TabbedFiles::GetTabInfo(hTabs, i).dirty && TabbedFiles::GetTabInfo(hTabs, i).last[0] != '?' && MessageBox(hWnd, buf, "Warning", MB_YESNO) == IDYES)
						SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, i).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, i).window), TabbedFiles::GetTabInfo(hTabs, i).window);
					fprintf(tabInfo, "%s\r\n", TabbedFiles::GetTabInfo(hTabs, i).name);

					unsigned nameLength = (int)strlen(TabbedFiles::GetTabInfo(hTabs, i).name) + 1;
					fwrite(&nameLength, sizeof(unsigned), 1, undoStorage);
					fwrite(TabbedFiles::GetTabInfo(hTabs, i).name, 1, nameLength, undoStorage);
					RichTextarea::SaveHistory(richEdits[i], undoStorage);

					DestroyWindow(richEdits[i]);
				}
				for(unsigned int i = 0; i < attachedEdits.size(); i++)
					DestroyWindow(attachedEdits[i]);
				fclose(undoStorage);
				fclose(tabInfo);
				RichTextarea::UnregisterTextarea();
				PostQuitMessage(0);
			}
			break;
		case WM_USER + 1:
			SetWindowText(hButtonCalc, "Run");

			if(runRes.result)
			{
				RefreshBreakpoints();
				FillVariableInfoTree();
				TabbedFiles::SetCurrentTab(hDebugTabs, 1);
			}else{
				_snprintf(result, 1024, "%s", GetLastNullcErrorWindows());
				result[1023] = '\0';
				SetWindowText(hCode, result);
				TabbedFiles::SetCurrentTab(hDebugTabs, 0);

				FillVariableInfoTree();
			}
			break;
		case WM_USER + 2:
		{
			ShowWindow(hContinue, SW_SHOW);
			RefreshBreakpoints();
			lastCodeLine = FillVariableInfoTree(true);
			if(TabbedFiles::GetCurrentTab(hDebugTabs) == 0)
				TabbedFiles::SetCurrentTab(hDebugTabs, 1);
			UpdateWatchedVariables();
		}
			break;
		case WM_NOTIFY:
			if(((LPNMHDR)lParam)->code == NM_RCLICK && ((LPNMHDR)lParam)->hwndFrom == hVars)
			{
				HTREEITEM selected = TreeView_GetDropHilight(hVars);
				if(selected)
				{
					TVITEMEX info;
					memset(&info, 0, sizeof(info));
					info.mask = TVIF_HANDLE | TVIF_PARAM;
					info.hItem = selected;
					TreeView_GetItem(hVars, &info);
					if(info.lParam)
					{
						POINT coords;
						GetCursorPos(&coords);
		
						HMENU contextMenu = CreatePopupMenu();
						InsertMenu(contextMenu, 0, MF_BYPOSITION | MF_STRING, 2001, "Watch variable");
						TrackPopupMenu(contextMenu, TPM_TOPALIGN | TPM_LEFTALIGN, coords.x, coords.y, 0, hWnd, NULL);

						TreeView_Select(hVars, selected, TVGN_CARET);
					}
				}
			}else if(((LPNMHDR)lParam)->code == TVN_GETDISPINFO && ((LPNMHDR)lParam)->hwndFrom == hVars){
				LPNMTVDISPINFO info = (LPNMTVDISPINFO)lParam;
				if(info->item.mask & TVIF_CHILDREN)
				{
					if(codeTypes && info->item.lParam && tiExtra[info->item.lParam].type && tiExtra[info->item.lParam].type->subCat == ExternTypeInfo::CAT_POINTER)
						info->item.cChildren = 1;
				}
			}else if(((LPNMHDR)lParam)->code == TVN_ITEMEXPANDING && ((LPNMHDR)lParam)->hwndFrom == hVars){
				LPNMTREEVIEW info = (LPNMTREEVIEW)lParam;

				TreeItemExtra *extra = info->itemNew.lParam ? &tiExtra[info->itemNew.lParam] : NULL;
				if(extra && extra->expandable)
				{
					codeTypes = stateRemote ? RemoteData::types : nullcDebugTypeInfo(NULL);
					char *ptr = extra->type->subCat == ExternTypeInfo::CAT_POINTER ? *(char**)extra->address : (char*)extra->address;
					const ExternTypeInfo &type = extra->type->subCat == ExternTypeInfo::CAT_POINTER ? codeTypes[extra->type->subType] : *extra->type;

					if(stateRemote)
					{
						PipeData data;
						data.cmd = DEBUG_BREAK_DATA;
						data.question = true;
						data.data.wholeSize = (extra->type->subCat == ExternTypeInfo::CAT_POINTER || extra->type->subCat == ExternTypeInfo::CAT_CLASS) ? type.size : ((NULLCArray*)extra->address)->len * codeTypes[type.subType].size;
						data.data.elemCount = 0;
						data.data.dataSize = (unsigned int)(intptr_t)(extra->type->subCat == ExternTypeInfo::CAT_POINTER ? ptr : ((NULLCArray*)extra->address)->ptr);
						if(!PipeSendRequest(data))
						{
							MessageBox(hWnd, "Failed to send request through pipe", "Error", MB_OK);
							return 0;
						}
						ptr = PipeReceiveResponce(data);
						if(!ptr)
						{
							MessageBox(hWnd, "Failed to send request through pipe", "Error", MB_OK);
							break;
						}
						if(!data.data.elemCount)
							break;
					}
					if(IsBadReadPtr(ptr, extra->type->size))
						break;
					// Delete item children
					while(HTREEITEM child = TreeView_GetChild(hVars, extra->item))
						TreeView_DeleteItem(hVars, child);
					if(extra->type->subCat == ExternTypeInfo::CAT_ARRAY)
					{
						ExternTypeInfo decoy = type;
						decoy.arrSize = ((NULLCArray*)extra->address)->len;
						decoy.defaultAlign = 0xff;
						if(IsBadReadPtr(stateRemote ? ptr : ((NULLCArray*)extra->address)->ptr, codeTypes[type.subType].size * decoy.arrSize))
							break;
						FillArrayVariableInfo(decoy, stateRemote ? ptr : ((NULLCArray*)extra->address)->ptr, extra->item);
						if(stateRemote)
							externalBlocks.push_back(ptr);
						break;
					}
					char name[256];

					const ExternTypeInfo &realType = (type.typeFlags & ExternTypeInfo::TYPE_IS_EXTENDABLE) ? codeTypes[*(unsigned*)ptr] : type;

					char *it = name;
					memset(name, 0, 256);
					it += safeprintf(it, 256 - int(it - name), "0x%x: %s ###", ptr, codeSymbols + realType.offsetToName);

					bool simpleType = realType.subCat == ExternTypeInfo::CAT_NONE || (realType.subCat == ExternTypeInfo::CAT_CLASS && realType.type != ExternTypeInfo::TYPE_COMPLEX);
					bool pointerType = realType.subCat == ExternTypeInfo::CAT_POINTER;

					if(&realType == &codeTypes[NULLC_TYPE_TYPEID])
						it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)ptr].offsetToName);
					else if(simpleType || pointerType)
						it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(realType, ptr));

					TVINSERTSTRUCT helpInsert;
					helpInsert.hParent = extra->item;
					helpInsert.hInsertAfter = TVI_LAST;
					helpInsert.item.cchTextMax = 0;
					helpInsert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
					helpInsert.item.pszText = name;
					helpInsert.item.cChildren = pointerType ? I_CHILDRENCALLBACK : (simpleType ? 0 : 1);
					helpInsert.item.lParam = pointerType ? tiExtra.size() : 0;
					if(pointerType)
						tiExtra.push_back(TreeItemExtra());

					HTREEITEM lastItem = TreeView_InsertItem(hVars, &helpInsert);
					if(pointerType)
						tiExtra.back() = TreeItemExtra(ptr, &realType, lastItem, true);

					FillVariableInfo(realType, ptr, lastItem);

					if(stateRemote)
						externalBlocks.push_back(ptr);
				}
			}
			break;
		case WM_COMMAND:
			wmId	= LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			if(wmId == 2001)
			{
				HTREEITEM selected = TreeView_GetSelection(hVars);
				char buf[256];
				TVITEMEX info;
				memset(&info, 0, sizeof(info));
				info.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
				info.hItem = selected;
				info.cchTextMax = 256;
				info.pszText = buf;
				TreeView_GetItem(hVars, &info);

				assert(info.lParam);
				TreeItemExtra *extra = &tiExtra[info.lParam];

				TVINSERTSTRUCT item;
				item.hParent = NULL;
				item.hInsertAfter = TVI_ROOT;
				item.item.mask = TVIF_TEXT | TVIF_PARAM;
				item.item.cchTextMax = 0;
				item.item.pszText = buf;
				item.item.hItem = selected;
				item.item.lParam = tiWatch.size();
				tiWatch.push_back(*extra);

				HTREEITEM parent = TreeView_InsertItem(hWatch, &item);
				FillVariableInfo(*extra->type, (char*)extra->address, parent);
			}else if((HWND)lParam == hContinue){
				ContinueAfterBreak();
			}else if((HWND)lParam == hButtonCalc){
				IdeRun(true);
			}else if((HWND)lParam == hNewFile){
				GetWindowText(hNewFilename, fileName, 512);
				SetWindowText(hNewFilename, "");
				if(!strstr(fileName, ".nc"))
					strcat(fileName, ".nc");
				char *filePart = NULL;
				GetFullPathName(fileName, 512, result, &filePart);

				// Check if file is already opened
				for(unsigned int i = 0; i < richEdits.size(); i++)
				{
					if(strcmp(TabbedFiles::GetTabInfo(hTabs, i).name, result) == 0)
					{
						TabbedFiles::SetCurrentTab(hTabs, i);
						return 0;
					}
				}
				FILE *fNew = fopen(fileName, "rb");

				ShowWindow(hNewTab, SW_HIDE);

				if(fNew)
				{
					int action = MessageBox(hWnd, "File already exists, overwrite?", "Warning", MB_YESNOCANCEL);
					if(action == IDYES)
					{
						richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, ::hWnd, NULL, hInst, NULL));
						TabbedFiles::AddTab(hTabs, result, richEdits.back());
						TabbedFiles::SetCurrentTab(hTabs, (int)richEdits.size() - 1);
						TabbedFiles::GetTabInfo(hTabs, (int)richEdits.size() - 1).dirty = true;
					}else if(action == IDNO){
						fclose(fNew);
						AddTabWithFile(fileName, hInst);
						TabbedFiles::SetCurrentTab(hTabs, (int)richEdits.size() - 1);
						ShowWindow(richEdits.back(), SW_SHOW);
					}
					fclose(fNew);
				}else{
					richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, ::hWnd, NULL, hInst, NULL));
					TabbedFiles::AddTab(hTabs, result, richEdits.back());
					TabbedFiles::SetCurrentTab(hTabs, (int)richEdits.size() - 1);
					TabbedFiles::GetTabInfo(hTabs, (int)richEdits.size() - 1).dirty = true;
				}
			}else if((HWND)lParam == hAttachBack){
				ShowWindow(hTabs, SW_SHOW);
				ShowWindow(hButtonCalc, SW_SHOW);
				ShowWindow(hResult, SW_SHOW);
				ShowWindow(hExecutionType, SW_SHOW);
				ShowWindow(hShowTemporaries, SW_SHOW);
				ShowWindow(TabbedFiles::GetTabInfo(hTabs, TabbedFiles::GetCurrentTab(hTabs)).window, SW_SHOW);

				ShowWindow(hAttachPanel, SW_HIDE);
				ShowWindow(hAttachDo, SW_HIDE);
				ShowWindow(hAttachBack, SW_HIDE);
				ShowWindow(hAttachTabs, SW_HIDE);
				ShowWindow(hAttachAdd, SW_HIDE);
				ShowWindow(hAttachAddName, SW_HIDE);

				ShowWindow(hContinue, SW_HIDE);

				if(stateRemote)
				{
					// Kill debug tracking
					TerminateThread(pipeThread, 0);
					// Send debug detach and continue commands
					PipeData data;
					data.cmd = DEBUG_DETACH;
					data.question = false;
					if(!PipeSendRequest(data))
						MessageBox(hWnd, "Failed to send request through pipe", "Error", MB_OK);
					closesocket(RemoteData::sck);
					// Remove text area windows
					for(unsigned int i = 0; i < attachedEdits.size(); i++)
					{
						TabbedFiles::RemoveTab(hAttachTabs, 0);
						DestroyWindow(attachedEdits[i]);
					}
					// Destroy data
					delete[] RemoteData::modules;
					RemoteData::modules = NULL;
					delete[] RemoteData::moduleNames;
					RemoteData::moduleNames = NULL;
					delete[] RemoteData::sourceInfo;
					RemoteData::sourceInfo = NULL;
					delete[] RemoteData::sourceCode;
					RemoteData::sourceCode = NULL;
					delete[] RemoteData::vars;
					RemoteData::vars = NULL;
					delete[] RemoteData::types;
					RemoteData::types = NULL;
					delete[] RemoteData::functions;
					RemoteData::functions = NULL;
					delete[] RemoteData::locals;
					RemoteData::locals = NULL;
					delete[] RemoteData::typeExtra;
					RemoteData::typeExtra = NULL;
					delete[] RemoteData::symbols;
					RemoteData::symbols = NULL;
					delete[] RemoteData::stackData;
					RemoteData::stackData = NULL;

					for(unsigned int i = 0; i < externalBlocks.size(); i++)
						delete[] externalBlocks[i];
					externalBlocks.clear();

					codeVars = NULL;
					codeTypes = NULL;
					codeFunctions = NULL;
					codeLocals = NULL;
					codeTypeExtra = NULL;
					codeSymbols = NULL;

					mainCodeWnd = NULL;

					stateRemote = false;
				}
				HMENU debugMenu = GetSubMenu(GetMenu(hWnd), 1);
				EnableMenuItem(debugMenu, ID_RUN, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(debugMenu, ID_DEBUG_ATTACHTOPROCESS, MF_BYCOMMAND | MF_ENABLED);
				stateAttach = false;
			}else if((HWND)lParam == hAttachDo){
				unsigned int ID = ListView_GetSelectionMark(hAttachList);

				sockaddr_in	saServer;
				if(ID >= 8)
				{
					unsigned short port = 7590;
					char *portStr = NULL;
					if(NULL != (portStr = strchr(Broadcast::nextAddress[ID - 8], ':')))
					{
						*portStr = 0;
						port = (unsigned short)atoi(portStr + 1);
					}

					hostent *localHost = gethostbyname(Broadcast::nextAddress[ID - 8]);
					char	*localIP = inet_ntoa(*(struct in_addr *)*localHost->h_addr_list);

					saServer.sin_family = AF_INET;
					saServer.sin_addr.s_addr = inet_addr(localIP);
					saServer.sin_port = htons((u_short)(port));
					if(portStr)
						*portStr = ':';
				}else{
					saServer.sin_family = AF_INET;
					saServer.sin_addr.s_addr = inet_addr(RemoteData::localIP);
					saServer.sin_port = htons((u_short)(7590 + ID));
				}
				RemoteData::sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if(connect(RemoteData::sck, (SOCKADDR*)&saServer, sizeof(saServer)))
				{
					closesocket(RemoteData::sck);
					MessageBox(hWnd, "Cannot attach debugger to selected process", "ERROR", MB_OK);
				}else{
					ShowWindow(hAttachPanel, SW_HIDE);
					ShowWindow(hAttachDo, SW_HIDE);
					ShowWindow(hAttachTabs, SW_SHOW);
					ShowWindow(hAttachAdd, SW_HIDE);
					ShowWindow(hAttachAddName, SW_HIDE);

					stateAttach = false;
					stateRemote = true;
					PipeInit();
				}
			}else if((HWND)lParam == hAttachAdd){
				if(Broadcast::nextCount == 8)
				{
					if(MessageBox(hWnd, "A maximum limit of custom addresses reached. Replace last address?", "Warning", MB_OKCANCEL) == IDOK)
						Broadcast::nextCount = 7;
					else
						break;
				}
				GetWindowText(hAttachAddName, Broadcast::nextAddress[Broadcast::nextCount], 128);
				Broadcast::nextAddress[Broadcast::nextCount][127] = 0;
				Broadcast::nextCount++;
			}
			// Parse the menu selections:
			switch (wmId)
			{
			case IDM_EXIT:
				DestroyWindow(hWnd);
				break;
			case ID_FILE_LOAD:
				GetCurrentDirectory(512, result + 512);
				if(GetOpenFileName(&openData))
				{
					const char *file = fileName;
					const char *path = fileName;
					const char *separator = "\\";
					// Skip path
					file += strlen(file) + 1;
					// Single file isn't divided by path\file, so step back
					if(!*file)
					{
						path = "";
						separator = "";
						file = fileName;
					}
					// For all files
					while(*file)
					{
						strcpy(result, path);
						strcat(result, separator);
						strcat(result, file);

						bool opened = false;
						// Check if file is already opened
						for(unsigned int i = 0; i < richEdits.size(); i++)
						{
							if(_stricmp(TabbedFiles::GetTabInfo(hTabs, i).name, result) == 0)
							{
								TabbedFiles::SetCurrentTab(hTabs, i);
								opened = true;
								break;
							}
						}
						if(!opened)
						{
							AddTabWithFile(result, hInst);
							TabbedFiles::SetCurrentTab(hTabs, (int)richEdits.size() - 1);
							ShowWindow(richEdits.back(), SW_SHOW);
						}
						file += strlen(file) + 1;
					}
				}
				SetCurrentDirectory(result + 512);
				break;
			case ID_FILE_SAVE:
				{
					unsigned int id = TabbedFiles::GetCurrentTab(hTabs);
					if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, id).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, id).window), TabbedFiles::GetTabInfo(hTabs, id).window))
					{
						TabbedFiles::GetTabInfo(hTabs, id).dirty = false;
						RichTextarea::ResetUpdate(TabbedFiles::GetTabInfo(hTabs, id).window);
						InvalidateRect(hTabs, NULL, true);
					}
				}
				break;
			case ID_FILE_SAVEALL:
				for(unsigned int i = 0; i < richEdits.size(); i++)
				{
					if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, i).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, i).window), TabbedFiles::GetTabInfo(hTabs, i).window))
					{
						TabbedFiles::GetTabInfo(hTabs, i).dirty = false;
						RichTextarea::ResetUpdate(TabbedFiles::GetTabInfo(hTabs, i).window);
						InvalidateRect(hTabs, NULL, true);
					}
				}
				break;
			case ID_TOGGLE_BREAK:
				{
					unsigned int id = TabbedFiles::GetCurrentTab(stateRemote ? hAttachTabs : hTabs);
					HWND wnd = TabbedFiles::GetTabInfo(stateRemote ? hAttachTabs : hTabs, id).window;
					unsigned int line = RichTextarea::GetCurrentLine(wnd);

					RichTextarea::LineIterator it = RichTextarea::GetLine(wnd, line);
					bool breakSet = it.GetExtra() == 0;
					// If breakpoint is not found, add one
					if(breakSet)
					{
						RichTextarea::SetStyleToLine(wnd, line, OVERLAY_BREAKPOINT);
						RichTextarea::SetLineExtra(wnd, line, EXTRA_BREAKPOINT);
					}else{
						RichTextarea::SetStyleToLine(wnd, line, OVERLAY_NONE);
						RichTextarea::SetLineExtra(wnd, line, EXTRA_NONE);
					}

					if(!runRes.finished)
					{
						unsigned int infoSize = 0;
						ExternSourceInfo *sourceInfo = nullcDebugSourceInfo(&infoSize);
						const char *fullSource = nullcDebugSource();
						unsigned int moduleSize = 0;
						ExternModuleInfo *modules = nullcDebugModuleInfo(&moduleSize);
						unsigned int matches = ConvertLineToInstruction(RichTextarea::GetCachedAreaText(wnd), line, fullSource, infoSize, sourceInfo, moduleSize, modules);
						for(unsigned k = 0; k < matches; k++)
						{
							if(breakSet)
								nullcDebugAddBreakpoint(byteCodePos[k]);
							else
								nullcDebugRemoveBreakpoint(byteCodePos[k]);
						}
						if(!matches)
						{
							if(breakSet)
							{
								RichTextarea::SetStyleToLine(wnd, line, OVERLAY_BREAKPOINT_INVALID);
								RichTextarea::SetLineExtra(wnd, line, EXTRA_BREAKPOINT_INVALID);
							}
						}
					}
					RichTextarea::UpdateArea(wnd);
					RichTextarea::ResetUpdate(wnd);

					if(stateRemote)
					{
						unsigned int matches = ConvertLineToInstruction(RichTextarea::GetAreaText(wnd), line, RemoteData::sourceCode, RemoteData::infoSize, RemoteData::sourceInfo, RemoteData::moduleCount, RemoteData::modules);
						for(unsigned k = 0; k < matches; k++)
						{
							PipeData data;
							data.cmd = DEBUG_BREAK_SET;
							data.question = true;
							data.debug.breakInst = byteCodePos[k];
							data.debug.breakSet = breakSet;
							if(!PipeSendRequest(data))
								MessageBox(hWnd, "Failed to send request through pipe", "Error", MB_OK);
						}
						if(!matches)
						{
							if(breakSet)
							{
								RichTextarea::SetStyleToLine(wnd, line, OVERLAY_BREAKPOINT_INVALID);
								RichTextarea::SetLineExtra(wnd, line, EXTRA_BREAKPOINT_INVALID);
							}
						}
					}
				}
				break;
			case ID_RUN_DEBUG:
				if(!stateRemote && !stateAttach && runRes.finished)
					IdeRun(true);
				if(stateRemote || !runRes.finished)
					ContinueAfterBreak();
				break;
			case ID_RUN:
				if(!stateRemote && !stateAttach && runRes.finished)
					IdeRun(false);
				if(stateRemote || !runRes.finished)
					ContinueAfterBreak();
				break;
			case ID_DEBUG_STEP:
				if(stateRemote || !runRes.finished)
				{
					breakCommand = NULLC_BREAK_STEP;
					ContinueAfterBreak();
				}
				break;
			case ID_DEBUG_STEP_INTO:
				if(stateRemote || !runRes.finished)
				{
					breakCommand = NULLC_BREAK_STEP_INTO;
					ContinueAfterBreak();
				}
				break;
			case ID_DEBUG_STEP_OUT:
				if(stateRemote || !runRes.finished)
				{
					breakCommand = NULLC_BREAK_STEP_OUT;
					ContinueAfterBreak();
				}
				break;
			case ID_DEBUG_ATTACHTOPROCESS:
			{
				ShowWindow(hTabs, SW_HIDE);
				ShowWindow(hButtonCalc, SW_HIDE);
				ShowWindow(hResult, SW_HIDE);
				ShowWindow(hExecutionType, SW_HIDE);
				ShowWindow(hShowTemporaries, SW_HIDE);
				ShowWindow(TabbedFiles::GetTabInfo(hTabs, TabbedFiles::GetCurrentTab(hTabs)).window, SW_HIDE);

				ShowWindow(hAttachPanel, SW_SHOW);
				ShowWindow(hAttachDo, SW_SHOW);
				ShowWindow(hAttachBack, SW_SHOW);
				ShowWindow(hAttachAdd, SW_SHOW);
				ShowWindow(hAttachAddName, SW_SHOW);

				Button_Enable(hAttachDo, false);

				HMENU debugMenu = GetSubMenu(GetMenu(hWnd), 1);
				EnableMenuItem(debugMenu, ID_RUN, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(debugMenu, ID_DEBUG_ATTACHTOPROCESS, MF_BYCOMMAND | MF_GRAYED);

				stateAttach = true;
			}
				break;
			case ID_SAVE_AS_HTML:
			{
				TabbedFiles::TabInfo currTab = TabbedFiles::GetTabInfo(hTabs, TabbedFiles::GetCurrentTab(hTabs));

				RichTextarea::BeginStyleUpdate(currTab.window);
				colorer->ColorText(currTab.window, (char*)RichTextarea::GetAreaText(currTab.window), RichTextarea::SetStyleToSelection);
				RichTextarea::EndStyleUpdate(currTab.window);
				const char *text = RichTextarea::GetCachedAreaText(currTab.window);
				const char *style = RichTextarea::GetAreaStyle(currTab.window);

				// Create file name
				safeprintf(result, 1024, "%s.html", currTab.last);
				// Convert point in original file name into an underscore
				char *curr = strchr(result, '.');
				if(curr && curr[5] != 0)
					*curr = '_';
				// Create file
				FILE *fHTML = fopen(result, "wb");
				if(!fHTML)
				{
					MessageBox(hWnd, "Cannot save file as html (file creation failed)", "Error", MB_OK);
					break;
				}
				fprintf(fHTML, "<!DOCTYPE html>\r\n");
				fprintf(fHTML, "<html>\r\n<head>\r\n<meta charset=\"utf-8\">\r\n<title>%s</title>\r\n", currTab.name);
				fprintf(fHTML, "<style type=\"text/css\">\r\n");
				fprintf(fHTML, "pre.code{ background: #eee; font-family: Consolas, Courier New, monospace; font-size: 10pt; }\r\n");
				fprintf(fHTML, "span.rword{color: #00f;}\r\n");
				fprintf(fHTML, "span.func{color: #880000;font-style: italic;}\r\n");
				fprintf(fHTML, "span.var{color: #555;}\r\n");
				fprintf(fHTML, "span.vardef{color: #323232;}\r\n");
				fprintf(fHTML, "span.bold{font-weight: bold;}\r\n");
				fprintf(fHTML, "span.real{color: #008800;}\r\n");
				fprintf(fHTML, "span.comment{color: #f0f;}\r\n");
				fprintf(fHTML, "span.string{color: #880000;}\r\n");
				fprintf(fHTML, "span.error{color: #f00;text-decoration: underline;}\r\n");
				fprintf(fHTML, "</style>\r\n");
				fprintf(fHTML, "</head>\r\n<body>\r\n<pre class=\"code\">\r\n");
				const char *cText = text;
				const char *cStyle = style;
				char lastStyle = COLOR_CODE;
				const char	*styleName[] = { "", "rword", "var", "vardef", "func", "", "bold", "string", "real", "real", "error", "comment" };
				while(*cText)
				{
					if(*cStyle != lastStyle)
					{
						if(lastStyle != COLOR_CODE && lastStyle != COLOR_TEXT)
							fprintf(fHTML, "</span>");
						lastStyle = *cStyle;
						assert((unsigned)lastStyle <= COLOR_COMMENT);
						if(lastStyle != COLOR_CODE && lastStyle != COLOR_TEXT)
							fprintf(fHTML, "<span class=\"%s\">", styleName[lastStyle]);
					}
					if(*cText == '\r')
					{
						fwrite("\r\n", 1, 2, fHTML);
						cText++;
						cStyle++;
					}else if(*cText == '<'){
						fwrite("&lt;", 1, 4, fHTML);
					}else if(*cText == '\t'){
						fwrite("    ", 1, 4, fHTML);
					}else if(*cText == '>'){
						fwrite("&gt;", 1, 4, fHTML);
					}else{
						fwrite(cText, 1, 1, fHTML);
					}
					cText++;
					cStyle++;
				}
				if(lastStyle != COLOR_CODE && lastStyle != COLOR_TEXT)
					fprintf(fHTML, "</span>");
				fprintf(fHTML, "</pre>\r\n</body>\r\n</html>");
				fclose(fHTML);
			}
				break;
			case ID_FILE_TRANSLATETOC:
			{
				if(TabbedFiles::TabInfo *activeTab = IdePrepareActiveSourceForBuild())
				{
					const char *source = RichTextarea::GetAreaText(activeTab->window);

					if(!nullcCompile(source))
					{
						SetWindowText(hCode, GetLastNullcErrorWindows());

						TabbedFiles::SetCurrentTab(hDebugTabs, 0);
						break;
					}

					// Create file name
					char cleanName[1024];
					safeprintf(cleanName, 1024, "%s", activeTab->last);

					if(char* pos = strrchr(cleanName, '.'))
						*pos = 0;

					safeprintf(result, 1024, "%s.cpp", cleanName);

					activeDependencies.clear();

					if(!nullcTranslateToC(result, "main", RegisterDependency))
					{
						nullcClean();

						SetWindowText(hCode, GetLastNullcErrorWindows());

						TabbedFiles::SetCurrentTab(hDebugTabs, 0);
					}
					else
					{
						nullcClean();

						std::string cmdLine;

						cmdLine += "g++ -g ";
						cmdLine += result;
						cmdLine += " -lstdc++";
						cmdLine += " -Itranslation";
						cmdLine += " -I../NULLC/translation";
						cmdLine += " -O2";

						std::string runtimeLocation = ResolveSourceFile("runtime.cpp");

						if(runtimeLocation.empty())
						{
							SetWindowText(hCode, "Failed to find 'runtime.cpp' input file");
							break;
						}

						cmdLine += " " + runtimeLocation;

						for(unsigned i = 0; i < activeDependencies.size(); i++)
						{
							std::string dependency = activeDependencies[i];

							cmdLine += " " + dependency;

							if(strstr(dependency.c_str(), "import_"))
							{
								char tmp[1024];
								safeprintf(tmp, 1024, "%s", dependency.c_str() + strlen("import_"));

								if(char *pos = strstr(tmp, "_nc.cpp"))
									strcpy(pos, "_bind.cpp");

								std::string bindLocation = ResolveSourceFile(tmp);

								if(!bindLocation.empty())
									cmdLine += " " + bindLocation;
							}
						}

						safeprintf(result, 1024, "%s.cmdline.txt", cleanName);

						if(FILE *fCmdLine = fopen(result, "wb"))
						{
							fprintf(fCmdLine, "%s", cmdLine.c_str());
							fclose(fCmdLine);
						}
						else
						{
							char msg[1024];
							safeprintf(msg, 1024, "Failed to save '%s' file", result);

							SetWindowText(hCode, msg);
						}
					}
				}
			}
				break;
			case ID_CLOSE_TAB:
				TabbedFiles::RemoveTab(hTabs, TabbedFiles::GetCurrentTab(hTabs));
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		
		case WM_PAINT:
			{
				hdc = BeginPaint(hWnd, &ps);
				EndPaint(hWnd, &ps);
			}
			break;
		case WM_TIMER:
		{
			if(stateAttach)
			{
				int selected = ListView_GetSelectionMark(hAttachList);

				LVITEM lvItem;
				lvItem.mask = LVIF_TEXT | LVIF_STATE;
				lvItem.state = 0;
				lvItem.stateMask = 0;
				for(unsigned int i = 0; i < (8 + Broadcast::nextCount); i++)
				{
					EnterCriticalSection(&pipeSection);
					if(!Broadcast::broadcastWorkers[i])
					{
						lvItem.iItem = i;
						lvItem.iSubItem = 0;
						lvItem.pszText = Broadcast::itemResult[i].address;
						ListView_SetItem(hAttachList, &lvItem);
						lvItem.iSubItem = 1;
						lvItem.pszText = Broadcast::itemResult[i].message;
						ListView_SetItem(hAttachList, &lvItem);
						lvItem.iSubItem = 2;
						lvItem.pszText = Broadcast::itemResult[i].pid;
						ListView_SetItem(hAttachList, &lvItem);
						Broadcast::broadcastWorkers[i] = i + 1;
						Broadcast::broadcastThreads[i] = CreateThread(NULL, 64*1024, BroadcastThread, &Broadcast::broadcastWorkers[i], NULL, 0);
					}
					LeaveCriticalSection(&pipeSection);
				}
				if(selected != -1)
					Button_Enable(hAttachDo, Broadcast::itemAvailable[selected]);
				ListView_SetSelectionMark(hAttachList, selected);
			}

			for(unsigned int i = 0; i < richEdits.size(); i++)
			{
				if(!TabbedFiles::GetTabInfo(hTabs, i).dirty && RichTextarea::NeedUpdate(TabbedFiles::GetTabInfo(hTabs, i).window))
				{
					TabbedFiles::GetTabInfo(hTabs, i).dirty = true;
					InvalidateRect(hTabs, NULL, false);
				}
			}

			unsigned int id = TabbedFiles::GetCurrentTab(hTabs);
			EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, TabbedFiles::GetTabInfo(hTabs, id).dirty ? MF_ENABLED : MF_DISABLED);

			TabbedFiles::TabInfo &info = stateRemote ? TabbedFiles::GetTabInfo(hAttachTabs, TabbedFiles::GetCurrentTab(hAttachTabs)) : TabbedFiles::GetTabInfo(hTabs, id);
			HWND wnd = info.window;
			if(!RichTextarea::NeedUpdate(wnd) || (GetTickCount()-lastUpdate < 100))
				break;
			if(info.name[0] != '?')
				SetWindowText(hCode, "");

			RichTextarea::ResetUpdate(wnd);
			needTextUpdate = false;
			lastUpdate = GetTickCount();

			const char *compileErr = NULL;
			if(!nullcCompile((char*)RichTextarea::GetAreaText(wnd)))
				compileErr = GetLastNullcErrorWindows();

			RichTextarea::BeginStyleUpdate(wnd);

			colorer->ColorText(wnd, (char*)RichTextarea::GetAreaText(wnd), RichTextarea::SetStyleToSelection);

			if(compileErr)
			{
				SetWindowText(hCode, compileErr);
				TabbedFiles::SetCurrentTab(hDebugTabs, 0);
			}
			RichTextarea::EndStyleUpdate(wnd);
			RichTextarea::UpdateArea(wnd);
		}
			break;
		case WM_GETMINMAXINFO:
		{
			MINMAXINFO	*info = (MINMAXINFO*)lParam;
			info->ptMinTrackSize.x = 400;
			info->ptMinTrackSize.y = 300;
		}
			break;
		case WM_SIZE:
		{
			unsigned int width = LOWORD(lParam), height = HIWORD(lParam);
			unsigned int mainPadding = 5, subPadding = 2;

			unsigned int middleHeight = 30;
			unsigned int heightTopandBottom = height - mainPadding * 2 - middleHeight - subPadding * 2;

			unsigned int topHeight = int(heightTopandBottom / 100.0 * 60.0);	// 60 %
			unsigned int bottomHeight = int(heightTopandBottom / 100.0 * 40.0);	// 40 %

			unsigned int middleOffsetY = mainPadding + topHeight + subPadding;

			unsigned int tabHeight = 20;

			if(hTabs)
				SetWindowPos(hTabs,			HWND_TOP, mainPadding, 4, width - mainPadding * 2, tabHeight, NULL);
			if(hAttachTabs)
				SetWindowPos(hAttachTabs,	HWND_TOP, mainPadding, 4, width - mainPadding * 2, tabHeight, NULL);

			areaWidth = width - mainPadding * 2;
			areaHeight = topHeight - tabHeight;
			for(unsigned int i = 0; i < richEdits.size(); i++)
			{
				if(richEdits[i])
					SetWindowPos(richEdits[i], HWND_TOP, mainPadding, mainPadding + tabHeight, width - mainPadding * 2, topHeight - tabHeight, NULL);
			}
			for(unsigned int i = 0; i < attachedEdits.size(); i++)
			{
				if(attachedEdits[i])
					SetWindowPos(attachedEdits[i], HWND_TOP, mainPadding, mainPadding + tabHeight, width - mainPadding * 2, topHeight - tabHeight, NULL);
			}

			if(hNewTab)
				SetWindowPos(hNewTab,		HWND_TOP, mainPadding, mainPadding + tabHeight, width - mainPadding * 2, topHeight - tabHeight, NULL);

			if(hAttachPanel)
				SetWindowPos(hAttachPanel,	HWND_TOP, mainPadding, mainPadding, width - mainPadding * 2, topHeight, NULL);

			if(hAttachList)
				SetWindowPos(hAttachList,	HWND_TOP, mainPadding, mainPadding, width - mainPadding * 4, topHeight - mainPadding * 2, NULL);

			unsigned int buttonWidth = 120;
			unsigned int resultWidth = width - 4 * buttonWidth - 3 * mainPadding - subPadding * 3;

			unsigned int calcOffsetX = mainPadding;
			unsigned int resultOffsetX = calcOffsetX * 2 + buttonWidth * 2 + subPadding;
			unsigned int x86OffsetX = resultOffsetX + buttonWidth + resultWidth + subPadding;

			if(hButtonCalc)
				SetWindowPos(hButtonCalc,	HWND_TOP, calcOffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);

			if(hResult)
				SetWindowPos(hResult,		HWND_TOP, resultOffsetX, middleOffsetY, resultWidth, middleHeight, NULL);

			if(hContinue)
				SetWindowPos(hContinue,		HWND_TOP, calcOffsetX * 2 + buttonWidth, middleOffsetY, buttonWidth, middleHeight, NULL);

			if(hExecutionType)
				SetWindowPos(hExecutionType,	HWND_TOP, x86OffsetX, middleOffsetY, buttonWidth, 300, NULL);

			if(hShowTemporaries)
				SetWindowPos(hShowTemporaries, HWND_TOP, x86OffsetX - buttonWidth - subPadding, middleOffsetY, buttonWidth, middleHeight, NULL);

			if(hAttachDo)
				SetWindowPos(hAttachDo,		HWND_TOP, calcOffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);

			if(hAttachAdd)
				SetWindowPos(hAttachAdd,	HWND_TOP, calcOffsetX * 2 + buttonWidth, middleOffsetY, buttonWidth, middleHeight, NULL);

			if(hAttachAddName)
				SetWindowPos(hAttachAddName,HWND_TOP, resultOffsetX, middleOffsetY, resultWidth, middleHeight, NULL);

			if(hAttachBack)
				SetWindowPos(hAttachBack,	HWND_TOP, x86OffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);

			unsigned int bottomOffsetY = middleOffsetY + middleHeight + subPadding;

			unsigned int leftOffsetX = mainPadding;
			if(hDebugTabs)
				SetWindowPos(hDebugTabs,	HWND_TOP, leftOffsetX, bottomOffsetY, width - mainPadding * 2, 20, NULL);

			if(hCode)
				SetWindowPos(hCode,			HWND_TOP, leftOffsetX, bottomOffsetY + 20, width - mainPadding * 2, bottomHeight - 16 - 20, NULL);

			if(hVars)
				SetWindowPos(hVars,			HWND_TOP, leftOffsetX, bottomOffsetY + 20, width - mainPadding * 2, bottomHeight - 16 - 20, NULL);

			if(hWatch)
				SetWindowPos(hWatch,		HWND_TOP, leftOffsetX, bottomOffsetY + 20, width - mainPadding * 2, bottomHeight - 16 - 20, NULL);

			if(hStatus)
				SetWindowPos(hStatus,		HWND_TOP, 0, height-16, width, height, NULL);

			if(hNewTab)
				InvalidateRect(hNewTab, NULL, true);

			if(hButtonCalc)
				InvalidateRect(hButtonCalc, NULL, true);

			if(hResult)
				InvalidateRect(hResult, NULL, true);

			if(hExecutionType)
				InvalidateRect(hExecutionType, NULL, true);

			if(hShowTemporaries)
				InvalidateRect(hShowTemporaries, NULL, true);

			if(hStatus)
				InvalidateRect(hStatus, NULL, true);

			if(hVars)
				InvalidateRect(hVars, NULL, true);

			if(hWatch)
				InvalidateRect(hWatch, NULL, true);

			if(hDebugTabs)
				InvalidateRect(hDebugTabs, NULL, true);
		}
			break;
		case WM_ERASEBKGND:
			break;
		}
	}__except(EXCEPTION_EXECUTE_HANDLER){
		assert(!"Exception in window procedure handler");
		SetWindowText(hResult, "ERROR: internal compiler error");
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
