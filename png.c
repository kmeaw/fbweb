/*
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>
#include "pngdatas.h"

typedef uint32_t u32;

#define abort_(...) do { abort__(__VA_ARGS__); return; } while(0)

void abort__(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
}

int x, y;

int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

static void read_data(png_structp pngPtr, png_bytep data, png_size_t length)
{
  void** a = (void**) png_get_io_ptr(pngPtr);
  memcpy (data, *a, length);
  *a = ((uint8_t *) *a) + length;
}

int LoadPNG(PngDatas *png, const char *filename)
{
  	png_bytepp pixel;
	void *ptr = png->png_in;
	int x, y;
        char header[8];    // 8 is the maximum size that can be checked

	png->bmp_out = NULL;

        if (png_sig_cmp(png->png_in, 0, 8))
                abort_("[read_png_file] File is not recognized as a PNG file");

        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

	png_set_read_fn(png_ptr, (void *) &ptr, &read_data);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_ALPHA, 0);

	png->width = info_ptr->width;
	png->height = info_ptr->height;
	png->wpitch = info_ptr->width * 4;

	png->bmp_out = malloc (png->width * png->height * 4);
	pixel = png_get_rows(png_ptr, info_ptr);

	for(y=0; y < png->height; y++)
	{
	  for(x=0; x < png->width; x++)
	    ((u32 *) png->bmp_out)[x + y * png->width] = (((*pixel) + x * 3)[0] << 0) + (((*pixel) + x * 3)[1] << 8) + (((*pixel) + x * 3)[2] << 16);
	  pixel++;
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

