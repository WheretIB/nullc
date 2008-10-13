#include "stdafx.h"
#include "SuperCalc.h"

#include "commctrl.h"
#include "richedit.h"
#pragma comment(lib, "comctl32.lib")
#include <windowsx.h>

#include <MMSystem.h>
#pragma comment(lib, "Winmm.lib")

#include "Colorer.h"
#include "Compiler.h"
#include "Executor.h"
#include "Executor_X86.h"

#define MAX_LOADSTRING 100

HINSTANCE hInst;
char szTitle[MAX_LOADSTRING];
char szWindowClass[MAX_LOADSTRING];

WORD				MyRegisterClass(HINSTANCE hInstance);
bool				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//Window handles
HWND hWnd;
HWND hButtonCalc;	//calculate button
HWND hButtonCalcX86;//calculate button
HWND hDoOptimize;	//optimization checkbox
HWND hTextArea;		//code text area (rich edit)
HWND hResult;		//label with execution result
HWND hCode;			//disabled text area for errors and asm-like code output
HWND hLog;			//disabled text area for log information of AST creation
HWND hVars;			//disabled text area that shows values of all variables in global scope

//colorer, compiler and executor
Colorer*	colorer;
Compiler*	compiler;
Executor*	executor;
ExecutorX86*	executorX86;
CommandList*		commands;

//for text update
bool needTextUpdate;
DWORD lastUpdate;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	needTextUpdate = true;
	lastUpdate = GetTickCount();

	commands = new CommandList();
	colorer = NULL;
	compiler = new Compiler(commands);
	executor = new Executor(commands, compiler->GetVariableInfo());
	executorX86 = new ExecutorX86(commands, compiler->GetVariableInfo());
	
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SUPERCALC, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if(!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}

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
	return (int) msg.wParam;
}

WORD MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_SUPERCALC);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_SUPERCALC;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

char* GetLastErrorDesc()
{
	char* msgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&msgBuf), 0, NULL);
	return msgBuf;
}

bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		100, 100, 900, 450, NULL, NULL, hInstance, NULL);
	if(!hWnd)
		return 0;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	hButtonCalc = CreateWindow("BUTTON", "Calculate", WS_CHILD,
		5, 185, 100, 30, hWnd, NULL, hInstance, NULL);
	if(!hButtonCalc)
		return 0;
	ShowWindow(hButtonCalc, nCmdShow);
	UpdateWindow(hButtonCalc);

	hButtonCalcX86 = CreateWindow("BUTTON", "Run Native X86", WS_CHILD,
		800-140, 185, 130, 30, hWnd, NULL, hInstance, NULL);
	if(!hButtonCalcX86)
		return 0;
	ShowWindow(hButtonCalcX86, nCmdShow);
	UpdateWindow(hButtonCalcX86);

	hDoOptimize = CreateWindow("BUTTON", "Optimize", BS_AUTOCHECKBOX | WS_CHILD,
		800-240, 185, 90, 30, hWnd, NULL, hInstance, NULL);
	if(!hDoOptimize)
		return 0;
	ShowWindow(hDoOptimize, nCmdShow);
	UpdateWindow(hDoOptimize);

	INITCOMMONCONTROLSEX commControlTypes;
	commControlTypes.dwSize = sizeof(INITCOMMONCONTROLSEX);
	commControlTypes.dwICC = ICC_TREEVIEW_CLASSES;
	int commControlsAvailable = InitCommonControlsEx(&commControlTypes);
	if(!commControlsAvailable)
		return 0;

	HMODULE sss = LoadLibrary("RICHED32.dll");

	FILE *startText = fopen("code.txt", "rb");
	char *fileContent = NULL;
	if(startText)
	{
		fseek(startText, 0, SEEK_END);
		UINT textSize = ftell(startText);
		fseek(startText, 0, SEEK_SET);
		fileContent = new char[textSize+1];
		fread(fileContent, 1, textSize, startText);
		fileContent[textSize] = 0;
		fclose(startText);
	}
	hTextArea = CreateWindow("RICHEDIT", fileContent ? fileContent : "int a = 5;\r\nint ref b = &a;\r\nreturn 1;",
		WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
		5, 5, 780, 175, hWnd, NULL, hInstance, NULL);
	if(!hTextArea)
		return 0;
	ShowWindow(hTextArea, nCmdShow);
	UpdateWindow(hTextArea);

	colorer = new Colorer(hTextArea);
	colorer->InitParser();

	SendMessage(hTextArea, EM_SETEVENTMASK, 0, ENM_CHANGE);
	UINT widt = (800-25)/4;

	hCode = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		5, 225, widt*2, 165, hWnd, NULL, hInstance, NULL);
	if(!hCode)
		return 0;
	ShowWindow(hCode, nCmdShow);
	UpdateWindow(hCode);

	hLog = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		2*widt+10, 200, widt-100, 165, hWnd, NULL, hInstance, NULL);
	if(!hLog)
		return 0;
	ShowWindow(hLog, nCmdShow);
	UpdateWindow(hLog);

	hVars = CreateWindow(WC_TREEVIEW, "", WS_CHILD | WS_BORDER | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS,
		3*widt+15, 225, widt, 165, hWnd, NULL, hInstance, NULL);
	if(!hVars)
		return 0;
	ShowWindow(hVars, nCmdShow);
	UpdateWindow(hVars);

	hResult = CreateWindow("STATIC", "The result will be here", WS_CHILD,
		110, 185, 300, 30, hWnd, NULL, hInstance, NULL);
	if(!hResult)
		return 0;
	ShowWindow(hResult, nCmdShow);
	UpdateWindow(hResult);

	PostMessage(hWnd, WM_SIZE, 0, (394<<16)+(900-16));

	SetTimer(hWnd, 1, 500, 0);
	return TRUE;
}
bool RunCallback(UINT cmdNum)
{
	std::string str;
	char num[32];
	str = std::string("SuperCalc [") + _itoa(cmdNum, num, 10) + "]";
	SetWindowText(hWnd, str.c_str());
	UpdateWindow(hWnd);
	static int ignore = false;
	if(cmdNum % 300000000 == 0 && !ignore)
	{
		int butSel = MessageBox(hWnd, "Code execution can take a long time. Do you wish to continue?\r\nPress Cancel if you don't want to see this warning again", "Warning: long execution time", MB_YESNOCANCEL);
		if(butSel == IDYES)
			return true;
		else if(butSel == IDNO)
			return false;
		else
			ignore = true;
	}
	return true;
}

