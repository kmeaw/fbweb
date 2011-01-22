#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>

static SDL_Surface *screen;

void gfxinit()
{
  if (SDL_Init (SDL_INIT_VIDEO) != 0)
  {
    fprintf (stderr, "Cannot init video.\n");
  }

  atexit(SDL_Quit);

  screen = SDL_SetVideoMode (720, 576, 32, SDL_HWSURFACE);
  if (screen == NULL)
  {
    fprintf (stderr, "Unable to set video mode: %s.\n", SDL_GetError ());
    abort ();
  }

  SDL_ShowCursor (0);
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
}

void waitFlip()
{
  if (SDL_MUSTLOCK (screen))
    if (SDL_LockSurface (screen))
    {
      fprintf (stderr, "Cannot lock surface: %s\n", SDL_GetError ());
    }
}

void clear(uint32_t color)
{
  SDL_FillRect (screen, NULL, color);
}

void pset(int x, int y, uint32_t color)
{
  ((uint32_t *)screen->pixels)[screen->pitch / sizeof(uint32_t) * y + x] = color;
}

void flip()
{
  if (SDL_MUSTLOCK (screen))
    SDL_UnlockSurface (screen);

  SDL_Flip (screen);
  SDL_Delay (10);
}


