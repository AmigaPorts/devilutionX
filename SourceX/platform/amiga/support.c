/*
 * Various support functions or the amiga
 */

//#include <malloc.h>
#include <time.h>

#include <dos/dos.h>
#include <exec/exec.h>
#include <vampire/vampire.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/vampire.h>

#include "../../../../defs.h"
#define FRAME_BUFFER_SZ     ((SCREEN_WIDTH)*(SCREEN_HEIGHT))

#define DIRTY               0       // 1 = 32 fps     0 = 29fps
#define DIRTY               0       // 1 = 32 fps     0 = 29fps

#define CHECK_FIRSTSCREEN   1       // costs 0 fps
#define ROLL_PTR            1

#include <SDL.h>

extern int _ZN3dvl10fullscreenE; // diablo.h
// extern void *_ZN3dvl6windowE;

#define pal_palette_version _ZN3dvl27pal_surface_palette_versionE
extern int pal_palette_version;

#define pal_palette _ZN3dvl7paletteE
extern SDL_Palette *pal_palette;

#define pal_surface _ZN3dvl11pal_surfaceE
extern SDL_Surface *pal_surface;

// #define gbRunGame _ZN3dvl9gbRunGameE
// extern int gbRunGame;

// #define gbRunGameResult _ZN3dvl15gbRunGameResultE
// extern int gbRunGameResult;

// #define MainMenuResult __ZN3dvl16gbProcessPlayersE

// #define PressEscKey _ZN3dvl11PressEscKeyEv
// extern int PressEscKey(void);

#define gamemenu_quit_game	_ZN3dvl18gamemenu_quit_gameEi
extern void gamemenu_quit_game(int);

#define mainmenu_restart_repintro _ZN3dvl25mainmenu_restart_repintroEv
extern void mainmenu_restart_repintro(void);

// #define mainmenu_Esc _ZN3dvl12mainmenu_EscEv
// extern void mainmenu_Esc(void);

// #define sgdwCursYOld _ZN3dvl12sgdwCursYOldE
// extern LONG sgdwCursYOld;

// #define sgdwCursHgtOld _ZN3dvl14sgdwCursHgtOldE
// extern LONG sgdwCursHgtOld;

// #define sgdwCursY _ZN3dvl9sgdwCursYE
// extern LONG sgdwCursY;

// #define sgdwCursHgt _ZN3dvl11sgdwCursHgtE
// extern LONG sgdwCursHgt;

UBYTE ac68080_saga = 0;
UBYTE ac68080_ammx = 0;
static USHORT copy_pane_mask = 0;

static UBYTE *bufmem = NULL;
static UBYTE started  = 0;
static struct Screen *game_screen;
static SDL_Surface   *saga_surface;

struct Library *VampireBase;
extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase;

/*****************************************************************************/
/* malloc replacement */

#define USE_DL_PREFIX

#define SANITY_CHK			0

#define lower_malloc		malloc
#define	lower_free			free

#define HAVE_MORECORE 		0

#define HAVE_MMAP			1
#define HAVE_MUNMAP			1
#define MMAP_CLEARS			0
#define HAVE_MREMAP 		0
#define LACKS_SYS_MMAN_H

#define MMAP				my_mmap
#define MUNMAP				my_munmap	
#define DIRECT_MMAP			MMAP

static void* MMAP(size_t len)
{
	void *p  = lower_malloc(len+4); // +1 to avoid contiguous
#if SANITY_CHK
	if(p) {
		ULONG *q = p;
		*q = q;
		p = ++q;
	}
	printf("MMAP(%d) = %p\n", len, p);
#endif
	return p;
}

static int MUNMAP(void *p, size_t len)
{
#if SANITY_CHK
	printf("MUNMAP(%p, %d)\n", p, len);
	if(p) {
		ULONG *q = p; --q;
		if(*q == q) p = q;
		else {
			errno = EINVAL;
			printf("Not MMAP!\n");
			return -1;
		}
	}
#endif
	lower_free(p);
	return 0;
}

#include "malloc.c"