char *variableData = NULL;
void FillComplexVariableInfo(TypeInfo* type, int address, HTREEITEM parent);
void FillArrayVariableInfo(TypeInfo* type, int address, HTREEITEM parent);

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

	for(UINT mn = 0; mn < type->memberData.size(); mn++)
	{
		TypeInfo::MemberInfo &mInfo = type->memberData[mn];

		sprintf(name, "%s %s = ", mInfo.type->GetTypeName().c_str(), mInfo.name.c_str());

		if(mInfo.type->type != TypeInfo::TYPE_COMPLEX && mInfo.type->arrLevel == 0)
			strcat(name, GetSimpleVariableValue(mInfo.type, address+type->memberData[mn].offset));

		if(mInfo.type->arrLevel == 1 && mInfo.type->subType->type == TypeInfo::TYPE_CHAR)
			sprintf(name+strlen(name), "\"%s\"", (char*)(variableData+address+type->memberData[mn].offset));

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		if(mInfo.type->arrLevel != 0)
		{
			FillArrayVariableInfo(mInfo.type, address+type->memberData[mn].offset, lastItem);
		}else if(mInfo.type->type == TypeInfo::TYPE_COMPLEX){
			FillComplexVariableInfo(mInfo.type, address+type->memberData[mn].offset, lastItem);
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
	for(UINT n = 0; n < type->arrSize; n++, address += subType->size)
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
			sprintf(name+strlen(name), "\"%s\"", (char*)(variableData+address));

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
	std::vector<VariableInfo> *varInfo = compiler->GetVariableInfo();
	TreeView_DeleteAllItems(hVars);

	TVINSERTSTRUCT helpInsert;
	helpInsert.hParent = NULL;
	helpInsert.hInsertAfter = TVI_ROOT;
	helpInsert.item.mask = TVIF_TEXT;
	helpInsert.item.cchTextMax = 0;

	UINT address = 0;
	char name[256];
	HTREEITEM lastItem;
	for(UINT i = 0; i < varInfo->size(); i++)
	{
		VariableInfo &currVar = (*varInfo)[i];
		address = currVar.pos;
		sprintf(name, "%d: %s%s %s = ", address, (currVar.isConst ? "const " : ""), (*currVar.varType).GetTypeName().c_str(), currVar.name.c_str());

		if(currVar.varType->type != TypeInfo::TYPE_COMPLEX && currVar.varType->arrLevel == 0)
			strcat(name, GetSimpleVariableValue(currVar.varType, address));

		if(currVar.varType->arrLevel == 1 && currVar.varType->subType->type == TypeInfo::TYPE_CHAR)
			sprintf(name+strlen(name), "\"%s\"", (char*)(variableData+address));

		helpInsert.item.pszText = name;
		lastItem = TreeView_InsertItem(hVars, &helpInsert);

		if(currVar.varType->arrLevel != 0)
		{
			FillArrayVariableInfo(currVar.varType, address, lastItem);
		}else if(currVar.varType->type == TypeInfo::TYPE_COMPLEX){
			FillComplexVariableInfo(currVar.varType, address, lastItem);
		}
		address += currVar.varType->size;
	}

	//just for fun, save the parameter data to bmp
	FILE *fBMP = fopen("funny.bmp", "wb");
	fwrite(variableData+24, 1, address-24, fBMP);
	fclose(fBMP);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static char* buf = NULL;
	if(!buf)
		buf = new char[400000];
	memset(buf, 0, GetWindowTextLength(hTextArea)+5);
	switch (message) 
	{
	
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam);
		if((HWND)lParam == hButtonCalc)
		{
			GetWindowText(hTextArea, buf, 400000);
			bool good;

			ostringstream ostr;
			try
			{
				good = compiler->Compile(buf);
				compiler->GenListing();
			}catch(const std::string& str){
				good = false;
				ostr << str;
			}
			if(good)
			{
				executorX86->GenListing();

				executor->SetCallback(RunCallback);
				try
				{
					UINT time = executor->Run();
					string val = executor->GetResult();
					ostr.precision(20);
					ostr << "The answer is: " << val << " [in: " << time << "]";

					variableData = executor->GetVariableData();
					FillVariableInfoTree();
				}catch(const std::string& str){
					ostr.str("");
					ostr << str;
				}
			}
			if(good)
				SetWindowText(hCode, compiler->GetListing().c_str());
			else
				SetWindowText(hCode, ostr.str().c_str());
			SetWindowText(hLog, compiler->GetLog().c_str());
			string str = ostr.str();
			if(good)
				SetWindowText(hResult, str.c_str());
		}
		if((HWND)lParam == hButtonCalcX86)
		{
			GetWindowText(hTextArea, buf, 400000);
			bool good;
			ostringstream ostr;
			try
			{
				good = compiler->Compile(buf);
				compiler->GenListing();
			}catch(const std::string& str){
				good = false;
				ostr << str;
			}
			bool opti = !!Button_GetCheck(hDoOptimize);
			executorX86->SetOptimization(opti);
			if(good)
			{
				try
				{
					executorX86->GenListing();
					UINT time = executorX86->Run();
					string val = executorX86->GetResult();
					ostr.precision(20);
					ostr << "The answer is: " << val << " [in: " << time << "]";

					variableData = executorX86->GetVariableData();
					FillVariableInfoTree();
				}catch(const std::string& str){
					ostr.str("");
					ostr << str;
				}
			}
			if(good)
				SetWindowText(hCode, compiler->GetListing().c_str());
			else
				SetWindowText(hCode, ostr.str().c_str());
			SetWindowText(hLog, executorX86->GetListing().c_str());
			string str = ostr.str();
			if(good)
				SetWindowText(hResult, str.c_str());
		}
		if((HWND)lParam == hTextArea)
		{
			if(wmEvent == EN_CHANGE)
			{
				needTextUpdate = true;
				lastUpdate = GetTickCount();
			}
		}
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			//DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_FILE_SAVE:
			break;
		case ID_FILE_LOAD:
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
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
	{
		if(!needTextUpdate || (GetTickCount()-lastUpdate < 500))
			break;
		bool bRetFocus = false;
		CHARRANGE cr;
		if(GetFocus() == hTextArea)
		{
			bRetFocus=true;
			SendMessage(hTextArea, (UINT)EM_EXGETSEL, 0L, (LPARAM)&cr);  
			SetFocus(hWnd);
		}
		string str = "";
		SetWindowText(hCode, str.c_str());
		ostringstream ostr;
		try
		{
			colorer->ColorText();
		}catch(const std::string& strerr){
			ostr << strerr;
			str = ostr.str();
			SetWindowText(hCode, str.c_str());
		}
		if(bRetFocus)
		{
			SetFocus(hTextArea);
			Edit_SetSel(hTextArea, cr.cpMin, cr.cpMax);
		}
		needTextUpdate = false;
	}
		break;
	case WM_LBUTTONUP:
		break;
	case WM_SIZE:
	{
		SetWindowPos(hTextArea, HWND_TOP, 5,5,LOWORD(lParam)-10, (int)(4.0/9.0*HIWORD(lParam)), NULL);
		SetWindowPos(hButtonCalc, HWND_TOP, 5,7+(int)(4.0/9.0*HIWORD(lParam)),100, 30, NULL);
		SetWindowPos(hButtonCalcX86, HWND_TOP, (int)(LOWORD(lParam))-135,7+(int)(4.0/9.0*HIWORD(lParam)),130, 30, NULL);
		SetWindowPos(hDoOptimize, HWND_TOP, (int)(LOWORD(lParam))-235,7+(int)(4.0/9.0*HIWORD(lParam)),95, 30, NULL);
		SetWindowPos(hResult, HWND_TOP, 110,7+(int)(4.0/9.0*HIWORD(lParam)),(int)(LOWORD(lParam))-345, 30, NULL);
		UINT widt = (LOWORD(lParam)-20)/4;
		SetWindowPos(hCode, HWND_TOP, 5,40+(int)(4.0/9.0*HIWORD(lParam)),2*widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
		SetWindowPos(hLog, HWND_TOP, 2*widt+10,40+(int)(4.0/9.0*HIWORD(lParam)),widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
		SetWindowPos(hVars, HWND_TOP, 3*widt+15,40+(int)(4.0/9.0*HIWORD(lParam)),widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
	}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}