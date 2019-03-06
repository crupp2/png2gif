
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
