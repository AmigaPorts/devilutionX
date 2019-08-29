#include <SDL.h>

#include "devilution.h"
#include "miniwin/ddraw.h"
#include "stubs.h"

namespace dvl {

BOOL SDrawUpdatePalette(unsigned int firstentry, unsigned int numentries, PALETTEENTRY *pPalEntries, int a4)
{
	assert(firstentry == 0);
	assert(numentries == 256);

	SDL_Color colors[256];
	for (unsigned int i = firstentry; i < numentries; i++) {
		SDL_Color *c = &colors[i];
		PALETTEENTRY *p = &pPalEntries[i];
		c->r = p->peRed;
		c->g = p->peGreen;
		c->b = p->peBlue;
#if !SDL_VERSION_ATLEAST(2, 0, 0)
		c->unused = SDL_ALPHA_OPAQUE;
#else
		c->a = SDL_ALPHA_OPAQUE;
#endif
	}

	assert(palette);
#if !SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetPalette(pal_surface, SDL_LOGPAL|SDL_PHYSPAL, colors, 0, 256);
	SDL_SetColors(surface, colors, 0, 256);
#endif

	if (SDL_SetPaletteColors(palette, colors, firstentry, numentries) <= -1) { // Todo(Amiga): Fix this!
		SDL_Log(SDL_GetError());
		return false;
	}

	return true;
}

} // namespace dvl
