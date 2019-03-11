/*
 Copyright (c) 2019 Cory Rupp
 
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

#ifndef _PNGREADER_H_
#define _PNGREADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#define BUFFSIZE 16384

typedef struct _PNGHeader {
    uint32_t Width;
    uint32_t Height;
    uint8_t BitDepth;
    uint8_t ColorType;
    uint8_t Compression;
    uint8_t Filter;
    uint8_t Interlace;
} PNGHeader;

typedef struct _PNGChunk
{
    uint8_t DataLength[4];   /* Size of Data field in bytes */
    uint32_t Length;
    char Type[5];         /* Code identifying the type of chunk */
    uint8_t Data[BUFFSIZE];       /* The actual data stored by the chunk */
    uint8_t CRC[4];          /* CRC-32 value of the Type and Data fields */
} PNGChunk;

void readPNGHeader(FILE* fid, PNGHeader *header);
void readPNGFrame(FILE* fid, uint32_t width, uint32_t height, uint8_t* frame, uint8_t bytesPerPixel);
void readPNGChunk(FILE* fid, PNGChunk *chunk);
void defilterPNGFrame(uint32_t width, uint32_t height, uint8_t* frame, uint8_t bytesPerPixel);
uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c);
int byteswap(uint8_t* bytes);
uint32_t inflateData(z_stream *zstrm, uint8_t *source, uint32_t sourcelen, uint8_t *dest, uint32_t destlen);
void zerr(int ret);

#endif