/*****************************************************************************/

static void stop(void)
{
    if(saga_surface) {
        SDL_FreeSurface(saga_surface);
        saga_surface = NULL;
    }
    if(bufmem) {
		dlfree(bufmem);
        bufmem = NULL;
    }
}
static void start(void)
{
    started = 255;
    atexit(stop);

    if (SysBase->AttnFlags &(1 << 10)) {
        ac68080_saga = 255; //!_ZN3dvl10fullscreenE; // disable if not fullscreen

        bufmem = dlmemalign(32/* byte alignment for saga */, 3*FRAME_BUFFER_SZ);
		if(bufmem) {
			saga_surface = SDL_CreateRGBSurfaceFrom(
				bufmem,
				SCREEN_WIDTH, SCREEN_HEIGHT, 8, SCREEN_WIDTH,
				0, 0, 0, 0
			);
			if(!saga_surface) {
				dlfree(bufmem);
				bufmem = NULL;
			}
		}

        if(!bufmem) ac68080_saga = 0;

        if(!VampireBase) VampireBase = OpenResource( V_VAMPIRENAME );
        if(VampireBase && VampireBase->lib_Version >= 45 &&
           (V_EnableAMMX( V_AMMX_V2 ) != VRES_ERROR) ) {
           ac68080_ammx = 255;
        }

        printf("Vampire accelerator detected");
        if(ac68080_ammx || ac68080_saga) {
            printf(". Using");
            if(ac68080_saga) {
                printf(" SAGA Direct Draw");
            }
            if(ac68080_ammx) {
                if(ac68080_saga) printf(" &");
                printf(" AMMX2");
            }
        }
        printf(".\n");
    }
}

static void chkSignals(void)
{
	static UBYTE closing;
	ULONG signal = SetSignal(0,0);
	if(closing) {
		SDL_Event sdlevent;
		sdlevent.type = SDL_KEYDOWN;
		sdlevent.key.keysym.sym = SDLK_ESCAPE;
		SDL_PushEvent(&sdlevent);
		sdlevent.type = SDL_KEYUP;
		sdlevent.key.keysym.sym = SDLK_ESCAPE;
		SDL_PushEvent(&sdlevent);
	}
	if(signal & SIGBREAKF_CTRL_E) {
		time_t t;
		SetSignal(0, SIGBREAKF_CTRL_E);

		t = time(0);
		printf("\nMemory statistics on %s", ctime(&t));
		dlmalloc_stats();
		printf("\n");
	}
	if(signal & SIGBREAKF_CTRL_C) {
		SetSignal(0, SIGBREAKF_CTRL_C);

		printf("Ctrl-C received\n");
		gamemenu_quit_game(0);
		closing = 255;
	}
}

static int ok(SDL_Surface *const surf)
{
    if(!started) start();
    if(!ac68080_saga) return 0;
    if(surf!=SDL_GetVideoSurface()) return 0;
    if(surf->w != SCREEN_WIDTH || surf->h != SCREEN_HEIGHT) return 0;
    return 1;
}

int vampire_Flip(SDL_Surface* surf)
{
    volatile UBYTE **dpy = (UBYTE**)0xDFF1EC; /* Frame buffer address */
//  volatile ULONG *pal = (ULONG*)0xDFF400;
    struct Screen *first_screen;
	static UBYTE panel_cpy_flag = 4;

	chkSignals();
	
#if DIRTY
    *dpy = (void*)(~31&(int)surf->pixels);
    return;
#endif

    if(!ok(surf)) goto legacy;

    surf = saga_surface;

#if CHECK_FIRSTSCREEN
    // check if screen has changed
    if(game_screen != (first_screen = IntuitionBase->FirstScreen)
    && first_screen->Height == SCREEN_HEIGHT
    && first_screen->Width  == SCREEN_WIDTH)
        game_screen = first_screen;

    // if we are running on the game scree
    if(first_screen == game_screen)
#endif
    {
        UBYTE *ptr = surf->pixels, *old;

        // display
        *dpy = ptr;

#if ROLL_PTR
        old = ptr;
        // advance ptr
        if(ptr >= bufmem + 2*FRAME_BUFFER_SZ)
            ptr -= 2*FRAME_BUFFER_SZ;
        else
            ptr += FRAME_BUFFER_SZ;
        surf->pixels = ptr;
		
        if(copy_pane_mask) {
			copy_pane_mask >>= 1;
			memcpy(ptr + PANEL_TOP*SCREEN_WIDTH, 
		           old + PANEL_TOP*SCREEN_WIDTH, 
				   PANEL_HEIGHT*SCREEN_WIDTH);
		}
#endif
    }
    return 0;
legacy:
    return SDL_Flip(surf);
}

