#include "diablo.h"
#include "../3rdParty/Storm/Source/storm.h"

DEVILUTION_BEGIN_NAMESPACE

int light_table_index;
int PitchTbl[BUFFER_HEIGHT];
DWORD sgdwCursWdtOld;
DWORD sgdwCursX;
DWORD sgdwCursY;
BYTE *gpBufEnd;
DWORD sgdwCursHgt;
DWORD level_cel_block;
DWORD sgdwCursXOld;
DWORD sgdwCursYOld;
char arch_draw_type;
DDSURFACEDESC DDS_desc;
int cel_transparency_active;
int level_piece_id;
DWORD sgdwCursWdt;
void (*DrawPlrProc)(int, int, int, int, int, BYTE *, int, int, int, int);
BYTE sgSaveBack[8192];
int draw_monster_num;
DWORD sgdwCursHgtOld;

/* data */

/* used in 1.00 debug */
char *szMonModeAssert[18] = {
	"standing",
	"walking (1)",
	"walking (2)",
	"walking (3)",
	"attacking",
	"getting hit",
	"dying",
	"attacking (special)",
	"fading in",
	"fading out",
	"attacking (ranged)",
	"standing (special)",
	"attacking (special ranged)",
	"delaying",
	"charging",
	"stoned",
	"healing",
	"talking"
};

char *szPlrModeAssert[12] = {
	"standing",
	"walking (1)",
	"walking (2)",
	"walking (3)",
	"attacking (melee)",
	"attacking (ranged)",
	"blocking",
	"getting hit",
	"dying",
	"casting a spell",
	"changing levels",
	"quitting"
};

void ClearCursor() // CODE_FIX: this was supposed to be in cursor.cpp
{
	sgdwCursWdt = 0;
	sgdwCursWdtOld = 0;
}

void DrawMissile(int x, int y, int sx, int sy, BOOL pre)
{
	int i, mx, my, nCel;
	MissileStruct *m;
	BYTE *pCelBuff;
	DWORD *pFrameTable;

	if (dMissile[x][y] == -1) {
		for (i = 0; i < nummissiles; i++) {
			/// ASSERT: assert(missileactive[i] < MAXMISSILES);
			if (missileactive[i] >= MAXMISSILES)
				break;
			m = &missile[missileactive[i]];
			if (m->_mix == x && m->_miy == y && m->_miPreFlag == pre && m->_miDrawFlag) {
				pCelBuff = m->_miAnimData;
				if (!pCelBuff) {
					// app_fatal("Draw Missile type %d: NULL Cel Buffer", m->_mitype);
					return;
				}
				nCel = m->_miAnimFrame;
				pFrameTable = (DWORD *)pCelBuff;
				if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
					// app_fatal("Draw Missile: frame %d of %d, missile type==%d", nCel, pFrameTable[0], m->_mitype);
					return;
				}
				mx = sx + m->_mixoff - m->_miAnimWidth2;
				my = sy + m->_miyoff;
				if (m->_miUniqTrans)
					Cl2DecodeFrm3(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth, m->_miUniqTrans + 3);
				else if (m->_miLightFlag)
					Cl2DecodeLightTbl(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth);
				else
					Cl2DecodeFrm1(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth);
			}
		}
	} else {
		m = &missile[dMissile[x][y] - 1];
		if (m->_miPreFlag == pre && m->_miDrawFlag) {
			pCelBuff = m->_miAnimData;
			if (!pCelBuff) {
				// app_fatal("Draw Missile 2 type %d: NULL Cel Buffer", m->_mitype);
				return;
			}
			nCel = m->_miAnimFrame;
			pFrameTable = (DWORD *)pCelBuff;
			if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
				// app_fatal("Draw Missile 2: frame %d of %d, missile type==%d", nCel, pFrameTable[0], m->_mitype);
				return;
			}
			mx = sx + m->_mixoff - m->_miAnimWidth2;
			my = sy + m->_miyoff;
			if (m->_miUniqTrans)
				Cl2DecodeFrm3(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth, m->_miUniqTrans + 3);
			else if (m->_miLightFlag)
				Cl2DecodeLightTbl(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth);
			else
				Cl2DecodeFrm1(mx, my, m->_miAnimData, m->_miAnimFrame, m->_miAnimWidth);
		}
	}
}

