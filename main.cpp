#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include <malloc.h>

extern "C" {
#include "sdl.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftglyph.h>
#include "ungpkg.h"
};

int W,H;
FT_Library  library;
FT_Face     face;

#include "get.h"
#include "xml.h"
#include "view.h"

int main()
{
  char *data = GET("/hpr/", 0);

  printf("{{{%s}}}\n", data);

  gfxinit(&W, &H);
  if (FT_Init_FreeType( &library ))
  {
    fprintf(stderr, "Cannot init freetype2.\n");
    return 1;
  }

  if (FT_New_Face( library,
	"/usr/share/fonts/corefonts/arial.ttf",
	0,
	&face ))
  {
    fprintf(stderr, "Cannot load font.\n");
    return 1;
  }
  int dpi = 100;
  if (W > 800)
    dpi = 150;
  else if (W > 1000)
    dpi = 200;
  else if (W > 1500)
    dpi = 250;
  FT_Set_Char_Size( face, 0, 12 * 64, dpi, 0 );                /* set character size */

  XMLEntity<View> *root = new XMLEntity<View>((const char **) &data);
  View *view = new View(*root);
  view->render();

  bool running = true;

  while (running)
  {
    waitFlip();
    flip();
    switch (getKey())
    {
      case 0:
        running = false;
        break;
      case 1:
        View::controller->go(-1);
	view->render();
	break;
      case 2:
        View::controller->go(1);
	view->render();
	break;
      case 3:
        if (View::controller->active_link)
	{
	  if (strstr (View::controller->active_link->uri, ".pkg"))
	  {
	    ungpkg (GET(View::controller->active_link->uri, 0));
	  }
	  else
	  {
	    data = GET(View::controller->active_link->uri, 0);
	    if (data)
	    {
	      delete view;
	      delete root;
	      View::reset();
	      clear (0);
	      root = new XMLEntity<View>((const char **) &data);
	      root->dump();
	      view = new View(*root);
	      view->render();
	    }
	  }
	}
	else
	{
	    data = GET("/hpr/", 0);
	    if (data)
	    {
	      delete view;
	      delete root;
	      clear (0);
	      root = new XMLEntity<View>((const char **) &data);
	      root->dump();
	      view = new View(*root);
	      view->render();
	    }
	}
	break;
    }
  }

  return 0;
}
