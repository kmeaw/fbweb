#ifndef _PNGDATAS_H_
#define _PNGDATAS_H_
#include <stdint.h>
typedef struct PngDatas {
        void * png_in;          // ignored except if char *filename == NULL in LoadPNG()
        size_t png_size;  // ignored except if char *filename == NULL  in LoadPNG()
        
        void * bmp_out;         // internally allocated (bmp 32 bits color ARGB format)

        int     wpitch;                 // output width pitch in bytes
        int width;                      // output
        int height;                     // output
} PngDatas;
#endif