void DrawDeadPlayer(int x, int y, int sx, int sy)
{
	int i, px, py, nCel;
	PlayerStruct *p;
	BYTE *pCelBuff;
	DWORD *pFrameTable;

	dFlags[x][y] &= ~BFLAG_DEAD_PLAYER;

	for (i = 0; i < MAX_PLRS; i++) {
		p = &plr[i];
		if (p->plractive && !p->_pHitPoints && p->plrlevel == (BYTE)currlevel && p->WorldX == x && p->WorldY == y) {
			pCelBuff = p->_pAnimData;
			if (!pCelBuff) {
				// app_fatal("Drawing dead player %d \"%s\": NULL Cel Buffer", i, p->_pName);
				break;
			}
			nCel = p->_pAnimFrame;
			pFrameTable = (DWORD *)pCelBuff;
			if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
				// app_fatal("Drawing dead player %d \"%s\": facing %d, frame %d of %d", i, p->_pName, p->_pdir, nCel, pFrameTable[0]);
				break;
			}
			dFlags[x][y] |= BFLAG_DEAD_PLAYER;
			px = sx + p->_pxoff - p->_pAnimWidth2;
			py = sy + p->_pyoff;
			DrawPlayer(i, x, y, px, py, p->_pAnimData, p->_pAnimFrame, p->_pAnimWidth);
		}
	}
}

void DrawPlayer(int pnum, int x, int y, int px, int py, BYTE *pCelBuff, int nCel, int nWidth)
{
	int l;
	DWORD *pFrameTable;

	if (dFlags[x][y] & BFLAG_LIT || plr[myplr]._pInfraFlag || !setlevel && !currlevel) {
		if (!pCelBuff) {
			// app_fatal("Drawing player %d \"%s\": NULL Cel Buffer", pnum, plr[pnum]._pName);
			return;
		}
		pFrameTable = (DWORD *)pCelBuff;
		if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
			/*
			const char *szMode = "unknown action";
			if(plr[pnum]._pmode <= 11)
				szMode = szPlrModeAssert[plr[pnum]._pmode];
			app_fatal(
				"Drawing player %d \"%s\" %s: facing %d, frame %d of %d",
				pnum,
				plr[pnum]._pName,
				szMode,
				plr[pnum]._pdir,
				nCel,
				pFrameTable[0]);
			*/
			return;
		}
		if (pnum == pcursplr)
			Cl2DecodeFrm2(165, px, py, pCelBuff, nCel, nWidth);
		if (pnum == myplr) {
			Cl2DecodeFrm1(px, py, pCelBuff, nCel, nWidth);
			if (plr[pnum].pManaShield)
				Cl2DecodeFrm1(
				    px + plr[pnum]._pAnimWidth2 - misfiledata[MFILE_MANASHLD].mAnimWidth2[0],
				    py,
				    misfiledata[MFILE_MANASHLD].mAnimData[0],
				    1,
				    misfiledata[MFILE_MANASHLD].mAnimWidth[0]);
		} else if (!(dFlags[x][y] & BFLAG_LIT) || plr[myplr]._pInfraFlag && light_table_index > 8) {
			Cl2DecodeFrm3(px, py, pCelBuff, nCel, nWidth, 1);
			if (plr[pnum].pManaShield)
				Cl2DecodeFrm3(
				    px + plr[pnum]._pAnimWidth2 - misfiledata[MFILE_MANASHLD].mAnimWidth2[0],
				    py,
				    misfiledata[MFILE_MANASHLD].mAnimData[0],
				    1,
				    misfiledata[MFILE_MANASHLD].mAnimWidth[0],
				    1);
		} else {
			l = light_table_index;
			if (light_table_index < 5)
				light_table_index = 0;
			else
				light_table_index -= 5;
			Cl2DecodeLightTbl(px, py, pCelBuff, nCel, nWidth);
			if (plr[pnum].pManaShield)
				Cl2DecodeLightTbl(
				    px + plr[pnum]._pAnimWidth2 - misfiledata[MFILE_MANASHLD].mAnimWidth2[0],
				    py,
				    misfiledata[MFILE_MANASHLD].mAnimData[0],
				    1,
				    misfiledata[MFILE_MANASHLD].mAnimWidth[0]);
			light_table_index = l;
		}
	}
}

