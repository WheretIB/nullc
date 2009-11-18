#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

namespace TabbedFiles
{
	void AddTab(HWND wnd, const char* filename);

	void RegisterTabbedFiles(const char *className, HINSTANCE hInstance);
	LRESULT CALLBACK TabbedFilesProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
}
