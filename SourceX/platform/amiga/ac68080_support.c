/*
 * Various support functions or the amiga
 */

//#include <malloc.h>
#include <time.h>

#include <dos/dos.h>
#include <exec/exec.h>
#include <vampire/vampire.h>
#include <intuition/intuitionbase.h>
#include <cybergraphics/cybergraphics.h>

#include <proto/exec.h>
#include <proto/vampire.h>
#include <proto/cybergraphics.h>


#include "../../../../defs.h"

#define FRAME_BUFFER_SZ     ((BUFFER_WIDTH)*(BUFFER_HEIGHT))

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

#define gamemenu_quit_game  _ZN3dvl18gamemenu_quit_gameEi
extern void gamemenu_quit_game(int);

// #define mainmenu_restart_repintro _ZN3dvl25mainmenu_restart_repintroEv
// extern void mainmenu_restart_repintro(void);

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

static USHORT copy_previous = 0;

static UBYTE *bufmem = NULL, *bufmem_roll;
static UBYTE started  = 0, last_was_pal, last_was_other, copy_all;
static struct Screen *game_screen;
static struct View *view;

struct Library *VampireBase;
extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase;

/*****************************************************************************/
/* stack requirements */

#define MINSTACK (128*1024)         /* 128kb */
#ifdef __SASC
__near                              /* sas/c */
#endif
size_t __stack = MINSTACK;          /* ixemul, vbcc */

/*****************************************************************************/
/* malloc replacement */

#define SANITY_CHK          0
#define REPLACE_SYS_MALLOC  1

#define USE_DL_PREFIX

#define lower_malloc        __real_malloc
#define lower_free          __real_free
extern void *lower_malloc(size_t);
extern void  lower_free(void *);

#define HAVE_MORECORE       0

#define HAVE_MMAP           1
#define HAVE_MUNMAP         1
#define MMAP_CLEARS         0
#define HAVE_MREMAP         0
#define LACKS_SYS_MMAN_H

#define MMAP                my_mmap
#define MUNMAP              my_munmap
#define DIRECT_MMAP         MMAP

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
    if(!p) {
        printf("Out of memory (%d bytes requested).\n", len+4);
        dlmalloc_stats();
    }
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

void *__wrap_malloc(size_t size)
{
#if REPLACE_SYS_MALLOC
    return dlmalloc(size);
#else
    return __real_malloc(size);
#endif
}
void __wrap_free(void *ptr)
{
#if REPLACE_SYS_MALLOC
    dlfree(ptr);
#else
    __real_free(ptr);
#endif
}

void *__wrap_realloc(void *ptr, size_t size)
{
#if REPLACE_SYS_MALLOC
    return dlrealloc(ptr, size);
#else
    extern void *__real_realloc(void *ptr, size_t size);
    return __real_realloc(ptr,size);
#endif
}

void *__wrap_calloc(size_t num, size_t size)
{
#if REPLACE_SYS_MALLOC
    return dlcalloc(num, size);
#else
    extern void *__real_calloc(size_t num, size_t size);
    return __real_calloc(num,size);
#endif
}

/*****************************************************************************/

void assert_fail (char *file, int line, char *exp) {
    printf("Assertion failed: %s\nFile: %s\nLine: %d\n",exp,file,line);
    while(1) Delay(50);
}

/*****************************************************************************/

static void stop(void)
{
    if(bufmem) {
        dlfree(bufmem);
        bufmem_roll = bufmem = NULL;
    }
}

static void start(void)
{
    started = 255;
    atexit(stop);

    if (SysBase->AttnFlags &(1 << 10)) {
        ac68080_saga = _ZN3dvl10fullscreenE ? 255:0; // disable if not fullscreen

        bufmem = dlmemalign(32/* byte alignment for saga */, 3*FRAME_BUFFER_SZ);
        if(bufmem) {
            bufmem_roll = bufmem + 2*FRAME_BUFFER_SZ;
        } else {
            ac68080_saga = 0;
        }

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
                printf(" AMMX");
            }
        }
        printf(".\n");
    }
}

SDL_Surface* vampire_MakeTripleBuffer(SDL_Surface *surf)
{
    if(!started) start();

    if(ac68080_saga
    &&  surf->w==BUFFER_WIDTH
    &&  surf->h==BUFFER_HEIGHT
    &&  surf->pitch==BUFFER_WIDTH
    ) {
        surf->flags |= SDL_PREALLOC;
        SDL_free(surf->pixels);
        surf->pixels = bufmem;
    } else ac68080_saga = 0;
    return surf;
}

