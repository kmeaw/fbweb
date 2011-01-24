#ifndef _VIEW_H_
#define _VIEW_H_

#include "xml.h"
#ifdef __POWERPC__
extern "C" {
#include <pngdec/pngdec.h>
#include <pngdec/loadpng.h>
}
#else
#include "pngdatas.h"
extern "C" int LoadPNG(PngDatas *png, const char *filename);
#endif
#include <string.h>
#include <math.h>
#include <stdint.h>

#define FONT_FACTOR 64
#define BADPNG	64

class Controller;

class View
{
  public:
  View(XMLEntity<View>&);
  View(XMLAttribute<View>&);
  View(XMLText<View>&);
  View() { init(); }
  void init();
  static void reset()
  {
    controller = 0;
  }
  View *children;
  View *last_child;
  View *next;
  View *parent;
  static Controller *controller;
  virtual void render();

  void add(View &view);
  virtual void pset(int x, int y, uint32_t color);
  virtual float px();
  virtual float py();
  virtual uint32_t c();
  virtual void px(float);
  virtual void py(float);
  virtual void c(uint32_t);
  virtual int font();
  virtual void font(int);

  protected:
  float _px, _py;
  int _font;
  uint32_t _c;
};

void View::font(int sz)
{
  if (parent)
    parent->font(sz);
  else
    _font = sz;
}

int View::font()
{
  if (parent)
    return parent->font();
  else
    return _font;
}

void View::c(uint32_t c)
{
  if (parent)
    parent->c(c);
  else
    _c = c;
}

uint32_t View::c()
{
  if (parent)
    return parent->c();
  else
    return _c;
}

void View::px(float x)
{
  if (parent)
    parent->px(x);
  else
    _px = x;
}

void View::py(float y)
{
  if (parent)
    parent->py(y);
  else
    _py = y;
}

float View::px()
{
  if (parent)
    return parent->px();
  else
    return _px;
}

float View::py()
{
  if (parent)
    return parent->py();
  else
    return _py;
}

void View::pset(int x, int y, uint32_t color)
{
  if (parent == NULL)
    ::pset (x, y, color);
  else
    parent->pset (x, y, color);
}

static size_t mbrlen(const char *str)
{
  size_t ret = 0;
  int bits;
  unsigned char c;
  const unsigned char *ptr = reinterpret_cast<const unsigned char*>(str);

  while (*ptr)
  {
    bits = 0;
    c = *ptr;
    while (c & 0x80)
    {
      c <<= 1;
      bits++;
    }

    if (bits >= 2 && bits <= 6)
      ptr += bits;
    else
      ptr++;

    ret++;
  }

  return ret;
}

static uint32_t mbrchr(const char **ptr)
{
  int bits = 0;
  uint32_t result = 0;
  unsigned char c = **reinterpret_cast<const unsigned char **>(ptr);

  (*ptr) += 1;

  while (c & 0x80)
  {
    c <<= 1;
    bits++;
  }

  if (bits == 0 || bits == 1 || bits > 6)
    return c & 0x7f;

  result = c & (0xff >> bits);
  while (--bits)
  {
    result <<= 6;
    c = **reinterpret_cast<const unsigned char**>(**ptr);
    (*ptr) += 1;
    result |= c & 0x3f;
  }

  return result;
}

class ViewText : public View
{
  protected:

  public:
  char *text;
  int err;