void DrawUi(int StartX, int StartY)
{
	if (automapflag) {
		DrawAutomap();
	}
	if (stextflag && !qtextflag)
		DrawSText();
	if (invflag) {
		DrawInv();
	} else if (sbookflag) {
		DrawSpellBook();
	}

	DrawDurIcon();

	if (chrflag) {
		DrawChr();
	} else if (questlog) {
		DrawQuestLog();
	} else if (plr[myplr]._pStatPts != 0 && !spselflag) {
		DrawLevelUpIcon();
	}
	if (uitemflag) {
		DrawUniqueInfo();
	}
	if (qtextflag) {
		DrawQText();
	}
	if (spselflag) {
		DrawSpellList();
	}
	if (dropGoldFlag) {
		DrawGoldSplit(dropGoldValue);
	}
	if (helpflag) {
		DrawHelp();
	}
	if (msgflag) {
		DrawDiabloMsg();
	}
	if (deathflag) {
		RedBack();
	} else if (PauseMode != 0) {
		gmenu_draw_pause();
	}

	DrawPlrMsg();
	gmenu_draw();
	doom_draw();
	DrawInfoBox();
	DrawLifeFlask();
	DrawManaFlask();
}

void DrawGame(int x, int y)
{
	int i, sx, sy, chunks, blocks;
	int wdt, nSrcOff, nDstOff;

	sx = ScrollInfo._sxoff + SCREEN_X;
	sy = ScrollInfo._syoff + 175;

	chunks = ceil(SCREEN_WIDTH / 64);
	blocks = ceil(VIEWPORT_HEIGHT / 32) + (MicroTileLen - 6);

	if (!zoomflag) {
		chunks = ceil(chunks / 2);
		blocks = ceil(blocks / 2);
	}

	// center view
	x -= chunks;
	y--;

	chunks++; // eflag
	if (zoomflag)
		chunks++; // eflag

	switch (ScrollInfo._sdir) {
	case SDIR_N:
		sy -= 32;
		x--;
		y--;
		blocks++;
		break;
	case SDIR_NE:
		sy -= 32;
		x--;
		y--;
		chunks++;
		blocks++;
		break;
	case SDIR_E:
		chunks++;
		break;
	case SDIR_SE:
		chunks++;
		blocks++;
		break;
	case SDIR_S:
		blocks++;
		break;
	case SDIR_SW:
		sx -= 64;
		x--;
		y++;
		chunks++;
		blocks++;
		break;
	case SDIR_W:
		sx -= 64;
		x--;
		y++;
		chunks++;
		break;
	case SDIR_NW:
		sx -= 64;
		sy -= 32;
		x -= 2;
		chunks++;
		blocks++;
		break;
	}

	/// ASSERT: assert(gpBuffer);
	gpBufEnd = &gpBuffer[PitchTbl[VIEWPORT_HEIGHT + SCREEN_Y]];
	for (i = 0; i < blocks; i++) {
		scrollrt_draw(x, y, sx, sy, chunks);
		y++;
		sx -= 32;
		sy += 16;
		scrollrt_draw(x, y, sx, sy, chunks);
		x++;
		sx += 32;
		sy += 16;
	}

	if (!zoomflag) {
		if (chrflag || questlog) {
			nSrcOff = SCREENXY(112, 159);
			nDstOff = SCREENXY(320, 350);
			wdt = (SCREEN_WIDTH - 320) / 2;
		} else if (invflag || sbookflag) {
			nSrcOff = SCREENXY(112, 159);
			nDstOff = SCREENXY(0, 350);
			wdt = (SCREEN_WIDTH - 320) / 2;
		} else {
			nSrcOff = SCREENXY(32, 159);
			nDstOff = SCREENXY(0, 350);
			wdt = SCREEN_WIDTH / 2;
		}

		/// ASSERT: assert(gpBuffer);

		int hgt;
		BYTE *src, *dst1, *dst2;

		src = &gpBuffer[nSrcOff];
		dst1 = &gpBuffer[nDstOff];
		dst2 = &gpBuffer[nDstOff + BUFFER_WIDTH];

		for (hgt = ceil(VIEWPORT_HEIGHT / 4); hgt != 0; hgt--, src -= BUFFER_WIDTH + wdt, dst1 -= 2 * (BUFFER_WIDTH + wdt), dst2 -= 2 * (BUFFER_WIDTH + wdt)) {
			for (i = wdt; i != 0; i--) {
				*dst1++ = *src;
				*dst1++ = *src;
				*dst2++ = *src;
				*dst2++ = *src;
				src++;
			}
		}
	}
}

