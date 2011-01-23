#ifndef _VIEW_H_
#define _VIEW_H_

#include "xml.h"
#include <string.h>
#include <math.h>

#define FONT_FACTOR 64

class View
{
  public:
  View(XMLEntity<View>&);
  View(XMLAttribute<View>&);
  View(XMLText<View>&);
  View() : children(0), next(0), _px(0), _py(0) {}
  View *children;
  View *last_child;
  View *next;
  View *parent;
  virtual void render();

  void add(View &view);
  virtual void pset(int x, int y, uint32_t color);
  virtual int px();
  virtual int py();
  virtual void px(int);
  virtual void py(int);

  protected:
  int _px, _py;
};

void View::px(int x)
{
  if (parent)
    parent->px(x);
  else
    _px = x;
}

void View::py(int y)
{
  if (parent)
    parent->py(y);
  else
    _py = y;
}

int View::px()
{
  if (parent)
    return parent->px();
  else
    return _px;
}

int View::py()
{
  if (parent)
    return parent->py();
  else
    return _py;
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

void View::pset(int x, int y, uint32_t color)
{
  if (parent == NULL)
    ::pset (x, y, color);
  else
    parent->pset (x, y, color);
}

void View::render()
{
  for(View *scan = children; scan; scan = scan->next)
    scan->render();
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
	printf ("%d > %d\n", x, W);
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
	printf ("Reset x to %d.\n", x);
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

  virtual void render()
  {
    for(View *scan = children; scan; scan = scan->next)
      scan->render();
    for(int x = 0; x < bw; x++)
    {
      parent->pset(bx + x, by, 0xff00);
      parent->pset(bx + x, by + bh, 0xff00);
    }
    for(int y = 0; y < bh; y++)
    {
      parent->pset(bx, by + y, 0xff00);
      parent->pset(bx + bw, by + y, 0xff00);
    }
  }

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
    color &= 0xff;
    parent->pset (x, y, color);
  }
};

View::View(XMLEntity<View>& e) : children(0) {
  View *target = this;
  if (!strcasecmp (e.name, "a"))
    add(*(target = new ViewLink(e.get("href"))));

  for(XMLEntity<View> *scan = e.children; scan; scan = scan->next)
    target->add(scan->visit());
}

View::View(XMLAttribute<View>& e) : children(0) {
}

View::View(XMLText<View>& e) : children(0) {
  add(*(new ViewText(e.text)));
}

#endif
