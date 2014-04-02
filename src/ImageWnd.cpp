#include "stdinc.h"
#include "ImageWnd.h"
#include "StyleCtrl.h"

namespace utlImage {

LRESULT CALLBACK ImageWnd::ImageWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
	switch(uMsg)
	{
	case WM_PAINT:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handlePaint();
			break;
		}
	case WM_MOUSEMOVE:
		{
			if(wParam & MK_LBUTTON || wParam & MK_RBUTTON)
			{
				ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
				imgWnd->handleDrawing(lParam);
			}
			return 0;
		}
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handleDrawStart(lParam);
			return 0;
		}
	case WM_RBUTTONUP:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handleDrawEnd(lParam);
			if(imgWnd->mode==SELECT)
				imgWnd->handleCommand(SEND_SEL);
			return 0;
		}
	case WM_LBUTTONUP:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handleDrawEnd(lParam);
			return 0;
		}
	case WM_COMMAND:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handleCommand(wParam);
			return 0;
		}
	case WM_SIZE:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			SendMessage(imgWnd->hToolbar, TB_AUTOSIZE, 0, 0);
			return 0;
		}
	case WM_KEYUP:
		{
			if(wParam==VK_RETURN)
			{
				ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
				if(GetKeyState(VK_MENU)&0x8000) // Alt pressed
					imgWnd->handleAltEnter();
				else
					imgWnd->handleCommand(SEND_SEL);
			}
			return 0;
		}
	case WM_UPDATEPEN:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			DWORD cc = static_cast<DWORD>(lParam);
			int r = (cc >> 16) & 0xFF;
			int g = (cc >> 8) & 0xFF;
			int b = cc & 0xFF;
			imgWnd->colorPen = 0xFF000000 | RGB(r,g,b);
			imgWnd->pen_w = static_cast<int>(wParam);

			SetFocus(imgWnd->hMain);
			return 0;
		}
	case WM_UPDATEBRUSH:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			DWORD cc = static_cast<DWORD>(lParam);
			int alpha = cc & 0xFF000000;
			int r = (cc >> 16) & 0xFF;
			int g = (cc >> 8) & 0xFF;
			int b = cc & 0xFF;
			imgWnd->colorBrush = alpha | RGB(r,g,b);
			imgWnd->pen_w = static_cast<int>(wParam);

			SetFocus(imgWnd->hMain);
			return 0;
		}
	case WM_DESTROY:
		{
			ImageWnd* imgWnd = static_cast<ImageWnd*>(GetProp(hWnd,L"IMGWND"));
			imgWnd->handleDestroy();
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ImageWnd::ToolbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, 
									   UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	if(msg == WM_NOTIFY)
	{
		LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
		HWND hQualityBar = reinterpret_cast<HWND>(dwRefData);
		if(	nmhdr->code == TTN_GETDISPINFO && 
			reinterpret_cast<HWND>(nmhdr->idFrom) == hQualityBar)
		{
			LPNMTTDISPINFO lpnmtdi = reinterpret_cast<LPNMTTDISPINFO>(lParam);
			int pos = static_cast<int>(SendMessage(hQualityBar, TBM_GETPOS, 0, 0));
			switch(pos)
			{
			case 0:
				wcscpy_s(lpnmtdi->szText, 80, L"PNG");
				return 0;
			case 1:
				wcscpy_s(lpnmtdi->szText, 80, L"JPEG 80%");
				return 0;
			case 2:
				wcscpy_s(lpnmtdi->szText, 80, L"JPEG 60%");
				return 0;
			case 3:
				wcscpy_s(lpnmtdi->szText, 80, L"JPEG 40%");
				return 0;
			case 4:
				wcscpy_s(lpnmtdi->szText, 80, L"JPEG 10%");
				return 0;
			default:
				wcscpy_s(lpnmtdi->szText, 80, L"Error");
				return 0;
			}
		}
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

void ImageWnd::init(GdiImage *img, wtwContactDef *cnts, int cnt_c)
{
	deinit();

	image = img;
	contacts = cnts;
	cnt_count = cnt_c;

	if (!image || !contacts || cnt_count == 0 || wtw == NULL)
		return;


	wtw->fnCall(WTW_SETTINGS_READ, (WTW_PARAM)config, NULL);
	mode = SELECT;
	colorPen = 	static_cast<ARGB>(wtwGetInt(wtw, config, Config::penColor, 0xFF000000)); // black
	colorBrush = static_cast<ARGB>(wtwGetInt(wtw, config, Config::brushColor, 0x00FFFFFF)); // white transparent
	pen_w = wtwGetInt(wtw, config, Config::penWidth, 1);

	UINT resX = static_cast<UINT>(GetSystemMetrics(SM_CXSCREEN));// res w
	UINT resY = static_cast<UINT>(GetSystemMetrics(SM_CYSCREEN));// res h
	UINT captionH = static_cast<UINT>(GetSystemMetrics(SM_CYCAPTION));// title bar h
	UINT frameX = static_cast<UINT>(GetSystemMetrics(SM_CXDLGFRAME));// frame w
	UINT frameY = static_cast<UINT>(GetSystemMetrics(SM_CYDLGFRAME));// frame h

	startx = 0; starty = 0;
	endx = resX; endy = resY;

	minimalW = max(image->width() + 2 * frameX,261);
	minimalH = image->height() + captionH + 2 * frameY + 29;

	hMain = CreateWindow(ImageWnd::wndClass.lpszClassName, L"Image", 
		WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
		CW_USEDEFAULT, CW_USEDEFAULT, minimalW, minimalH,
		NULL, NULL, ImageWnd::wndClass.hInstance, NULL);

	// Creating toolbar
	InitCommonControls();
	TBBUTTON tbb[MAX_BTN] = {
		{0, SEP1, TBSTATE_HIDDEN, BTNS_SEP, {0}, 0L, 0},
		{0, SEP2, TBSTATE_HIDDEN, BTNS_SEP, {0}, 0L, 0}, 
		{SELECT, SELECT, TBSTATE_HIDDEN|TBSTATE_CHECKED, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Zaznaczenie"}, 
		{PIXEL, PIXEL, TBSTATE_HIDDEN, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Ołówek"},
		{LINE, LINE, TBSTATE_HIDDEN, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Linia"},
		{RECTANGLE, RECTANGLE, TBSTATE_HIDDEN, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Prostokąt"},
		{ELLIPSE, ELLIPSE, TBSTATE_HIDDEN, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Elipsa"}, 
		{TEXTWRITE, TEXTWRITE, TBSTATE_HIDDEN, BTNS_CHECKGROUP, {0}, 0L, (INT_PTR)L"Tekst"},
		{100, SEP3, TBSTATE_HIDDEN, BTNS_SEP, {0}, 0L, 0},
		{100, SEP4, TBSTATE_HIDDEN, BTNS_SEP, {0}, 0L, 0},
		{12, HIDE_BUTTON, TBSTATE_ENABLED, BTNS_SEP, {0}, 0L, 0}, 
		{SEND_SEL, SEND_SEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0L, (INT_PTR)L"Wyślij zaznaczony"}, 
		{SEND_ALL, SEND_ALL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0L, (INT_PTR)L"Wyślij cały"}, 
		{FTP, FTP, TBSTATE_HIDDEN, BTNS_BUTTON, {0}, 0L, (INT_PTR)L"Wyślij na FTP"}, 
		{IMGSHACK, IMGSHACK, TBSTATE_HIDDEN, BTNS_BUTTON, {0}, 0L, (INT_PTR)L"Wyślij na Imgur"},
		{CANCEL, CANCEL, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0L, (INT_PTR)L"Anuluj"}
	};

	hToolbar = CreateToolbarEx (hMain, 
		WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|CCS_BOTTOM, 
		0, MAX_BTN, NULL, (UINT)hToolbarBitmap, tbb, MAX_BTN, 32, 32, 32, 32, sizeof(TBBUTTON));
	SendMessage(hToolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	
	TBBUTTONINFO buttInfo; // after creating style control we set the size of its "button" on toolbar
	RECT rect;
	SendMessage(hToolbar, TB_GETITEMRECT, HIDE_BUTTON, reinterpret_cast<LPARAM>(&rect)); // getting the toolbat height
	hStyleCtrl = StyleCtrl::CreateStyleCtrl(0,0,rect.bottom-rect.top,hToolbar);
	PostMessage(hStyleCtrl,StyleCtrl::WM_SETPENWIDTH,0,static_cast<LPARAM>(pen_w));
	PostMessage(hStyleCtrl,StyleCtrl::WM_SETPENCOLOR,0,static_cast<LPARAM>(rbg_bgr(colorPen)));
	PostMessage(hStyleCtrl,StyleCtrl::WM_SETBRUSHCOLOR,0,static_cast<LPARAM>(rbg_bgr(colorBrush)));
	PostMessage(hStyleCtrl,StyleCtrl::WM_SETTRANSPARENT,0,
		static_cast<LPARAM>(wtwGetInt(wtw, config, Config::transparent, 0)));
	GetClientRect(hStyleCtrl,&rect); // getting control width
	ShowWindow(hStyleCtrl, SW_HIDE);
	buttInfo.cbSize = sizeof(TBBUTTONINFO);
	buttInfo.dwMask = TBIF_SIZE;
	buttInfo.cx = static_cast<WORD>(rect.right-rect.left); 
	SendMessage(hToolbar, TB_SETBUTTONINFO, 
		static_cast<WPARAM>(SEP1), reinterpret_cast<LPARAM>(&buttInfo)); // setting proper width on toolbar

	if(wtw->fnExists(utlFTP::WTW_FTP_SEND_FILE))
	{
		buttInfo.dwMask = TBIF_STATE;
		buttInfo.cx = 0;
		buttInfo.fsState = TBSTATE_ENABLED;
		SendMessage(hToolbar, TB_SETBUTTONINFO, 
			static_cast<WPARAM>(FTP), reinterpret_cast<LPARAM>(&buttInfo)); // hiding/unhiding ftp button
	}

	if(wtw->fnExists(WTW_IMAGESHACK_SEND_IMAGE))
	{
		buttInfo.dwMask = TBIF_STATE;
		buttInfo.cx = 0;
		buttInfo.fsState = TBSTATE_ENABLED;
		SendMessage(hToolbar, TB_SETBUTTONINFO, 
			static_cast<WPARAM>(IMGSHACK), reinterpret_cast<LPARAM>(&buttInfo)); // hiding/unhiding imageshack button
	}

	// Text edit for text in the picture
	hTextEdit = CreateWindow(L"EDIT",L"",WS_CHILD|WS_BORDER|WS_DISABLED|ES_AUTOHSCROLL, 
		0, 0, 0, 0, hToolbar, 0, 0, 0);
	
	// Picture quality bar
	hQualityBar = CreateWindow(TRACKBAR_CLASS, 0, WS_CHILD|TBS_TOOLTIPS|TBS_BOTH|TBS_AUTOTICKS|TBS_TRANSPARENTBKGND, 
		0, 0, 0, 0, hToolbar, 0, 0, 0);
	SendMessage(hQualityBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, GdiImage::LAST-1));
	HWND hQualityTooltips = reinterpret_cast<HWND>(SendMessage(hQualityBar, TBM_GETTOOLTIPS, 0, 0));
	SetWindowSubclass(hToolbar, ToolbarProc, 0, reinterpret_cast<DWORD_PTR>(hQualityBar));
	TOOLINFO ti;
	memset(&ti, 0, sizeof(TOOLINFO));
	ti.cbSize = sizeof(TOOLINFO);
	ti.hinst = ImageWnd::wndClass.hInstance;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	ti.hwnd = hQualityTooltips;
	SendMessage(hQualityTooltips, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&ti));

	// Hiding button
	SendMessage(hToolbar, TB_GETITEMRECT, static_cast<WPARAM>(HIDE_BUTTON), reinterpret_cast<LPARAM>(&rect));
	hHide = CreateWindowW(L"BUTTON",L">",WS_CHILD|WS_VISIBLE, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hToolbar, (HMENU)HIDE_BUTTON, 0, 0);

	HFONT hDefaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
	SendMessage(hTextEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hDefaultFont), MAKELPARAM(TRUE, 0));
	SendMessage(hQualityBar, WM_SETFONT, reinterpret_cast<WPARAM>(hDefaultFont), MAKELPARAM(TRUE, 0));

	SetProp(hMain, L"IMGWND", this);
	if (image->width() >= resX || image->height() >= resY) // if size of image is bigger than screen - maximize the image wnd
		ShowWindow(hMain, SW_SHOWMAXIMIZED);
	else
		ShowWindow(hMain, SW_SHOW);

	hidden = (wtwGetInt(wtw, config, Config::expanded, 0) == 1);
	handleCommand(HIDE_BUTTON);

	if(wtwGetInt(wtw, config, Config::fullscreen, 0) == 1)
		handleAltEnter();

	SendMessage(hQualityBar, TBM_SETPOS, TRUE, 
		static_cast<LPARAM>(wtwGetInt(wtw, config, Config::def_quality, 0)));
}

void utlImage::ImageWnd::deinit()
{
	if (hMain)
	{
		wtwSetInt(wtw, config, Config::def_quality, 
			static_cast<int>(SendMessage(hQualityBar, TBM_GETPOS, 0, 0)));
		DestroyWindow(hMain);
		hMain = NULL;
	}
	if (image)
	{
		delete image;
		image = NULL;
	}
	if (contacts)
	{
		delete contacts;
		contacts = NULL;
		cnt_count = 0;
	}
}

ImageWnd::~ImageWnd()
{
	deinit();
}

void ImageWnd::handlePaint()
{
	PAINTSTRUCT ps;
	if (image)
	{
		HDC hdc = BeginPaint(hMain, &ps);
		image->draw(hdc);
		EndPaint(hMain, &ps);
	}
}

void ImageWnd::handleDrawStart(LPARAM mPos)
{
	startx = LOWORD(mPos);
	starty = HIWORD(mPos);
	endx = startx; endy = starty;
	InvalidateRect(hMain,NULL,false);
	if(mode==TEXTWRITE)
	{
		char text[64];
		GetWindowTextA(hTextEdit,text,64);
		image->drawTextOnMe(LOWORD(mPos),HIWORD(mPos),colorPen,text);
		RECT r = {LOWORD(mPos),HIWORD(mPos),
			LOWORD(mPos)+(strlen(text)+1)*16,HIWORD(mPos)+CHAR_H+1};
		InvalidateRect(hMain,&r,false);
	}
}

void ImageWnd::handleDrawEnd(LPARAM mPos)
{
	endx = LOWORD(mPos);
	endy = HIWORD(mPos);

	if(mode==RECTANGLE||mode==ELLIPSE) // erase draw focus
	{
		RECT r = {min(startx,endx),min(starty,endy),max(startx,endx),max(starty,endy)};
		HDC hdc = GetDC(hMain);
		DrawFocusRect(hdc, &r); 
		ReleaseDC(hMain,hdc);
	}

	switch(mode)
	{
	case LINE: image->drawLineOnMe(startx,starty,endx,endy,colorPen,pen_w); break;
	case RECTANGLE: image->drawRectOnMe(min(startx,endx),min(starty,endy),max(startx,endx),max(starty,endy),colorPen,colorBrush,pen_w); break;
	case ELLIPSE: image->drawEllipseOnMe(min(startx,endx),min(starty,endy),max(startx,endx),max(starty,endy),colorPen,colorBrush,pen_w);
	}

	if(mode==LINE||mode==RECTANGLE||mode==ELLIPSE) // show what we've drawn
	{
		RECT r = {min(startx,endx)-(pen_w>>1),min(starty,endy)-(pen_w>>1),
			max(startx,endx)+pen_w-(pen_w>>1),max(starty,endy)+pen_w-(pen_w>>1)};
		InvalidateRect(hMain,&r,false);
	}
}

void ImageWnd::handleDrawing(LPARAM mPos)
{
	switch(mode)
	{
	case RECTANGLE:
	case ELLIPSE:
	case SELECT:
		{
			HDC hdc = GetDC(hMain);
			RECT r = {min(startx,endx),min(starty,endy),
				max(startx,endx),max(starty,endy)};
			DrawFocusRect(hdc, &r); // we erase the previous (focus rect is xor)
			endx = LOWORD(mPos);
			endy = HIWORD(mPos);
			RECT r2 = {min(startx,endx),min(starty,endy),
				max(startx,endx),max(starty,endy)};
			DrawFocusRect(hdc, &r2); // and draw a new one
			ReleaseDC(hMain, hdc);
			break;
		}
	case PIXEL:
		{
			RECT r = {min(startx,LOWORD(mPos))-(pen_w>>1),min(starty,HIWORD(mPos))-(pen_w>>1),
				max(startx,LOWORD(mPos))+pen_w-(pen_w>>1),max(starty,HIWORD(mPos))+pen_w-(pen_w>>1)};
			image->drawLineOnMe(startx,starty,LOWORD(mPos),HIWORD(mPos),colorPen,pen_w);
			InvalidateRect(hMain,&r,false);
			startx = LOWORD(mPos);
			starty = HIWORD(mPos);
		}
	}
}

void ImageWnd::handleCommand(WPARAM wParam)
{
	if(HIWORD(wParam)!=0) // menu only
		return;

	int src = LOWORD(wParam);
	if(src==SELECT || src==PIXEL || src==LINE || src==RECTANGLE || src==ELLIPSE)
		EnableWindow(hTextEdit,false);

	switch(src)
	{
	case SELECT:
		mode = SELECT;
		break;
	case PIXEL:
		mode = PIXEL;
		break;
	case LINE:
		mode = LINE;
		break;
	case RECTANGLE:
		mode = RECTANGLE;
		break;
	case ELLIPSE:
		mode = ELLIPSE;
		break;
	case TEXTWRITE:
		EnableWindow(hTextEdit,true);
		mode = TEXTWRITE;
		break;
	case HIDE_BUTTON:
		{
			TBBUTTONINFO buttInfo;
			RECT rect;
			memset(&buttInfo, 0, sizeof(TBBUTTONINFO));
			buttInfo.cbSize = sizeof(TBBUTTONINFO);
			buttInfo.dwMask = TBIF_STATE;
			if(hidden)
			{
				hidden = false;
				buttInfo.fsState = TBSTATE_ENABLED;
				SetWindowText(hHide, L"<");
				ShowWindow(hStyleCtrl, SW_SHOW);
			}
			else
			{
				hidden = true;
				buttInfo.fsState = TBSTATE_HIDDEN;
				ShowWindow(hStyleCtrl, SW_HIDE);
				ShowWindow(hTextEdit, SW_HIDE);
				ShowWindow(hQualityBar, SW_HIDE);
				SetWindowText(hHide, L">");
			} 
			for(int i=SEP1;i<HIDE_BUTTON;i++)
				SendMessage(hToolbar, TB_SETBUTTONINFO, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&buttInfo));
			SendMessage(hToolbar, TB_GETITEMRECT, HIDE_BUTTON, reinterpret_cast<LPARAM>(&rect));
			MoveWindow(hHide, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, true);
			if(!hidden)
			{
				SendMessage(hToolbar, TB_GETITEMRECT, SEP3, reinterpret_cast<LPARAM>(&rect));
				MoveWindow(hTextEdit, rect.left + 1, rect.top + ((rect.bottom-rect.top) >> 2), 99, ((rect.bottom-rect.top) >> 1), false);
				ShowWindow(hTextEdit, SW_SHOW);

				SendMessage(hToolbar, TB_GETITEMRECT, SEP4, reinterpret_cast<LPARAM>(&rect));
				MoveWindow(hQualityBar, rect.left + 1, rect.top - 1, 98, (rect.bottom-rect.top)-2, false);
				ShowWindow(hQualityBar, SW_SHOW);
			}
			wtwSetInt(wtw, config, Config::expanded, hidden?0:1);
			break;
		}
	case SEND_ALL:
		if (image)
		{
			startx = 0; starty = 0;
			endx = image->width();
			endy = image->height();
		}
	case SEND_SEL:
		if (image)
		{
			if(startx==endx || starty==endy)
			{
				MessageBox(hMain, L"Brak zaznaczonego obszaru.\r\nZaznaczenia dokonuje się przez przesunięcie kursora,\r\nrównocześnie trzymając lewy lub prawy przycisk myszy.", L"Obrazek nie wysłany", MB_ICONERROR);
				break;
			}
			
			wchar_t path[MAX_PATH+1];
			GdiImage::quality quality;
			getPathAndQuality(path, MAX_PATH, quality);
			image->saveToFile(path, quality, startx, starty, endx, endy);

			// sending image
			if (!wtw || !contacts || cnt_count == 0)
				return;

			for (int i = 0; i < cnt_count; i++)
			{
				int ret = sendImage(path,contacts[i]);
				switch (ret)
				{
					case SIS_OK: // success
						deinit();
						break;
					case SIS_ERROR: // unknown error
						MessageBox(hMain, L"Protokół zwrócił błąd przy wysyłaniu", L"Obrazek nie wysłany", MB_ICONERROR);
						break;
					case SIS_FILE_TOO_BIG: // image too big, sending JPEG in worse quality
						quality = GdiImage::JPEG_80;
						getPathAndQuality(path, MAX_PATH, quality);
						while(quality < GdiImage::LAST)
						{
							image->saveToFile(path, quality, startx, starty, endx, endy);
							ret = sendImage(path,contacts[i]);
							if(ret!=SIS_OK && ret != SIS_FILE_TOO_BIG)
							{
								MessageBox(hMain, L"Protokół zwrócił błąd przy wysyłaniu", L"Obrazek nie wysłany", MB_ICONERROR);
								break;
							}
							if(ret==SIS_OK)
							{
								deinit();
								break;
							}
							if(ret==SIS_FILE_TOO_BIG && quality==GdiImage::LAST-1) // if last option didn't succeded
							{
								MessageBox(hMain, L"Obrazek jest zbyt duży", L"Obrazek nie wysłany", MB_ICONERROR);
								break;
							}
							quality = static_cast<GdiImage::quality>(quality + 1);
						}	
						break;
					case SIS_NOT_SUPPORTED:
						MessageBox(hMain, L"Kontakt nie obsługuje osbierania obrazków", L"Obrazek nie wysłany", MB_ICONERROR);
						break;
					case SIS_NO_CONNECTION:
						MessageBox(hMain, L"Brak połączenia", L"Obrazek nie wysłany", MB_ICONERROR);
						break;
					case SIS_FILE_ERROR:
						MessageBox(hMain, L"Błąd odczytu pliku z obrazkiem", L"Obrazek nie wysłany", MB_ICONERROR);
						break;
				}
			}
		}
		break;
	case FTP:
		{
			utlFTP::wtwFtpFile ftpInfo;
			initStruct(ftpInfo);

			if (!wtw || !contacts || cnt_count == 0)
				return;

			//WTWFUNCTION f = wtw->fnGet(utlFTP::WTW_FTP_SEND_FILE,0);
			wchar_t path[MAX_PATH+1];
			GdiImage::quality quality;
			getPathAndQuality(path, MAX_PATH, quality);
			image->saveToFile(path, quality, startx, starty, endx, endy);
			wcscpy(ftpInfo.filePath,path);
			for (int i = 0; i < cnt_count; i++)
			{
				ftpInfo.contact = contacts[i];
				wtw->fnCall(utlFTP::WTW_FTP_SEND_FILE,reinterpret_cast<WPARAM>(&ftpInfo),0);
			}

			deinit();
			break;
		}
	case IMGSHACK:
		{
			wtwImagesHackFile shackInfo;
			if (!wtw || !contacts || cnt_count == 0)
				return;

			wchar_t path[MAX_PATH+1];
			GdiImage::quality quality;
			getPathAndQuality(path, MAX_PATH, quality);
			image->saveToFile(path, quality, startx, starty, endx, endy);

			shackInfo.filePath = path;
			for (int i = 0; i < cnt_count; i++)
			{
				shackInfo.contactId = contacts[i].id;
				shackInfo.netClass = contacts[i].netClass;
				shackInfo.netId = contacts[i].netId;
				wtw->fnCall(WTW_IMAGESHACK_SEND_IMAGE,reinterpret_cast<WPARAM>(&shackInfo),0);
			}

			deinit();
			break;
		}
	case CANCEL:
		deinit();
	}
	SetFocus(hMain);
}

void ImageWnd::handleAltEnter()
{
	DWORD dwStyle = static_cast<DWORD>(GetWindowLongA(hMain, GWL_STYLE));
	if(image->width()!=GetSystemMetrics(SM_CXSCREEN) || 
		image->height()!=GetSystemMetrics(SM_CYSCREEN)) // only for screen captures
		return;
	if (dwStyle & WS_OVERLAPPEDWINDOW)
	{
		ShowWindow(hToolbar,SW_HIDE);
		dwStyle &= ~WS_OVERLAPPEDWINDOW;
		dwStyle |= WS_POPUP;
		SetWindowLongA(hMain, GWL_STYLE, dwStyle);
		SetWindowPos(hMain, HWND_TOPMOST, 0, 0, image->width(), image->height(), SWP_SHOWWINDOW | SWP_FRAMECHANGED);
		wtwSetInt(wtw, config, Config::fullscreen, 1);
	}
	else
	{
		ShowWindow(hToolbar,SW_SHOW);
		dwStyle &= ~WS_POPUP;
		dwStyle |= WS_OVERLAPPEDWINDOW;
		SetWindowLongA(hMain, GWL_STYLE, dwStyle);
		SetWindowPos(hMain, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, image->width(), image->height(), SWP_SHOWWINDOW | SWP_FRAMECHANGED);
		wtwSetInt(wtw, config, Config::fullscreen, 0);
	}
}

int ImageWnd::sendImage(wchar_t *path, wtwContactDef &contact)
{
	wchar_t fn[512] = {0};
	wtwPicture pict;
	pict.contactData = contact;
	pict.fullFilePath = path;
	wsprintf(fn, L"%s/%d/%s", contact.netClass, contact.netId, WTW_PF_SEND_PICT);
	return static_cast<int>(wtw->fnCall(fn, reinterpret_cast<WTW_PARAM>(&pict), NULL));
}

void ImageWnd::handleDestroy()
{
	wtwSetInt(wtw, config, Config::penColor, static_cast<int>(colorPen));
	wtwSetInt(wtw, config, Config::brushColor, static_cast<int>(colorBrush));
	wtwSetInt(wtw, config, Config::penWidth, pen_w);
	if(IsDlgButtonChecked(hStyleCtrl,StyleCtrl::ID_BACKCHECK))
		wtwSetInt(wtw, config, Config::transparent, 1);
	else
		wtwSetInt(wtw, config, Config::transparent, 0);
	wtw->fnCall(WTW_SETTINGS_WRITE, reinterpret_cast<WTW_PARAM>(config), 0);
	hMain = NULL; // to prevent DestroyWindow
	deinit();
}

void ImageWnd::getPathAndQuality(wchar_t* path, size_t maxLen, GdiImage::quality& quality)
{
	wchar_t temp_path[MAX_PATH+1];

	// bad hack, for SIS_FILE_TOO_BIG
	if(quality != GdiImage::JPEG_80)
	{
		quality = GdiImage::PNG;
		int quality_pos = static_cast<int>(SendMessage(hQualityBar, TBM_GETPOS, 0, 0));
		if(	quality_pos > static_cast<int>(GdiImage::PNG) && 
			quality_pos < static_cast<int>(GdiImage::LAST)) 
			quality = static_cast<GdiImage::quality>(quality_pos);
	}

	GetTempPath(MAX_PATH, temp_path);
	if(quality == GdiImage::PNG)
		swprintf(path, MAX_PATH, L"%s%d.png", temp_path, time(NULL));
	else
		swprintf(path, MAX_PATH, L"%s%d.jpg", temp_path, time(NULL));
}

};
