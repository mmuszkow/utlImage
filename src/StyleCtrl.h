#pragma once

#include "stdinc.h"

#define WM_UPDATEPEN	WM_USER+15104
#define WM_UPDATEBRUSH	WM_USER+15105

inline ARGB rbg_bgr(ARGB argb) // arbg to abgr and vice-versa
{
	return ((argb&0xFF000000)|((argb&0x000000FF)<<16)|(argb&0x0000FF00)|((argb&0x00FF0000)>>16));
}

namespace StyleCtrl {
	static const int WM_SETPENCOLOR = RegisterWindowMessage(L"StyleCtrl::SetPenColor");
	static const int WM_SETBRUSHCOLOR = RegisterWindowMessage(L"StyleCtrl::SetBrushColor");
	static const int WM_SETPENWIDTH = RegisterWindowMessage(L"StyleCtrl::SetPenWidth");
	static const int WM_SETTRANSPARENT = RegisterWindowMessage(L"StyleCtrl::SetTransparent");
	static const wchar_t* className = L"StyleControl";
	enum { ID_PENCOLOR, ID_BRUSHCOLOR, ID_UPDOWN, ID_BACKCHECK };

	LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool Register(HINSTANCE hInst);
	HWND CreateStyleCtrl(int x, int y, int h, HWND hParent);
};