static __attribute__((noinline)) void doChkSignals(void)
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

static void chkSignals(void)
{
    static UBYTE closing, ctr;
    if(!ctr) {
        ctr = 4;
        doChkSignals();
    } else {
        --ctr;
    }
}

static void blitRect(UBYTE *dst, UBYTE *src, UWORD x, UWORD y, size_t w, UWORD h)
{
    src += SCREENXY(x,y);
    dst += SCREENXY(x,y);
    memcpy(dst-x, src-x, BUFFER_WIDTH*h);

    // do {
        // memcpy(dst, src, w);
        // src += BUFFER_WIDTH;
        // dst += BUFFER_WIDTH;
    // } while(--h);
}

// check if palette has changed
static void doPalette(void)
{
    static int last_version = 0;
    if(last_version!=pal_palette_version) {
        last_version = pal_palette_version;
        SDL_SetColors(SDL_GetVideoSurface(), pal_palette->colors, 0, pal_palette->ncolors);
    }
}

// set saga regs to dsplay screen at ptr (modulo bytes to skip at end of line)
static void setFrameBufferRegs(UBYTE *ptr, UWORD modulo)
{
    volatile UBYTE **dpy = (UBYTE**)0xDFF1EC; /* Frame buffer address */
    volatile UWORD  *mod =  (UWORD*)0xDFF1E6; /* Frame buffer modulo */

    *dpy = ptr;
    *mod = modulo;
}

static void doFlip(void)
{
    struct Screen *first_screen;

#if DIRTY
    // hacky way to debug
    *dpy = (void*)(~31&(int)pal_surface->pixels);
    return;
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
        UBYTE *ptr = pal_surface->pixels;
        LONG   dlt = ptr == bufmem_roll ? -2*FRAME_BUFFER_SZ : FRAME_BUFFER_SZ;

        setFrameBufferRegs(ptr + SCREENXY(0,0), BUFFER_WIDTH - SCREEN_WIDTH);

#if ROLL_PTR
    // printf("ptr=%p // %p %p %p %p\n", ptr,bufmem, bufmem+FRAME_BUFFER_SZ, bufmem+2*FRAME_BUFFER_SZ, bufmem+3*FRAME_BUFFER_SZ);

        // need to copy parts of previous screen?
        if(copy_previous)
        {
            --copy_previous;
            if(copy_all)    blitRect(ptr+dlt, ptr, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            else            blitRect(ptr+dlt, ptr, PANEL_LEFT, PANEL_TOP, PANEL_WIDTH, PANEL_HEIGHT);
        }

        // advance ptr
        pal_surface->pixels = (ptr += dlt);
#endif
    }
    doPalette();
}

int vampire_Flip(const SDL_Surface* surf)
{
    chkSignals();

    if(last_was_pal) {
        last_was_pal = 0;
        doFlip();
    } else {
        return SDL_Flip(surf);
    }
}

int vampire_BlitSurface(SDL_Surface *src, SDL_Rect *srcRect,
                        SDL_Surface *dst, SDL_Rect *dstRect)
{
    if(ac68080_saga && src==pal_surface) {
        last_was_pal = 255;
        if(!srcRect || srcRect->h>=SCREEN_HEIGHT) {
            copy_all = 255;
            copy_previous = 3;
        } else if(srcRect->y + srcRect->h > PANEL_Y // something drawn in panel
            && !(srcRect->w==288 && srcRect->h==60) // ignore descpane
        ) {
            copy_all = 0;
            copy_previous = 3;
        }
        return 0;
    } else return SDL_BlitSurface(src, srcRect, dst, dstRect);
}

#define min(a,b) ((a)<=(b)?(a):(b))

int simple_BlitSurface(SDL_Surface *src, SDL_Rect *srcRect,
                        SDL_Surface *dst, SDL_Rect *dstRect)
{
    register UBYTE *s, *d;
    UWORD  w;
    WORD   h;

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
}

// get rid of SDL's saga detection because it isn't necessary
// and produce garbage when not fullscreen during splash-logo-video
extern int __real_SDL_VideoInit (const char *driver_name, Uint32 flags);
int __wrap_SDL_VideoInit (const char *driver_name, Uint32 flags)
{
    ULONG bak = SysBase->AttnFlags;
    int ret;

    SysBase->AttnFlags &= ~(1 << 10);
    ret = __real_SDL_VideoInit(driver_name, flags);
    SysBase->AttnFlags = bak;

    return ret;
}