  ViewText(char *_text) : text(_text) {}
  virtual void render()
  {
    FT_UInt glyph_index;
    char *dup = strdup (text);
    char *ptr = dup;
    char *strt = dup;
    char *brk = NULL;
    uint32_t c;
    FT_GlyphSlot glyph;

    int x = px(), y = py();
    int bx = x, by = y;
    int w, h;

    while (*ptr)
    {
      c = mbrchr (const_cast<const char**>(&ptr));
      glyph_index = FT_Get_Char_Index( face, c );
      if ((err = FT_Load_Char( face, c, FT_LOAD_NO_BITMAP )))
      {
	fprintf (stderr, "Cannot load glyph: %X, error %u!\n", c, err);
	abort ();
      }
    
      glyph = face->glyph;
      if (c < 0x80 && strchr (" \t\n\r", c))
      {
	ptr[-1] = ' ';
	brk = ptr - 1;
	bx = x;
	by = y;
      }
      
      w = face->glyph->advance.x >> 6;
      h = face->glyph->advance.y >> 6;

      x += w;
      y += h;

      if (x >= W)
      {
	if (brk)
	{
	  ptr = brk + 1;
	  *brk = '\n';
	  x = 0;
	  y = by + w;
	  brk = NULL;
	}
	else if (px() > 0)
	{
	  ptr = strt;
	  px((x = 0));
	  py((y = py() + h));
	}
	else
	{
	  x = 0;
	  y += h;
	}
	continue;
      }
    }

    ptr = dup;
    while (*ptr)
    {
      c = mbrchr (const_cast<const char**>(&ptr));
      if (c == '\n')
	glyph_index = FT_Get_Char_Index( face, ' ' );
      else
	glyph_index = FT_Get_Char_Index( face, c );
      if ((err = FT_Load_Char( face, c, FT_LOAD_RENDER )))
      {
	fprintf (stderr, "Cannot load glyph: %X, error %u!\n", c, err);
	abort ();
      }

      if (FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL ))
      {
	fprintf (stderr, "Cannot render glyph!\n");
	abort ();
      }

      if (c == '\n')
      {
	px(0);
	py(py() + face->size->metrics.height / FONT_FACTOR);
      }
      else
      {
	for (int y = 0; y < face->glyph->bitmap.rows; y++)
	  for (int x = 0; x < face->glyph->bitmap.width; x++)
	  {
	    c = face->glyph->bitmap.buffer[y * face->glyph->bitmap.width + x];
	    c = c | (c << 8) | (c << 16);
	    pset (px() + x, 20 + py() + y - face->glyph->bitmap_top, c);
	  }

	px(px() + (face->glyph->advance.x >> 6));
	py(py() + (face->glyph->advance.y >> 6));
      }
    }

    free (dup);
  }
};

class ViewPicture : public View
{
  virtual void render() {}
};

class ViewLink : public View
{
  public:
  char *uri;
  int bx, by, bw, bh;

  virtual void render();
  ViewLink(XMLAttribute<View> *attr)
  {
    if (attr)
      uri = attr->value;
    else
      uri = 0;

    bx = by = bw = bh = 0;
  }

  virtual void pset(int x, int y, uint32_t color)
  {
    if (bw == 0)
    {
      bx = x;
      by = y;
      bw = 1;
      bh = 1;
    }
    else
    {
      if (x >= bx + bw)
	bw = x - bx + 1;
      else if (x < bx)
      {
	bw += bx - x;
	bx = x;
      }

      if (y >= by + bh)
	bh = y - by + 1;
      else if (y < by)
      {
	bh += by - y;
	by = y;
      }
    }
    parent->pset (x, y, color);
  }

  virtual uint32_t c()
  {
    return 0xff;
  }
};

class ViewImage : public View
{
  public:
  int w, h;
  size_t sz;
  PngDatas png;

  ViewImage(XMLAttribute<View> *attr)
  {
    png.bmp_out = 0;

    if (!attr || !attr->value)
      return;

    size_t sz;
    png.png_in = GET (attr->value, &sz);
    png.png_size = sz & 0xffffffff;

    if (!png.png_in)
      return;

    LoadPNG(&png, 0);
  }

