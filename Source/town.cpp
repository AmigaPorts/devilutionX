#include "diablo.h"
#include "../3rdParty/StormLib/src/StormPort.h"

DEVILUTION_BEGIN_NAMESPACE

void town_special(BYTE *pBuff, int nCel)
{
#if 0
	int nDataSize;
	BYTE *pRLEBytes;
	DWORD *pFrameTable;

	pFrameTable = (DWORD *)pSpecialCels;
	pRLEBytes = &pSpecialCels[pFrameTable[nCel]];
	nDataSize = pFrameTable[nCel + 1] - pFrameTable[nCel];
	Cel2DecDatOnly(pBuff, pRLEBytes, nDataSize, 64);
#endif
}

void town_draw_town(BYTE *pBuff, int sx, int sy, int dx, int dy)
{
	int mi, px, py;
	char bv, bArch;

	bArch = dArch[sx][sy];

	if (dItem[sx][sy] != 0) {
		bv = dItem[sx][sy] - 1;
		px = dx - item[bv]._iAnimWidth2;
		if (bv == pcursitem) {
			CelDecodeClr(181, px, dy, item[bv]._iAnimData, item[bv]._iAnimFrame, item[bv]._iAnimWidth);
		}
		/// ASSERT: assert(item[bv]._iAnimData);
		CelDrawHdrOnly(px, dy, item[bv]._iAnimData, item[bv]._iAnimFrame, item[bv]._iAnimWidth);
	}
	if (dFlags[sx][sy] & BFLAG_MONSTLR) {
		mi = -(dMonster[sx][sy - 1] + 1);
		px = dx - towner[mi]._tAnimWidth2;
		if (mi == pcursmonst) {
			CelDecodeClr(166, px, dy, towner[mi]._tAnimData, towner[mi]._tAnimFrame, towner[mi]._tAnimWidth);
		}
		/// ASSERT: assert(towner[mi]._tAnimData);
		CelDrawHdrOnly(px, dy, towner[mi]._tAnimData, towner[mi]._tAnimFrame, towner[mi]._tAnimWidth);
	}
	if (dMonster[sx][sy] > 0) {
		mi = dMonster[sx][sy] - 1;
		px = dx - towner[mi]._tAnimWidth2;
		if (mi == pcursmonst) {
			CelDecodeClr(166, px, dy, towner[mi]._tAnimData, towner[mi]._tAnimFrame, towner[mi]._tAnimWidth);
		}
		/// ASSERT: assert(towner[mi]._tAnimData);
		CelDrawHdrOnly(px, dy, towner[mi]._tAnimData, towner[mi]._tAnimFrame, towner[mi]._tAnimWidth);
	}
	if (dFlags[sx][sy] & BFLAG_PLAYERLR) {
		bv = -(dPlayer[sx][sy - 1] + 1);
		px = dx + plr[bv]._pxoff - plr[bv]._pAnimWidth2;
		py = dy + plr[bv]._pyoff;
		if (bv == pcursplr) {
			Cl2DecodeFrm2(165, px, py, plr[bv]._pAnimData, plr[bv]._pAnimFrame, plr[bv]._pAnimWidth);
		}
		/// ASSERT: assert(plr[bv]._pAnimData);
		Cl2DecodeFrm1(px, py, plr[bv]._pAnimData, plr[bv]._pAnimFrame, plr[bv]._pAnimWidth);
	}
	if (dFlags[sx][sy] & BFLAG_DEAD_PLAYER) {
		DrawDeadPlayer(sx, sy, dx, dy);
	}
	if (dPlayer[sx][sy] > 0) {
		bv = dPlayer[sx][sy] - 1;
		px = dx + plr[bv]._pxoff - plr[bv]._pAnimWidth2;
		py = dy + plr[bv]._pyoff;
		if (bv == pcursplr) {
			Cl2DecodeFrm2(165, px, py, plr[bv]._pAnimData, plr[bv]._pAnimFrame, plr[bv]._pAnimWidth);
		}
		/// ASSERT: assert(plr[bv]._pAnimData);
		Cl2DecodeFrm1(px, py, plr[bv]._pAnimData, plr[bv]._pAnimFrame, plr[bv]._pAnimWidth);
	}
	if (dFlags[sx][sy] & BFLAG_MISSILE) {
		DrawMissile(sx, sy, dx, dy, 0);
	}
	if (bArch != 0) {
		town_special(pBuff, bArch);
	}
}

