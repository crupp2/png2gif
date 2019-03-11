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

#ifndef _MEDIANCUT_H_
#define _MEDIANCUT_H_

#include <stdlib.h>
#include "pixel.h"

typedef struct _CutBin {
    uint8_t minR;
    uint8_t maxR;
    uint8_t minG;
    uint8_t maxG;
    uint8_t minB;
    uint8_t maxB;
    SortedPixel* buffer;  // Buffer containing pixels pointing to the starting position of this bin
    uint32_t length;  // Number of pixels in this bin
} CutBin;

void sortR(SortedPixel* buffer, uint32_t length);
void sortG(SortedPixel* buffer, uint32_t length);
void sortB(SortedPixel* buffer, uint32_t length);
void medianCut(SortedPixel* buffer, uint32_t length, int tablebitsize);
CutBin splitBin(CutBin* bin);
void getRange(CutBin* bin);

#endif
