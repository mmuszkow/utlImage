#include "stdinc.h"
#include "GdiImage.h"
#include "ImageWnd.h"
#include "StyleCtrl.h"
using namespace utlImage;

WTWPLUGINFO plugInfo = {
	sizeof(WTWPLUGINFO),						// rozmiar struktury
	L"utlImage",								// nazwa wtyczki
	L"Wycinki ekranu, schowka, pliku, rysowanie na wycinkach",	// opis wtyczki
	L"© 2010-2014 Maciej Muszkowski",			// prawa autorskie
	L"Maciej Muszkowski",						// autor
	L"maciek.muszkowski@gmail.com",				// dane do kontaktu z autorem
	L"http://www.alset.pl/Maciek",				// strona www autora
	L"",										// url do pliku xml z danymi do autoupdate
	PLUGIN_API_VERSION,							// wersja api
	MAKE_QWORD(1, 2, 0, 0),						// wersja wtyczki
	WTW_CLASS_UTILITY,							// klasa wtyczki
	NULL,										// f-cja wywolana podczas klikniecia "o wtyczce"
	L"{D6EC55A2-C764-11DF-BC24-B10EE0D72085}",	// guid (jezeli chcemy tylko jedna instancje wtyczki)
	NULL,										// dependencies
	0, 0, 0, 0									// zarezerwowane (4 pola)
};

WTWFUNCTIONS*		wtw = NULL;
WTWFUNCTIONS*		ImageWnd::wtw = NULL;
void*				ImageWnd::config = NULL;
HBITMAP				ImageWnd::hToolbarBitmap = NULL;
ImageWnd			imgWnd;
HANDLE				menuRebuildHook;
wtwMenuItemDef		menuDef;
ULONG_PTR           gdiplusToken;

WNDCLASS ImageWnd::wndClass = {
	CS_HREDRAW | CS_VREDRAW,
	ImageWnd::ImageWndProc,
	0,
	0,
	0, // hInst - later
	LoadIcon(NULL, IDI_APPLICATION),
	LoadCursor(NULL, IDC_CROSS),
	static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)),
	0,
	L"ScreenshotWnd"
};

enum imageType {SCREENSHOT, CLIPBOARD, FROMFILE};

namespace MenuId {
	const wchar_t screen[] = L"utlImage/Screen";
	const wchar_t clip[] = L"utlImage/Clipboard";
	const wchar_t file[] = L"utlImage/File";
};

namespace MenuCaption {
	const wchar_t screen[] = L"Wyślij zrzut ekranu...";
	const wchar_t clip[] = L"Wyślij obrazek ze schowka...";
	const wchar_t file[] = L"Wyślij fragment obrazka...";
	const wchar_t fromFileTitle[] = L"Obrazek do wysłania";
};

WTW_PTR onMenuRebuild(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr)
{
	wtwMenuCallbackEvent* event = reinterpret_cast<wtwMenuCallbackEvent*>(wParam);
	if(!event || !event->pInfo)
	{
		__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"onMenuRebuild: event or event->pInfo is NULL");
		return -1;
	}

	for(int i = 0; i < event->pInfo->iContacts; ++i) {
		wtwContactDef& cnt = event->pInfo->pContacts[i];

		wtwPresenceDef pr;

		wchar_t fn[255] = {0};
		wsprintf(fn, L"%s/%d/%s", cnt.netClass, cnt.netId, WTW_PF_STATUS_GET);
		wtw->fnCall(fn,reinterpret_cast<WTW_PARAM>(&pr),NULL);

		if(pr.curStatus != WTW_PRESENCE_OFFLINE) // add ony if connected to that network
		{
			event->slInt.add(event->itemsToShow, MenuId::screen);
			event->slInt.add(event->itemsToShow, MenuId::clip);
			event->slInt.add(event->itemsToShow, MenuId::file);
		}
	}
	return 0;
}

WTW_PTR onMenuClick(WTW_PARAM wParam, WTW_PARAM lParam, void* ptr) {
	
	wtwMenuItemDef* menuItem = reinterpret_cast<wtwMenuItemDef*>(wParam);
	wtwMenuPopupInfo* menuPopupInfo = reinterpret_cast<wtwMenuPopupInfo*>(lParam);
	if(!menuItem || !menuPopupInfo)
	{
		__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"onMenuClick: menuItem or menuPopupInfo is NULL");
		return -1;
	}

	wtwContactDef* contacts = new wtwContactDef[menuPopupInfo->iContacts]; // copy cnts, will be uses in image window
	for(int i=0;i<menuPopupInfo->iContacts;i++)
		contacts[i] = menuPopupInfo->pContacts[i];

	GdiImage* image = new GdiImage(); // will be used in image wnd

	int imgType = reinterpret_cast<int>(menuItem->ownerData); 
	switch(imgType)
	{
	case SCREENSHOT:
		Sleep(300);
		if(!image->fromScreenshot())
		{
			__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"Could not take screenshot");
			delete image;
			delete contacts;
			return 0;
		}
		break;
	case CLIPBOARD:
		if(!image->fromClipboard())
		{
			delete image;
			delete contacts;
			return 0;
		}
		break;
	case FROMFILE:
		{
			OPENFILENAME ofn;
			wchar_t path[MAX_PATH+1];

			path[0] = '\0';
			memset(&ofn,0,sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.lpstrFile = path;
			ofn.nMaxFile = MAX_PATH;
			ofn.hwndOwner = 0;
			ofn.lpstrFilter = WTW_DEFAULT_IMAGES_FITER;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			ofn.lpstrDefExt= L"";
			ofn.lpstrInitialDir= L"";
			ofn.lpstrTitle = MenuCaption::fromFileTitle;

			if(!GetOpenFileName(&ofn) || !image->fromFile(path))
			{
				__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"Could not open picture from file %s", path);
				delete image;
				delete contacts;
				return 0;
			}
		}
	}

	imgWnd.init(image,contacts,menuPopupInfo->iContacts);

	return 0;
}

