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
#include "NULLC/ParseClass.h"

#include "Colorer.h"

#include "UnitTests.h"

#include "GUI/RichTextarea.h"
#include "GUI/TabbedFiles.h"

// NULLC modules
#include "NULLC/includes/file.h"
#include "NULLC/includes/io.h"
#include "NULLC/includes/math.h"
#include "NULLC/includes/string.h"

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
HWND hWnd;
HWND hButtonCalc;	// calculate button
HWND hJITEnabled;	// jit enable button
HWND hTabs;
HWND hNewTab, hNewFilename, hNewFile;
HWND hResult;		// label with execution result
HWND hCode;			// disabled text area for errors and asm-like code output
HWND hVars;			// disabled text area that shows values of all variables in global scope
HWND hStatus;

unsigned int areaWidth = 400, areaHeight = 300;

HFONT	fontMonospace, fontDefault;

Colorer*	colorer;

std::vector<HWND>	richEdits;

// for text update
bool needTextUpdate;
DWORD lastUpdate;

char *variableData = NULL;
void FillComplexVariableInfo(TypeInfo* type, int address, HTREEITEM parent);
void FillArrayVariableInfo(TypeInfo* type, int address, HTREEITEM parent);

int myGetTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return int(temp*1000.0);
}

double myGetPreciseTime()
{
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	double temp = double(count.QuadPart) / double(freq.QuadPart);
	return temp*1000.0;
}

void draw_rect(int x, int y, int width, int height, int color)
{
	(void)x; (void)y; (void)width; (void)height; (void)color;
	//DWORD written;
	//char buf[64];
	//printf("%d %d %d %d %d\r\n", x, y, width, height, color);
	//fwrite(buf, strlen(buf), 1, zeuxOut);
}

