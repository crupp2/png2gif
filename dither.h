#ifndef _DITHER_H_
#define _DITHER_H_

#include <stdlib.h>
#include "pixel.h"

void dither(SortedPixel* unique, int nunique, SortedPixel* frame, uint32_t width, uint32_t height);
uint32_t findClosestColor(SortedPixel* unique, int nunique, SortedPixel pixel);

#endif
