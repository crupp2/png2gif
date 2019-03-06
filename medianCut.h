
#ifndef _MEDIANCUT_H_
#define _MEDIANCUT_H_

#include "gifWriter.h"

void sortR(SortedPixel* buffer, uint32_t length);
void sortG(SortedPixel* buffer, uint32_t length);
void sortB(SortedPixel* buffer, uint32_t length);
void medianCut(SortedPixel* buffer, uint32_t length, int tablebitsize);
CutBin splitBin(CutBin* bin);
void getRange(CutBin* bin);

#endif
