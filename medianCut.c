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

#include "medianCut.h"


void medianCut(SortedPixel* unique, uint32_t length, int tablebitsize){
    // Find optimal 256 color set using the median cut algorithm
    // We will be hard-coding 0x00 and 0xFF since they are so common, so really we want 254 colors
    // This requires first finding 256 colors, then figuring out which two to get rid of
    // Once finished, find the mean for each bin and set the pixels accordingly
    
    int tablesize = 1 << tablebitsize;  // = 2^tablebitsize
    
    SortedPixel* uniqueptr;
    
    CutBin bin[tablesize];
    bin[0].buffer = unique;
    bin[0].length = length;
    
    // If length <= tablesize, then we don't need to do any cutting, we just set each unique color with a colorindex and return
    if(length <= tablesize){
        for(int i=0;i<length;i++){
            unique[i].colorindex = i;
        }
        return;
    }
    
#if DEBUG
    printf("Doing median cut\n");
#endif
    
    // Loop over the cut sets, making 2^n cuts per loop
    for(int n=0;n<tablebitsize;n++){
        uint8_t exp = 1 << n;
        uint8_t step = 1 << (tablebitsize-n);
        uint8_t stepnext = 1 << (tablebitsize-1-n);
#if DEBUG
        printf("n=%d, exp=%d, step=%d, stepnext=%d\n", n, exp, step, stepnext);
#endif
        
        // Loop over the 2^n bins that exist and cut each, creating a new bin in a new location
        for(int i=0;i<exp;i++){
            
            // Perform the cut
            bin[i*step+stepnext] = splitBin(&bin[i*step]);
#if DEBUG
            printf("i*step+stepnext=%d, i*step=%d\n", i*step+stepnext, i*step);
#endif
        }
    }
    
    // Loop over all the bins and find the mean color and set the bin to that color
    for(int i=0;i<tablesize;i++){
#if DEBUG
        printf("bin #%i\n",i);
#endif
        uint32_t meanR = 0;
        uint32_t meanG = 0;
        uint32_t meanB = 0;
        uniqueptr = bin[i].buffer;
        
        // Find mean
        for(int j=0;j<bin[i].length;j++){
            meanR += uniqueptr->R;
            meanG += uniqueptr->G;
            meanB += uniqueptr->B;
            uniqueptr++;
        }
#if DEBUG
        printf("bin meanRGB=%i,%i,%i\n",meanR,meanG,meanB);
#endif
        meanR /= bin[i].length;
        meanG /= bin[i].length;
        meanB /= bin[i].length;
#if DEBUG
        printf("bin length=%i\n",bin[i].length);
        printf("bin color=%i,%i,%i\n",meanR,meanG,meanB);
#endif
        
        // Set mean as color for each entry in the bin
        uniqueptr = bin[i].buffer;
        for(int j=0;j<bin[i].length;j++){
            uniqueptr->pixel = (meanB << 16) + (meanG << 8) + (meanR << 0);
            uniqueptr->R = meanR;
            uniqueptr->G = meanG;
            uniqueptr->B = meanB;
            uniqueptr->colorindex = i;
            uniqueptr++;
        }
    }
    
}

CutBin splitBin(CutBin* bin){
    // Cut each bin at the median
    CutBin newbin;
    
    // Find ranges of each color
    getRange(bin);
    
    // Find the largest range (bias against B with R over G in tiebreaker)
    if((bin->maxR-bin->minR) >= (bin->maxG-bin->minG)){
        if((bin->maxR-bin->minR) >= (bin->maxB-bin->minB)){
            // R is largest
            sortR(bin->buffer, bin->length);
        }else{
            // B is largest
            sortB(bin->buffer, bin->length);
        }
    }else{
        if((bin->maxG-bin->minG) >= (bin->maxB-bin->minB)){
            // G is largest
            sortG(bin->buffer, bin->length);
        }else{
            // B is largest
            sortB(bin->buffer, bin->length);
        }
    }
    
    // Now split it in two
    newbin.length = bin->length/2;  // Integer division
    bin->length -= newbin.length;
    newbin.buffer = bin->buffer + bin->length;
    
    return newbin;
}

int comparefcnR(const void* first, const void* second){
    return (int)((SortedPixel*)first)->R - (int)((SortedPixel*)second)->R;
}

int comparefcnG(const void* first, const void* second){
    return ((SortedPixel*)first)->G - ((SortedPixel*)second)->G;
}

int comparefcnB(const void* first, const void* second){
    return ((SortedPixel*)first)->B - ((SortedPixel*)second)->B;
}

void getRange(CutBin* bin){
    
    // For each color, sort and then get its range
    
    sortR(bin->buffer, bin->length);
    bin->minR = bin->buffer[0].R;
    bin->maxR = bin->buffer[bin->length-1].R;
    
    sortG(bin->buffer, bin->length);
    bin->minG = bin->buffer[0].G;
    bin->maxG = bin->buffer[bin->length-1].G;
    
    sortB(bin->buffer, bin->length);
    bin->minB = bin->buffer[0].B;
    bin->maxB = bin->buffer[bin->length-1].B;
}

void sortR(SortedPixel* buffer, uint32_t length){
    qsort((void*)buffer, length, sizeof(SortedPixel), comparefcnR);
#if DEBUG
    printf("Sorted R (max, min): %i, %i\n", buffer[length-1].R, buffer[0].R);
#endif
}

void sortG(SortedPixel* buffer, uint32_t length){
    qsort((void*)buffer, length, sizeof(SortedPixel), comparefcnG);
#if DEBUG
    printf("Sorted G (max, min): %i, %i\n", buffer[length-1].G, buffer[0].G);
#endif
}

void sortB(SortedPixel* buffer, uint32_t length){
    qsort((void*)buffer, length, sizeof(SortedPixel), comparefcnB);
#if DEBUG
    printf("Sorted B (max, min): %i, %i\n", buffer[length-1].B, buffer[0].B);
#endif
}
