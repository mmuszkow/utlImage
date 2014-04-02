#include "stdinc.h"
#include "StyleCtrl.h"
#include "resource.h"

namespace StyleCtrl {

	bool Register(HINSTANCE hInst)
	{
		static WNDCLASS wndClass = {
			CS_HREDRAW | CS_VREDRAW,
			wndProc,
			0,
			0,
			hInst,
			LoadIcon(NULL, IDI_APPLICATION),
			LoadCursor(NULL, IDC_ARROW),
			reinterpret_cast<HBRUSH>(COLOR_WINDOW+1),
			0,
			className };
		return (RegisterClass(&wndClass) != 0);
	}

	HWND CreateStyleCtrl(int x, int y, int h, HWND hParent)
	{

		HWND hMain = CreateWindow(className,L"Image",WS_CHILD,x,y,(h<<2)+16+50,h,hParent,0,0,0);
		SetProp(hMain,L"PENCOLOR",reinterpret_cast<HANDLE>(0xFF000000));
		SetProp(hMain,L"BRUSHCOLOR",reinterpret_cast<HANDLE>(0xFFFFFFFF));

		HWND hButtonL = CreateWindow(L"BUTTON",L"...",WS_VISIBLE|WS_CHILD,0,0,h,h,hMain,(HMENU)ID_PENCOLOR,0,0);
		HWND hButtonR = CreateWindow(L"BUTTON",L"...",WS_VISIBLE|WS_CHILD,h,0,h,h,hMain,(HMENU)ID_BRUSHCOLOR,0,0);
		HWND hCheck = CreateWindow(L"BUTTON",L"Tło",WS_VISIBLE|WS_CHILD|BS_AUTOCHECKBOX,(h<<2)+16+2,0,48,h,hMain,(HMENU)ID_BACKCHECK,0,0);

		InitCommonControls();
		HWND hUpDown = CreateWindow(UPDOWN_CLASS,L"",WS_CHILD|WS_VISIBLE,(h<<2),0,16,h,hMain,(HMENU)ID_UPDOWN,0,0);
		SendMessage(hUpDown, UDM_SETRANGE, 0, static_cast<LPARAM>(MAKELONG(15, 1))); 
		SendMessage(hUpDown, UDM_SETPOS, 0, static_cast<LPARAM>(MAKELONG(1, 0)));

		HFONT hDefaultFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
		SendMessage(hCheck, WM_SETFONT, reinterpret_cast<WPARAM>(hDefaultFont), MAKELPARAM(TRUE, 0));
		SendMessage(hButtonL, WM_SETFONT, reinterpret_cast<WPARAM>(hDefaultFont), MAKELPARAM(TRUE, 0));
		SendMessage(hButtonR, WM_SETFONT, reinterpret_cast<WPARAM>(hDefaultFont), MAKELPARAM(TRUE, 0));

		ShowWindow(hMain,SW_SHOW);

		return hMain;
	}

	void updatePen(HWND hWnd, int w = 0)
	{
		RECT ctrlRect;
		GetClientRect(hWnd,&ctrlRect);
		int buttonS = ctrlRect.bottom-ctrlRect.top;
		RECT r = {buttonS<<1,0,4*buttonS,buttonS};
		InvalidateRect(hWnd,&r,true);

		int brushColor = reinterpret_cast<int>(GetProp(hWnd,L"BRUSHCOLOR"));
		if(IsDlgButtonChecked(hWnd,ID_BACKCHECK))
		{
			brushColor |= 0xFF000000;
			SetProp(hWnd,L"BRUSHCOLOR",reinterpret_cast<HANDLE>(brushColor));
		}
		else
		{
			brushColor &= 0x00FFFFFF;
			SetProp(hWnd,L"BRUSHCOLOR",reinterpret_cast<HANDLE>(brushColor));
		}
		
		if(w==0)
			w = LOWORD(SendMessage(GetDlgItem(hWnd,ID_UPDOWN), UDM_GETPOS, 0, 0));
		SendMessage(GetParent(GetParent(hWnd)),WM_UPDATEPEN,
			w,reinterpret_cast<LPARAM>(GetProp(hWnd,L"PENCOLOR"))); // TODO: 2x GetParent, not very elegant
		SendMessage(GetParent(GetParent(hWnd)),WM_UPDATEBRUSH,
			w,static_cast<LPARAM>(brushColor));				// TODO: 2x GetParent, not very elegant
	}

	LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch(uMsg)
		{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				int h = ps.rcPaint.bottom-ps.rcPaint.top; // line 
				HWND hUpDown = GetDlgItem(hWnd,ID_UPDOWN);
				int w = LOWORD(SendMessage(hUpDown, UDM_GETPOS, 0, 0));

				COLORREF penColor = reinterpret_cast<COLORREF>(GetProp(hWnd,L"PENCOLOR"));
				COLORREF brushColor = reinterpret_cast<COLORREF>(GetProp(hWnd,L"BRUSHCOLOR"));
				brushColor &= 0x00FFFFFF; // clearing alpha bits, otherwise GDI will paint black rect

				HBRUSH hBackground = CreateSolidBrush(brushColor); // background
				SelectObject(hdc,hBackground);
				SelectObject(hdc,GetStockObject(NULL_PEN));
				Rectangle(hdc,h<<1,0,h<<2,h);
				DeleteObject(hBackground);

				HPEN hPen = CreatePen(PS_SOLID,1,penColor); // line
				HBRUSH hBrush=CreateSolidBrush(penColor);
				SelectObject(hdc,hBrush);
				SelectObject(hdc,hPen);
				Rectangle(hdc,(h<<1)+2,(h>>1)-1-(w>>1),(h<<2)-2,(h>>1)-1-(w>>1)+w);
				DeleteObject(hBrush);
				DeleteObject(hPen);

				EndPaint(hWnd, &ps);
				break;
			}
		case WM_NOTIFY:
			if(reinterpret_cast<LPNMHDR>(lParam)->code==UDN_DELTAPOS)
			{
				LPNMUPDOWN np = reinterpret_cast<LPNMUPDOWN>(lParam);
				updatePen(hWnd,np->iPos+np->iDelta);
			}
			return 0;
		case WM_CTLCOLORSTATIC:
			{
				HDC hdcStatic = reinterpret_cast<HDC>(wParam);
				SetBkColor(hdcStatic, GetSysColor(COLOR_WINDOW));
				return (INT_PTR)CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			}

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case ID_PENCOLOR:
				{
					CHOOSECOLOR cc;	// common dialog box structure 
					static COLORREF acrCustClr[16];	// array of custom colors 
					static DWORD rgbCurrent = 
						reinterpret_cast<DWORD>(GetProp(hWnd,L"PENCOLOR")); // initial color selection

					// Initialize CHOOSECOLOR 
					memset((&cc), 0, (sizeof(cc)));
					cc.lStructSize = sizeof(cc);
					cc.hwndOwner = hWnd;
					cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
					cc.rgbResult = rgbCurrent;
					cc.Flags = CC_RGBINIT;

					if (ChooseColor(&cc) == TRUE)
					{
						SetProp(hWnd,L"PENCOLOR",reinterpret_cast<HANDLE>(cc.rgbResult));
						updatePen(hWnd);
					}
					break;
				}
			case ID_BRUSHCOLOR:
				{
					CHOOSECOLOR cc;	// common dialog box structure 
					static COLORREF acrCustClr[16];	// array of custom colors 
					static DWORD rgbCurrent = 
						reinterpret_cast<DWORD>(GetProp(hWnd,L"BRUSHCOLOR")); // initial color selection

					// Initialize CHOOSECOLOR 
					memset((&cc), 0, (sizeof(cc)));
					cc.lStructSize = sizeof(cc);
					cc.hwndOwner = hWnd;
					cc.lpCustColors = static_cast<LPDWORD>(acrCustClr);
					cc.rgbResult = rgbCurrent;
					cc.Flags = CC_RGBINIT;

					if (ChooseColor(&cc) == TRUE)
					{
						SetProp(hWnd,L"BRUSHCOLOR",reinterpret_cast<HANDLE>(cc.rgbResult));
						updatePen(hWnd);
					}
					break;
				}
			case ID_BACKCHECK:
				updatePen(hWnd);
			}
			return 0;
		}

		if(uMsg == WM_SETPENCOLOR) { // to ommit C2051 error
			SetProp(hWnd,L"PENCOLOR",reinterpret_cast<HANDLE>(lParam&0x00FFFFFF)); // remove alpha
			updatePen(hWnd);
			return 0;
		} else if(uMsg == WM_SETPENWIDTH) {
			SendMessage(GetDlgItem(hWnd,ID_UPDOWN), UDM_SETPOS, 0, 
				static_cast<LPARAM>(MAKELONG(static_cast<short>(lParam), 0))); 
			updatePen(hWnd,static_cast<short>(lParam));
			return 0;
		} else if(uMsg == WM_SETBRUSHCOLOR) {
			SetProp(hWnd,L"BRUSHCOLOR",reinterpret_cast<HANDLE>(lParam&0x00FFFFFF)); // remove alpha
			updatePen(hWnd);
			return 0;
		} else if(uMsg == WM_SETTRANSPARENT) {
			if(lParam==1)
				CheckDlgButton(hWnd,ID_BACKCHECK,BST_CHECKED);
			else
				CheckDlgButton(hWnd,ID_BACKCHECK,BST_UNCHECKED);
			updatePen(hWnd);
			return 0;
		}
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
};
