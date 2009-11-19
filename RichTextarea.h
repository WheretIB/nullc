#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

#define FONT_STYLE_COUNT 16
#define DEFAULT_STRING_LENGTH 64
#define TAB_SIZE 4

namespace RichTextarea
{
	bool SetTextStyle(unsigned int id, unsigned char red, unsigned char green, unsigned char blue, bool bold, bool italics, bool underline);

	void BeginStyleUpdate(HWND wnd);
	void SetStyleToSelection(HWND wnd, unsigned int start, unsigned int end, int style);
	void EndStyleUpdate(HWND wnd);

	void ClearAreaText(HWND wnd);
	const char* GetAreaText(HWND wnd);
	void SetAreaText(HWND wnd, const char *text);

	void UpdateArea(HWND wnd);
	bool NeedUpdate(HWND wnd);
	void ResetUpdate(HWND wnd);

	void SetStatusBar(HWND status, unsigned int barWidth);

	void RegisterTextarea(const char *className, HINSTANCE hInstance);
	void UnregisterTextarea();

	LRESULT CALLBACK TextareaProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
	VOID CALLBACK AreaCursorUpdate(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
}
