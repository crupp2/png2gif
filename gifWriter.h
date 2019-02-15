
#ifndef _GIFWRITER_H_
#include <stdio.h>
#include <stdlib.h>

typedef struct _SortedPixel {
    uint32_t pixel;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint32_t frameindex;
    uint32_t sortedindex;
    uint8_t colorindex;
    uint32_t npixel;
} SortedPixel;

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

void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, uint16_t delay);
void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
void writeGIFImageCompressed(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
void writeGIFImageCompressed9bit(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
void writeGIFImageUncompressed256(FILE* fid, uint8_t* frame, uint32_t length);
void writeGIFImageUncompressed128(FILE* fid, uint8_t* frame, uint32_t length);
uint32_t shrinkGIF(SortedPixel* buffer, uint32_t width, uint32_t height);
void sortR(SortedPixel* buffer, uint32_t length);
void sortG(SortedPixel* buffer, uint32_t length);
void sortB(SortedPixel* buffer, uint32_t length);
void medianCut(SortedPixel* buffer, uint32_t length);
CutBin splitBin(CutBin* bin);
void getRange(CutBin* bin);
void dither();
uint32_t convert9to8(uint16_t* input, uint8_t* output, uint32_t length);
uint32_t packLSB(uint16_t* input, uint8_t* output, uint32_t length, uint8_t startnbits, uint32_t* widthjumps);

// From LZWlib.cpp
int LZWcompress(uint8_t* input, uint32_t inlen, uint16_t* output, uint32_t* widthjumps);
int LZWcompress9bit(uint8_t* input, uint32_t inlen, uint16_t* output);

#endif