void SetTownMicros()
{
	int i, x, y, lv;
	WORD *pPiece;
	MICROS *pMap;

	MicroTileLen = 16;

	for (y = 0; y < MAXDUNY; y++) {
		for (x = 0; x < MAXDUNX; x++) {
			lv = (dPiece[x][y]);
			pMap = &dpiece_defs_map_1[IsometricCoord(x, y)];
			if (lv != 0) {
				lv--;
				pPiece = ((WORD *)&pLevelPieces[32 * lv]);
				for (i = 0; i < 16; i++) {
					pMap->mt[i] = (pPiece[(i & 1) + 14 - (i & 0xE)]);
				}
			} else {
				for (i = 0; i < 16; i++) {
					pMap->mt[i] = 0;
				}
			}
		}
	}
}

void T_FillSector(BYTE *P3Tiles, BYTE *pSector, int xi, int yi, int w, int h)
{
	int i, j, xx, yy;
	long v1, v2, v3, v4, ii;

	ii = 4;
	yy = yi;
	for (j = 0; j < h; j++) {
		xx = xi;
		for (i = 0; i < w; i++) {
			WORD *Map;

			Map = (WORD *)&pSector[ii];
			if (BSWAP_INT16_UNSIGNED(*Map)) {
				v1 = BSWAP_INT16_UNSIGNED(*((WORD *)&P3Tiles[(*Map - 1) * 8]) + 1);
				v2 = BSWAP_INT16_UNSIGNED(*((WORD *)&P3Tiles[(*Map - 1) * 8] + 1) + 1);
				v3 = BSWAP_INT16_UNSIGNED(*((WORD *)&P3Tiles[(*Map - 1) * 8] + 2) + 1);
				v4 = BSWAP_INT16_UNSIGNED(*((WORD *)&P3Tiles[(*Map - 1) * 8] + 3) + 1);
			} else {
				v1 = 0;
				v2 = 0;
				v3 = 0;
				v4 = 0;
			}
			dPiece[xx][yy] = v1;
			dPiece[xx + 1][yy] = v2;
			dPiece[xx][yy + 1] = v3;
			dPiece[xx + 1][yy + 1] = v4;
			xx += 2;
			ii += 2;
		}
		yy += 2;
	}
}

void T_FillTile(BYTE *P3Tiles, int xx, int yy, int t)
{
	long v1, v2, v3, v4;

	v1 = *((WORD *)&P3Tiles[(t - 1) * 8]) + 1;
	v2 = *((WORD *)&P3Tiles[(t - 1) * 8] + 1) + 1;
	v3 = *((WORD *)&P3Tiles[(t - 1) * 8] + 2) + 1;
	v4 = *((WORD *)&P3Tiles[(t - 1) * 8] + 3) + 1;

	dPiece[xx][yy] = v1;
	dPiece[xx + 1][yy] = v2;
	dPiece[xx][yy + 1] = v3;
	dPiece[xx + 1][yy + 1] = v4;
}

