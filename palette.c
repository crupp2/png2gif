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


void getP685gPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique){
    // 6-8-5 palette
    // 240 colors taken up by the color palette
    // Gray range is used for 15 of the remaining entries
    // The last entry is reserved for the transparent index
    
    SortedPixel* paletteptr;
    
    // Build palette
    uint8_t R[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    uint8_t G[8] = {0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
    uint8_t B[5] = {0x00, 0x40, 0x80, 0xbf, 0xff};
    
    int count = 0;
    paletteptr = palette;
    for(int i=0;i<6;i++){           // R
        for(int j=0;j<8;j++){       // G
            for(int k=0;k<5;k++){   // B
                paletteptr->pixel = (B[k] << 16) + (G[j] << 8) + (R[i] << 0);
                paletteptr->R = R[i];
                paletteptr->G = G[j];
                paletteptr->B = B[k];
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
    
    // Insert evenly distributed grays
    // Step by 0x10 (16) for 15 grays
    for(int i=1;i<16;i++){
        int value = 0x10*i;
        paletteptr->pixel = (value << 16) + (value << 8) + (value << 0);
        paletteptr->R = value;
        paletteptr->G = value;
        paletteptr->B = value;
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


void getP676gPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique){
    // 6-7-6 palette
    // 252 colors taken up by the color palette
    // Gray range is used for 3 of the remaining entries
    // The last entry is reserved for the transparent index
    
    SortedPixel* paletteptr;
    
    // Build palette
    uint8_t R[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    uint8_t G[7] = {0x00, 0x2a, 0x55, 0x80, 0xaa, 0xd4, 0xff};
    uint8_t B[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    
    int count = 0;
    paletteptr = palette;
    for(int i=0;i<6;i++){           // R
        for(int j=0;j<7;j++){       // G
            for(int k=0;k<6;k++){   // B
                paletteptr->pixel = (B[k] << 16) + (G[j] << 8) + (R[i] << 0);
                paletteptr->R = R[i];
                paletteptr->G = G[j];
                paletteptr->B = B[k];
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
    
    // Insert evenly distributed grays
    // Step by 0x40 (64) for 3 grays
    for(int i=1;i<16;i++){
        int value = 0x40*i;
        paletteptr->pixel = (value << 16) + (value << 8) + (value << 0);
        paletteptr->R = value;
        paletteptr->G = value;
        paletteptr->B = value;
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


void get884Palette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique){
    // 8-8-4 palette
    // All 256 entries are occupied by the color palette
    
    SortedPixel* paletteptr;
    
    // Build palette
    uint8_t R[8] = {0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
    uint8_t G[8] = {0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff};
    uint8_t B[4] = {0x00, 0x55, 0xaa, 0xff};
    
    int count = 0;
    paletteptr = palette;
    for(int i=0;i<8;i++){           // R
        for(int j=0;j<8;j++){       // G
            for(int k=0;k<4;k++){   // B
                paletteptr->pixel = (B[k] << 16) + (G[j] << 8) + (R[i] << 0);
                paletteptr->R = R[i];
                paletteptr->G = G[j];
                paletteptr->B = B[k];
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
}


void getWebPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique){
    // 6-6-6 web palette
    // 216 entries are occupied by the color palette
    // The rest of the entries are ignored
    
    SortedPixel* paletteptr;
    
    // Build palette
    uint8_t R[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    uint8_t G[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    uint8_t B[6] = {0x00, 0x33, 0x66, 0x99, 0xcc, 0xff};
    
    int count = 0;
    paletteptr = palette;
    for(int i=0;i<6;i++){           // R
        for(int j=0;j<6;j++){       // G
            for(int k=0;k<6;k++){   // B
                paletteptr->pixel = (B[k] << 16) + (G[j] << 8) + (R[i] << 0);
                paletteptr->R = R[i];
                paletteptr->G = G[j];
                paletteptr->B = B[k];
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


void getGrayPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique, int tablebitsize){
    // Gray palette
    // All colors taken up by the color palette
    // Number of colors depends on the table size
    
    SortedPixel* paletteptr;
    
    // Build palette
    int tablesize = 1 << tablebitsize;
    int tablejump = 0x100 >> (tablebitsize-1);
    int count = 0;
    paletteptr = palette;
    
    // Insert evenly distributed grays
    // Step by 0x100>>tablebitsize for 1<<tablebitsize grays
    for(int i=0;i<(tablesize-1);i++){
        int value = tablejump*i;
        paletteptr->pixel = (value << 16) + (value << 8) + (value << 0);
        paletteptr->R = value;
        paletteptr->G = value;
        paletteptr->B = value;
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
    
    // Add white at the end
    paletteptr->pixel = (0xff << 16) + (0xff << 8) + (0xff << 0);
    paletteptr->R = 0xff;
    paletteptr->G = 0xff;
    paletteptr->B = 0xff;
    paletteptr->residualR = 0;
    paletteptr->residualG = 0;
    paletteptr->residualB = 0;
    paletteptr->colorindex = count;
#if DEBUG
    printf("palettecolor[RGB] in bin: 0x%08x[0x%02x,0x%02x,0x%02x]\n",paletteptr->pixel,paletteptr->R,paletteptr->G,paletteptr->B);
    printf("tablesize=%i\n", tablesize);
#endif
}


void getColorPalette(SortedPixel* palette, SortedPixel* unique, uint32_t nunique, int tablebitsize, GIFOptStruct gifopts){
 
    switch(gifopts.colorpalette){
        case P685g:
            getP685gPalette(palette, unique, nunique);
            break;
        case P676g:
            getP676gPalette(palette, unique, nunique);
            break;
        case P884:
            get884Palette(palette, unique, nunique);
            break;
        case Pweb:
            getWebPalette(palette, unique, nunique);
            break;
        case Pgray:
            getGrayPalette(palette, unique, nunique, tablebitsize);
            break;
        case Pmedian:
        default:
            doMedianCut(palette, unique, nunique, tablebitsize, gifopts);
            break;
    }
}