void scrollrt_draw(int x, int y, int sx, int sy, int chunks)
{
	int i, j;
	BYTE *dst;
	MICROS *pMap;

	/// ASSERT: assert(gpBuffer);

	pMap = &dpiece_defs_map_1[IsometricCoord(x, y)];

	for (j = 0; j < chunks; j++) {
		if (y >= 0 && y < MAXDUNY && x >= 0 && x < MAXDUNX) {
			level_piece_id = dPiece[x][y];
			light_table_index = dLight[x][y];
			if (level_piece_id != 0) {
				dst = &gpBuffer[sx + PitchTbl[sy]];
				cel_transparency_active = (BYTE)(nTransTable[level_piece_id] & TransList[dTransVal[x][y]]);
				arch_draw_type = 1;
				level_cel_block = pMap->mt[0];
				if (level_cel_block != 0) {
					RenderTile(dst);
				}
				arch_draw_type = 2;
				level_cel_block = pMap->mt[1];
				if (level_cel_block != 0) {
					RenderTile(dst + 32);
				}
				arch_draw_type = 0;
				for (i = 2; i < MicroTileLen; i += 2) {
					dst -= BUFFER_WIDTH * 32;
					level_cel_block = pMap->mt[i];
					if (level_cel_block != 0) {
						RenderTile(dst);
					}
					level_cel_block = pMap->mt[i + 1];
					if (level_cel_block != 0) {
						RenderTile(dst + 32);
					}
				}
				if (leveltype != DTYPE_TOWN) {
					scrollrt_draw_dungeon(&gpBuffer[sx + PitchTbl[sy]], x, y, sx, sy);
				} else {
					town_draw_town(&gpBuffer[sx + PitchTbl[sy]], x, y, sx, sy);
				}
			}
		}
		x++;
		y--;
		sx += 64;
		pMap++;
	}
}

