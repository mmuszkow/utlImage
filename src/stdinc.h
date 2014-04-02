#ifndef _MSC_VER
#error "This code can be only compiled using Visual Studio"
#endif

#pragma once

#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1

#include <WinSock2.h> // without this there was "windows.h" including error
#include <plInterface.h>

#include <iostream>
#include <Commctrl.h>
#include <gdiplus.h>
#include <ctime>
#include <cmath>
#include "resource.h"

#ifdef _DEBUG
# define CRTDBG_MAP_ALLOC
# include <stdlib.h>
# include <crtdbg.h>
#endif

using namespace std;
using namespace Gdiplus;
using namespace Gdiplus::DllExports;

#ifdef WIN32
#pragma comment (lib,"Gdiplus.lib")
#pragma comment (lib,"Comctl32.lib")
#else
#pragma comment (lib,"Gdiplus64.lib")
#pragma comment (lib,"Comctl64.lib")
#endif

#define LOG_TAG	L"IMG"