int vampire_BlitSurface(SDL_Surface *src, SDL_Rect *srcRect,
                        SDL_Surface *dst, SDL_Rect *dstRect)
{
    if(ok(dst)) {
        static int last_version;
        // if(srcRect==NULL || srcRect->w==SCREEN_HEIGHT) {
            // /*resync*/
            // int ret = SDL_BlitSurface(src, srcRect, dst, dstRect);
            // UBYTE *ptr = (UBYTE*)(~31&(31+(ULONG)bufmem));
            // memcpy(ptr, dst->pixels, FRAME_BUFFER_SZ); ptr += FRAME_BUFFER_SZ;
            // memcpy(ptr, dst->pixels, FRAME_BUFFER_SZ); ptr += FRAME_BUFFER_SZ;
            // memcpy(ptr, dst->pixels, FRAME_BUFFER_SZ); ptr += FRAME_BUFFER_SZ;
            // return ret;
        // }
		if(srcRect 
		&& srcRect->w < SCREEN_WIDTH			// ignore full screen
		&& !(srcRect->w==288 && srcRect->h==60) // ignore descpane
		&& srcRect->y + srcRect->h >= PANEL_TOP + SCREEN_Y
		)	copy_pane_mask = 4; // we need to copy 3 times the panel if something was drawn there
        if(last_version!=pal_palette_version) {
            last_version = pal_palette_version;
            SDL_SetColors(saga_surface, pal_palette->colors, 0, pal_palette->ncolors);
        }
        return SDL_BlitSurface(src, srcRect, saga_surface, dstRect);
    }
    return SDL_BlitSurface(src, srcRect, dst, dstRect);
}

#define min(a,b) ((a)<=(b)?(a):(b))

int zzzvampire_BlitSurface(SDL_Surface *src, SDL_Rect *srcRect,
                        SDL_Surface *dst, SDL_Rect *dstRect)
{
    register UBYTE *s, *d;
    UWORD  w;
    WORD   h;

    if(!ok(dst)) goto legacy;

    // replace sdl video output by our own
    dst = saga_surface;

    if(!srcRect) {
        static SDL_Rect r;
        r.w = src->w;
        r.h = src->h;
        srcRect = &r;
    }
    if(!dstRect) {
        static SDL_Rect r;
        r.w = dst->w;
        r.h = dst->h;
        dstRect = &r;
    }

    s = src->pixels + srcRect->x + srcRect->y*src->pitch;
    d = dst->pixels + dstRect->x + dstRect->y*dst->pitch;

    w = min(srcRect->w, dstRect->w);
    h = min(srcRect->h, dstRect->h);

    w = min(w, dst->w - dstRect->x);
    h = min(h, dst->h - dstRect->y);

    // printf("Blit %d %d (%d %d) (%d %d)\n", w,h, srcRect->x, srcRect->y, dstRect->x, dstRect->y);

    if(w == src->pitch && w == dst->pitch) {
        memcpy(d, s, w*(UWORD)h);
    } else for(; 1 + --h;)  {
        memcpy(d, s, w);
        s += src->pitch;
        d += dst->pitch;
    }
    return 0;
legacy:
    // if(srcRect && dstRect)
    // return SDL_LowerBlit(src, srcRect, dst, dstRect);
    return SDL_BlitSurface(src, srcRect, dst, dstRect);
}
