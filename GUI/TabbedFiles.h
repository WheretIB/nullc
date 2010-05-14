#pragma once

#include <assert.h>
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>

namespace TabbedFiles
{
	struct TabInfo
	{
		const char		*name, *last;
		unsigned int	width;
		HWND			window;
		bool			dirty;
	};

	void AddTab(HWND wnd, const char* filename, HWND childWindow);
	void SetNewTabWindow(HWND wnd, HWND newTab);
	void RemoveTab(HWND wnd, unsigned int tab);

	unsigned int GetCurrentTab(HWND wnd);
	void	SetCurrentTab(HWND wnd, unsigned int id);

	TabInfo& GetTabInfo(HWND wnd, unsigned int id);

	void	SetOnCloseTab(HWND wnd, void (*OnClose)(TabInfo &tab));

	void RegisterTabbedFiles(const char *className, HINSTANCE hInstance);
	LRESULT CALLBACK TabbedFilesProc(HWND hWnd, unsigned int message, WPARAM wParam, LPARAM lParam);
}
