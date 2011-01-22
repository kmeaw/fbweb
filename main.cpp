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
};

class HTMLEntity { 
  public:
  HTMLEntity *next;
  HTMLEntity *children;
  char *name;

  HTMLEntity() : next(0), children(0) {}
  HTMLEntity(const char **data);

  virtual void render()
  {
    HTMLEntity *ent;
    printf ("<%s>\n", name);
    for (ent = children; ent; ent = ent->next)
      ent->render();
    printf ("</%s>\n", name);
  }
};

class HTMLAttribute : public HTMLEntity {
  public:
  char *value;

  HTMLAttribute(const char *_name, char *_value)
  	: value(_value)
  {
    name = strdup(_name);
  }

  HTMLAttribute(const char **data)
  {
    char quot;
    const char *name_start = *data;
    const char *value_start;
    while (**data && !strchr (" \t\n\r>=", **data))
      (*data) += 1;
    name = (char*) malloc (*data - name_start + 1);
    memcpy (name, name_start, *data - name_start);
    name[*data - name_start] = 0;
    if (**data == '=')
    {
      (*data) += 1;
      quot = **data;
      if (quot != '\'' && quot != '"')
	quot = 0;
      else
	(*data) += 1;
      value_start = *data;
      if (quot)
	while (**data && **data != quot)
	  (*data) += 1;
      else
	while (**data && !strchr (" \t\n\r>", **data))
	  (*data) += 1;
      value = (char*) malloc (*data - value_start + 1);
      memcpy (value, value_start, *data - value_start);
      value[*data - value_start] = 0;
      if (quot)
	(*data) += 1;
    }
    else
      value = NULL;
    while (**data && strchr (" \t\n\r", **data))
      (*data) += 1;
  }

  virtual void render()
  {
    printf ("attr: %s = %s.\n", name, value);
  }
};

class HTMLText : public HTMLEntity {
  public:
  char *text;

  HTMLText(char *_text) : text(_text) {}
  HTMLText(const char **data) 
  {
    const char *text_start = *data;
    const char *text_end;
    while (**data && **data != '<')
      (*data) += 1;
    text_end = *data;
    while (text_end > text_start && !strchr(" \n\r\t", *text_end))
      text_end--;
    text = (char *) malloc(text_end - text_start + 1);
    memcpy (text, text_start, text_end - text_start);
    text[text_end - text_start] = 0;
  }

  virtual void render()
  {
    printf ("text: %s\n", text);
  }
};

  HTMLEntity::HTMLEntity(const char **data)
  {
    HTMLEntity *last_child = NULL;
    printf ("HTMLEntity::HTMLEntity(%s);\n", *data);

    while (**data != '<')
      (*data) ++;
    
    const char *name_start = *data + 1;
    while (**data && !strchr (" \t\n\r>", **data))
      (*data) ++;

    name = (char *) malloc (*data - name_start + 1);
    memcpy (name, name_start, *data - name_start);
    name[*data - name_start] = 0;

    while (**data && strchr (" \t\n\r", **data))
      (*data) ++;

    while (**data && **data != '>')
    {
      HTMLAttribute *attr = new HTMLAttribute(data);
      if (last_child)
      {
	last_child->next = attr;
	last_child = attr;
      }
      else
	last_child = children = attr;
    }

    HTMLEntity *ent;
    while (**data && strchr (" \t\n\r>", **data))
      (*data) ++;


    while (**data)
    {
      while (**data && strchr (" \t\n\r", **data))
	(*data) ++;
      if (**data == '<')
      {
	if ((*data)[1] == '/')
	  break;
	else
	  ent = new HTMLEntity(data);
      }
      else
	ent = new HTMLText(data);

      if (last_child)
      {
	last_child->next = ent;
	last_child = ent;
      }
      else
	last_child = children = ent;
    }
  }

#define PRINTF(fmt, args...) printf(fmt, ## args)

char *GET(const char *path)
{
  static char buf[512], *ptr;
  size_t sz;
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  // servaddr.sin_len = sizeof(servaddr);
  servaddr.sin_family = AF_INET;
  PRINTF("inet_pton\n");
#if __POWERPC__
  servaddr.sin_addr.s_addr = 0x59b3f3b0;
#else
  servaddr.sin_addr.s_addr = 0xb0f3b359;
#endif
  servaddr.sin_port = htons(80);
  if (connect (sock, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
  {
    PRINTF("Connection failed.\n");
    return NULL;
  }
  PRINTF("connect\n");
  sprintf(buf, "GET %s HTTP/1.0\r\nHost: kmeaw.com\r\n\r\n", path);
  write(sock, buf, strlen(buf));
  PRINTF("write\n");

  ptr = buf;
  PRINTF("read\n");
#ifdef __POWERPC__
  for(ptr = buf; ptr - buf <= 4 || *((uint32_t *)(ptr - 4)) != 0x0d0a0d0a; read(sock, ptr++, 1));
#else
  for(ptr = buf; ptr - buf <= 4 || *((uint32_t *)(ptr - 4)) != 0x0a0d0a0d; read(sock, ptr++, 1));
#endif
  PRINTF("got %lu bytes.\n", ptr - buf);
  *ptr = 0;
  ptr = strstr(buf, "Content-Length:");
  if (!ptr)
  {
    PRINTF("No Content-Length: %s.\n", buf);
    return NULL;
  }
  ptr += strlen("Content-Length: ");
  sz = atoi(ptr);
  PRINTF("sz = %zd.\n", sz);
  ptr = (char *) malloc(sz + 1);
  read(sock, ptr, sz);
  close(sock);
  ptr[sz] = 0;
  return ptr;
}

void render(HTMLEntity *root, int x, int y)
{
  clear (0);
}

HTMLEntity* parse(const char **data)
{
  return new HTMLEntity(data);
}

int main()
{
  char *data;
  
  data = GET("/hpr/");

  PRINTF("parse\n");
  HTMLEntity * root = parse((const char **) &data);

  gfxinit();

  PRINTF("loop\n");

  root->render();

  while (0)
  {
    waitFlip();
    render(root, 0, 0);
    flip();
  }

  return 0;
}
