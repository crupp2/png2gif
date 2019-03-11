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

#include <stdio.h>
#include <math.h>

#include "dither.h"

#define DEBUG 0

void dither(SortedPixel* palette, int npalette, SortedPixel* frame, uint32_t width, uint32_t height){
    // Dither the image in frame of size width x height using color table in unique of size nunique
    // Use non-serpentine Floyd-Steinberg dithering
    
    // Pseudo code from https://en.wikipedia.org/wiki/Floyd–Steinberg_dithering
//    for each y from top to bottom
//        for each x from left to right
//            oldpixel  := pixel[x][y]
//            newpixel  := find_closest_palette_color(oldpixel)
//            pixel[x][y]  := newpixel
//            quant_error  := oldpixel - newpixel
//            pixel[x + 1][y    ] := pixel[x + 1][y    ] + quant_error * 7 / 16
//            pixel[x - 1][y + 1] := pixel[x - 1][y + 1] + quant_error * 3 / 16
//            pixel[x    ][y + 1] := pixel[x    ][y + 1] + quant_error * 5 / 16
//            pixel[x + 1][y + 1] := pixel[x + 1][y + 1] + quant_error * 1 / 16
    
    SortedPixel oldpixel, newpixel;
    uint32_t ind;
    float errorR, errorG, errorB;
    
#if DEBUG
    printf("npalette=%i\n",npalette);
    printf("palette[0] RGB=0d{%i,%i,%i}\n", palette[0].R, palette[0].G, palette[0].B);
    printf("palette[1] RGB=0d{%i,%i,%i}\n", palette[1].R, palette[1].G, palette[1].B);
#endif

    for(uint32_t j=0; j<height; j++){
        for(uint32_t i=0; i<width; i++){
            
            // Get pixel
            oldpixel = frame[j*width+i];
#if DEBUG
            printf("Pixel colorRGB=0d{%i,%i,%i}\n", oldpixel.R, oldpixel.G, oldpixel.B);
            printf("Pixel propagated colorRGB={%f,%f,%f}\n", oldpixel.residualR + (float)oldpixel.R, oldpixel.residualG + (float)oldpixel.G, oldpixel.residualB + (float)oldpixel.B);
#endif
            
            // Find closest color
            ind = findClosestColor(palette, npalette, oldpixel);
            newpixel = palette[ind];
#if DEBUG
            printf("dist=%f\n", newpixel.residualR);
            printf("Selected pixel colorRGB=0d{%i,%i,%i}\n", newpixel.R, newpixel.G, newpixel.B);
#endif
            
            // Get quantization error for each color
            //FIXME: This needs to include quantization error, also need to watch for negative overflow
            errorR = oldpixel.residualR + (float)oldpixel.R - (float)newpixel.R;
            errorG = oldpixel.residualG + (float)oldpixel.G - (float)newpixel.G;
            errorB = oldpixel.residualB + (float)oldpixel.B - (float)newpixel.B;
#if DEBUG
            printf("Pixel errorRGB=0d{%i,%i,%i}\n", oldpixel.R - newpixel.R, oldpixel.G - newpixel.G, oldpixel.B - newpixel.B);
            printf("Propagated errorRGB={%f,%f,%f}\n", errorR, errorG, errorB);
#endif
            
            // Set pixel color to the closest color
            oldpixel.colorindex = newpixel.colorindex;
            frame[j*width+i] = oldpixel;
            
            // Distribute quantization error to other pixels
            // Pixel to right
            if(i < (width-1)){
                frame[j*width+i+1].residualR += errorR * 7 / 16;
                frame[j*width+i+1].residualG += errorG * 7 / 16;
                frame[j*width+i+1].residualB += errorB * 7 / 16;
            }
            if(j < (height-1)){
                // Pixel below and left
                if(i > 0){
                    frame[(j+1)*width+i-1].residualR += errorR * 3 / 16;
                    frame[(j+1)*width+i-1].residualG += errorG * 3 / 16;
                    frame[(j+1)*width+i-1].residualB += errorB * 3 / 16;
                }
                
                // Pixel below
                frame[(j+1)*width+i].residualR += errorR * 5 / 16;
                frame[(j+1)*width+i].residualG += errorG * 5 / 16;
                frame[(j+1)*width+i].residualB += errorB * 5 / 16;
                
                // Pixel below and right
                if(i < (width-1)){
                    frame[(j+1)*width+i+1].residualR += errorR * 1 / 16;
                    frame[(j+1)*width+i+1].residualG += errorG * 1 / 16;
                    frame[(j+1)*width+i+1].residualB += errorB * 1 / 16;
                }
            }
            
        }  // for i
    }  // for j
    
}

int comparefcn_residualR(const void* first, const void* second){
    float diff = ((SortedPixel*)first)->residualR - ((SortedPixel*)second)->residualR;
    if(diff > 0){
        return 1;
    }else if(diff < 0){
        return -1;
    }else{
        return 0;
    }
}

uint32_t findClosestColor(SortedPixel* palette, int npalette, SortedPixel pixel){
    // Returns the color index of the color table color closest to the color of pixel
    // This algorithm is crazy slow
    
    float R, G, B;
    
    R = (float)pixel.R + pixel.residualR;
    G = (float)pixel.G + pixel.residualG;
    B = (float)pixel.B + pixel.residualB;
    
    // Find the distance to all palette entries
    // Store the distance in palette.residualR since it is not being used
    float dist;
    for(int i=0; i<npalette; i++){
        // Unnecessary to take the sqrt since it is equally applied to all entries
//        dist = sqrt(powf(R-(float)palette[i].R, 2) + powf(G-(float)palette[i].G, 2) + powf(B-(float)palette[i].B, 2));
        dist = powf(R-(float)palette[i].R, 2) + powf(G-(float)palette[i].G, 2) + powf(B-(float)palette[i].B, 2);
        palette[i].residualR = dist;
    }
    
    // Sort the palette by the distance, which should all be >= 0
    qsort((void*)palette, npalette, sizeof(SortedPixel), comparefcn_residualR);
    
    // After sorting, the closest pixel will always be the first
    return 0;
}
