/*
 * Various support functions or the amiga
 */
 
//#include <malloc.h>
 
#include <dos/dos.h>
#include <exec/exec.h>
#include <vampire/vampire.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/vampire.h>

#include "../../../../defs.h"
#define FRAME_BUFFER_SZ  	((SCREEN_WIDTH)*(SCREEN_HEIGHT))

#define DIRTY               0		// 1 = 32 fps     0 = 29fps

#define CHECK_FIRSTSCREEN	1		// costs 0 fps
#define CHECK_SURFACE       0
#define ROLL_PTR            0

#include <SDL.h>

extern int _ZN3dvl10fullscreenE; // diablo.h
// extern int _ZN3dvl27pal_surface_palette_versionE;
// extern void *_ZN3dvl6windowE;
// extern SDL_Palette *_ZN3dvl7paletteE;

UBYTE ac68080_saga = 0;
UBYTE ac68080_ammx = 0;

static UBYTE *bufmem = NULL;
static UBYTE started  = 0;
static struct Screen *game_screen;
static struct SDL_Surface *game_surface;

struct Library *VampireBase;
extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase;

static void stop(void)
{
	if(bufmem) {
		FreeMem(bufmem, 3*FRAME_BUFFER_SZ + 31);
		bufmem = NULL;
	}
}
static void start(void)
{
	started = 255;	
	atexit(stop);

	if (SysBase->AttnFlags &(1 << 10)) {
		ac68080_saga = 255; //!_ZN3dvl10fullscreenE; // disable if not fullscreen
		
		bufmem = AllocMem(3*FRAME_BUFFER_SZ + 31, MEMF_PUBLIC|MEMF_CLEAR);
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

int vampire_Flip(SDL_Surface* const surf) 
{
	volatile UBYTE **dpy = (UBYTE**)0xDFF1EC; /* Frame buffer address */
//	volatile ULONG *pal = (ULONG*)0xDFF400;
	struct Screen *first_screen;
	
	if(!started) start();
		
#if DIRTY
	*dpy = (void*)(~31&(int)surf->pixels); 
	return;
#endif
	
	// check if saga is on or if surface is the game surface
	if(!ac68080_ammx) goto legacy;

#if CHECK_SURFACE
	if(surf != game_surface && surf->h == SCREEN_HEIGHT && surf->pitch<=SCREEN_HEIGHT)
		game_surface = surf;
	if(surf != game_surface) goto legacy;
#endif

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
		UBYTE *ptr = surf->pixels;
		
		// if ptr ouside our memory, then use out aligned one (this is done only once per game surface)
		if((ULONG)(ptr - bufmem) >= (ULONG)(3*FRAME_BUFFER_SZ+31)) { // ! \\ ULONG trick
			// if not a std surface, do nothing
			if(surf->flags & SDL_PREALLOC) goto legacy;
			
			// aligned memory
			ptr = (UBYTE*)(~31&(ULONG)(bufmem + 31));
			
			// sync aligned memory content with current surface
			CopyMemQuick(surf->pixels, ptr, FRAME_BUFFER_SZ);
			
			// replace surface pixels by our aliged memory
			SDL_free(surf->pixels); 
			surf->flags |= SDL_PREALLOC; // <== tell SDL not to bother with theses pixels
			surf->pixels = ptr;
		}

		// display
		*dpy = ptr;
		
#if ROLL_PTR
		// advance ptr
		ptr += FRAME_BUFFER_SZ;
		if(ptr >= bufmem + 3*FRAME_BUFFER_SZ) ptr -= 3*FRAME_BUFFER_SZ;
		surf->pixels = ptr;
#endif
		
		return 0;
	}
	
legacy:
	return SDL_Flip(surf);
}

#define min(a,b) ((a)<=(b)?(a):(b))

int vampire_BlitSurface(SDL_Surface *src, SDL_Rect *srcRect,
						   SDL_Surface *dst, SDL_Rect *dstRect) 
{
	register UBYTE *s, *d;
	ULONG  w;
	WORD   h;
	
	// if(dst!=SDL_GetVideoSurface()) 
		// return __real_SDL_BlitSurface(src, srcRect, dst, dstRect);
	
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
	
	if(w == src->pitch && w == dst->pitch) {
		memcpy(d, s, w*(UWORD)h);
		return 0;
	} 
	
	for(; 1 + --h;)  {
		memcpy(d, s, w);
		s += src->pitch;
		d += dst->pitch;
	}
	return 0;
}
