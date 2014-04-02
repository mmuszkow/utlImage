#pragma once

#include "stdinc.h"
#include "GdiImage.h"
#include "../../utlFTP/utlFTP.h"
#include "wtwImageShack_external.h"

namespace utlImage {
	namespace Config {
		static const wchar_t penWidth[] = L"utlImage/PenWidth";
		static const wchar_t penColor[] = L"utlImage/PenColor";
		static const wchar_t brushColor[] = L"utlImage/BrushColor";
		static const wchar_t transparent[] = L"utlImage/Transparent";
		static const wchar_t expanded[] = L"utlImage/ToolbarExpanded";
		static const wchar_t fullscreen[] = L"utlImage/Fullscreen";
		static const wchar_t def_quality[] = L"utlImage/DefaultQuality";
	};

	class ImageWnd {
		static LRESULT CALLBACK ImageWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK ImageWnd::ToolbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		GdiImage* image;
		wtwContactDef* contacts;
		int cnt_count;
		int startx, starty, endx, endy;
		HWND hMain;
		int mode;
		ARGB colorPen;
		ARGB colorBrush;
		int pen_w;

		enum {SEP1,SEP2,SELECT,PIXEL,LINE,RECTANGLE,ELLIPSE,TEXTWRITE,SEP3,SEP4,HIDE_BUTTON,SEND_SEL,SEND_ALL,FTP,IMGSHACK,CANCEL}; 
		static const int MAX_BTN = CANCEL+1;
		HWND hToolbar;
		HWND hStyleCtrl;
		HWND hHide;
		HWND hTextEdit;
		HWND hQualityBar;
		
		bool hidden;
		unsigned int minimalW, minimalH;
	public:
		static HBITMAP hToolbarBitmap;

		static WNDCLASS wndClass;
		static WTWFUNCTIONS* wtw;
		static void* config;

		ImageWnd() : hMain(NULL), image(NULL), contacts(NULL), cnt_count(0), mode(SELECT), hidden(true) {}
		void init(GdiImage *img, wtwContactDef *cnts, int cnt_c);
		void deinit();
		~ImageWnd();

		void getPathAndQuality(wchar_t* path, size_t maxLen, GdiImage::quality& quality);
		int sendImage(wchar_t *path, wtwContactDef &contact);

		void handlePaint();
		void handleDrawStart(LPARAM mPos);
		void handleDrawEnd(LPARAM mPos);
		void handleDrawing(LPARAM mPos);
		void handleCommand(WPARAM wParam);
		void handleAltEnter();
		void handleDestroy();
	};
};
