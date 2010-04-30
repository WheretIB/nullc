#include "stdafx.h"
#include "SuperCalc.h"

//#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

#include "commctrl.h"
#pragma comment(lib, "comctl32.lib")
#include <windowsx.h>

#pragma warning(disable: 4201)
#include <MMSystem.h>
#pragma comment(lib, "Winmm.lib")
#pragma warning(default: 4201)

#include <iostream>

#include "NULLC/nullc.h"
#include "NULLC/nullc_debug.h"

#include "Colorer.h"

#include "UnitTests.h"

#include "GUI/RichTextarea.h"
#include "GUI/TabbedFiles.h"

// NULLC modules
#include "NULLC/includes/file.h"
#include "NULLC/includes/io.h"
#include "NULLC/includes/math.h"
#include "NULLC/includes/string.h"
#include "NULLC/includes/vector.h"
#include "NULLC/includes/list.h"
#include "NULLC/includes/map.h"
#include "NULLC/includes/hashmap.h"
#include "NULLC/includes/random.h"
#include "NULLC/includes/time.h"
#include "NULLC/includes/gc.h"

#include "NULLC/includes/window.h"

#include "NULLC/includes/canvas.h"

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
HWND hContinue;		// Button that continues an interupted execution
HWND hJITEnabled;	// JiT enable check box
HWND hTabs;	
HWND hNewTab, hNewFilename, hNewFile;
HWND hResult;		// label with execution result
HWND hCode;			// disabled text area for error messages and other information
HWND hVars;			// disabled text area that shows values of all variables in global scope
HWND hStatus;		// Main window status bar


unsigned int areaWidth = 400, areaHeight = 300;

HFONT	fontMonospace, fontDefault;

Colorer*	colorer;

std::vector<HWND>	richEdits;

const unsigned int INIT_BUFFER_SIZE = 4096;
char	initError[INIT_BUFFER_SIZE];

// for text update
bool needTextUpdate;
DWORD lastUpdate;

void FillArrayVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent);
void FillComplexVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent);
void FillVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent);

double myGetPreciseTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return temp*1000.0;
}

HANDLE breakResponse = NULL;

void IDEDebugBreak()
{
	SendMessage(hWnd, WM_USER + 2, 0, 0);
	WaitForSingleObject(breakResponse, INFINITE);
}



int APIENTRY WinMain(HINSTANCE	hInstance,
					HINSTANCE	hPrevInstance,
					LPTSTR		lpCmdLine,
					int			nCmdShow)
{
	(void)lpCmdLine;
	(void)hPrevInstance;

	MSG msg;
	HACCEL hAccelTable;

	needTextUpdate = true;
	lastUpdate = GetTickCount();

#ifdef _DEBUG
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
#endif

	bool runUnitTests = false;
	if(runUnitTests)
	{
		AllocConsole();

		freopen("CONOUT$", "w", stdout);
		freopen("CONIN$", "r", stdin);

		RunTests();
	}

	nullcInit("Modules\\");

	memset(initError, 0, INIT_BUFFER_SIZE);

	// in possible, load precompiled modules from nullclib.ncm
	FILE *modulePack = fopen("nullclib.ncm", "rb");
	if(!modulePack)
	{
		strcat(initError, "WARNING: Failed to open precompiled module file nullclib.ncm\r\n");
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
		strcat(initError, "ERROR: Failed to init std.vector module\r\n");
	if(!nullcInitListModule())
		strcat(initError, "ERROR: Failed to init std.list module\r\n");
	if(!nullcInitMapModule())
		strcat(initError, "ERROR: Failed to init std.map module\r\n");
	if(!nullcInitHashmapModule())
		strcat(initError, "ERROR: Failed to init std.hashmap module\r\n");
	if(!nullcInitRandomModule())
		strcat(initError, "ERROR: Failed to init std.random module\r\n");
	if(!nullcInitTimeModule())
		strcat(initError, "ERROR: Failed to init std.time module\r\n");
	if(!nullcInitGCModule())
		strcat(initError, "ERROR: Failed to init std.gc module\r\n");

	nullcLoadModuleBySource("ide.debug", "void _debugBreak();");
	nullcAddModuleFunction("ide.debug", (void(*)())IDEDebugBreak, "_debugBreak", 0);

	colorer = NULL;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SUPERCALC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	lastUpdate = 0;

	// Perform application initialization:
	if(!InitInstance(hInstance, nCmdShow)) 
		return 0;

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SUPERCALC);

	// Main message loop:
	while(GetMessage(&msg, NULL, 0, 0))
	{
		if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	delete colorer;

	nullcTerminate();

	return (int) msg.wParam;
}

WORD MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= 0;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SUPERCALC);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW);
	wcex.lpszMenuName	= (LPCTSTR)IDC_SUPERCALC;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

