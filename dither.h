#ifndef _DITHER_H_
#define _DITHER_H_

#include <stdlib.h>
#include "pixel.h"

void dither(SortedPixel* unique, int nuinque, SortedPixel* frame, uint32_t width, uint32_t height);

#endif
