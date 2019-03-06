
#include "dither.h"

void dither(SortedPixel* unique, int nunique, SortedPixel* frame, uint32_t width, uint32_t height){
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

    for(uint32_t j=0; j<height; j++){
        for(uint32_t i=0; i<width; i++){
            
            // Get pixel
            oldpixel = frame[j*width+i];
            
            // Find closest color
            ind = findClosestColor(unique, nunique, oldpixel);
            newpixel = unique[ind];
            
            // Get quantization error for each color
            //FIXME: This needs to include quantization error, also need to watch for negative overflow
            errorR = oldpixel.residualR + (float)oldpixel.R - (float)newpixel.R;
            errorG = oldpixel.residualG + (float)oldpixel.G - (float)newpixel.G;
            errorB = oldpixel.residualB + (float)oldpixel.B - (float)newpixel.B;
            
            // Set pixel color to the closest color
            oldpixel.colorindex = newpixel.colorindex;
            
            // Distribute quantization error to other pixels
            // Pixel to right
            if(i < (width-1)){
                oldpixel = frame[j*width+i+1];
                oldpixel.residualR += errorR * 7 / 16;
                oldpixel.residualG += errorG * 7 / 16;
                oldpixel.residualB += errorB * 7 / 16;
            }
            if(j < (height-1)){
                // Pixel below and left
                if(i > 0){
                    oldpixel = frame[(j+1)*width+i-1];
                    oldpixel.residualR += errorR * 3 / 16;
                    oldpixel.residualG += errorG * 3 / 16;
                    oldpixel.residualB += errorB * 3 / 16;
                }
                
                // Pixel below
                oldpixel = frame[(j+1)*width+i];
                oldpixel.residualR += errorR * 5 / 16;
                oldpixel.residualG += errorG * 5 / 16;
                oldpixel.residualB += errorB * 5 / 16;
                
                // Pixel below and right
                if(i < (width-1)){
                    oldpixel = frame[(j+1)*width+i+1];
                    oldpixel.residualR += errorR * 1 / 16;
                    oldpixel.residualG += errorG * 1 / 16;
                    oldpixel.residualB += errorB * 1 / 16;
                }
            }
            
        }  // for i
    }  // for j
    
}

uint32_t findClosestColor(SortedPixel* unique, int nunique, SortedPixel pixel){
    return 0;
}
