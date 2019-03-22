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

#ifndef _GIFWRITER_H_
#define _GIFWRITER_H_

#include <stdio.h>
#include <stdlib.h>
#include "pixel.h"


// Set up an enum for the palettes and an array with the corresponding number of palette bits (0 if variable)
enum _Palettes {P685g, P676g, P884, Pweb, Pmedian, Pgray, PgrayT};

typedef struct _GIFOptStruct {
    uint16_t delay;
    enum _Palettes colorpalette;
    int dither;
    int colortablebitsize;
    int forcebw;
    SortedPixel* palette;  // This will eventually point to the palette
} GIFOptStruct;

GIFOptStruct newGIFOptStructInst();
void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, GIFOptStruct gifopts);
void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, GIFOptStruct gifopts);
uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, GIFOptStruct gifopts);
void writeGIFImageCompressed(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, int tablebitsize);
void writeGIFImageCompressed9bit(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
void writeGIFImageUncompressed256(FILE* fid, uint8_t* frame, uint32_t length);
void writeGIFImageUncompressed128(FILE* fid, uint8_t* frame, uint32_t length);
uint32_t convert9to8(uint16_t* input, uint8_t* output, uint32_t length);
uint32_t packLSB(uint16_t* input, uint8_t* output, uint32_t length, uint8_t startnbits, uint64_t* remain, int* startshift, uint32_t* widthjumps, int islast);

// From LZWlib.cpp
int LZWcompress(uint8_t** input, uint32_t *inlen, uint16_t** output, uint32_t* widthjumps, uint8_t initialcodesize);
int LZWcompress9bit(uint8_t* input, uint32_t inlen, uint16_t* output);

#endif
