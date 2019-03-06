
#ifndef _PIXEL_H_
#define _PIXEL_H_

typedef struct _SortedPixel {
    uint32_t pixel;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint32_t frameindex;
    uint32_t sortedindex;
    uint8_t colorindex;
    uint32_t npixel;
    float residual;  // Used for storing residual pixel error when dithering
} SortedPixel;

#endif
