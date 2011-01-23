#ifndef _XML_H_
#define _XML_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

template <class T> class XMLAttribute;

template <class T> class XMLEntity { 
  public:
  XMLEntity<T> *next;
  XMLEntity<T> *children;
  XMLEntity<T> *attributes;
  char *name;

  XMLEntity<T>() : next(0), children(0), attributes(0), name(0) {}
  XMLEntity<T>(const char **data);

  XMLAttribute<T>* get(const char *name);
  virtual T& visit();

  virtual void dump()
  {
    printf("XMLEntity name=%s:\n", name);
    for (XMLAttribute<T> *scan = static_cast<XMLAttribute<T>*>(attributes); scan; scan = static_cast<XMLAttribute<T>*>(scan->next))
      scan->dump();
    for (XMLEntity<T> *scan = children; scan; scan = scan->next)
      scan->dump();
  }
};

template <class T> class XMLAttribute : public XMLEntity<T> {
  public:
  char *value;
  XMLAttribute(const char *_name, char *_value);
  XMLAttribute(const char **data);
  virtual T& visit();

  virtual void dump()
  {
    printf("XMLAttribute name=%s value=%s;\n", this->name, value);
  }
};

template <class T> class XMLText : public XMLEntity<T> {
  public:
  char *text;
  XMLText(const char **data); 
  XMLText(char *_text);
  virtual T& visit();

  virtual void dump()
  {
    printf("XMLText value={{{%s}}};\n", text);
  }
};


  template <class T> 
  XMLAttribute<T>::XMLAttribute(const char *_name, char *_value)
  	: value(_value)
  {
    this->name = strdup(_name);
    this->children = this->next = 0;
  }

  template <class T> 
  XMLAttribute<T>::XMLAttribute(const char **data)
  {
    char quot;
    const char *name_start = *data;
    const char *value_start;
    while (**data && !strchr (" \t\n\r>=", **data))
      (*data) += 1;
    this->name = (char*) malloc (*data - name_start + 1);
    memcpy (this->name, name_start, *data - name_start);
    this->name[*data - name_start] = 0;
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
      this->value = (char*) malloc (*data - value_start + 1);
      memcpy (value, value_start, *data - value_start);
      this->value[*data - value_start] = 0;
      if (quot)
	(*data) += 1;
    }
    else
      this->value = NULL;
    while (**data && strchr (" \t\n\r", **data))
      (*data) += 1;
  }

  template <class T> 
  XMLText<T>::XMLText(char *_text) : text(_text) {}

  template <class T> 
  XMLText<T>::XMLText(const char **data) 
  {
    const char *text_start = *data;
    const char *text_end;
    while (**data && **data != '<')
      (*data) += 1;
    text_end = *data;
    while (text_end > text_start && strchr(" \n\r\t", *text_end))
      text_end--;
    text = (char *) malloc(text_end - text_start + 1);
    memcpy (text, text_start, text_end - text_start);
    text[text_end - text_start] = 0;
  }

  template <class T> 
  XMLEntity<T>::XMLEntity(const char **data)
  {
    XMLEntity<T> *last_child = NULL;
    XMLEntity<T> *last_attribute = NULL;

    next = attributes = children = 0;

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
      if (**data == '/')
      {
	while (**data && **data != '>')
	  (*data) ++;
	if (**data)
	  (*data) ++;
	return;
      }

      XMLAttribute<T> *attr = new XMLAttribute<T>(data);
      if (last_attribute)
      {
	last_attribute->next = attr;
	last_attribute = attr;
      }
      else
	last_attribute = attributes = attr;
    }

    XMLEntity<T> *ent;
    while (**data && strchr (" \t\n\r>", **data))
      (*data) ++;

    while (**data)
    {
      while (**data && strchr (" \t\n\r", **data))
	(*data) ++;
      if (**data == '<')
      {
	if ((*data)[1] == '/')
	{
	  while(**data && **data != '>')
	    (*data) += 1;
	  if (**data == '>')
	    (*data) += 1;
	  break;
	}
	else
	  ent = new XMLEntity<T>(data);
      }
      else
	ent = new XMLText<T>(data);

      if (last_child)
      {
	last_child->next = ent;
	last_child = ent;
      }
      else
	last_child = children = ent;
    }
  }

  template <class T> 
  XMLAttribute<T>* XMLEntity<T>::get(const char *name)
  {
    XMLAttribute<T> *scan;

    for (scan = static_cast<XMLAttribute<T>*>(attributes); scan; scan = static_cast<XMLAttribute<T>*>(scan->next))
      if (!strcasecmp (scan->name, name))
	return scan;

    return NULL;
  }

  template <class T> 
  T& XMLEntity<T>::visit()
  {
    return *new T(*this);
  }

  template <class T> 
  T& XMLAttribute<T>::visit()
  {
    return *new T(*this);
  }

  template <class T> 
  T& XMLText<T>::visit()
  {
    return *new T(*this);
  }
#endif
