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

#include <string.h>
#include "palette.h"
#include "medianCut.h"

#define DEBUG 0

void get884Palette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique){
    
    SortedPixel* paletteptr;
    
    // Build palette
    uint8_t P1[8] = {0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};   // R, G
    uint8_t P2[4] = {0x00, 0x55, 0xaa, 0xff};                           // B
    
    int count = 0;
    paletteptr = palette;
    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            for(int k=0;k<4;k++){
                paletteptr->pixel = (P2[k] << 16) + (P1[j] << 8) + (P1[i] << 0);
                paletteptr->R = P1[i];
                paletteptr->G = P1[j];
                paletteptr->B = P2[k];
                paletteptr->residualR = 0;
                paletteptr->residualG = 0;
                paletteptr->residualB = 0;
                paletteptr->colorindex = count;
#if DEBUG
                printf("palettecolor[RGB] in bin: 0x%08x[0x%02x,0x%02x,0x%02x]\n",paletteptr->pixel,paletteptr->R,paletteptr->G,paletteptr->B);
#endif
                paletteptr++;
                count++;
            }
        }
    }
    
    // Plod through unique and find the nearest pixel in the palette
    int ind = findClosestColor(palette, 256, unique[0]);
    unique[0].colorindex = ind;
    for(int i=0;i<nunique;i++){
        if(unique[i].colorindex == unique[i-1].colorindex){
            continue;
        }else{
            ind = findClosestColor(palette, 256, unique[i]);
            unique[i].colorindex = ind;
    #if DEBUG
            printf("#pixels(color#)[color]{palette#} in bin: %i(%i)[0x%08x]{0x%08x}\n",unique[i].npixel,i,unique[i].pixel,ind);
    #endif
        }
    }
}

void doMedianCut(SortedPixel* palette, SortedPixel* unique, uint32_t nunique, int tablebitsize, GIFOptStruct gifopts){
    
    // Shrink the color pallete to an optimal set via median cut
    medianCut(unique, nunique, tablebitsize);
    
    // If requested then force black and white to the color table
    // Find the closest colors overwrite them
    // Only do this if using the median cut algorithm
    if(gifopts.forcebw > 0){
#if DEBUG
        printf("nunique=%i\n", nunique);
        printf("tablebitsize=%i\n", tablebitsize);
#endif
        SortedPixel pix;
        pix.residualR = 0;
        pix.residualG = 0;
        pix.residualB = 0;
        uint32_t ind;
        
        // Find closest to black
        pix.R = 0;
        pix.G = 0;
        pix.B = 0;
        ind = findClosestColor(unique, nunique, pix);
        uint8_t blackcolorindex = unique[ind].colorindex;
        
        // Find closest to white
        pix.R = 0xff;
        pix.G = 0xff;
        pix.B = 0xff;
        ind = findClosestColor(unique, nunique, pix);
        uint8_t whitecolorindex = unique[ind].colorindex;
        
        // Now apply that color to all unique colors that have the same colorindex
        for(int i=0;i<nunique;i++){
            if(unique[i].colorindex == blackcolorindex){
                unique[i].R = 0;
                unique[i].G = 0;
                unique[i].B = 0;
                unique[i].pixel = 0;
            }else if(unique[i].colorindex == whitecolorindex){
                unique[i].R = 0xff;
                unique[i].G = 0xff;
                unique[i].B = 0xff;
                unique[i].pixel = 0xffffff;
            }
        }
    }

    // Put colors into palette
    int count = 1;
    // Write first color
    memcpy(&(palette[0].pixel), &(unique[0].pixel), 3);
    for(int i=1;i<nunique;i++){
        if(unique[i].colorindex == unique[i-1].colorindex){
            continue;
        }else{
#if DEBUG
            printf("#pixels(#colors)[color] in bin: %i(%i)[0x%08x]\n",unique[i].npixel,i,unique[i].pixel);
#endif
//            palette[count].pixel = unique[i].pixel;
            memcpy(&(palette[count].pixel), &(unique[i].pixel), 3);
            count++;
        }
    }
#if DEBUG
    printf("#pixels(#colors)[color]  in bin: %i(%i)[0x%08x]\n",unique[nunique-1].npixel,nunique,unique[nunique].pixel);
#endif
    
    // Fill in rest of palette with 0x0, if necessary
    int tablesize = 1 << tablebitsize;
    while(count < tablesize){
        palette[count].pixel = 0;
        count++;
    }
}


void getColorPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique, int tablebitsize, GIFOptStruct gifopts){
 
    switch(gifopts.colorpalette){
        case P685g:
//            break;
        case P676g:
//            break;
        case P884:
            get884Palette(palette, unique, nunique);
            break;
        case Pweb:
//            break;
        case Pgray:
//            break;
        case Pmedian:
        default:
            doMedianCut(palette, unique, nunique, tablebitsize, gifopts);
            break;
    }
}
