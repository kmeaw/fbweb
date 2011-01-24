#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __POWERPC__
#include <psl1ght/lv2.h>
#endif

#include <malloc.h>

extern "C" {
#include "debug.h"
};

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
  bool pkg_installed = false;
#if DEBUG
  debug_wait_for_client();
#endif

  gfxinit(&W, &H);
  char *data = GET("/hpr/", 0);
  if (FT_Init_FreeType( &library ))
  {
    PRINTF("Cannot init freetype2.\n");
    return 1;
  }

  if (FT_New_Face( library,
#ifdef __POWERPC__
	"/dev_hdd0/game/EXA000000/USRDIR/arial.ttf",
#else
	"arial.ttf",
#endif
	0,
	&face ))
  {
    PRINTF("Cannot load font.\n");
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
  PRINTF("XML parsed.\n");
  View *view = new View(*root);
  PRINTF("View ready.\n");
  waitFlip();
  view->render();
  copy();
  flip();
  PRINTF("Render ok.\n");
  bool running = true;
  PRINTF("In main loop.\n");

  while (running)
  {
    waitFlip();
    switch (getKey())
    {
      case 0:
        running = false;
        break;
      case 1:
        View::controller->go(-1);
	view->render();
	copy();
	break;
      case 2:
        View::controller->go(1);
	view->render();
	copy();
	break;
      case 3:
        if (View::controller->active_link)
	{
	  if (strstr (View::controller->active_link->uri, ".pkg"))
	  {
	    ungpkg (GET(View::controller->active_link->uri, 0));
	    pkg_installed = true;
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
	      copy();
	    }
	  }
	}
	break;
    }
    flip();
  }

#ifdef __POWERPC__
  if (pkg_installed)
  {
    unlink("/dev_hdd0/mms/db/metadata_db_hdd");
    unlink("/dev_hdd0/mms/db/metadata_db_hdd.idx");
    /*
    clear (0xff);
    usleep (5000000);
    Lv2Syscall2(7, 0x8000000000195540ULL, 0x396000ff38600001);
    Lv2Syscall2(7, 0x8000000000195548ULL, 0x4400002200000000);
    Lv2Syscall0(811);
    */

    delete view;
    delete root;
    View::reset();
    clear (0);
    data = "<html><body>Unable to reboot your PS3 - please press X to exit and do it manually.</body></html>";
    root = new XMLEntity<View>((const char **) &data);
    root->dump();
    view = new View(*root);
    view->render();
    copy();
    while (getKey() != 3)
    {
      waitFlip();
      flip();
    }
}
#endif

  return 0;
}
