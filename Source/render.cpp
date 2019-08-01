#include "diablo.h"

DEVILUTION_BEGIN_NAMESPACE

#define NO_OVERDRAW
#define USE_SPEEDCELS
#include <SDL_endian.h>


typedef enum {
	RT_SQUARE,
	RT_TRANSPARENT,
	RT_LTRIANGLE,
	RT_RTRIANGLE,
	RT_LTRAPEZOID,
	RT_RTRAPEZOID
};

static DWORD RightMask[32] = {
	0xE0000000, 0xF0000000,
	0xFE000000, 0xFF000000,
	0xFFE00000, 0xFFF00000,
	0xFFFE0000, 0xFFFF0000,
	0xFFFFE000, 0xFFFFF000,
	0xFFFFFE00, 0xFFFFFF00,
	0xFFFFFFE0, 0xFFFFFFF0,
	0xFFFFFFFE, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

static DWORD LeftMask[32] = {
	0x00000003, 0x0000000F,
	0x0000003F, 0x000000FF,
	0x000003FF, 0x00000FFF,
	0x00003FFF, 0x0000FFFF,
	0x0003FFFF, 0x000FFFFF,
	0x003FFFFF, 0x00FFFFFF,
	0x03FFFFFF, 0x0FFFFFFF,
	0x3FFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

static DWORD WallMask[32] = {
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000,
	0x00000000, 0x00000000
};

static DWORD SolidMask[32] = {
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

inline static void RenderLine(BYTE **dst, BYTE **src, int n, BYTE *tbl, DWORD mask)
{
	int i;

#ifdef NO_OVERDRAW
	if (zoomflag) {
		if ((*dst) < &gpBuffer[(0 + 160) * BUFFER_WIDTH]
		    || (*dst) > &gpBuffer[(VIEWPORT_HEIGHT + 160) * BUFFER_WIDTH]) {
			(*src) += n;
			(*dst) += n;
			return;
		}
	} else {
		if ((*dst) < &gpBuffer[(-17 + 160) * BUFFER_WIDTH]
		    || (*dst) > &gpBuffer[(160 + 160) * BUFFER_WIDTH]) {
			(*src) += n;
			(*dst) += n;
			return;
		}
	}
#endif

	if (mask == 0xFFFFFFFF) {
		if (light_table_index == lightmax) {
			(*src) += n;
			for (i = 0; i < n; i++, (*dst)++) {
				(*dst)[0] = 0;
			}
#ifdef USE_SPEEDCELS
		} else if (tbl == NULL) {
#else
		} else if (light_table_index == 0) {
#endif
			for (i = n & 3; i != 0; i--, (*src)++, (*dst)++) {
				(*dst)[0] = (*src)[0];
			}
			for (i = n >> 2; i != 0; i--, (*src) += 4, (*dst) += 4) {
				((DWORD *)(*dst))[0] = ((DWORD *)(*src))[0];
			}
		} else {
			for (i = 0; i < n; i++, (*src)++, (*dst)++) {
				(*dst)[0] = tbl[(*src)[0]];
			}
		}
	} else {
		if (light_table_index == lightmax) {
			(*src) += n;
			for (i = 0; i < n; i++, (*dst)++, mask <<= 1) {
				if (mask & 0x80000000) {
					(*dst)[0] = 0;
				} else {
					(*dst)[0] = pTransTbl[0][(*dst)[0]];
				}
			}
#ifdef USE_SPEEDCELS
		} else if (tbl == NULL) {
#else
		} else if (light_table_index == 0) {
#endif
			for (i = 0; i < n; i++, (*src)++, (*dst)++, mask <<= 1) {
				if (mask & 0x80000000) {
					(*dst)[0] = (*src)[0];
				} else {
					(*dst)[0] = pTransTbl[(*src)[0]][(*dst)[0]];
				}
			}
		} else {
			for (i = 0; i < n; i++, (*src)++, (*dst)++, mask <<= 1) {
				if (mask & 0x80000000) {
					(*dst)[0] = tbl[(*src)[0]];
				} else {
					(*dst)[0] = pTransTbl[tbl[(*src)[0]]][(*dst)[0]];
				}
			}
		}
	}
}

void RenderTile(BYTE *pBuff)
{
	int i, j;
	char c, v, tile;
	BYTE *src, *dst, *tbl;
	DWORD m, *mask, *pFrameTable;

	dst = pBuff;
	pFrameTable = (DWORD *)pDungeonCels;

	src = &pDungeonCels[pFrameTable[level_cel_block & 0xFFF]];
	tile = (level_cel_block & 0x7000) >> 12;
	tbl = &pLightTbl[256 * light_table_index];

#ifdef USE_SPEEDCELS
	if (light_table_index == lightmax || light_table_index == 0) {
		if (level_cel_block & 0x8000) {
			level_cel_block = SpeedFrameTbl[level_cel_block & 0xFFF][0] + (level_cel_block & 0xF000);
		}
		src = &pDungeonCels[pFrameTable[level_cel_block & 0xFFF]];
		tile = (level_cel_block & 0x7000) >> 12;
		tbl = NULL;
	} else if (level_cel_block & 0x8000) {
		src = &pSpeedCels[SpeedFrameTbl[level_cel_block & 0xFFF][light_table_index]];
		tile = (level_cel_block & 0x7000) >> 12;
		tbl = NULL;
	}
#endif

	mask = &SolidMask[31];

	if (cel_transparency_active) {
		if (arch_draw_type == 0) {
			mask = &WallMask[31];
		}
		if (arch_draw_type == 1 && tile != RT_LTRIANGLE) {
			c = block_lvid[level_piece_id];
			if (c == 1 || c == 3) {
				mask = &LeftMask[31];
			}
		}
		if (arch_draw_type == 2 && tile != RT_RTRIANGLE) {
			c = block_lvid[level_piece_id];
			if (c == 2 || c == 3) {
				mask = &RightMask[31];
			}
		}
	}
/*
#ifdef _DEBUG
	if (GetAsyncKeyState(VK_MENU) & 0x8000) {
		mask = &SolidMask[31];
	}
#endif
*/
	switch (tile) {
	case RT_SQUARE:
		for (i = 32; i != 0; i--, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32, tbl, *mask);
		}
		break;
	case RT_TRANSPARENT:
		for (i = 32; i != 0; i--, dst -= BUFFER_WIDTH + 32, mask--) {
			m = *mask;
			for (j = 32; j != 0; j -= v, m <<= v) {
				v = *src++;
				if (v >= 0) {
					RenderLine(&dst, &src, v, tbl, m);
				} else {
					v = -v;
					dst += v;
				}
			}
		}
		break;
	case RT_LTRIANGLE:
		for (i = 30; i >= 0; i -= 2, dst -= BUFFER_WIDTH + 32, mask--) {
			src += i & 2;
			dst += i;
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
		}
		for (i = 2; i != 32; i += 2, dst -= BUFFER_WIDTH + 32, mask--) {
			src += i & 2;
			dst += i;
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
		}
		break;
	case RT_RTRIANGLE:
		for (i = 30; i >= 0; i -= 2, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
			src += i & 2;
			dst += i;
		}
		for (i = 2; i != 32; i += 2, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
			src += i & 2;
			dst += i;
		}
		break;
	case RT_LTRAPEZOID:
		for (i = 30; i >= 0; i -= 2, dst -= BUFFER_WIDTH + 32, mask--) {
			src += i & 2;
			dst += i;
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
		}
		for (i = 16; i != 0; i--, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32, tbl, *mask);
		}
		break;
	case RT_RTRAPEZOID:
		for (i = 30; i >= 0; i -= 2, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32 - i, tbl, *mask);
			src += i & 2;
			dst += i;
		}
		for (i = 16; i != 0; i--, dst -= BUFFER_WIDTH + 32, mask--) {
			RenderLine(&dst, &src, 32, tbl, *mask);
		}
		break;
	}
}

void trans_rect(int x, int y, int w, int h)
{
	int row, col;
	BYTE *dst = &gpBuffer[SCREENXY(x, y)];
	for (row = 0; row < h; row++) {
		for (col = 0; col < w; col++) {
			*dst = pTransTbl[0][*dst];
			dst++;
		}
		dst += BUFFER_WIDTH - w;
	}
}

DEVILUTION_END_NAMESPACE