char typeTest(int x, short y, char z, int d, long long u, float m, int s, double k, int t)
{
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);

	printf("%d %d %d %d %I64d %f %d %f %d", x, y, z, d, u, m, s, k, t);
	return 12;
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

	#define REGISTER(func, proto) nullcAddExternalFunction((void (*)())func, proto)
	REGISTER(draw_rect, "void draw_rect(int x, int y, int width, int height, int color);");

	REGISTER(typeTest, "char typeTest(int x, short y, char z, int d, long u, float m, int s, double k, int t);");

	REGISTER(myGetTime, "int clock();");

	colorer = NULL;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SUPERCALC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	lastUpdate = 0;

	// Perform application initialization:
	if(!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	nullcInitFileModule();
	nullcInitIOModule();
	nullcInitMathModule();
	nullcInitStringModule();

	nullcInitCanvasModule();
	nullcInitWindowModule();

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

	nullcDeinitFileModule();
	nullcDeinitIOModule();
	nullcDeinitMathModule();
	nullcDeinitStringModule();

	nullcDeinitCanvasModule();
	nullcDeinitWindowModule();

	nullcDeinit();

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

	hButtonCalc = CreateWindow("BUTTON", "Calculate", WS_VISIBLE | WS_CHILD, 5, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	if(!hButtonCalc)
		return 0;
	SendMessage(hButtonCalc, WM_SETFONT, (WPARAM)fontDefault, 0);

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

	hCode = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
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

const char* GetSimpleVariableValue(TypeInfo* type, int address)
{
	static char val[256];
	if(type->type == TypeInfo::TYPE_INT)
	{
		sprintf(val, "%d", *((int*)&variableData[address]));
	}else if(type->type == TypeInfo::TYPE_SHORT){
		sprintf(val, "%d", *((short*)&variableData[address]));
	}else if(type->type == TypeInfo::TYPE_CHAR){
		if(*((unsigned char*)&variableData[address]))
			sprintf(val, "'%c' (%d)", *((unsigned char*)&variableData[address]), (int)(*((unsigned char*)&variableData[address])));
		else
			sprintf(val, "0");
	}else if(type->type == TypeInfo::TYPE_FLOAT){
		sprintf(val, "%f", *((float*)&variableData[address]));
	}else if(type->type == TypeInfo::TYPE_LONG){
		sprintf(val, "%I64d", *((long long*)&variableData[address]));
	}else if(type->type == TypeInfo::TYPE_DOUBLE){
		sprintf(val, "%f", *((double*)&variableData[address]));
	}else{
		sprintf(val, "not basic type");
	}
	return val;
}

void FillComplexVariableInfo(TypeInfo* type, int address, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	char name[256];
	HTREEITEM lastItem;

	for(TypeInfo::MemberVariable *curr = type->firstVariable; curr; curr = curr->next)
	{
		TypeInfo::MemberVariable &mInfo = *curr;

		sprintf(name, "%s %s = ", mInfo.type->GetFullTypeName(), mInfo.name);

		if(mInfo.type->type != TypeInfo::TYPE_COMPLEX && mInfo.type->arrLevel == 0)
			strcat(name, GetSimpleVariableValue(mInfo.type, address + mInfo.offset));

		if(mInfo.type->arrLevel == 1 && mInfo.type->arrSize != -1 && mInfo.type->subType->type == TypeInfo::TYPE_CHAR)
			sprintf(name+strlen(name), "\"%s\"", (char*)(variableData + address + mInfo.offset));
		if(mInfo.type->arrSize == -1)
			sprintf(name+strlen(name), "address: %d, size: %d", *((int*)&variableData[address]), *((int*)&variableData[address+4]));

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		if(mInfo.type->arrLevel != 0)
		{
			FillArrayVariableInfo(mInfo.type, address + mInfo.offset, lastItem);
		}else if(mInfo.type->type == TypeInfo::TYPE_COMPLEX){
			FillComplexVariableInfo(mInfo.type, address + mInfo.offset, lastItem);
		}
	}
}

void FillArrayVariableInfo(TypeInfo* type, int address, HTREEITEM parent)
{
	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = parent;
	helpInsert.hInsertAfter = TVI_LAST;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	TypeInfo* subType = type->subType;
	char name[256];
	HTREEITEM lastItem;

	unsigned int arrSize = type->arrSize;
	if(arrSize == -1)
	{
		arrSize = *((int*)&variableData[address+4]);
		address = *((int*)&variableData[address]) - (int)(intptr_t)variableData;
	}
	for(unsigned int n = 0; n < arrSize; n++, address += subType->size)
	{
		if(n > 100)
		{
			sprintf(name, "[%d]-[%d]...", n, type->arrSize);
			helpInsert.item.pszText = name;
			lastItem = TreeView_InsertItem(hVars, &helpInsert);
			break;
		}
		sprintf(name, "[%d]: ", n);

		if(subType->arrLevel == 1 && subType->subType->type == TypeInfo::TYPE_CHAR)
			sprintf(name+strlen(name), "\"%s\"", subType->arrSize != -1 ? (char*)(variableData+address) : *((char**)(variableData + address)));
		if(subType->arrSize == -1)
			sprintf(name+strlen(name), "address: %d, size: %d", *((int*)&variableData[address]), *((int*)&variableData[address+4]));

		if(subType->type != TypeInfo::TYPE_COMPLEX && subType->arrLevel == 0)
			strcat(name, GetSimpleVariableValue(subType, address));

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		if(subType->arrLevel != 0)
		{
			FillArrayVariableInfo(subType, address, lastItem);
		}else if(subType->type == TypeInfo::TYPE_COMPLEX){
			FillComplexVariableInfo(subType, address, lastItem);
		}
	}
}

void FillVariableInfoTree()
{
	unsigned int varCount = 0;
	VariableInfo **varInfo = (VariableInfo**)nullcGetVariableInfo(&varCount);
	TreeView_DeleteAllItems(hVars);

	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = NULL;
	helpInsert.hInsertAfter = TVI_ROOT;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	unsigned int addressShift = 0;
	unsigned int currModule = (*varInfo)->pos >> 24;

	char name[256];
	HTREEITEM lastItem;
	for(unsigned int i = 0; i < varCount; i++)
	{
		VariableInfo &currVar = *(*(varInfo+i));
		if(currModule != (currVar.pos >> 24))
		{
			addressShift += ((*(varInfo+i-1))->pos & 0x00ffffff) + (*(varInfo+i-1))->varType->size;
			currModule = currVar.pos >> 24;
		}
		unsigned int address = (currVar.pos & 0x00ffffff) + addressShift;
		sprintf(name, "%d: %s %.*s = ", address, (*currVar.varType).GetFullTypeName(), currVar.name.end-currVar.name.begin, currVar.name.begin);

		if(currVar.varType->type != TypeInfo::TYPE_COMPLEX && currVar.varType->arrLevel == 0)
			strcat(name, GetSimpleVariableValue(currVar.varType, address));

		char *cPos = name+strlen(name);
		if(currVar.varType->arrLevel == 1 && currVar.varType->subType->type == TypeInfo::TYPE_CHAR)
			_snprintf(cPos, 255-int(cPos - name), "\"%s\"", currVar.varType->arrSize != -1 ? (char*)(variableData + address) : *((char**)(variableData + address)));
		name[255] = 0;
		cPos = name+strlen(name);
		if(currVar.varType->arrSize == -1)
			_snprintf(cPos, 255-int(cPos - name), "address: %d, size: %d", *((int*)&variableData[address]), *((int*)&variableData[address+4]));
		name[255] = 0;

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		if(currVar.varType->arrLevel != 0)
		{
			FillArrayVariableInfo(currVar.varType, address, lastItem);
		}else if(currVar.varType->type == TypeInfo::TYPE_COMPLEX){
			FillComplexVariableInfo(currVar.varType, address, lastItem);
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
		SetWindowText(hButtonCalc, "Calculate");

		if(runRes.result)
		{
			const char *val = nullcGetResult();

			_snprintf(result, 1024, "The answer is: %s [in %f]", val, runRes.time);
			result[1023] = '\0';
			SetWindowText(hResult, result);

			variableData = (char*)nullcGetVariableData();
			FillVariableInfoTree();
		}else{
			_snprintf(result, 1024, "%s", nullcGetRuntimeError());
			result[1023] = '\0';
			SetWindowText(hCode, result);
		}
		break;
	case WM_COMMAND:
		wmId	= LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if((HWND)lParam == hButtonCalc)
		{
			if(!runRes.finished)
			{
				TerminateThread(calcThread, 0);
				SetWindowText(hButtonCalc, "Calculate");
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

			nullres good = nullcCompile(source);
			nullcSaveListing("asm.txt");
		
			if(good)
			{
				char *bytecode = NULL;
				nullcGetBytecode(&bytecode);
				nullcClean();
				if(!nullcLinkCode(bytecode, 1))
				{
					good = false;
					SetWindowText(hCode, nullcGetRuntimeError());
				}
				delete[] bytecode;
			}else{
				SetWindowText(hCode, nullcGetCompilationError());
			}

			if(good)
			{
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
		unsigned int resultWidth = width - 2 * buttonWidth - 2 * mainPadding - subPadding * 3;

		unsigned int calcOffsetX = mainPadding;
		unsigned int resultOffsetX = calcOffsetX + buttonWidth + subPadding;
		unsigned int x86OffsetX = resultOffsetX + resultWidth + subPadding;

		SetWindowPos(hButtonCalc,	HWND_TOP, calcOffsetX, middleOffsetY, buttonWidth, middleHeight, NULL);
		SetWindowPos(hResult,		HWND_TOP, resultOffsetX, middleOffsetY, resultWidth, middleHeight, NULL);
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