void T_Pass3()
{
	int xx, yy, x;
	BYTE *P3Tiles, *pSector;

	for (yy = 0; yy < MAXDUNY; yy += 2) {
		for (xx = 0; xx < MAXDUNX; xx += 2) {
			dPiece[xx][yy] = 0;
			dPiece[xx + 1][yy] = 0;
			dPiece[xx][yy + 1] = 0;
			dPiece[xx + 1][yy + 1] = 0;
		}
	}

	P3Tiles = LoadFileInMem("Levels\\TownData\\Town.TIL", NULL);
	pSector = LoadFileInMem("Levels\\TownData\\Sector1s.DUN", NULL);
	T_FillSector(P3Tiles, pSector, 46, 46, 25, 25);
	mem_free_dbg(pSector);
	pSector = LoadFileInMem("Levels\\TownData\\Sector2s.DUN", NULL);
	T_FillSector(P3Tiles, pSector, 46, 0, 25, 23);
	mem_free_dbg(pSector);
	pSector = LoadFileInMem("Levels\\TownData\\Sector3s.DUN", NULL);
	T_FillSector(P3Tiles, pSector, 0, 46, 23, 25);
	mem_free_dbg(pSector);
	pSector = LoadFileInMem("Levels\\TownData\\Sector4s.DUN", NULL);
	T_FillSector(P3Tiles, pSector, 0, 0, 23, 23);
	mem_free_dbg(pSector);

	if (gbMaxPlayers == 1) {
		if (!(plr[myplr].pTownWarps & 1)) {
			T_FillTile(P3Tiles, 48, 20, 320);
		}
		if (!(plr[myplr].pTownWarps & 2)) {
			T_FillTile(P3Tiles, 16, 68, 332);
			T_FillTile(P3Tiles, 16, 70, 331);
		}
		if (!(plr[myplr].pTownWarps & 4)) {
			for (x = 36; x < 46; x++) {
				T_FillTile(P3Tiles, x, 78, random(0, 4) + 1);
			}
		}
	}

	if (quests[13]._qactive != 3 && quests[13]._qactive) {
		T_FillTile(P3Tiles, 60, 70, 342);
	} else {
		T_FillTile(P3Tiles, 60, 70, 71);
	}

	mem_free_dbg(P3Tiles);
}

void CreateTown(int entry)
{
	int x, y;

	dminx = 10;
	dminy = 10;
	dmaxx = 84;
	dmaxy = 84;

	if (entry == 0) {
		ViewX = 75;
		ViewY = 68;
	} else if (entry == 1) {
		ViewX = 25;
		ViewY = 31;
	} else if (entry == 7) {
		if (TWarpFrom == 5) {
			ViewX = 49;
			ViewY = 22;
		}
		if (TWarpFrom == 9) {
			ViewX = 18;
			ViewY = 69;
		}
		if (TWarpFrom == 13) {
			ViewX = 41;
			ViewY = 81;
		}
	}

	T_Pass3();
	memset(dLight, 0, sizeof(dLight));
	memset(dFlags, 0, sizeof(dFlags));
	memset(dPlayer, 0, sizeof(dPlayer));
	memset(dMonster, 0, sizeof(dMonster));
	memset(dObject, 0, sizeof(dObject));
	memset(dItem, 0, sizeof(dItem));
	memset(dArch, 0, sizeof(dArch));

	for (y = 0; y < MAXDUNY; y++) {
		for (x = 0; x < MAXDUNX; x++) {
			if (dPiece[x][y] == 360) {
				dArch[x][y] = 1;
			} else if (dPiece[x][y] == 358) {
				dArch[x][y] = 2;
			} else if (dPiece[x][y] == 129) {
				dArch[x][y] = 6;
			} else if (dPiece[x][y] == 130) {
				dArch[x][y] = 7;
			} else if (dPiece[x][y] == 128) {
				dArch[x][y] = 8;
			} else if (dPiece[x][y] == 117) {
				dArch[x][y] = 9;
			} else if (dPiece[x][y] == 157) {
				dArch[x][y] = 10;
			} else if (dPiece[x][y] == 158) {
				dArch[x][y] = 11;
			} else if (dPiece[x][y] == 156) {
				dArch[x][y] = 12;
			} else if (dPiece[x][y] == 162) {
				dArch[x][y] = 13;
			} else if (dPiece[x][y] == 160) {
				dArch[x][y] = 14;
			} else if (dPiece[x][y] == 214) {
				dArch[x][y] = 15;
			} else if (dPiece[x][y] == 212) {
				dArch[x][y] = 16;
			} else if (dPiece[x][y] == 217) {
				dArch[x][y] = 17;
			} else if (dPiece[x][y] == 216) {
				dArch[x][y] = 18;
			}
		}
	}

	SetTownMicros();
}

DEVILUTION_END_NAMESPACE
