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

#include <stdlib.h>
#include <math.h>
#include "pixel.h"


//int comparefcn_residualR(const void* first, const void* second){
//    float diff = ((SortedPixel*)first)->residualR - ((SortedPixel*)second)->residualR;
//    if(diff > 0){
//        return 1;
//    }else if(diff < 0){
//        return -1;
//    }else{
//        return 0;
//    }
//}

uint32_t findClosestColor(SortedPixel* palette, int npalette, SortedPixel pixel){
    // Returns the color index of the color table color closest to the color of pixel
    // This algorithm is crazy slow
    
    float R, G, B;
    
    R = (float)pixel.R + pixel.residualR;
    G = (float)pixel.G + pixel.residualG;
    B = (float)pixel.B + pixel.residualB;
    
    // Find the distance to all palette entries
    // Store the distance in palette.residualR since it is not being used
    float dist, closestDist = 0x7fffffff;  // Initialize to max float
    int closestIndex = 0;
    for(int i=0; i<npalette; i++){
        // Unnecessary to take the sqrt since it is equally applied to all entries
        //        dist = sqrt(powf(R-(float)palette[i].R, 2) + powf(G-(float)palette[i].G, 2) + powf(B-(float)palette[i].B, 2));
        dist = powf(R-(float)palette[i].R, 2) + powf(G-(float)palette[i].G, 2) + powf(B-(float)palette[i].B, 2);
//        palette[i].residualR = dist;
        if(dist < closestDist){
            closestDist = dist;
            closestIndex = i;
        }
    }
    
    // Sort the palette by the distance, which should all be >= 0
//    qsort((void*)palette, npalette, sizeof(SortedPixel), comparefcn_residualR);
    
    // After sorting, the closest pixel will always be the first
    return closestIndex;
}
