//From: Visual Style Menus in MSDN

#include "Bitmap.h"

HMODULE hUxTheme = NULL;
FN_GetBufferedPaintBits pfnGetBufferedPaintBits = NULL;
FN_BeginBufferedPaint pfnBeginBufferedPaint = NULL;
FN_EndBufferedPaint pfnEndBufferedPaint = NULL;

bool InitTheming()
{
	hUxTheme = ::LoadLibrary(TEXT("UxTheme.dll"));
	if (hUxTheme == NULL)
		return false;
	pfnGetBufferedPaintBits = (FN_GetBufferedPaintBits)::GetProcAddress(hUxTheme, "GetBufferedPaintBits");
	pfnBeginBufferedPaint = (FN_BeginBufferedPaint)::GetProcAddress(hUxTheme, "BeginBufferedPaint");
	pfnEndBufferedPaint = (FN_EndBufferedPaint)::GetProcAddress(hUxTheme, "EndBufferedPaint");
	if ((pfnGetBufferedPaintBits == NULL) || (pfnBeginBufferedPaint == NULL) || (pfnEndBufferedPaint == NULL)) {
		pfnGetBufferedPaintBits = NULL;
		pfnBeginBufferedPaint = NULL;
		pfnEndBufferedPaint = NULL;
		return false;
	}

	return true;
}

bool DeinitTheming()
{
	pfnGetBufferedPaintBits = NULL;
	pfnBeginBufferedPaint = NULL;
	pfnEndBufferedPaint = NULL;
	FreeLibrary(hUxTheme);
	hUxTheme = NULL;

	return true;
}

void InitBitmapInfo(BITMAPINFO *pbmi, ULONG cbInfo, LONG cx, LONG cy, WORD bpp)
{
	ZeroMemory(pbmi, cbInfo);
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biCompression = BI_RGB;

	pbmi->bmiHeader.biWidth = cx;
	pbmi->bmiHeader.biHeight = cy;
	pbmi->bmiHeader.biBitCount = bpp;
}

HRESULT Create32BitHBITMAP(HDC hdc, const SIZE *psize, void **ppvBits, HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;
	InitBitmapInfo(&bmi, sizeof(bmi), psize->cx, psize->cy, 32);

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);
	if (hdcUsed) {
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		if (hdc != hdcUsed) {
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT ConvertToPARGB32(HDC hdc, ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow)
{
	BITMAPINFO bmi;
	InitBitmapInfo(&bmi, sizeof(bmi), sizImage.cx, sizImage.cy, 32);

	HRESULT hr = E_OUTOFMEMORY;
	HANDLE hHeap = GetProcessHeap();
	void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
	if (pvBits) {
		hr = E_UNEXPECTED;
		if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight) {
			ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
			ARGB *pargbMask = static_cast<ARGB *>(pvBits);

			for (ULONG y = bmi.bmiHeader.biHeight; y; --y) {
				for (ULONG x = bmi.bmiHeader.biWidth; x; --x) {
					if (*pargbMask++) {
						// transparent pixel
						*pargb++ = 0;
					} else {
						// opaque pixel
						*pargb++ |= 0xFF000000;
					}
				}
				pargb += cxDelta;
			}
			hr = S_OK;
		}
		HeapFree(hHeap, 0, pvBits);
	}
	return hr;
}

bool HasAlpha(ARGB *pargb, SIZE& sizImage, int cxRow)
{
	ULONG cxDelta = cxRow - sizImage.cx;
	for (ULONG y = sizImage.cy; y; --y) {
		for (ULONG x = sizImage.cx; x; --x) {
			if (*pargb++ & 0xFF000000) {
				return true;
			}
		}
		pargb += cxDelta;
	}
	return false;
}

HRESULT ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon)
{
	RGBQUAD *prgbQuad;
	int cxRow;
	HRESULT hr = pfnGetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
	if (SUCCEEDED(hr)) {
		ARGB *pargb = reinterpret_cast<ARGB *>(prgbQuad);
		if (!HasAlpha(pargb, sizIcon, cxRow)) {
			ICONINFO info;
			if (GetIconInfo(hicon, &info)) {
				if (info.hbmMask) {
					hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
				}
				DeleteObject(info.hbmColor);
				DeleteObject(info.hbmMask);
			}
		}
	}
	return hr;
}

HBITMAP IconToBitmapPARGB32(HICON hIcon, DWORD cx, DWORD cy)
{
	HRESULT hr = E_OUTOFMEMORY;
	HBITMAP hBmp = NULL;

	if (!hIcon)
		return NULL;

	SIZE sizIcon;
	sizIcon.cx = cx;
	sizIcon.cy = cy;

	RECT rcIcon;
	SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);

	HDC hdcDest = CreateCompatibleDC(NULL);
	if (hdcDest) {
		hr = Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp);
		if (SUCCEEDED(hr)) {
			hr = E_FAIL;

			HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hBmp);
			if(hbmpOld) {
				BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				BP_PAINTPARAMS paintParams = {0, 0, 0, 0};
				paintParams.cbSize = sizeof(paintParams);
				paintParams.dwFlags = BPPF_ERASE;
				paintParams.pBlendFunction = &bfAlpha;

				HDC hdcBuffer;
				HPAINTBUFFER hPaintBuffer = pfnBeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
				if(hPaintBuffer) {
					if(DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL)) {
						// If icon did not have an alpha channel, we need to convert buffer to PARGB.
						hr = ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
					}

					// This will write the buffer contents to the destination bitmap.
					pfnEndBufferedPaint(hPaintBuffer, TRUE);
				}
				SelectObject(hdcDest, hbmpOld);
			}
		}
		DeleteDC(hdcDest);
	}

	DestroyIcon(hIcon);
	if (SUCCEEDED(hr)) {
		return hBmp;
	}
	DeleteObject(hBmp);
	return NULL;
}