void scrollrt_draw_dungeon(BYTE *pBuff, int sx, int sy, int dx, int dy)
{
	int px, py, nCel, nMon, negMon, p, tx, ty;
	char bFlag, bDead, bObj, bItem, bPlr, bArch, bMap, negPlr, dd;
	DeadStruct *pDeadGuy;
	ItemStruct *pItem;
	PlayerStruct *pPlayer;
	MonsterStruct *pMonster;
	BYTE *pCelBuff;
	DWORD *pFrameTable;

	/// ASSERT: assert((DWORD)sx < MAXDUNX);
	/// ASSERT: assert((DWORD)sy < MAXDUNY);
	bFlag = dFlags[sx][sy];
	bDead = dDead[sx][sy];
	bObj = dObject[sx][sy];
	bItem = dItem[sx][sy];
	bPlr = dPlayer[sx][sy];
	bArch = dArch[sx][sy];
	bMap = dTransVal[sx][sy];
	nMon = dMonster[sx][sy];

	/// ASSERT: assert((DWORD)(sy-1) < MAXDUNY);
	negPlr = dPlayer[sx][sy - 1];
	negMon = dMonster[sx][sy - 1];

	if (visiondebug && bFlag & BFLAG_LIT) {
		Cel2DecodeHdrOnly(pBuff, pSquareCel, 1, 64);
	}
	tx = dx - 96;
	ty = dy - 16;

	if (MissilePreFlag && bFlag & BFLAG_MISSILE) {
		DrawMissile(sx, sy, dx, dy, 1);
	}
	if (light_table_index < lightmax) {
		if (bDead != 0) {
			pDeadGuy = &dead[(bDead & 0x1F) - 1];
			dd = (bDead >> 5) & 7;
			px = dx - pDeadGuy->_deadWidth2;
			pCelBuff = pDeadGuy->_deadData[dd];
			/// ASSERT: assert(pDeadGuy->_deadData[dd] != NULL);
			if (pCelBuff != NULL) {
				pFrameTable = (DWORD *)pDeadGuy->_deadData[dd];
				nCel = pDeadGuy->_deadFrame;
				if (nCel >= 1 && pFrameTable[0] <= 50 && nCel <= (int)pFrameTable[0]) {
					if (pDeadGuy->_deadtrans != 0) {
						Cl2DecodeFrm5(px, dy, pCelBuff, nCel, pDeadGuy->_deadWidth, pDeadGuy->_deadtrans);
					} else {
						Cl2DecodeFrm6(px, dy, pCelBuff, pDeadGuy->_deadFrame, pDeadGuy->_deadWidth);
					}
				} else {
					// app_fatal("Clipped dead sub: frame %d of %d, deadnum==%d", nCel, pFrameTable[0], (bDead & 0x1F) - 1);
				}
			}
		}
		if (bObj != 0) {
			DrawObject(sx, sy, dx, dy, 1);
		}
	}
	if (bItem != 0) {
		pItem = &item[bItem - 1];
		if (!pItem->_iPostDraw) {
			/// ASSERT: assert((unsigned char)bItem <= MAXITEMS);
			if ((BYTE)bItem <= MAXITEMS) {
				pCelBuff = pItem->_iAnimData;
				if (pCelBuff != NULL) {
					pFrameTable = (DWORD *)pCelBuff;
					nCel = pItem->_iAnimFrame;
					if (nCel >= 1 && pFrameTable[0] <= 50 && nCel <= (int)pFrameTable[0]) {
						px = dx - pItem->_iAnimWidth2;
						if (bItem - 1 == pcursitem) {
							CelDrawHdrClrHL(181, px, dy, pCelBuff, nCel, pItem->_iAnimWidth);
						}
						Cel2DecodeHdrLight(px, dy, pItem->_iAnimData, pItem->_iAnimFrame, pItem->_iAnimWidth);
					} else {
						/*
						app_fatal(
							"Draw Clipped \"%s\" Item: frame %d of %d, item type==%d",
							pItem->_iIName,
							nCel,
							pFrameTable[0],
							pItem->_itype);
						*/
					}
				} else {
					// app_fatal("Draw Item \"%s\" Clipped 1: NULL Cel Buffer", pItem->_iIName);
				}
			}
		}
	}
	if (bFlag & BFLAG_PLAYERLR) {
		p = -(negPlr + 1);
		if ((DWORD)p < MAX_PLRS) {
			pPlayer = &plr[p];
			px = dx + pPlayer->_pxoff - pPlayer->_pAnimWidth2;
			py = dy + pPlayer->_pyoff;
			DrawPlayer(p, sx, sy - 1, px, py, pPlayer->_pAnimData, pPlayer->_pAnimFrame, pPlayer->_pAnimWidth);
		} else {
			// app_fatal("draw player clipped: tried to draw illegal player %d", p);
		}
	}
	if (bFlag & BFLAG_MONSTLR && (bFlag & BFLAG_LIT || plr[myplr]._pInfraFlag) && negMon < 0) {
		draw_monster_num = -(negMon + 1);
		if ((DWORD)draw_monster_num < MAXMONSTERS) {
			pMonster = &monster[draw_monster_num];
			if (!(pMonster->_mFlags & 1)) {
				if (pMonster->MType != NULL) {
					px = dx + pMonster->_mxoff - pMonster->MType->width2;
					py = dy + pMonster->_myoff;
					if (draw_monster_num == pcursmonst) {
						Cl2DecodeClrHL(233, px, py, pMonster->_mAnimData, pMonster->_mAnimFrame, pMonster->MType->width);
					}
					DrawMonster(sx, sy, px, py, draw_monster_num);
				} else {
					// app_fatal("Draw Monster \"%s\" Clipped: uninitialized monster", pMonster->mName);
				}
			}
		} else {
			// app_fatal("Draw Monster Clipped: tried to draw illegal monster %d", draw_monster_num);
		}
	}
	if (bFlag & BFLAG_DEAD_PLAYER) {
		DrawDeadPlayer(sx, sy, dx, dy);
	}
	if (bPlr > 0) {
		p = bPlr - 1;
		if ((DWORD)p < MAX_PLRS) {
			pPlayer = &plr[p];
			px = dx + pPlayer->_pxoff - pPlayer->_pAnimWidth2;
			py = dy + pPlayer->_pyoff;
			DrawPlayer(p, sx, sy, px, py, pPlayer->_pAnimData, pPlayer->_pAnimFrame, pPlayer->_pAnimWidth);
		} else {
			// app_fatal("draw player clipped: tried to draw illegal player %d", p);
		}
	}
	if (nMon > 0 && (bFlag & BFLAG_LIT || plr[myplr]._pInfraFlag)) {
		draw_monster_num = nMon - 1;
		if ((DWORD)draw_monster_num < MAXMONSTERS) {
			pMonster = &monster[draw_monster_num];
			if (!(pMonster->_mFlags & 1)) {
				if (pMonster->MType != NULL) {
					px = dx + pMonster->_mxoff - pMonster->MType->width2;
					py = dy + pMonster->_myoff;
					if (draw_monster_num == pcursmonst) {
						Cl2DecodeClrHL(233, px, py, pMonster->_mAnimData, pMonster->_mAnimFrame, pMonster->MType->width);
					}
					DrawMonster(sx, sy, px, py, draw_monster_num);
				} else {
					// app_fatal("Draw Monster \"%s\" Clipped: uninitialized monster", pMonster->mName);
				}
			}
		} else {
			// app_fatal("Draw Monster Clipped: tried to draw illegal monster %d", draw_monster_num);
		}
	}
	if (bFlag & BFLAG_MISSILE) {
		DrawMissile(sx, sy, dx, dy, 0);
	}
	if (bObj != 0 && light_table_index < lightmax) {
		DrawObject(sx, sy, dx, dy, 0);
	}
	if (bItem != 0) {
		pItem = &item[bItem - 1];
		if (pItem->_iPostDraw) {
			/// ASSERT: assert((unsigned char)bItem <= MAXITEMS);
			if ((BYTE)bItem <= MAXITEMS) {
				pCelBuff = pItem->_iAnimData;
				if (pCelBuff != NULL) {
					pFrameTable = (DWORD *)pCelBuff;
					nCel = pItem->_iAnimFrame;
					if (nCel >= 1 && pFrameTable[0] <= 50 && nCel <= (int)pFrameTable[0]) {
						px = dx - pItem->_iAnimWidth2;
						if (bItem - 1 == pcursitem) {
							CelDrawHdrClrHL(181, px, dy, pCelBuff, nCel, pItem->_iAnimWidth);
						}
						Cel2DecodeHdrLight(px, dy, pItem->_iAnimData, pItem->_iAnimFrame, pItem->_iAnimWidth);
					} else {
						/*
						app_fatal(
							"Draw Clipped \"%s\" Item 2: frame %d of %d, item type==%d",
							pItem->_iIName,
							nCel,
							pFrameTable[0],
							pItem->_itype);
						*/
					}
				} else {
					// app_fatal("Draw Item \"%s\" Clipped 2: NULL Cel Buffer", pItem->_iIName);
				}
			}
		}
	}
	if (bArch != 0) {
		cel_transparency_active = (BYTE)TransList[bMap];
		Cel2DecodeLightTrans(pBuff, pSpecialCels, bArch, 64);
	}
}

