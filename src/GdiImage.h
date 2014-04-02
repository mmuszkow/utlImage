#pragma once

#include "stdinc.h"
#include "font.h"

namespace utlImage {

	class GdiImage
	{
		GpBitmap* image;

		static CLSID getEncoder(wchar_t *encName);
		static CLSID clsidPNGEncoder;
		static CLSID clsidJPEGEncoder;
	public:
		enum quality { PNG,JPEG_80,JPEG_60,JPEG_40,JPEG_10,LAST };

		GdiImage() : image(NULL) {}
		~GdiImage() 
		{
			if(image)
				GdipDisposeImage(image);
		}
		// from ......
		bool fromClipboard();
		bool fromHDC(HDC dc, int w, int h);
		inline bool fromScreenshot()
		{
			return fromHDC(GetWindowDC(HWND_DESKTOP),GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
		}

		bool fromFile(wchar_t* path)
		{
			if(GdipCreateBitmapFromFile(path,&image) != Ok)
				return false;
			resizeToResolution();
			return true;
		}
		
		bool fromHBITMAP(HBITMAP hBitmap)
		{
			if(GdipCreateBitmapFromHBITMAP(hBitmap, 0, &image) != Ok)
				return false;
			resizeToResolution();
			return true;
		}
		// properties
		unsigned int width();
		unsigned int height();
		inline GpBitmap* bitmap() { return image; }
		// functions
		void draw(HDC hdc);
		bool saveToFile(wchar_t *path, quality qual, int x1, int y1, int x2, int y2);
		void drawPixelOnMe(int x, int y, ARGB color, int w);
		void drawLineOnMe(int x1, int y1, int x2, int y2, ARGB color, int w);
		void drawRectOnMe(int x1, int y1, int x2, int y2, ARGB colorPen, ARGB colorBrush, int w);
		void drawEllipseOnMe(int x1, int y1, int x2, int y2, ARGB colorPen, ARGB colorBrush, int w);
		void drawTextOnMe(int x, int y, ARGB color, char *txt);
		void resizeToResolution();
	};
}