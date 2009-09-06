#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

#define FONT_STYLE_COUNT 16
#define DEFAULT_STRING_LENGTH 64
#define TAB_SIZE 4

bool SetTextStyle(unsigned int id, unsigned char red, unsigned char green, unsigned char blue, bool bold, bool italics, bool underline);

void BeginStyleUpdate();
void SetStyleToSelection(unsigned int start, unsigned int end, int style);
void EndStyleUpdate();

void RegisterTextarea(const char *className, HINSTANCE hInstance);

const char* GetAreaText();
void SetAreaText(const char *text);

void UpdateArea();
bool NeedUpdate();
void ResetUpdate();

void InputChar(char ch);
void InputEnter();

LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