void DrawMonster(int x, int y, int mx, int my, int m)
{
	int nCel;
	char trans;
	BYTE *pCelBuff;
	DWORD *pFrameTable;

	if ((DWORD)m >= MAXMONSTERS) {
		// app_fatal("Draw Monster: tried to draw illegal monster %d", m);
		return;
	}

	pCelBuff = monster[m]._mAnimData;
	if (!pCelBuff) {
		// app_fatal("Draw Monster \"%s\": NULL Cel Buffer", monster[m].mName);
		return;
	}

	nCel = monster[m]._mAnimFrame;
	pFrameTable = (DWORD *)pCelBuff;
	if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
		/*
		const char *szMode = "unknown action";
		if(monster[m]._mmode <= 17)
			szMode = szMonModeAssert[monster[m]._mmode];
		app_fatal(
			"Draw Monster \"%s\" %s: facing %d, frame %d of %d",
			monster[m].mName,
			szMode,
			monster[m]._mdir,
			nCel,
			pFrameTable[0]);
		*/
		return;
	}

	if (!(dFlags[x][y] & BFLAG_LIT)) {
		Cl2DecodeFrm3(mx, my, monster[m]._mAnimData, monster[m]._mAnimFrame, monster[m].MType->width, 1);
	} else {
		trans = 0;
		if (monster[m]._uniqtype)
			trans = monster[m]._uniqtrans + 4;
		if (monster[m]._mmode == MM_STONE)
			trans = 2;
		if (plr[myplr]._pInfraFlag && light_table_index > 8)
			trans = 1;
		if (trans)
			Cl2DecodeFrm3(mx, my, monster[m]._mAnimData, monster[m]._mAnimFrame, monster[m].MType->width, trans);
		else
			Cl2DecodeLightTbl(mx, my, monster[m]._mAnimData, monster[m]._mAnimFrame, monster[m].MType->width);
	}
}