char* GetLastErrorDesc()
{
	char* msgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msgBuf), 0, NULL);
	return msgBuf;
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

bool SaveFileFromTab(const char *file, const char *data)
{
	FILE *fSave = fopen(file, "wb");
	if(!fSave)
	{
		MessageBox(hWnd, "File cannot be saved", "Warning", MB_OK);
		return false;
	}else{
		fwrite(data, 1, strlen(data), fSave);
		fclose(fSave);
	}
	return true;
}

void CloseTabWithFile(TabbedFiles::TabInfo &info)
{
	if(info.dirty && MessageBox(hWnd, "File was changed. Save changes?", "Warning", MB_YESNO) == IDYES)
	{
		SaveFileFromTab(info.name, RichTextarea::GetAreaText(info.window));
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

	hJITEnabled = CreateWindow("BUTTON", "X86 JIT", WS_VISIBLE | BS_AUTOCHECKBOX | WS_CHILD, 800-140, 185, 130, 30, hWnd, NULL, hInstance, NULL);
	if(!hJITEnabled)
		return 0;
	SendMessage(hJITEnabled, WM_SETFONT, (WPARAM)fontDefault, 0);

	INITCOMMONCONTROLSEX commControlTypes;
	commControlTypes.dwSize = sizeof(INITCOMMONCONTROLSEX);
	commControlTypes.dwICC = ICC_TREEVIEW_CLASSES;
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

	// Load tab information
	FILE *tabInfo = fopen("nullc_tab.cfg", "rb");
	if(!tabInfo)
	{
		richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, hWnd, NULL, hInstance, NULL));
		TabbedFiles::AddTab(hTabs, "main.nc", richEdits.back());
		ShowWindow(richEdits.back(), SW_HIDE);

		RichTextarea::SetAreaText(richEdits.back(), "");
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

	TabbedFiles::SetOnCloseTab(hTabs, CloseTabWithFile);
	TabbedFiles::SetNewTabWindow(hTabs, hNewTab = CreateWindow("STATIC", "", WS_CHILD | SS_GRAYFRAME, 5, 25, 780, 175, hWnd, NULL, hInstance, NULL));
	HWND createPanel = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD, 5, 5, 190, 60, hNewTab, NULL, hInstance, NULL);
	/*HWND panel0 = */CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_ETCHEDFRAME, 0, 0, 190, 60, createPanel, NULL, hInstance, NULL);
	HWND panel1 = CreateWindow("STATIC", "File name: ", WS_VISIBLE | WS_CHILD, 5, 5, 100, 20, createPanel, NULL, hInstance, NULL);
	hNewFilename = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD, 80, 5, 100, 20, createPanel, NULL, hInstance, NULL);
	hNewFile = CreateWindow("BUTTON", "Create", WS_VISIBLE | WS_CHILD, 5, 30, 100, 25, createPanel, NULL, hInstance, NULL);

	SetWindowLong(createPanel, GWL_WNDPROC, (LONG)(intptr_t)WndProc);

	SendMessage(panel1, WM_SETFONT, (WPARAM)fontDefault, 0);
	SendMessage(hNewFilename, WM_SETFONT, (WPARAM)fontDefault, 0);
	SendMessage(hNewFile, WM_SETFONT, (WPARAM)fontDefault, 0);

	UpdateWindow(hTabs);

	if(!richEdits.empty())
		ShowWindow(richEdits[0], SW_SHOW);
	else
		ShowWindow(hNewTab, SW_SHOW);

	RichTextarea::SetStatusBar(hStatus, 900);

	RichTextarea::SetTextStyle(0,    0,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(1,    0,   0, 255, false, false, false);
	RichTextarea::SetTextStyle(2,  128, 128, 128, false, false, false);
	RichTextarea::SetTextStyle(3,   50,  50,  50, false, false, false);
	RichTextarea::SetTextStyle(4,  136,   0,   0, false,  true, false);
	RichTextarea::SetTextStyle(5,    0,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(6,    0,   0,   0,  true, false, false);
	RichTextarea::SetTextStyle(7,  136,   0,   0, false, false, false);
	RichTextarea::SetTextStyle(8,    0, 150,   0, false, false, false);
	RichTextarea::SetTextStyle(9,    0, 150,   0, false,  true, false);
	RichTextarea::SetTextStyle(10, 255,   0,   0, false, false,  true);
	RichTextarea::SetTextStyle(11, 255,   0, 255, false, false, false);

	unsigned int width = (800 - 25) / 4;

	hCode = CreateWindow("EDIT", initError, WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		5, 225, width*2, 165, hWnd, NULL, hInstance, NULL);
	if(!hCode)
		return 0;
	ShowWindow(hCode, nCmdShow);
	UpdateWindow(hCode);
	SendMessage(hCode, WM_SETFONT, (WPARAM)fontMonospace, 0);

	hVars = CreateWindow(WC_TREEVIEW, "", WS_CHILD | WS_BORDER | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS,
		3*width+15, 225, width, 165, hWnd, NULL, hInstance, NULL);
	if(!hVars)
		return 0;
	ShowWindow(hVars, nCmdShow);
	UpdateWindow(hVars);

	hResult = CreateWindow("STATIC", "The result will be here", WS_CHILD, 110, 185, 300, 30, hWnd, NULL, hInstance, NULL);
	if(!hResult)
		return 0;
	ShowWindow(hResult, nCmdShow);
	UpdateWindow(hResult);
	SendMessage(hResult, WM_SETFONT, (WPARAM)fontDefault, 0);

	PostMessage(hWnd, WM_SIZE, 0, (394 << 16) + (900 - 16));

	SetTimer(hWnd, 1, 500, 0);
	return TRUE;
}

// zero-terminated safe sprintf
int	safeprintf(char* dst, size_t size, const char* src, ...)
{
	va_list args;
	va_start(args, src);

	int result = vsnprintf(dst, size, src, args);
	dst[size-1] = '\0';
	return result;
}

ExternTypeInfo	*codeTypes = NULL;
ExternFuncInfo	*codeFuntions = NULL;
ExternLocalInfo	*codeLocals = NULL;
unsigned int	*codeTypeExtra = NULL;
char			*codeSymbols = NULL;

const char* GetBasicVariableInfo(const ExternTypeInfo& type, char* ptr)
{
	static char val[256];

	switch(type.type)
	{
	case ExternTypeInfo::TYPE_CHAR:
		if(*(unsigned char*)ptr)
			safeprintf(val, 256, "'%c' (%d)", *(unsigned char*)ptr, (int)*(unsigned char*)ptr);
		else
			safeprintf(val, 256, "0");
		break;
	case ExternTypeInfo::TYPE_SHORT:
		safeprintf(val, 256, "%d", *(short*)ptr);
		break;
	case ExternTypeInfo::TYPE_INT:
		safeprintf(val, 256, type.subType == 0 ? "%d" : "0x%x", *(int*)ptr);
		break;
	case ExternTypeInfo::TYPE_LONG:
		safeprintf(val, 256, "%I64d", *(long long*)ptr);
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

void FillArrayVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	char name[256];
	HTREEITEM lastItem;

	unsigned int arrSize = (type.arrSize == ~0u) ? *(unsigned int*)(ptr + 4) : type.arrSize;
	if(type.arrSize == ~0u)
	{
		arrSize = *(unsigned int*)(ptr + 4);
		ptr = *(char**)ptr;
	}

	ExternTypeInfo	&subType = codeTypes[type.subType];
	for(unsigned int i = 0; i < arrSize; i++, ptr += subType.size)
	{
		if(i > 100)
			break;
		char *it = name;
		memset(name, 0, 256);

		it += safeprintf(it, 256 - int(it - name), "[%d]: ", i);

		if(subType.subCat == ExternTypeInfo::CAT_NONE || type.subCat == ExternTypeInfo::CAT_POINTER)
			it += safeprintf(it, 256 - int(it - name), " %s", GetBasicVariableInfo(subType, ptr));
		else if(&subType == &codeTypes[8])	// for typeid
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(ptr)].offsetToName);

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		FillVariableInfo(subType, ptr, lastItem);
	}
	if(arrSize > 100)
	{
		safeprintf(name, 256, "[100]-[%d]...", 100, arrSize);
		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);
	}
}

void FillComplexVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	char name[256];
	HTREEITEM lastItem;

	const char *memberName = codeSymbols + type.offsetToName + (unsigned int)strlen(codeSymbols + type.offsetToName) + 1;
	for(unsigned int i = 0; i < type.memberCount; i++)
	{
		char *it = name;
		memset(name, 0, 256);

		ExternTypeInfo	&memberType = codeTypes[codeTypeExtra[type.memberOffset + i]];

		it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + memberType.offsetToName, memberName);

		if(memberType.subCat == ExternTypeInfo::CAT_NONE || type.subCat == ExternTypeInfo::CAT_POINTER)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(memberType, ptr));
		else if(&memberType == &codeTypes[8])	// for typeid
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(ptr)].offsetToName);

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		FillVariableInfo(memberType, ptr, lastItem);

		memberName += (unsigned int)strlen(memberName) + 1;
		ptr += memberType.size;	// $$ alignment?
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
	helpInsert.item.pszText = name;
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

	if(nullcGetCurrentExecutor(NULL) == NULLC_X86)
		return;

	ExternFuncInfo	&func = codeFuntions[*(int*)(ptr + 4)];
	ExternTypeInfo	&returnType = codeTypes[codeTypeExtra[type.memberOffset]];

	char *it = name;
	it += safeprintf(it, 256 - int(it - name), "function %d %s %s(", *(int*)(ptr + 4), codeSymbols + returnType.offsetToName, codeSymbols + func.offsetToName);
	for(unsigned int arg = 0; arg < func.paramCount; arg++)
	{
		ExternLocalInfo &lInfo = codeLocals[func.offsetToFirstLocal + arg];
		it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);
	}
	it += safeprintf(it, 256 - int(it - name), ")");

	helpInsert.item.pszText = name;
	TreeView_InsertItem(hVars, &helpInsert);

	safeprintf(name, 256, "void ref context = 0x%x", *(int*)(ptr));
	helpInsert.item.pszText = name;
	HTREEITEM contextList = TreeView_InsertItem(hVars, &helpInsert);

	TVINSERTSTRUCT nextInsert;
	nextInsert.hParent = contextList;
	nextInsert.hInsertAfter = TVI_LAST;
	nextInsert.item.mask = TVIF_TEXT;
	nextInsert.item.cchTextMax = 0;

	ExternFuncInfo::Upvalue *upvalue = *(ExternFuncInfo::Upvalue**)(ptr);

	ExternLocalInfo *externals = &codeLocals[func.offsetToFirstLocal + func.localCount];
	for(unsigned int i = 0; i < func.externCount; i++)
	{
		char *it = name;
		ExternTypeInfo &externType = codeTypes[externals[i].type];

		it += safeprintf(it, 256 - int(it - name), "%s %s", codeSymbols + externType.offsetToName, codeSymbols + externals[i].offsetToName);

		if(externType.subCat == ExternTypeInfo::CAT_NONE || type.subCat == ExternTypeInfo::CAT_POINTER)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(externType, (char*)upvalue->ptr));
		else if(&externType == &codeTypes[8])	// for typeid
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(upvalue->ptr)].offsetToName);

		nextInsert.item.pszText = name;
		HTREEITEM lastItem = TreeView_InsertItem(hVars, &nextInsert);

		FillVariableInfo(externType, (char*)upvalue->ptr, lastItem);

		upvalue = (ExternFuncInfo::Upvalue*)((int*)upvalue + ((externals[i].size >> 2) < 3 ? 3 : 1 + (externals[i].size >> 2)));
	}
}

