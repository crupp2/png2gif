/*
 Copyright (c) 2019, Cory Rupp
 
 This code is released under the MIT License:
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#ifndef _PIXEL_H_
#define _PIXEL_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct _SortedPixel {
    uint32_t pixel;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t colorindex;
    uint32_t frameindex;
    uint32_t sortedindex;
    uint32_t npixel;
    // Used for storing residual pixel error when dithering
    float residualR;
    float residualG;
    float residualB;
} SortedPixel;

uint32_t findClosestColor(SortedPixel* palette, int npalette, SortedPixel pixel);
void palettizeColors(SortedPixel* palette, int tablesize, SortedPixel* unique, uint32_t nunique);

#endif