void DrawObject(int x, int y, int ox, int oy, BOOL pre)
{
	int sx, sy, xx, yy, nCel;
	char bv;
	BYTE *pCelBuff;
	DWORD *pFrameTable;

	if (dObject[x][y] > 0) {
		bv = dObject[x][y] - 1;
		if (object[bv]._oPreFlag != pre)
			return;
		sx = ox - object[bv]._oAnimWidth2;
		sy = oy;
	} else {
		bv = -(dObject[x][y] + 1);
		if (object[bv]._oPreFlag != pre)
			return;
		xx = object[bv]._ox - x;
		yy = object[bv]._oy - y;
		sx = (xx << 5) + ox - object[bv]._oAnimWidth2 - (yy << 5);
		sy = oy + (yy << 4) + (xx << 4);
	}

	/// ASSERT: assert((unsigned char)bv < MAXOBJECTS);
	if ((BYTE)bv >= MAXOBJECTS)
		return;

	pCelBuff = object[bv]._oAnimData;
	if (!pCelBuff) {
		// app_fatal("Draw Object type %d: NULL Cel Buffer", object[bv]._otype);
		return;
	}

	nCel = object[bv]._oAnimFrame;
	pFrameTable = (DWORD *)pCelBuff;
	if (nCel < 1 || pFrameTable[0] > 50 || nCel > (int)pFrameTable[0]) {
		// app_fatal("Draw Object: frame %d of %d, object type==%d", nCel, pFrameTable[0], object[bv]._otype);
		return;
	}

	if (bv == pcursobj)
		CelDecodeClr(194, sx, sy, object[bv]._oAnimData, object[bv]._oAnimFrame, object[bv]._oAnimWidth);
	if (object[bv]._oLight) {
		CelDecodeHdrLightOnly(sx, sy, object[bv]._oAnimData, object[bv]._oAnimFrame, object[bv]._oAnimWidth);
	} else {
		/// ASSERT: assert(object[bv]._oAnimData);
		if (object[bv]._oAnimData) // BUGFIX: _oAnimData was already checked, this is redundant
			CelDrawHdrOnly(sx, sy, object[bv]._oAnimData, object[bv]._oAnimFrame, object[bv]._oAnimWidth);
	}
}

void ClearScreenBuffer()
{
	lock_buf(3);

	/// ASSERT: assert(gpBuffer);

	int i;
	BYTE *dst;

	dst = &gpBuffer[SCREENXY(0, 0)];

	for (i = 0; i < SCREEN_HEIGHT; i++, dst += BUFFER_WIDTH) {
		memset(dst, 0, SCREEN_WIDTH);
	}

	unlock_buf(3);
}

#ifdef _DEBUG
void ScrollView()
{
	BOOL scroll;

	if (pcurs >= CURSOR_FIRSTITEM)
		return;

	scroll = FALSE;

	if (MouseX < 20) {
		if (dmaxy - 1 <= ViewY || dminx >= ViewX) {
			if (dmaxy - 1 > ViewY) {
				ViewY++;
				scroll = TRUE;
			}
			if (dminx < ViewX) {
				ViewX--;
				scroll = TRUE;
			}
		} else {
			ViewY++;
			ViewX--;
			scroll = TRUE;
		}
	}
	if (MouseX > SCREEN_WIDTH - 20) {
		if (dmaxx - 1 <= ViewX || dminy >= ViewY) {
			if (dmaxx - 1 > ViewX) {
				ViewX++;
				scroll = TRUE;
			}
			if (dminy < ViewY) {
				ViewY--;
				scroll = TRUE;
			}
		} else {
			ViewY--;
			ViewX++;
			scroll = TRUE;
		}
	}
	if (MouseY < 20) {
		if (dminy >= ViewY || dminx >= ViewX) {
			if (dminy < ViewY) {
				ViewY--;
				scroll = TRUE;
			}
			if (dminx < ViewX) {
				ViewX--;
				scroll = TRUE;
			}
		} else {
			ViewX--;
			ViewY--;
			scroll = TRUE;
		}
	}
	if (MouseY > SCREEN_HEIGHT - 20) {
		if (dmaxy - 1 <= ViewY || dmaxx - 1 <= ViewX) {
			if (dmaxy - 1 > ViewY) {
				ViewY++;
				scroll = TRUE;
			}
			if (dmaxx - 1 > ViewX) {
				ViewX++;
				scroll = TRUE;
			}
		} else {
			ViewX++;
			ViewY++;
			scroll = TRUE;
		}
	}

	if (scroll)
		ScrollInfo._sdir = SDIR_NONE;
}

void EnableFrameCount()
{
	frameflag = frameflag == 0;
	framestart = GetTickCount();
}
#endif

void scrollrt_draw_game_screen(BOOL draw_cursor)
{
	if (draw_cursor) {
		lock_buf(0);
		scrollrt_draw_cursor_item();
		unlock_buf(0);
	}

	DrawMain(SCREEN_HEIGHT);
}