void FillVariableInfo(const ExternTypeInfo& type, char* ptr, HTREEITEM parent)
{
	if(&type == &codeTypes[8])	// typeid
		return;
	if(&type == &codeTypes[7])
	{
		FillAutoInfo(ptr, parent);	// auto ref
		return;
	}

	switch(type.subCat)
	{
	case ExternTypeInfo::CAT_FUNCTION:
		FillFunctionPointerInfo(type, ptr, parent);
		break;
	case ExternTypeInfo::CAT_CLASS:
		FillComplexVariableInfo(type, ptr, parent);
		break;
	case ExternTypeInfo::CAT_ARRAY:
		FillArrayVariableInfo(type, ptr, parent);
		break;
	}
}

void FillVariableInfoTree()
{
	TreeView_DeleteAllItems(hVars);

	TVINSERTSTRUCT	helpInsert;
	helpInsert.hParent = NULL;
	helpInsert.hInsertAfter = TVI_ROOT;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;
	HTREEITEM	lastItem;

	char	*data = (char*)nullcGetVariableData();

	unsigned int	variableCount = 0;
	unsigned int	functionCount = 0;
	ExternVarInfo	*vars = nullcDebugVariableInfo(&variableCount);
	codeTypes		= nullcDebugTypeInfo(NULL);
	codeFuntions	= nullcDebugFunctionInfo(&functionCount);
	codeLocals		= nullcDebugLocalInfo(NULL);
	codeTypeExtra	= nullcDebugTypeExtraInfo(NULL);
	codeSymbols		= nullcDebugSymbols();

	char name[256];
	unsigned int offset = 0;

	for(unsigned int i = 0; i < variableCount; i++)
	{
		ExternTypeInfo	&type = codeTypes[vars[i].type];

		char *it = name;
		memset(name, 0, 256);
		it += safeprintf(it, 256 - int(it - name), "0x%x: %s %s", data+vars[i].offset, codeSymbols + type.offsetToName, codeSymbols + vars[i].offsetToName);

		if(type.subCat == ExternTypeInfo::CAT_NONE || type.subCat == ExternTypeInfo::CAT_POINTER)
			it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(type, data + vars[i].offset));
		else if(&type == &codeTypes[8])	// for typeid
			it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(data + vars[i].offset)].offsetToName);

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		FillVariableInfo(type, data + vars[i].offset, lastItem);

		if(vars[i].offset + type.size > offset)
			offset = vars[i].offset + type.size;
	}

	nullcDebugBeginCallStack();
	while(int address = nullcDebugGetStackFrame())
	{
		// Find corresponding function
		int funcID = -1;
		for(unsigned int i = 0; i < functionCount; i++)
		{
			if(address >= codeFuntions[i].address && address < (codeFuntions[i].address + codeFuntions[i].codeSize))
			{
				funcID = i;
			}
		}

		// If we are not in global scope
		if(funcID != -1)
		{
			ExternFuncInfo	&function = codeFuntions[funcID];

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

			helpInsert.item.pszText = name;
			lastItem = TreeView_InsertItem(hVars, &helpInsert);

			TVINSERTSTRUCT localInfo;
			localInfo.hParent = lastItem;
			localInfo.hInsertAfter = TVI_LAST;
			localInfo.item.mask = TVIF_TEXT;
			localInfo.item.cchTextMax = 0;

			unsigned int offsetToNextFrame = 4;
			// Check every function local
			for(unsigned int i = 0; i < function.localCount; i++)
			{
				// Get information about local
				ExternLocalInfo &lInfo = codeLocals[function.offsetToFirstLocal + i];

				char *it = name;
				it += safeprintf(it, 256, "0x%x: %s %s", data + offset + lInfo.offset, codeSymbols + codeTypes[lInfo.type].offsetToName, codeSymbols + lInfo.offsetToName);

				if(codeTypes[lInfo.type].subCat == ExternTypeInfo::CAT_NONE || codeTypes[lInfo.type].subCat == ExternTypeInfo::CAT_POINTER)
					it += safeprintf(it, 256 - int(it - name), " = %s", GetBasicVariableInfo(codeTypes[lInfo.type], data + offset + lInfo.offset));
				else if(lInfo.type == 8)	// for typeid
					it += safeprintf(it, 256 - int(it - name), " = %s", codeSymbols + codeTypes[*(int*)(data + offset + lInfo.offset)].offsetToName);

				localInfo.item.pszText = name;
				TreeView_InsertItem(hVars, &localInfo);

				FillVariableInfo(codeTypes[lInfo.type], data + vars[i].offset, lastItem);

				if(lInfo.offset + lInfo.size > offsetToNextFrame)
					offsetToNextFrame = lInfo.offset + lInfo.size;
			}
			offset += offsetToNextFrame;
		}
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
	rres.finished = true;
	rres.result = goodRun;
	SendMessage(rres.wnd, WM_USER + 1, 0, 0);
	ExitThread(goodRun);
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

	switch(message) 
	{
	case WM_CREATE:
		runRes.finished = true;
		runRes.wnd = hWnd;

		breakResponse = CreateEvent(NULL, false, false, "NULLC Debug Break Continue Event");
		break;
	case WM_DESTROY:
		{
			FILE *tabInfo = fopen("nullc_tab.cfg", "wb");
			for(unsigned int i = 0; i < richEdits.size(); i++)
			{
				fprintf(tabInfo, "%s\r\n", TabbedFiles::GetTabInfo(hTabs, i).name);
				DestroyWindow(richEdits[i]);
			}
			fclose(tabInfo);
		}
		RichTextarea::UnregisterTextarea();
		PostQuitMessage(0);
		break;
	case WM_USER + 1:
		SetWindowText(hButtonCalc, "Run");

		if(runRes.result)
		{
			const char *val = nullcGetResult();

			_snprintf(result, 1024, "The answer is: %s [in %f]", val, runRes.time);
			result[1023] = '\0';
			SetWindowText(hResult, result);

			FillVariableInfoTree();
		}else{
			_snprintf(result, 1024, "%s", nullcGetLastError());
			result[1023] = '\0';
			SetWindowText(hCode, result);

			FillVariableInfoTree();
		}
		break;
	case WM_USER + 2:
		ShowWindow(hContinue, SW_SHOW);
		FillVariableInfoTree();

		break;
	case WM_COMMAND:
		wmId	= LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if((HWND)lParam == hContinue)
		{
			ShowWindow(hContinue, SW_HIDE);
			SetEvent(breakResponse);
		}else if((HWND)lParam == hButtonCalc){
			if(!runRes.finished){
				TerminateThread(calcThread, 0);
				ShowWindow(hContinue, SW_HIDE);
				SetWindowText(hButtonCalc, "Run");
				runRes.finished = true;
				break;
			}
			unsigned int id = TabbedFiles::GetCurrentTab(hTabs);
			const char *source = RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, id).window);

			for(unsigned int i = 0; i < richEdits.size(); i++)
			{
				if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, i).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, i).window)))
				{
					TabbedFiles::GetTabInfo(hTabs, i).dirty = false;
					RichTextarea::ResetUpdate(TabbedFiles::GetTabInfo(hTabs, i).window);
					InvalidateRect(hTabs, NULL, true);
				}
			}