extern "C" {

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	ImageWnd::wndClass.hInstance = hinstDLL;
	return 1;
}

WTWPLUGINFO* __stdcall queryPlugInfo(DWORD /*apiVersion*/, DWORD /*masterVersion*/) {
    return &plugInfo;
}

int __stdcall pluginLoad(DWORD /*callReason*/, WTWFUNCTIONS* fn) {

#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	GdiplusStartupInput gdiplusStartupInput;

	if(RegisterClass(&ImageWnd::wndClass) == 0 || !StyleCtrl::Register(ImageWnd::wndClass.hInstance))
	{
		__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"Could register windows classes");
		return -1;
	}
	
	if(GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0) != Gdiplus::Ok) // GDI+ start
	{
		__LOG_F(wtw, WTW_LOG_LEVEL_ERROR, LOG_TAG, L"Could not start GDI+");
		return -2;
	}
	
	/////////////// adding menu items ////////////////////////////////////////////
	wtw = fn;
	ImageWnd::wtw = fn;

	initStruct(menuDef);
	
	menuDef.menuID = WTW_MENU_ID_CONTACT_SEND;
	menuDef.callback = onMenuClick;

	menuDef.itemId = MenuId::screen;
	menuDef.menuCaption = MenuCaption::screen;
	menuDef.ownerData = reinterpret_cast<void*>(SCREENSHOT);
	wtw->fnCall(WTW_MENU_ITEM_ADD, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	menuDef.itemId = MenuId::clip;
	menuDef.menuCaption = MenuCaption::clip;
	menuDef.ownerData = reinterpret_cast<void*>(CLIPBOARD);
	wtw->fnCall(WTW_MENU_ITEM_ADD, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	menuDef.itemId = MenuId::file;
	menuDef.menuCaption = MenuCaption::file;
	menuDef.ownerData = reinterpret_cast<void*>(FROMFILE);
	wtw->fnCall(WTW_MENU_ITEM_ADD, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	menuRebuildHook = wtw->evHook(WTW_EVENT_MENU_REBUILD, onMenuRebuild, NULL);

	wtwMyConfigFile configName;
	initStruct(configName);
	configName.bufferSize = MAX_PATH + 1;
	configName.pBuffer = new wchar_t[MAX_PATH + 1];
	wtw->fnCall(WTW_SETTINGS_GET_MY_CONFIG_FILE, 
		reinterpret_cast<WTW_PARAM>(&configName), 
		reinterpret_cast<WTW_PARAM>(ImageWnd::wndClass.hInstance));
	

	wtw->fnCall(WTW_SETTINGS_INIT_EX, 
		reinterpret_cast<WTW_PARAM>(configName.pBuffer), 
		reinterpret_cast<WTW_PARAM>(&ImageWnd::config));

	delete[] configName.pBuffer;

	wtwGraphics wg;
	wg.hInst = ImageWnd::wndClass.hInstance;
	wg.graphId = L"utlImage/Toolbar";
	wg.resourceId = 0;
	wg.filePath = L"utlImageToolbar.png";
	wg.flags = WTW_GRAPH_FLAG_RELATIVE_DEF_PATH;
	if(wtw->fnCall(WTW_GRAPH_LOAD,reinterpret_cast<WTW_PARAM>(&wg), 0) == 1)
	{
		initStruct(wg);
		wg.graphId = L"utlImage/Toolbar";
		wg.flags = WTW_GRAPH_FLAG_GENERATE_HBITMAP;
		ImageWnd::hToolbarBitmap = 
			reinterpret_cast<HBITMAP>(wtw->fnCall(WTW_GRAPH_GET_IMAGE,reinterpret_cast<WTW_PARAM>(&wg),NULL));
	}	
	else
		ImageWnd::hToolbarBitmap = 
			LoadBitmap(ImageWnd::wndClass.hInstance, MAKEINTRESOURCE(IDB_TOOLBAR));

    return 0;
}

int __stdcall pluginUnload(DWORD /*callReason*/) {
	
	wtw->fnCall(WTW_SETTINGS_DESTROY, reinterpret_cast<WTW_PARAM>(ImageWnd::config), reinterpret_cast<WTW_PARAM>(ImageWnd::wndClass.hInstance));

	initStruct(menuDef);
	
	menuDef.menuID = WTW_MENU_ID_CONTACT_SEND;
	menuDef.itemId = MenuId::screen;
	wtw->fnCall(WTW_MENU_ITEM_DELETE, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	menuDef.itemId = MenuId::clip;
	wtw->fnCall(WTW_MENU_ITEM_DELETE, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	menuDef.itemId = MenuId::file;
	wtw->fnCall(WTW_MENU_ITEM_DELETE, reinterpret_cast<WTW_PARAM>(&menuDef), NULL);

	UnregisterClass(ImageWnd::wndClass.lpszClassName,ImageWnd::wndClass.hInstance);
	GdiplusShutdown(gdiplusToken);
	
	if(menuRebuildHook)
		wtw->evUnhook(menuRebuildHook);

	if(ImageWnd::hToolbarBitmap)
		DeleteObject(ImageWnd::hToolbarBitmap);

	return 0;
}

} // extern "C"