void scrollrt_draw_cursor_item()
{
	int i, mx, my, col;
	BYTE *src, *dst;

	/// ASSERT: assert(! sgdwCursWdt);

	if (pcurs <= 0 || cursW == 0 || cursH == 0) {
		return;
	}

	mx = MouseX - 1;
	if (mx < 0) {
		mx = 0;
	} else if (mx > SCREEN_WIDTH - 1) {
		return;
	}
	my = MouseY - 1;
	if (my < 0) {
		my = 0;
	} else if (my > SCREEN_HEIGHT - 1) {
		return;
	}

	mx++;
	my++;
	gpBufEnd = &gpBuffer[PitchTbl[SCREEN_HEIGHT + SCREEN_Y] - cursW - 2];

	if (pcurs >= CURSOR_FIRSTITEM) {
		col = PAL16_YELLOW + 5;
		if (plr[myplr].HoldItem._iMagical != 0) {
			col = PAL16_BLUE + 5;
		}
		if (!plr[myplr].HoldItem._iStatFlag) {
			col = PAL16_RED + 5;
		}
		CelDrawHdrClrHL(col, mx + SCREEN_X, my + cursH + SCREEN_Y - 1, pCursCels, pcurs, cursW);
		if (col != PAL16_RED + 5) {
			Cel2DrawHdrOnly(mx + SCREEN_X, my + cursH + SCREEN_Y - 1, pCursCels, pcurs, cursW);
		} else {
			Cel2DrawHdrLightRed(mx + SCREEN_X, my + cursH + SCREEN_Y - 1, pCursCels, pcurs, cursW, 1);
		}
	} else {
		Cel2DrawHdrOnly(mx + SCREEN_X, my + cursH + SCREEN_Y - 1, pCursCels, pcurs, cursW);
	}
}

void DrawMain(int dwHgt)
{
	int ysize;
	DWORD dwTicks;
	BOOL retry;
	HRESULT hDDVal;

	ysize = dwHgt;

	if (!gbActive || lpDDSPrimary == NULL) {
		return;
	}


	/// ASSERT: assert(ysize >= 0 && ysize <= 480); // SCREEN_HEIGHT

	if (ysize > 0) {
		DoBlitScreen(0, 0, SCREEN_WIDTH, ysize);
	}

#ifdef _DEBUG
	DrawFPS();
#endif
}

#ifdef _DEBUG
void DrawFPS()
{
	DWORD tc, frames;
	char String[12];
	HDC hdc;

	if (frameflag && gbActive) {
		frameend++;
		tc = GetTickCount();
		frames = tc - framestart;
		if (tc - framestart >= 1000) {
			framestart = tc;
			framerate = 1000 * frameend / frames;
			frameend = 0;
		}
		if (framerate > 99)
			framerate = 99;
		wsprintf(String, "%2d", framerate);
		if (!lpDDSPrimary->GetDC(&hdc)) {
			TextOut(hdc, 0, 400, String, strlen(String));
			lpDDSPrimary->ReleaseDC(hdc);
		}
	}
}
#endif

void DoBlitScreen(DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt)
{
	int nSrcOff, nDstOff, nSrcWdt, nDstWdt;
	DWORD dwTicks;
	HRESULT hDDVal;
	RECT SrcRect;

	/// ASSERT: assert(! (dwX & 3));
	/// ASSERT: assert(! (dwWdt & 3));

	if (lpDDSBackBuf != NULL) {
		SrcRect.left = dwX + SCREEN_X;
		SrcRect.top = dwY + SCREEN_Y;
		SrcRect.right = SrcRect.left + dwWdt - 1;
		SrcRect.bottom = SrcRect.top + dwHgt - 1;
		/// ASSERT: assert(! gpBuffer);
		dwTicks = GetTickCount();
		while (1) {
			hDDVal = lpDDSPrimary->BltFast(dwX, dwY, lpDDSBackBuf, &SrcRect, DDBLTFAST_WAIT);
			if (hDDVal == DD_OK) {
				break;
			}
			if (dwTicks - GetTickCount() > 5000) {
				break;
			}
			Sleep(1);
			if (hDDVal == DDERR_SURFACELOST) {
				return;
			}
			if (hDDVal != DDERR_WASSTILLDRAWING && hDDVal != DDERR_SURFACEBUSY) {
				break;
			}
		}
		if (hDDVal != DDERR_SURFACELOST
		    && hDDVal != DDERR_WASSTILLDRAWING
		    && hDDVal != DDERR_SURFACEBUSY
		    && hDDVal != DD_OK) {
			DD_ERR_MSG(hDDVal);
		}
	}
}

void DrawAndBlit()
{
	if (!gbRunGame) {
		return;
	}

	lock_buf(0);
	ClearScreenBuffer();
	DrawGame(ViewX, ViewY);
	DrawUi(ViewX, ViewY);

	ClearCtrlPan();
	UpdateLifeFlask();
	UpdateManaFlask();
	DrawCtrlPan();
	DrawInvBelt();
	DrawTalkPan();
	scrollrt_draw_cursor_item();
	unlock_buf(0);

	DrawMain(SCREEN_HEIGHT);
}

DEVILUTION_END_NAMESPACE
