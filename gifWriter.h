
#ifndef _GIFWRITER_H_
#define _GIFWRITER_H_

#include <stdio.h>
#include <stdlib.h>

void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, uint16_t delay);
void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
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