  virtual void render()
  {
    if (!png.bmp_out)
    {
      int cx = px(), cy = py();
      for (int y = 0; y < BADPNG; y++)
	for (int x = 0; x < BADPNG; x++)
	  pset(cx + x, cy + y, (x ^ y) & 1 ? 0xff : 0xff00);
      px(0);
      py(cy + BADPNG);
      return;
    }
    else
    {
      int cx = px(), cy = py();
      const uint32_t *ptr = reinterpret_cast<uint32_t*>(png.bmp_out);
      for (int y = 0; y < png.height; y++)
	for (int x = 0; x < png.width; x++)
	  pset(cx + x, cy + y, *ptr++);
      px(0); py(cy + png.height);
    }
  }
};

class ViewBr : public View
{
  public:
  ViewBr()
  {
  }

  virtual void render()
  {
    if (FT_Load_Char( face, ' ', FT_LOAD_NO_BITMAP ))
    {
      fprintf (stderr, "Cannot load space!\n");
      abort ();
    }

    py(py() + (face->size->metrics.height >> 6));
    px(0);

    View::render();
  }
};

class Controller
{
  public:
  ViewLink **array;
  int count;
  int id;
  ViewLink *active_link;

  Controller() : array(0), count(0), active_link(0), id(0)
  {
    printf("Controller::Controller();\n");
  }

  void add(ViewLink *link)
  {
    if (!active_link)
      active_link = link;
    array = (ViewLink **) realloc (array, sizeof (ViewLink *) * (count + 1));
    array[count] = link;
    count++;
  }

  void go(int d)
  {
    if (!count)
      return;

    id = (id + d) % count;
    active_link = array[id];
  }

  void merge(Controller *c)
  {
    for (int i = 0; i < c->count; i++)
      add(c->array[i]);
  }
};

  void ViewLink::render()
  {
    uint32_t c = 0x4000;
    for(View *scan = children; scan; scan = scan->next)
      scan->render();
    if (controller->active_link == this)
      c = 0xffff;
    for(int x = -2; x < bw + 2; x++)
    {
      parent->pset (bx + x, by - 1, c >> 8);
      parent->pset (bx + x, by + bh + 1, c >> 8);
      parent->pset (bx + x, by - 2, c);
      parent->pset (bx + x, by + bh + 2, c);
    }
    for(int y = -2; y < bh + 2; y++)
    {
      parent->pset(bx - 1, by + y, c >> 8);
      parent->pset(bx + bw + 1, by + y, c >> 8);
      parent->pset(bx - 2, by + y, c);
      parent->pset(bx + bw + 2, by + y, c);
    }
  }

void View::add(View &view)
{
  if (children == NULL)
    children = last_child = &view;
  else
  {
    last_child->next = &view;
    last_child = &view;
  }

  view.next = 0;
  view.parent = this;
}

View::View(XMLEntity<View>& e) {
  init();
  View *target = this;

  if (!strcasecmp (e.name, "a"))
  {
    add(*(target = new ViewLink(e.get("href"))));
    controller->add(reinterpret_cast<ViewLink*>(target));
  }
  else if (!strcasecmp (e.name, "br"))
  {
    add(*(target = new ViewBr()));
  }
  else if (!strcasecmp (e.name, "img"))
    add(*(target = new ViewImage(e.get("src"))));
  else if (!strcasecmp (e.name, "h1"))
  {
    font(font() + 10);
    for(XMLEntity<View> *scan = e.children; scan; scan = scan->next)
      target->add(scan->visit());
    font(font() - 10);
    return;
  }

  for(XMLEntity<View> *scan = e.children; scan; scan = scan->next)
    target->add(scan->visit());
}

void View::render()
{
  for(View *scan = children; scan; scan = scan->next)
    scan->render();
  if (!parent)
  {
    px(0); py(0); c(0xffffff);
  }
}

void View::init() {
  children = parent = last_child = next = 0;
  _px = _py = 0;
  _c = 0xffffff;
  _font = 12;
  if (!View::controller)
    View::controller = new Controller();
}

View::View(XMLAttribute<View>& e) {
  init();
}

View::View(XMLText<View>& e) {
  init();
  add(*(new ViewText(e.text)));
}

Controller* View::controller = 0;
#endif