#ifndef _DEBUG
			FreeConsole();
#endif
			SetWindowText(hCode, "");
			SetWindowText(hResult, "");

			nullcSetExecutor(Button_GetCheck(hJITEnabled) ? NULLC_X86 : NULLC_VM);

			nullres good = nullcBuild(source);
			nullcSaveListing("asm.txt");
		
			if(!good)
			{
				SetWindowText(hCode, nullcGetLastError());
			}else{
				SetWindowText(hButtonCalc, "Abort");
				calcThread = CreateThread(NULL, 1024*1024, CalcThread, &runRes, NULL, 0);
			}
		}else if((HWND)lParam == hNewFile){
			GetWindowText(hNewFilename, fileName, 512);
			SetWindowText(hNewFilename, "");
			if(!strstr(fileName, ".nc"))
				strcat(fileName, ".nc");
			// Check if file is already opened
			for(unsigned int i = 0; i < richEdits.size(); i++)
			{
				if(strcmp(TabbedFiles::GetTabInfo(hTabs, i).name, fileName) == 0)
				{
					TabbedFiles::SetCurrentTab(hTabs, i);
					return 0;
				}
			}
			FILE *fNew = fopen(fileName, "rb");

			char *filePart = NULL;
			GetFullPathName(fileName, 512, result, &filePart);
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
				}
				fclose(fNew);
			}else{
				richEdits.push_back(CreateWindow("NULLCTEXT", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER, 5, 25, areaWidth, areaHeight, ::hWnd, NULL, hInst, NULL));
				TabbedFiles::AddTab(hTabs, result, richEdits.back());
				TabbedFiles::SetCurrentTab(hTabs, (int)richEdits.size() - 1);
				TabbedFiles::GetTabInfo(hTabs, (int)richEdits.size() - 1).dirty = true;
			}
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
					}
					file += strlen(file) + 1;
				}
			}
			SetCurrentDirectory(result + 512);
			break;
		case ID_FILE_SAVE:
			{
				unsigned int id = TabbedFiles::GetCurrentTab(hTabs);
				if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, id).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, id).window)))
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
				if(SaveFileFromTab(TabbedFiles::GetTabInfo(hTabs, i).name, RichTextarea::GetAreaText(TabbedFiles::GetTabInfo(hTabs, i).window)))
				{
					TabbedFiles::GetTabInfo(hTabs, i).dirty = false;
					RichTextarea::ResetUpdate(TabbedFiles::GetTabInfo(hTabs, i).window);
					InvalidateRect(hTabs, NULL, true);
				}
			}
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
		unsigned int id = TabbedFiles::GetCurrentTab(hTabs);

		EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, TabbedFiles::GetTabInfo(hTabs, id).dirty ? MF_ENABLED : MF_DISABLED);

		for(unsigned int i = 0; i < richEdits.size(); i++)
		{
			if(!TabbedFiles::GetTabInfo(hTabs, i).dirty && RichTextarea::NeedUpdate(TabbedFiles::GetTabInfo(hTabs, i).window))
			{
				TabbedFiles::GetTabInfo(hTabs, i).dirty = true;
				InvalidateRect(hTabs, NULL, false);
			}
		}

		HWND wnd = TabbedFiles::GetTabInfo(hTabs, id).window;
		if(!RichTextarea::NeedUpdate(wnd) || (GetTickCount()-lastUpdate < 100))
			break;
		SetWindowText(hCode, "");

		RichTextarea::BeginStyleUpdate(wnd);
		if(!colorer->ColorText(wnd, (char*)RichTextarea::GetAreaText(wnd), RichTextarea::SetStyleToSelection))
		{
			SetWindowText(hCode, colorer->GetError().c_str());
		}
		RichTextarea::EndStyleUpdate(wnd);
		RichTextarea::UpdateArea(wnd);
		RichTextarea::ResetUpdate(wnd);
		needTextUpdate = false;
		lastUpdate = GetTickCount();
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
		SetWindowPos(hTabs,			HWND_TOP, mainPadding, 4, width - mainPadding * 2, tabHeight, NULL);

		areaWidth = width - mainPadding * 2;
		areaHeight = topHeight - tabHeight;
		for(unsigned int i = 0; i < richEdits.size(); i++)
			SetWindowPos(richEdits[i],	HWND_TOP, mainPadding, mainPadding + tabHeight, width - mainPadding * 2, topHeight - tabHeight, NULL);
		SetWindowPos(hNewTab,		HWND_TOP, mainPadding, mainPadding + tabHeight, width - mainPadding * 2, topHeight - tabHeight, NULL);

		unsigned int buttonWidth = 120;
		unsigned int resultWidth = width - 3 * buttonWidth - 3 * mainPadding - subPadding * 3;

		unsigned int calcOffsetX = mainPadding;
		unsigned int resultOffsetX = calcOffsetX * 2 + buttonWidth * 2 + subPadding;
		unsigned int x86OffsetX = resultOffsetX + resultWidth + subPadding;

		SetWindowPos(hButtonCalc,	HWND_TOP, calcOffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);
		SetWindowPos(hResult,		HWND_TOP, resultOffsetX, middleOffsetY, resultWidth, middleHeight, NULL);
		SetWindowPos(hContinue,		HWND_TOP, calcOffsetX * 2 + buttonWidth, middleOffsetY, buttonWidth, middleHeight, NULL);
		SetWindowPos(hJITEnabled,	HWND_TOP, x86OffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);

		unsigned int bottomOffsetY = middleOffsetY + middleHeight + subPadding;

		unsigned int bottomWidth = width - 2 * mainPadding - 2 * subPadding;
		unsigned int leftOffsetX = mainPadding;
		unsigned int leftWidth = int(bottomWidth / 100.0 * 75.0);	// 75 %
		unsigned int rightOffsetX = leftOffsetX + leftWidth + subPadding;
		unsigned int rightWidth = int(bottomWidth / 100.0 * 25.0);	// 25 %

		SetWindowPos(hCode,			HWND_TOP, leftOffsetX, bottomOffsetY, leftWidth, bottomHeight-16, NULL);
		SetWindowPos(hVars,			HWND_TOP, rightOffsetX, bottomOffsetY, rightWidth, bottomHeight-16, NULL);

		SetWindowPos(hStatus,		HWND_TOP, 0, height-16, width, height, NULL);

		InvalidateRect(hNewTab, NULL, true);
		InvalidateRect(hButtonCalc, NULL, true);
		InvalidateRect(hResult, NULL, true);
		InvalidateRect(hJITEnabled, NULL, true);
		InvalidateRect(hStatus, NULL, true);
		InvalidateRect(hVars, NULL, true);
	}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}