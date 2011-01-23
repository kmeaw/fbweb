#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>

static SDL_Surface *screen;

void gfxinit(int *w, int *h)
{
  if (SDL_Init (SDL_INIT_VIDEO) != 0)
  {
    fprintf (stderr, "Cannot init video.\n");
  }

  atexit(SDL_Quit);

  *w = 720;
  *h = 576;

  screen = SDL_SetVideoMode (*w, *h, 32, SDL_HWSURFACE);
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

void rect(int x, int y, int w, int h, uint32_t color)
{
  SDL_Rect dstrect = { .x = x, .y = y, .w = w, .h = h };
  SDL_FillRect (screen, &dstrect, color);
}

void pset(int x, int y, uint32_t color)
{
  if (x < 0 || y < 0 || x >= screen->w || y >= screen->h) return;
  ((uint32_t *)screen->pixels)[screen->pitch / sizeof(uint32_t) * y + x] = color;
}

void flip()
{
  if (SDL_MUSTLOCK (screen))
    SDL_UnlockSurface (screen);

  SDL_Flip (screen);
  SDL_Delay (10);
}

int getKey()
{
  SDL_Event event;

  if (!SDL_PollEvent (&event))
    return -1;

  if (event.type == SDL_QUIT)
    return 0;
  else if (event.type == SDL_KEYUP)
  {
    if (event.key.keysym.sym == SDLK_UP)
      return 1;
    else if (event.key.keysym.sym == SDLK_DOWN)
      return 2;
    else if (event.key.keysym.sym == SDLK_RETURN)
      return 3;
  }

  return -1;
}
