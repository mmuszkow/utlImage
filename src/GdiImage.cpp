#include "stdinc.h"
#include "GdiImage.h"

namespace utlImage {

CLSID GdiImage::clsidPNGEncoder = GdiImage::getEncoder(L"image/png");
CLSID GdiImage::clsidJPEGEncoder = GdiImage::getEncoder(L"image/jpeg");

void GdiImage::resizeToResolution()
{
	if(!image)
		return;

	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	if(nScreenWidth <= 0 || nScreenHeight <= 0)
		return;

	unsigned int imgW = width(), imgH = height();
	if(	imgW > static_cast<unsigned int>(nScreenWidth) || 
		imgH > static_cast<unsigned int>(nScreenHeight))
	{
		GpImage* rescaled; 
		Status status = GenericError;

		if(imgW > imgH)
		{
			float ratio = nScreenWidth / static_cast<float>(imgW);
			status = GdipGetImageThumbnail(image, nScreenWidth, static_cast<UINT>(ratio * imgH), 
				&rescaled, NULL, NULL);
		}
		else
		{
			float ratio = nScreenHeight / static_cast<float>(imgH);
			status = GdipGetImageThumbnail(image, static_cast<UINT>(ratio * imgW), nScreenHeight, 
				&rescaled, NULL, NULL);
		}

		if(status != Ok)
			return;

		GdipDisposeImage(image);
		image = static_cast<GpBitmap*>(rescaled);
	}
}

bool GdiImage::fromClipboard()
{
	bool ok = (OpenClipboard(0) != 0);
	if (ok)
	{
		HBITMAP hbm = static_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
		if (hbm)
			ok = fromHBITMAP(hbm);
		else
			ok = false;
		CloseClipboard();
	}
	return ok;
}

bool GdiImage::fromHDC(HDC dc, int w, int h)
{
	HDC cdc = CreateCompatibleDC(dc);
	HBITMAP cbm = static_cast<HBITMAP>(SelectObject(cdc, CreateCompatibleBitmap(dc, w, h)));
	BitBlt(cdc, 0, 0, w, h, dc, 0, 0, SRCCOPY);
	cbm = static_cast<HBITMAP>(SelectObject(cdc, cbm));
	bool ok = fromHBITMAP(cbm);
	DeleteObject(cbm);
	DeleteDC(cdc);
	return ok;
}

unsigned int GdiImage::width()
{
	unsigned int w = 0;
	if (image)
		GdipGetImageWidth(image, &w);
	return w;
}

unsigned int GdiImage::height()
{
	unsigned int h = 0;
	if (image)
		GdipGetImageHeight(image, &h);
	return h;
}

void GdiImage::draw(HDC hdc)
{
	if (!image)
		return;
	GpGraphics *g;
	GdipCreateFromHDC(hdc, &g);
	GdipDrawImageRectI(g, image, 0, 0, width(), height());
	GdipDeleteGraphics(g);
}

bool GdiImage::saveToFile(wchar_t *path, quality qual, int x1, int y1, int x2, int y2)
{
	GpBitmap *destBmp;
	if (x1 > x2)
	{
		int swap = x2;
		x2 = x1;
		x1 = swap;
	}
	if (y1 > y2)
	{
		int swap = y2;
		y2 = y1;
		y1 = swap;
	}
	if (x2 > static_cast<int>(width()) || y2 > static_cast<int>(height()))
		return false;
	GdipCloneBitmapAreaI(x1, y1, x2 - x1, y2 - y1, PixelFormatDontCare, image, &destBmp);

	EncoderParameters encParams;
	long quality = 100L;
	encParams.Count = 1;
	encParams.Parameter[0].Guid = EncoderQuality;
	encParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encParams.Parameter[0].NumberOfValues = 1;
	encParams.Parameter[0].Value = &quality;

	switch(qual)
	{
	case PNG: 
		GdipSaveImageToFile(destBmp, path, &clsidPNGEncoder, 0); 
		break;
	case JPEG_80:
		quality = 80L;
		GdipSaveImageToFile(destBmp, path, &clsidJPEGEncoder, &encParams); 
		break;
	case JPEG_60:
		quality = 60L;
		GdipSaveImageToFile(destBmp, path, &clsidJPEGEncoder, &encParams); 
		break;
	case JPEG_40:
		quality = 40L;
		GdipSaveImageToFile(destBmp, path, &clsidJPEGEncoder, &encParams); 
		break;
	case JPEG_10:
		quality = 10L;
		GdipSaveImageToFile(destBmp, path, &clsidJPEGEncoder, &encParams); 
		break;
	}

	GdipDisposeImage(destBmp);
	return true;
}

void GdiImage::drawPixelOnMe(int x, int y, ARGB color, int w)
{
	for (int i = -(w>>1); i < w-(w>>1); i++)
		for (int j = -(w>>1); j < w-(w>>1); j++)
			GdipBitmapSetPixel(image, x + i, y + j, color);
}

void GdiImage::drawLineOnMe(int x1, int y1, int x2, int y2, ARGB color, int w)
{
	// line DDA algorithm
	int dy = y2 - y1;
	int dx = x2 - x1;
	float t = 0.5f;								// offset for rounding

	drawPixelOnMe(x1,y1,color,w);
	if(abs(dx) > abs(dy)) {							// slope < 1
		float m = (float) dy / (float) dx;		// compute slope
		t += y1;
		dx = (dx < 0) ? -1 : 1;
		m *= dx;
		while (x1 != x2) {
			x1 += dx;							// step to next x value
			t += m;								// add slope to y value
			drawPixelOnMe(x1,(int)t,color,w);
		}
	} else {									// slope >= 1
		float m = (float) dx / (float) dy;		// compute slope
		t += x1;
		dy = (dy < 0) ? -1 : 1;
		m *= dy;
		while (y1 != y2) {
			y1 += dy;                           // step to next y value
			t += m;                             // add slope to x value
			drawPixelOnMe((int)t,y1,color,w);
		}
	}
}

void GdiImage::drawRectOnMe(int x1, int y1, int x2, int y2, ARGB colorPen, ARGB colorBrush, int w)
{
	int x,y;
	if(colorBrush >> 24 != 0) // if not transparent
	{
		for(x = x1+1; x < x2; x++)
			for(y = y1+1; y <y2; y++)
				drawPixelOnMe(x, y, colorBrush, 1);
	}
	for(x = x1; x < x2 + 1; x++)
	{
		drawPixelOnMe(x, y1, colorPen, w);
		drawPixelOnMe(x, y2, colorPen, w);
	}
	for(y = y1; y < y2 + 1; y++)
	{
		drawPixelOnMe(x1, y, colorPen, w);
		drawPixelOnMe(x2, y, colorPen, w);
	}
}

void GdiImage::drawEllipseOnMe(int x1, int y1, int x2, int y2, ARGB colorPen, ARGB colorBrush, int w)
{
	// Bresenham's algorithm
	int rx = abs(x2-x1)>>1;
	int ry = abs(y2-y1)>>1;
	int xc = min(x1,x2)+rx;
	int yc = min(y1,y2)+ry;

	long long int rx_2 = rx*rx,   ry_2 = ry*ry;
    long long int p = ry_2 - rx_2*ry + (ry_2>>2);
    int x = 0,       y = ry;
    long long int two_ry_2_x = 0, two_rx_2_y = (rx_2<<1)*y;
    drawPixelOnMe(xc,yc+y,colorPen,w);
	drawPixelOnMe(xc,yc-y,colorPen,w);

	if(rx==0 || ry==0)
		return;

    while(two_rx_2_y >= two_ry_2_x){
          ++x;
          two_ry_2_x += (ry_2<<1);
          p +=  two_ry_2_x + ry_2;
          if(p >= 0){
               --y;
               two_rx_2_y -= (rx_2<<1);
               p -= two_rx_2_y ;
          }

		  if(colorBrush >> 24 != 0) // if not transparent
			  for(int i=xc-x;i<xc+x+1;i++)
			  {
				  drawPixelOnMe(i,yc+y,colorBrush,1);
				  drawPixelOnMe(i,yc-y,colorBrush,1);
			  }
          drawPixelOnMe(xc+x,yc+y,colorPen,w);
		  drawPixelOnMe(xc-x,yc+y,colorPen,w);
		  drawPixelOnMe(xc-x,yc-y,colorPen,w);
		  drawPixelOnMe(xc+x,yc-y,colorPen,w);
    }
    p = (long long int)(ry_2*(x+1/2.0)*(x+1/2.0) + rx_2*(y-1)*(y-1) - rx_2*ry_2);
    while (y>=0) {
          p += rx_2;
          --y;
          two_rx_2_y -= (rx_2<<1);
          p -= two_rx_2_y;
          if(p <= 0) {
               ++x;
               two_ry_2_x += (ry_2<<1);
               p += two_ry_2_x;
          }

		  if(colorBrush >> 24 != 0) // if not transparent
			  for(int i=xc-x;i<xc+x+1;i++)
			  {
				  drawPixelOnMe(i,yc+y,colorBrush,1);
				  drawPixelOnMe(i,yc-y,colorBrush,1);
			  }
          drawPixelOnMe(xc+x,yc+y,colorPen,w);
		  drawPixelOnMe(xc-x,yc+y,colorPen,w);
		  drawPixelOnMe(xc-x,yc-y,colorPen,w);
		  drawPixelOnMe(xc+x,yc-y,colorPen,w);
    }
}

void GdiImage::drawTextOnMe(int x, int y, ARGB color, char *txt)
{
		int pos = 0;
		while (txt[pos] // while char != '\0'
		)
		{
				if (txt[pos] >= 0x20 && txt[pos] < 0x80 // if char visible
				)
						for (int row = 0; row < CHAR_H; row++)
								for (int col = 15; col >= 0; col--)
										if (((font_data[txt[pos] - 0x20][row] >> col) & 1) == 0)
												drawPixelOnMe(x - (col - 15) + pos * 16, y + row, color, 1);
				pos++;
		}
}

CLSID GdiImage::getEncoder(wchar_t *encName)
{
		CLSID retGUID;
		UINT encNum; // number of image encoders
		UINT encSize; // size, in bytes, of the image encoder array

		CLSIDFromString(L"{00000000-0000-0000-0000-000000000000}",&retGUID); // default value

		ImageCodecInfo *pImageCodecInfo;
		GetImageEncodersSize(&encNum, &encSize);
		pImageCodecInfo = (ImageCodecInfo*)malloc(encSize);
		GetImageEncoders(encNum, encSize, pImageCodecInfo);
		for (UINT j = 0; j < encNum; ++j)
				if (wcscmp(pImageCodecInfo[j].MimeType, encName) == 0)
						retGUID = pImageCodecInfo[j].Clsid;
		free(pImageCodecInfo);

		return retGUID;
}

};
