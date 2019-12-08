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

short ac68080_saga = 0;
short ac68080_ammx = 0;

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
		ac68080_saga = 1; //!_ZN3dvl10fullscreenE; // disable if not fullscreen
		
		bufmem = AllocMem(3*FRAME_BUFFER_SZ + 31, MEMF_PUBLIC|MEMF_CLEAR);
		if(!bufmem) ac68080_saga = 0;

		if(!VampireBase) VampireBase = OpenResource( V_VAMPIRENAME );
		if(VampireBase && VampireBase->lib_Version >= 45 && 
		   (V_EnableAMMX( V_AMMX_V2 ) != VRES_ERROR) ) {
		   ac68080_ammx = 1;
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

int vampire_Flip(SDL_Surface *surf) 
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
		
		// if ptr ouside our memory
		if((ULONG)(ptr - bufmem) >= (ULONG)(3*FRAME_BUFFER_SZ+31)) { // ! \\ ULONG trick
			if(surf->flags & SDL_PREALLOC) goto legacy;
			
			ptr = (UBYTE*)(~31&(ULONG)(bufmem + 31));
			CopyMemQuick(surf->pixels, ptr, FRAME_BUFFER_SZ);
			SDL_free(surf->pixels); 
			surf->flags |= SDL_PREALLOC;
			surf->pixels = ptr;
		}

		// display
		*dpy = ptr;
		
		// advance ptr
#if ROLL_PTR
		ptr += FRAME_BUFFER_SZ;
		if(ptr >= bufmem + 3*FRAME_BUFFER_SZ) ptr -= 3*FRAME_BUFFER_SZ;
		surf->pixels = ptr;
#endif
		
		return 0;
	}
	
legacy:
	return SDL_Flip(surf);
}
