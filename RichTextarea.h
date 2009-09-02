#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

bool SetTextStyle(unsigned int id, char red, char green, char blue, bool bold, bool italics, bool underline);
void SetStyleToSelection(unsigned int start, unsigned int end, int style);

void RegisterTextarea(const char *className, HINSTANCE hInstance);

const char* GetAreaText();
void SetAreaText(const char *text);

void UpdateArea();
bool NeedUpdate();

void InputChar(char ch);
void InputEnter();

LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
