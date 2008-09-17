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
HWND hButtonCalcX86;	//calculate button
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

bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		100, 100, 800, 450, NULL, NULL, hInstance, NULL);
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

	InitCommonControls();
	HMODULE sss = LoadLibrary("RICHED32.dll");

	//hTextArea = CreateWindow("RICHEDIT", "func double test(float x, float y){ /*teste*/return x**2*y; }\r\nvar int a=5;\r\nvar float b, c[3]=14**2-134;\r\nvar double d[10];\r\nfor(var int i = 0; i< 10; i++)\r\nd[i] = test(i*2, i-2);\r\nvar double n=1;\r\nwhile(1){ n*=2; if(n>1000) break; }\r\nreturn 2+test(2, 3)+a**b;", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
	//	5, 5, 780, 175, hWnd, NULL, hInstance, NULL);
	//hTextArea = CreateWindow("RICHEDIT", "func int test(int x, int y, int z){return x*y+z;}\r\nreturn 1+test(2, 3, 4);", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
	//	5, 5, 780, 175, hWnd, NULL, hInstance, NULL);
	hTextArea = CreateWindow("RICHEDIT", "var int a = 5;\r\nvar int ref b = &a;\r\nreturn 1;", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
		5, 5, 780, 175, hWnd, NULL, hInstance, NULL);
	if(!hTextArea)
		return 0;
	ShowWindow(hTextArea, nCmdShow);
	UpdateWindow(hTextArea);

	colorer = new Colorer(hTextArea);
	colorer->InitParser();

	SendMessage(hTextArea, EM_SETEVENTMASK, 0, ENM_CHANGE);
	UINT widt = (800-25)/3;

	hCode = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		5, 225, widt, 165, hWnd, NULL, hInstance, NULL);
	if(!hCode)
		return 0;
	ShowWindow(hCode, nCmdShow);
	UpdateWindow(hCode);

	hLog = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		widt+10, 225, widt, 165, hWnd, NULL, hInstance, NULL);
	if(!hLog)
		return 0;
	ShowWindow(hLog, nCmdShow);
	UpdateWindow(hLog);

	hVars = CreateWindow("EDIT", "", WS_CHILD | WS_BORDER |  WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
		2*widt+15, 225, widt, 165, hWnd, NULL, hInstance, NULL);
	if(!hVars)
		return 0;
	ShowWindow(hVars, nCmdShow);
	UpdateWindow(hVars);

	hResult = CreateWindow("STATIC", "The result will be here", WS_CHILD,
		110, 185, 400, 30, hWnd, NULL, hInstance, NULL);
	if(!hResult)
		return 0;
	ShowWindow(hResult, nCmdShow);
	UpdateWindow(hResult);

	PostMessage(hWnd, WM_SIZE, 0, (394<<16)+784);

	SetTimer(hWnd, 1, 100, 0);
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
			//mparser.SetRichEdit(hTextArea);
			
			ostringstream ostr;
			try
			{
				//mparser.Color(buf);
				good = compiler->Compile(buf);//mparser.Parse(buf);
				compiler->GenListing();
			}catch(const std::string& str){
				good = false;
				ostr << str;
			}
			if(good)
			{
				executorX86->GenListing();

				executor->SetCallback(RunCallback);//mparser.SetCallback(RunCallback);
				try
				{
					UINT time = executor->Run();
					string val = executor->GetResult();
					ostr.precision(20);
					ostr << "The answer is: " << val << " [in: " << time << "]";

					SetWindowText(hVars, executor->GetVarInfo().c_str());
				}catch(const std::string& str){
					ostr.str("");
					ostr << str;
				}
				//mparser.GetListing();
			}
			if(good)
				SetWindowText(hCode, compiler->GetListing().c_str());
			else
				SetWindowText(hCode, ostr.str().c_str());
			//SetWindowText(hLog, compiler->GetLog().c_str());
			SetWindowText(hLog, executorX86->GetListing().c_str());
			string str = ostr.str();
			if(good)
				SetWindowText(hResult, str.c_str());
			//delete[] buf;
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
			if(good)
			{
				try
				{
					executorX86->GenListing();
					UINT time = executorX86->Run();
					string val = executorX86->GetResult();
					ostr.precision(20);
					ostr << "The answer is: " << val << " [in: " << time << "]";

					SetWindowText(hVars, executorX86->GetVarInfo().c_str());
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
		SetWindowPos(hResult, HWND_TOP, 110,7+(int)(4.0/9.0*HIWORD(lParam)),(int)(LOWORD(lParam))-250, 30, NULL);
		UINT widt = (LOWORD(lParam)-20)/3;
		SetWindowPos(hCode, HWND_TOP, 5,40+(int)(4.0/9.0*HIWORD(lParam)),widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
		SetWindowPos(hLog, HWND_TOP, widt+10,40+(int)(4.0/9.0*HIWORD(lParam)),widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
		SetWindowPos(hVars, HWND_TOP, 2*widt+15,40+(int)(4.0/9.0*HIWORD(lParam)),widt, (int)(4.0/9.0*HIWORD(lParam)), NULL);
	}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}