#include "diablo.h"
#include "../3rdParty/Storm/Source/storm.h"
#include "../3rdParty/StormLib/src/StormPort.h"

DEVILUTION_BEGIN_NAMESPACE

char gbPixelCol;  // automap pixel color 8-bit (palette entry)
BOOL gbRotateMap; // flip - if y < x
int orgseed;
int sgnWidth;
int sglGameSeed;
static CCritSect sgMemCrit;
int SeedCount;
BOOL gbNotInView; // valid - if x/y are in bounds

const int rand_increment = 1;
const int rand_multiplier = 0x015A4E35;

void CelDrawDatOnly(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (width & 1) {
					dst[0] = src[0];
					src++;
					dst++;
				}
				width >>= 1;
				if (width & 1) {
					dst[0] = src[0];
					dst[1] = src[1];
					src += 2;
					dst += 2;
				}
				width >>= 1;
				for (; width; width--) {
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
					dst[3] = src[3];
					src += 4;
					dst += 4;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void CelDecodeOnly(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	CelDecDatOnly(&gpBuffer[sx + PitchTbl[sy]], pCelBuff, nCel, nWidth);
}

void CelDecDatOnly(BYTE *pBuff, BYTE *pCelBuff, int nCel, int nWidth, bool hdr, bool lighting, bool transparency)
{
	int nDataStart, nDataSize;
	BYTE *pRLEBytes;
	DWORD *pFrameTable;

	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);

	pRLEBytes = &pCelBuff[nStart];
	if (hdr)
		nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	else
		nDataStart = 0;

	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;	if (CelCap == 8)
		nDataSize -= nDataStart;

	pRLEBytes += nDataStart;

	if (transparency && cel_transparency_active)
		CelDecDatLightTrans(pBuff, pRLEBytes, nDataSize, nWidth);
	else if (lighting && light_table_index)
		CelDecDatLightOnly(pBuff, pRLEBytes, nDataSize, nWidth);
	else
		CelDrawDatOnly(pBuff, pRLEBytes, nDataSize, nWidth);
}

void CelDrawHdrOnly(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	CelDecDatOnly(&gpBuffer[sx + PitchTbl[sy]], pCelBuff, nCel, nWidth, true);
}

void CelDecDatLightOnly(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	BYTE *tbl;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	tbl = &pLightTbl[light_table_index * 256];
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (width & 1) {
					dst[0] = tbl[src[0]];
					src++;
					dst++;
				}
				width >>= 1;
				if (width & 1) {
					dst[0] = tbl[src[0]];
					dst[1] = tbl[src[1]];
					src += 2;
					dst += 2;
				}
				width >>= 1;
				for (; width; width--) {
					dst[0] = tbl[src[0]];
					dst[1] = tbl[src[1]];
					dst[2] = tbl[src[2]];
					dst[3] = tbl[src[3]];
					src += 4;
					dst += 4;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void CelDecDatLightTrans(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	BYTE *tbl;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	tbl = &pLightTbl[light_table_index * 256];
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (width & 1) {
					dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
					src++;
					dst++;
				}
				width >>= 1;
				if (width & 1) {
					dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
					dst[1] = pTransTbl[tbl[src[1]]][dst[1]];
					src += 2;
					dst += 2;
				}
				width >>= 1;
				for (; width; width--) {
					dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
					dst[1] = pTransTbl[tbl[src[1]]][dst[1]];
					dst[2] = pTransTbl[tbl[src[2]]][dst[2]];
					dst[3] = pTransTbl[tbl[src[3]]][dst[3]];
					src += 4;
					dst += 4;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void CelDecodeHdrLightOnly(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	CelDecDatOnly(&gpBuffer[sx + PitchTbl[sy]], pCelBuff, nCel, nWidth, true, true);
}

void CelDrawHdrLightRed(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, char light)
{
	int nDataStart, nDataSize, w, idx;
	BYTE *pRLEBytes, *dst, *tbl;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;	if (CelCap == 8)
		nDataSize -= nDataStart;

	pRLEBytes += nDataStart;
	dst = &gpBuffer[sx + PitchTbl[sy]];

	idx = light4flag ? 1024 : 4096;
	if (light == 2)
		idx += 256;
	if (light >= 4)
		idx += (light - 1) << 8;

	BYTE width;
	BYTE *end;

	tbl = &pLightTbl[idx];
	end = &pRLEBytes[nDataSize];

	for (; pRLEBytes != end; dst -= BUFFER_WIDTH + nWidth) {
		for (w = nWidth; w;) {
			width = *pRLEBytes++;
			if (!(width & 0x80)) {
				w -= width;
				while (width) {
					*dst = tbl[*pRLEBytes];
					pRLEBytes++;
					dst++;
					width--;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void Cel2DecDatOnly(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;
	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (dst < gpBufEnd) {
					if (width & 1) {
						dst[0] = src[0];
						src++;
						dst++;
					}
					width >>= 1;
					if (width & 1) {
						dst[0] = src[0];
						dst[1] = src[1];
						src += 2;
						dst += 2;
					}
					width >>= 1;
					for (; width; width--) {
						dst[0] = src[0];
						dst[1] = src[1];
						dst[2] = src[2];
						dst[3] = src[3];
						src += 4;
						dst += 4;
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void Cel2DrawHdrOnly(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	Cel2DecodeHdrOnly(&gpBuffer[sx + PitchTbl[sy]], pCelBuff, nCel, nWidth);
}

void Cel2DecodeHdrOnly(BYTE *pBuff, BYTE *pCelBuff, int nCel, int nWidth, bool lighting, bool transparency)
{
	int nDataStart, nDataSize;
	BYTE *pRLEBytes;
	DWORD *pFrameTable;

	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;

	pFrameTable = (DWORD *)pCelBuff;

	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[CelSkip]);

	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;	nDataCap = *(WORD *)&pRLEBytes[CelCap];
	nDataSize -= nDataStart;

	pRLEBytes += nDataStart;

	if (transparency && cel_transparency_active)
		Cel2DecDatLightTrans(pBuff, pRLEBytes, nDataSize, nWidth);
	else if (lighting && light_table_index)
		Cel2DecDatLightOnly(pBuff, pRLEBytes, nDataSize, nWidth);
	else
		Cel2DecDatOnly(pBuff, pRLEBytes, nDataSize, nWidth);
}

void Cel2DecDatLightOnly(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	BYTE *tbl;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;
	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	tbl = &pLightTbl[light_table_index * 256];
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (dst < gpBufEnd) {
					if (width & 1) {
						dst[0] = tbl[src[0]];
						src++;
						dst++;
					}
					width >>= 1;
					if (width & 1) {
						dst[0] = tbl[src[0]];
						dst[1] = tbl[src[1]];
						src += 2;
						dst += 2;
					}
					width >>= 1;
					for (; width; width--) {
						dst[0] = tbl[src[0]];
						dst[1] = tbl[src[1]];
						dst[2] = tbl[src[2]];
						dst[3] = tbl[src[3]];
						src += 4;
						dst += 4;
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void Cel2DecDatLightTrans(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	BYTE *tbl;

	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;
	/// ASSERT: assert(pRLEBytes != NULL);
	if (!pRLEBytes)
		return;
	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;

	int i;
	BYTE width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	tbl = &pLightTbl[light_table_index * 256];
	w = nWidth;

	for (; src != &pRLEBytes[nDataSize]; dst -= BUFFER_WIDTH + w) {
		for (i = w; i;) {
			width = *src++;
			if (!(width & 0x80)) {
				i -= width;
				if (dst < gpBufEnd) {
					if (width & 1) {
						dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
						src++;
						dst++;
					}
					width >>= 1;
					if (width & 1) {
						dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
						dst[1] = pTransTbl[tbl[src[1]]][dst[1]];
						src += 2;
						dst += 2;
					}
					width >>= 1;
					for (; width; width--) {
						dst[0] = pTransTbl[tbl[src[0]]][dst[0]];
						dst[1] = pTransTbl[tbl[src[1]]][dst[1]];
						dst[2] = pTransTbl[tbl[src[2]]][dst[2]];
						dst[3] = pTransTbl[tbl[src[3]]][dst[3]];
						src += 4;
						dst += 4;
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void Cel2DecodeHdrLight(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	Cel2DecodeHdrOnly(&gpBuffer[sx + PitchTbl[sy]], pCelBuff, nCel, nWidth, true);
}

void Cel2DrawHdrLightRed(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, char light)
{
	int nDataStart, nDataSize, w, idx;
	BYTE *pRLEBytes, *dst, *tbl;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	if (!nDataStart)
		return;

	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;	if (CelCap == 8)
	nDataSize -= nDataStart;

	pRLEBytes += nDataStart;
	dst = &gpBuffer[sx + PitchTbl[sy]];

	idx = light4flag ? 1024 : 4096;
	if (light == 2)
		idx += 256;
	if (light >= 4)
		idx += (light - 1) << 8;

	tbl = &pLightTbl[idx];

	BYTE width;
	BYTE *end;

	end = &pRLEBytes[nDataSize];

	for (; pRLEBytes != end; dst -= BUFFER_WIDTH + nWidth) {
		for (w = nWidth; w;) {
			width = *pRLEBytes++;
			if (!(width & 0x80)) {
				w -= width;
				if (dst < gpBufEnd) {
					while (width) {
						*dst = tbl[*pRLEBytes];
						pRLEBytes++;
						dst++;
						width--;
					}
				} else {
					pRLEBytes += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelDecodeRect(BYTE *pBuff, int hgt, int wdt, BYTE *pCelBuff, int nCel, int nWidth)
{
	BYTE *pRLEBytes, *dst, *end;

	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(pBuff != NULL);
	if (!pBuff)
		return;

	int i;
	BYTE width;
	DWORD *pFrameTable;

	pFrameTable = (DWORD *)&pCelBuff[4 * nCel];
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[0]);
	pRLEBytes =  &pCelBuff[nStart];
	end = &pRLEBytes[BSWAP_INT32_UNSIGNED(pFrameTable[1]) - nStart];
	dst = &pBuff[hgt * wdt];
	for (; pRLEBytes != end; dst -= wdt + nWidth) {
		for (i = nWidth; i;) {
			width = *pRLEBytes++;
			if (!(width & 0x80)) {
				i -= width;
				if (width & 1) {
					dst[0] = pRLEBytes[0];
					pRLEBytes++;
					dst++;
				}
				width >>= 1;
				if (width & 1) {
					dst[0] = pRLEBytes[0];
					dst[1] = pRLEBytes[1];
					pRLEBytes += 2;
					dst += 2;
				}
				width >>= 1;
				while (width) {
					dst[0] = pRLEBytes[0];
					dst[1] = pRLEBytes[1];
					dst[2] = pRLEBytes[2];
					dst[3] = pRLEBytes[3];
					pRLEBytes += 4;
					dst += 4;
					width--;
				}
			} else {
				width = -(char)width;
				dst += width;
				i -= width;
			}
		}
	}
}

void CelDecodeClr(char col, int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	int nDataStart, nDataSize, w;
	BYTE *pRLEBytes, *dst;

	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;

	BYTE width;
	BYTE *end, *src;
	DWORD *pFrameTable;

	pFrameTable = (DWORD *)&pCelBuff[4 * nCel];
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[0]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);

	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[1]) - nStart - nDataStart;

	src = pRLEBytes + nDataStart;
	end = &src[nDataSize];
	dst = &gpBuffer[sx + PitchTbl[sy]];

	for (; src != end; dst -= BUFFER_WIDTH + nWidth) {
		for (w = nWidth; w;) {
			width = *src++;
			if (!(width & 0x80)) {
				w -= width;
				while (width) {
					if (*src++) {
						dst[-BUFFER_WIDTH] = col;
						dst[-1] = col;
						dst[1] = col;
						dst[BUFFER_WIDTH] = col;
					}
					dst++;
					width--;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void CelDrawHdrClrHL(char col, int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	int nDataStart, nDataSize, w;
	BYTE *pRLEBytes, *dst;

	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(gpBuffer);
	if (!gpBuffer)
		return;

	BYTE width;
	BYTE *end, *src;
	DWORD *pFrameTable;

	pFrameTable = (DWORD *)&pCelBuff[4 * nCel];
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[0]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[1]) - nStart - nDataStart;
	src = pRLEBytes + nDataStart;
	end = &src[nDataSize];
	dst = &gpBuffer[sx + PitchTbl[sy]];

	for (; src != end; dst -= BUFFER_WIDTH + nWidth) {
		for (w = nWidth; w;) {
			width = *src++;
			if (!(width & 0x80)) {
				w -= width;
				if (dst < gpBufEnd) {
					if (dst >= gpBufEnd - BUFFER_WIDTH) {
						while (width) {
							if (*src++) {
								dst[-BUFFER_WIDTH] = col;
								dst[-1] = col;
								dst[1] = col;
							}
							dst++;
							width--;
						}
					} else {
						while (width) {
							if (*src++) {
								dst[-BUFFER_WIDTH] = col;
								dst[-1] = col;
								dst[1] = col;
								dst[BUFFER_WIDTH] = col;
							}
							dst++;
							width--;
						}
					}
				} else {
					src += width;
					dst += width;
				}
			} else {
				width = -(char)width;
				dst += width;
				w -= width;
			}
		}
	}
}

void ENG_set_pixel(int sx, int sy, BYTE col)
{
	BYTE *dst;

	/// ASSERT: assert(gpBuffer);

	if (sy < 0 || sy >= SCREEN_HEIGHT + SCREEN_Y || sx < SCREEN_X || sx >= SCREEN_WIDTH + SCREEN_X)
		return;

	dst = &gpBuffer[sx + PitchTbl[sy]];

	if (dst < gpBufEnd)
		*dst = col;
}

void engine_draw_pixel(int sx, int sy)
{
	BYTE *dst;

	/// ASSERT: assert(gpBuffer);

	if (gbRotateMap) {
		if (gbNotInView && (sx < 0 || sx >= SCREEN_HEIGHT + SCREEN_Y || sy < SCREEN_X || sy >= SCREEN_WIDTH + SCREEN_X))
			return;
		dst = &gpBuffer[sy + PitchTbl[sx]];
	} else {
		if (gbNotInView && (sy < 0 || sy >= SCREEN_HEIGHT + SCREEN_Y || sx < SCREEN_X || sx >= SCREEN_WIDTH + SCREEN_X))
			return;
		dst = &gpBuffer[sx + PitchTbl[sy]];
	}

	if (dst < gpBufEnd)
		*dst = gbPixelCol;
}

// Exact copy from https://github.com/erich666/GraphicsGems/blob/dad26f941e12c8bf1f96ea21c1c04cd2206ae7c9/gems/DoubleLine.c
// Except:
// * not in view checks
// * global variable instead of reverse flag
// * condition for pixels_left < 0 removed

/*
Symmetric Double Step Line Algorithm
by Brian Wyvill
from "Graphics Gems", Academic Press, 1990
*/

#define GG_SWAP(A, B) \
	{                 \
		(A) ^= (B);   \
		(B) ^= (A);   \
		(A) ^= (B);   \
	}
#define GG_ABSOLUTE(I, J, K) (((I) - (J)) * ((K) = (((I) - (J)) < 0 ? -1 : 1)))

void DrawLine(int x0, int y0, int x1, int y1, BYTE col)
{
	int i, dx, dy, steps;
	float ix, iy, sx, sy;

	dx = x1 - x0;
	dy = y1 - y0;
	steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
	ix = dx / (float)steps;
	iy = dy / (float)steps;
	sx = x0;
	sy = y0;

	for (i = 0; i <= steps; i++, sx += ix, sy += iy) {
		ENG_set_pixel(sx, sy, col);
	}
}

int GetDirection(int x1, int y1, int x2, int y2)
{
	int mx, my;
	int md, ny;

	mx = x2 - x1;
	my = y2 - y1;

	if (mx >= 0) {
		if (my >= 0) {
			md = DIR_S;
			if (2 * mx < my)
				md = DIR_SW;
		} else {
			my = -my;
			md = DIR_E;
			if (2 * mx < my)
				md = DIR_NE;
		}
		if (2 * my < mx)
			return DIR_SE;
	} else {
		ny = -mx;
		if (my >= 0) {
			md = DIR_W;
			if (2 * ny < my)
				md = DIR_SW;
		} else {
			my = -my;
			md = DIR_N;
			if (2 * ny < my)
				md = DIR_NE;
		}
		if (2 * my < ny)
			return DIR_NW;
	}

	return md;
}

void SetRndSeed(int s)
{
	SeedCount = 0;
	sglGameSeed = s;
	orgseed = s;
}

int GetRndSeed()
{
	SeedCount++;
	sglGameSeed = rand_multiplier * sglGameSeed + rand_increment;
	return abs(sglGameSeed);
}

int random(BYTE idx, int v)
{
	if (v <= 0)
		return 0;
	if (v >= 0xFFFF)
		return GetRndSeed() % v;
	return (GetRndSeed() >> 16) % v;
}

void engine_debug_trap(BOOL show_cursor)
{
	/*
	TMemBlock *pCurr;

	sgMemCrit.Enter();
	while(sgpMemBlock != NULL) {
		pCurr = sgpMemBlock->pNext;
		SMemFree(sgpMemBlock, "C:\\Diablo\\Direct\\ENGINE.CPP", 1970);
		sgpMemBlock = pCurr;
	}
	sgMemCrit.Leave();
*/
}

BYTE *DiabloAllocPtr(DWORD dwBytes)
{
	BYTE *buf;

	sgMemCrit.Enter();
	buf = (BYTE *)SMemAlloc(dwBytes, __FILE__, __LINE__, 0);
	sgMemCrit.Leave();

	if (buf == NULL) {
		ERR_DLG(IDD_DIALOG2, GetLastError());
	}

	return buf;
}

void mem_free_dbg(void *p)
{
	if (p) {
		sgMemCrit.Enter();
		SMemFree(p, __FILE__, __LINE__, 0);
		sgMemCrit.Leave();
	}
}

BYTE *LoadFileInMem(char *pszName, DWORD *pdwFileLen)
{
	HANDLE file;
	BYTE *buf;
	int fileLen;

	WOpenFile(pszName, &file, FALSE);
	fileLen = WGetFileSize(file, NULL);

	if (pdwFileLen)
		*pdwFileLen = fileLen;

	if (!fileLen)
		app_fatal("Zero length SFILE:\n%s", pszName);

	buf = (BYTE *)DiabloAllocPtr(fileLen);

	WReadFile(file, buf, fileLen);
	WCloseFile(file);

	return buf;
}

DWORD LoadFileWithMem(const char *pszName, void *p)
{
	DWORD dwFileLen;
	HANDLE hsFile;

	/// ASSERT: assert(pszName);
	if (p == NULL) {
		app_fatal("LoadFileWithMem(NULL):\n%s", pszName);
	}

	WOpenFile(pszName, &hsFile, FALSE);

	dwFileLen = WGetFileSize(hsFile, NULL);
	if (dwFileLen == 0) {
		app_fatal("Zero length SFILE:\n%s", pszName);
	}

	WReadFile(hsFile, p, dwFileLen);
	WCloseFile(hsFile);

	return dwFileLen;
}

void Cl2ApplyTrans(BYTE *p, BYTE *ttbl, int nCel)
{
	int i, nDataSize;
	char width;
	BYTE *dst;
	DWORD *pFrameTable;

	/// ASSERT: assert(p != NULL);
	/// ASSERT: assert(ttbl != NULL);

	for (i = 1; i <= nCel; i++) {
		pFrameTable = BSWAP_INT32_UNSIGNED((DWORD *)&p[4 * i]);
		dst = &p[pFrameTable[0] + 10];
		nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[1]) - (BSWAP_INT32_UNSIGNED(pFrameTable[0]) - 10);
		while (nDataSize) {
			width = *dst++;
			nDataSize--;
			/// ASSERT: assert(nDataSize >= 0);
			if (width < 0) {
				width = -width;
				if (width > 65) {
					nDataSize--;
					/// ASSERT: assert(nDataSize >= 0);
					*dst = ttbl[*dst];
					dst++;
				} else {
					nDataSize -= width;
					/// ASSERT: assert(nDataSize >= 0);
					while (width) {
						*dst = ttbl[*dst];
						dst++;
						width--;
					}
				}
			}
		}
	}
}

void Cl2DecodeFrm1(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, bool light)
{
	int nDataStart, nDataSize;
	BYTE *pRLEBytes, *pBuff;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	nDataSize -= nDataStart;
	pRLEBytes += nDataStart;

	pBuff = &gpBuffer[sx + PitchTbl[sy]];

	if (light && light_table_index)
		Cl2DecDatLightTbl1(pBuff, pRLEBytes, nDataSize, nWidth, &pLightTbl[light_table_index * 256]);
	else
		Cl2DecDatFrm1(pBuff, pRLEBytes, nDataSize, nWidth);
}

void Cl2DecDatFrm1(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	char width;
	BYTE fill;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				fill = *src++;
				w -= width;
				while (width) {
					*dst = fill;
					dst++;
					width--;
				}
				if (!w) {
					w = nWidth;
					dst -= BUFFER_WIDTH + w;
				}
				continue;
			} else {
				nDataSize -= width;
				w -= width;
				while (width) {
					*dst = *src;
					src++;
					dst++;
					width--;
				}
				if (!w) {
					w = nWidth;
					dst -= BUFFER_WIDTH + w;
				}
				continue;
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = nWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void Cl2DecodeFrm2(char col, int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	int nDataStart, nDataSize;
	BYTE *pRLEBytes;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[pFrameTable[nCel]];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	Cl2DecDatFrm2(
	    &gpBuffer[sx + PitchTbl[sy]],
	    pRLEBytes + nDataStart,
	    nDataSize - nDataStart,
	    nWidth,
	    col);
}

void Cl2DecDatFrm2(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth, char col)
{
	int w;
	char width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				if (*src++) {
					w -= width;
					dst[-1] = col;
					dst[width] = col;
					while (width) {
						dst[-BUFFER_WIDTH] = col;
						dst[BUFFER_WIDTH] = col;
						dst++;
						width--;
					}
					if (!w) {
						w = nWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				w -= width;
				while (width) {
					if (*src++) {
						dst[-1] = col;
						dst[1] = col;
						dst[-BUFFER_WIDTH] = col;
						dst[BUFFER_WIDTH] = col;
					}
					dst++;
					width--;
				}
				if (!w) {
					w = nWidth;
					dst -= BUFFER_WIDTH + w;
				}
				continue;
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = nWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void Cl2DecodeFrm3(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, char light)
{
	int nDataStart, nDataSize, idx, nSize;
	BYTE *pRLEBytes, *pBuff;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	nSize = nDataSize - nDataStart;
	pRLEBytes += nDataStart;
	pBuff = &gpBuffer[sx + PitchTbl[sy]];

	idx = light4flag ? 1024 : 4096;
	if (light == 2)
		idx += 256;
	if (light >= 4)
		idx += (light - 1) << 8;

	Cl2DecDatLightTbl1(
	    pBuff,
	    pRLEBytes,
	    nSize,
	    nWidth,
	    &pLightTbl[idx]);
}

void Cl2DecDatLightTbl1(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *pTable)
{
	int w;
	char width;
	BYTE fill;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;
	sgnWidth = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				fill = pTable[*src++];
				w -= width;
				while (width) {
					*dst = fill;
					dst++;
					width--;
				}
				if (!w) {
					w = sgnWidth;
					dst -= BUFFER_WIDTH + w;
				}
				continue;
			} else {
				nDataSize -= width;
				w -= width;
				while (width) {
					*dst = pTable[*src];
					src++;
					dst++;
					width--;
				}
				if (!w) {
					w = sgnWidth;
					dst -= BUFFER_WIDTH + w;
				}
				continue;
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = sgnWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void Cl2DecodeFrm4(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, bool hdr, bool lighting)
{
	DWORD *pFrameTable;
	int nDataStart, nDataSize;
	BYTE *pRLEBytes, *pBuff;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	nDataSize -= nDataStart;
	pRLEBytes += nDataStart;
	pBuff = &gpBuffer[sx + PitchTbl[sy]];

	if (lighting && light_table_index)
		Cl2DecDatLightTbl2(pBuff, pRLEBytes, nDataSize, nWidth, &pLightTbl[light_table_index * 256]);
	else
		Cl2DecDatFrm4(pBuff, pRLEBytes, nDataSize, nWidth);
}

void Cl2DecDatFrm4(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
	int w;
	char width;
	BYTE fill;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				fill = *src++;
				if (dst < gpBufEnd) {
					w -= width;
					while (width) {
						*dst = fill;
						dst++;
						width--;
					}
					if (!w) {
						w = nWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < gpBufEnd) {
					w -= width;
					while (width) {
						*dst = *src;
						src++;
						dst++;
						width--;
					}
					if (!w) {
						w = nWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				} else {
					src += width;
				}
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = nWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void Cl2DecodeClrHL(char col, int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth)
{
	int nDataStart, nDataSize;
	BYTE *pRLEBytes;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	gpBufEnd -= BUFFER_WIDTH;
	Cl2DecDatClrHL(
	    &gpBuffer[sx + PitchTbl[sy]],
	    pRLEBytes + nDataStart,
	    nDataSize - nDataStart,
	    nWidth,
	    col);
	gpBufEnd += BUFFER_WIDTH;
}

void Cl2DecDatClrHL(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth, char col)
{
	int w;
	char width;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				if (*src++ && dst < gpBufEnd) {
					w -= width;
					dst[-1] = col;
					dst[width] = col;
					while (width) {
						dst[-BUFFER_WIDTH] = col;
						dst[BUFFER_WIDTH] = col;
						dst++;
						width--;
					}
					if (!w) {
						w = nWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < gpBufEnd) {
					w -= width;
					while (width) {
						if (*src++) {
							dst[-1] = col;
							dst[1] = col;
							dst[-BUFFER_WIDTH] = col;
							dst[BUFFER_WIDTH] = col;
						}
						dst++;
						width--;
					}
					if (!w) {
						w = nWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				} else {
					src += width;
				}
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = nWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void Cl2DecodeFrm5(int sx, int sy, BYTE *pCelBuff, int nCel, int nWidth, char light)
{
	int nDataStart, nDataSize, idx, nSize;
	BYTE *pRLEBytes, *pBuff;
	DWORD *pFrameTable;

	/// ASSERT: assert(gpBuffer != NULL);
	if (!gpBuffer)
		return;
	/// ASSERT: assert(pCelBuff != NULL);
	if (!pCelBuff)
		return;
	/// ASSERT: assert(nCel > 0);
	if (nCel <= 0)
		return;

	pFrameTable = (DWORD *)pCelBuff;
	/// ASSERT: assert(nCel <= (int) pFrameTable[0]);
	int nStart = BSWAP_INT32_UNSIGNED(pFrameTable[nCel]);
	pRLEBytes = &pCelBuff[nStart];
	nDataStart = BSWAP_INT16_UNSIGNED(*(WORD *)&pRLEBytes[0]);
	nDataSize = BSWAP_INT32_UNSIGNED(pFrameTable[nCel + 1]) - nStart;

	nSize = nDataSize - nDataStart;
	pRLEBytes += nDataStart;
	pBuff = &gpBuffer[sx + PitchTbl[sy]];

	idx = light4flag ? 1024 : 4096;
	if (light == 2)
		idx += 256;
	if (light >= 4)
		idx += (light - 1) << 8;

	Cl2DecDatLightTbl2(
	    pBuff,
	    pRLEBytes,
	    nSize,
	    nWidth,
	    &pLightTbl[idx]);
}

void Cl2DecDatLightTbl2(BYTE *pBuff, BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *pTable)
{
	int w;
	char width;
	BYTE fill;
	BYTE *src, *dst;

	src = pRLEBytes;
	dst = pBuff;
	w = nWidth;
	sgnWidth = nWidth;

	while (nDataSize) {
		width = *src++;
		nDataSize--;
		if (width < 0) {
			width = -width;
			if (width > 65) {
				width -= 65;
				nDataSize--;
				fill = pTable[*src++];
				if (dst < gpBufEnd) {
					w -= width;
					while (width) {
						*dst = fill;
						dst++;
						width--;
					}
					if (!w) {
						w = sgnWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				}
			} else {
				nDataSize -= width;
				if (dst < gpBufEnd) {
					w -= width;
					while (width) {
						*dst = pTable[*src];
						src++;
						dst++;
						width--;
					}
					if (!w) {
						w = sgnWidth;
						dst -= BUFFER_WIDTH + w;
					}
					continue;
				} else {
					src += width;
				}
			}
		}
		while (width) {
			if (width > w) {
				dst += w;
				width -= w;
				w = 0;
			} else {
				dst += width;
				w -= width;
				width = 0;
			}
			if (!w) {
				w = sgnWidth;
				dst -= BUFFER_WIDTH + w;
			}
		}
	}
}

void PlayInGameMovie(char *pszMovie)
{
	PaletteFadeOut(8);
	play_movie(pszMovie, 0);
	ClearScreenBuffer();
	drawpanflag = 255;
	scrollrt_draw_game_screen(1);
	PaletteFadeIn(8);
	drawpanflag = 255;
}

DEVILUTION_END_NAMESPACE
