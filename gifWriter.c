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

#include "gifWriter.h"
#include "dither.h"
#include "palette.h"

#define MAXCODESIZE 12
#define DEBUG 0


// Corresponds to definition in gifWriter.h: enum _Palettes {P685g, P676g, P884, Pweb, Pmedian, Pgray, PgrayT};
const int _Palette_nbits[] = {8, 8, 8, 8, 0, 0, 8};
const int _Palette_size[] = {255, 255, 256, 216, 0, 0, 255};

GIFOptStruct newGIFOptStructInst(){
    // Set defaults
    GIFOptStruct gifopts;
    
    gifopts.delay = 25;
    gifopts.dither = 0;
    gifopts.colorpalette = P685g;
    gifopts.colortablebitsize = 0;
    gifopts.forcebw = 0;
    gifopts.palette = malloc(sizeof(SortedPixel)*256);  // This leaks, but is used until program exit
    memset(gifopts.palette, 0, sizeof(SortedPixel)*256);
    
    return gifopts;
}

// Write the color palette
void writeColorPalette(FILE* fid, SortedPixel* palette, int tablesize){
    // Put into a temporary array and then write to file in one big chunk
    uint8_t frame[3*tablesize];
    uint8_t* frameptr = frame;
    for(int i=0;i<tablesize;i++){
        memcpy(frameptr, &(palette[i].pixel), 3);
        frameptr += 3;
    }
    fwrite(frame, sizeof(uint8_t), 3*tablesize, fid);
}

// Set transparent indices
void setTransparent(uint8_t* frame, uint8_t* lastframe, uint32_t npixel){
    for(int i=0;i<npixel;i++){
        if(*frame == *lastframe){
            *frame = 0xff;  // This is the transparent index for all palettes
        }
        frame++;
        lastframe++;
    }
}

void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, GIFOptStruct gifopts){

    uint8_t head[] = "\x47\x49\x46\x38\x39\x61";
    
    printf("Writing gif header\n");

    // Error checking
    if (width == 0 || height == 0){
    printf("Error: Image has zero width or height.\n");
    exit(-1);
    }
    
    // Write gif header
    fwrite(head, 6, 1, fid);
    
    // Write Logical screen descriptor
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, sizeof(uint16_t), 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, sizeof(uint16_t), 1, fid);
    
    // If using a global color table then create it and write it here
    // Pmedian and Pgray do not do this because they are variable size
    if(_Palette_nbits[gifopts.colorpalette] != 0){
        // Get the color palette
        getColorPalette(gifopts.palette, NULL, 0, 8, gifopts);
        
        // Write the palette to the global color table
        // Technically the table size doesn't have to be 256, this code should be fixed if a future table is not
        fputc('\xF7', fid);  // global color table is size 768 (256 RGB colors)
        fputc('\x00', fid);  // Background color is pixel #0
        fputc('\x00', fid);  // No pixel aspect ratio
        writeColorPalette(fid, gifopts.palette, 256);
    }else{
        // No global color table (well, only black and white are defined, but we use a local color table so it doesn't matter)
        fputc('\xF0', fid);  // global color table is size 6 (2 RGB colors)
        fputc('\x00', fid);  // Background color is pixel #0
        fputc('\x00', fid);  // No pixel aspect ratio
        fwrite("\xFF\xFF\xFF\x00\x00\x00", 6, 1, fid);
    }
}
    
void writeGIFAppExtension(FILE* fid){

    // Write application extension block to enable animation looping. Should only be written if the file is an animation (i.e., more than one frame supplied)
    fwrite("\x21\xFF\x0B\x4E\x45\x54\x53\x43\x41\x50\x45\x32\x2E\x30\x03\x01\x00\x00\x00", 19, 1, fid);
}

void writeGIFFrame(FILE* fid, uint8_t* frame, uint8_t* lastframe, uint32_t width, uint32_t height, GIFOptStruct gifopts, int isFirstFrame){

#if DEBUG
    printf("Writing gif local image descriptor\n");
    printf("width=%d, height=%d\n", width, height);
#endif

    // Error checking
    if (width == 0 || height == 0){
    printf("Error: Image has zero width or height.\n");
    exit(-1);
    }
    
    // Write graphics control extension block
    fwrite("\x21\xF9\x04", 3, 1, fid);
    // Write the packed byte
    // Set transparent color flag if palette has a transparent index
    switch (gifopts.colorpalette){
        case P685g:
        case P676g:
        case Pweb:
        case PgrayT:
            fwrite("\x05", 1, 1, fid);
            break;
        default:
            fwrite("\x04", 1, 1, fid);
            break;
    }
    // Write delay time
    fwrite(&gifopts.delay, sizeof(uint16_t), 1, fid);
    // Write the transparent color index (always at 0xff)
    // Setting this regardless because the transparent color flag determines whether it is used
    fwrite("\xff", 1, 1, fid);
    // Write the block terminator
    fwrite("\x00", 1, 1, fid);
    
    // Write local image descriptor
    fputc('\x2C', fid);
    fwrite("\x00\x00\x00\x00", 4, 1, fid);
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, 2, 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, 2, 1, fid);
    // Wait to write the packed byte until after we know the minimum table size
    
    // Write local color table (if necessary) and palettize the image
    int tablebitsize = writeGIFLCT(fid, frame, lastframe, width, height, gifopts, isFirstFrame);
    
    // Write image data
    printf("Writing gif frame data\n");
    writeGIFImageCompressed(fid, frame, width, height, tablebitsize);
//    writeGIFImageCompressed9bit(fid, frame, width, height);
//    writeGIFImageUncompressed256(fid, frame, width*height);
//    writeGIFImageUncompressed128(fid, frame, width*height);
    
}

void writeGIFImageCompressed(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, int tablebitsize){
    
    printf("Writing compressed frame\n");
    
    uint32_t length = width*height;
    
    uint8_t startnbits = tablebitsize+1;  // Start the LZW table at 9 bit codes
    if(startnbits < 3){
        startnbits = 3;
    }
    uint16_t clearcode = 1 << (startnbits-1);
    uint16_t stopcode = clearcode + 1;
#if DEBUG
    printf("startnbits=%i\n", startnbits);
    printf("clearcode=0x%x\n", clearcode);
    printf("stopcode=0x%x\n", stopcode);
#endif
    
    uint32_t widthjumps[10] = {0,0,0,0,0,0,0,0,0,0};  // Location in number of codes where code width increases by one (room for jumps from 2 to 12)
    uint16_t* buffer = malloc(sizeof(uint16_t)*length);  // frame size
    uint8_t* output = malloc(sizeof(uint8_t)*2*length);    // Don't really know the final size after compression, so let's guess 2*npixel
    uint8_t* frameptr;
    uint16_t* bufferptr;
    uint8_t* outputptr;
    frameptr = frame;
    
    // Write the LZW minimum code size byte
    putc(startnbits-1, fid);
    
    // Compress the frame with LZW
    // Compression occurs until the LZW table is full, then it needs to be started again
    int ncodes = 0;
    int packedn = 0;
    int ret = 0;
    int startshift = 0;
    uint64_t remain = 0;
    int islast = 0;
    uint32_t inlen = sizeof(uint8_t)*width*height;
    
    // Start the compressed codes with a clear code
    buffer[0] = clearcode;
    ret++;
    ncodes++;
    bufferptr = buffer+1;
    
    while(islast == 0){
#if DEBUG
        printf("bufferptr=0x%x\n", bufferptr);
#endif
        ret += LZWcompress(&frameptr, &inlen, &bufferptr, widthjumps, startnbits);
        ncodes += ret;
#if DEBUG
        printf("frameptr=0x%x\n", frameptr);
        printf("inlen=%i\n", inlen);
        printf("ncodes=%i\n", ncodes);
        printf("widthjumps={%i,%i,%i,%i,%i,%i,%i,%i,%i,%i}\n",widthjumps[0],widthjumps[1],widthjumps[2],widthjumps[3]
               ,widthjumps[4],widthjumps[5],widthjumps[6],widthjumps[7]
               ,widthjumps[8],widthjumps[9]);
#endif
        
        if(inlen == 0){
            // End table with stop code
#if DEBUG
            printf("End of codes reached, adding stop code\n");
#endif
            *bufferptr = stopcode;
            ret++;
            ncodes++;
            islast = 1;
        }else{
            // Reset table with clear code
#if DEBUG
            printf("*bufferptr=0x%x\n", *bufferptr);
#endif
            *bufferptr = clearcode;
#if DEBUG
            printf("bufferptr=0x%x\n", bufferptr);
#endif
            ret++;
            ncodes++;
        }
                   
        bufferptr = buffer;
        
        // Pack all codes into bytes
        packedn += packLSB(bufferptr, &output[packedn], ret, startnbits, &remain, &startshift, widthjumps, islast);
        
        // Reset
        bufferptr = buffer;
        ret = 0;
    }
    
    // Write chunks of encoded bytes
    int chunksize = 254;
    outputptr = output;
    while(packedn > chunksize){
        putc((uint8_t) chunksize, fid);  // Number of bytes in data chunk
        fwrite(outputptr, 1, chunksize, fid);
        outputptr += chunksize;
        packedn -= chunksize;
#if DEBUG
        printf("packedn=%i\n", packedn);
#endif
    }
    
    // Write the remainder
    // Include signal for last data chunk
    fwrite(&packedn, 1, 1, fid);  // Number of bytes in data chunk
    fwrite(outputptr, 1, packedn, fid);
    
    // Write signal for last data chunk
    putc('\x00', fid);
    
    // Free allocated memory
    free(buffer);
    free(output);
    
}

void writeGIFImageCompressed9bit(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height){
    
    printf("Writing compressed frame\n");
    
    uint32_t length = width*height;
    
    uint16_t* buffer = malloc(sizeof(uint16_t)*length);  // frame size
    uint8_t* output = malloc(sizeof(uint8_t)*length);    // frame size
    uint8_t* frameptr;
    uint16_t* bufferptr;
    uint8_t* outputptr;
    frameptr = frame;
    
    // Write the LZW minimum code size byte
    putc(8, fid);
    
    // Compress the frame with LZW
    int n = 1;
    buffer[0] = 0x100;  // LZW table clear
    // Full frame through LZW
    n += LZWcompress9bit(frame, sizeof(uint8_t)*width*height, &buffer[1]);
    printf("n=%i\n", n);
    buffer[n] = 0x101;  // LZW table end
    n++;
    
    bufferptr = buffer;
    
    // convert 9-bit codes to bytes
    n = convert9to8(bufferptr, output, n);
    outputptr = output;

    int chunksize = 255;
    while(n > chunksize){
        putc((uint8_t) chunksize, fid);  // Number of bytes in data chunk
        fwrite(outputptr, 1, chunksize, fid);
        outputptr += chunksize;
        n -= chunksize;
        printf("n=%i\n", n);
    }
    
    // Write the remainder
    fwrite(&n, 1, 1, fid);  // Number of bytes in data chunk
    fwrite(outputptr, 1, n, fid);
    
    // Write signal for last data chunk
    putc('\x00', fid);
    
    // Free allocated memory
    free(buffer);
    free(output);
}

void writeGIFImageUncompressed256(FILE* fid, uint8_t* frame, uint32_t length){
    
    printf("Writing uncompressed 256 color frame\n");
    
    uint16_t* buffer = malloc(sizeof(uint16_t)*256);  // color table size
    uint8_t* output = malloc(sizeof(uint8_t)*256);  // color table size
    uint8_t* frameptr;
    
    int n = length;
    frameptr = frame;
    
    // Write the LZW minimum code size byte
    putc('\x08', fid);
    
    // Copy the buffer into ?? 9-bit codes that will fit into chunksize byte chunks
    int chunksize = 252;  // 254 bytes is the max to prevent LZW table increase with an 8-bit code
    int ncodes = (8*chunksize)/9;  // 8*chunksize bits available will be enough space for ncodes 9-bit codes.
    while(n > ncodes){
        putc((uint8_t) chunksize, fid);  // Number of bytes in data chunk
        // Copy frame bytes into 16-bit buffer, including LZW table clear code
        buffer[0] = 0x100;  // LZW table clear
        for(int i=1;i<ncodes;i++){
            buffer[i] = (uint16_t) *frameptr++;
        }
        // Convert 9-bit codes into bytes
        convert9to8(buffer, output, ncodes);
        
        // Write bytes to file
        fwrite(output, 1, chunksize, fid);
        n -= (ncodes-1);
    }
    
    // Write the remainder, truncating n to a uint8_t
    // Include signal for last data chunk
    uint8_t rem = (uint8_t) (n+1);
    buffer[0] = 0x100;  // LZW table clear
    for(int i=1;i<(rem+1);i++){
        buffer[i] = (uint16_t) *frameptr++;
    }
    buffer[rem+1] = 0x101;  // LZW table end
    // Convert 9-bit codes into bytes
    n = convert9to8(buffer, output, rem+1);
    fwrite(&n, 1, 1, fid);  // Number of bytes in data chunk
    fwrite(output, 1, n, fid);
    
    // Write signal for last data chunk
    putc('\x00', fid);
    
    // Free allocated memory
    free(buffer);
    free(output);
    
}

void writeGIFImageUncompressed128(FILE* fid, uint8_t* frame, uint32_t length){
    
    printf("Writing uncompressed 128 color frame\n");
    
    int n = length;
    
    // Write the LZW minimum code size byte
    putc('\x07', fid);
    
    // Write the frame in chunksize byte chunks
    int chunksize = 126;  // 126 is the max to prevent LZW table increase with an 8-bit code
    int pos = 0;
    while(n > chunksize){
        putc((uint8_t) (chunksize+1), fid);  // Number of bytes in data chunk
        putc('\x80', fid);  // LZW table clear
        fwrite(&frame[pos], 1, chunksize, fid);
        pos += chunksize;
        n -= chunksize;
    }
    
    // Write the remainder, truncating n to a uint8_t
    uint8_t rem = (uint8_t) (n+1);
    fwrite(&rem, 1, 1, fid);  // Number of bytes in data chunk
    putc('\x80', fid);  // LZW table clear
    fwrite(&frame[pos], 1, n, fid);
    
    // Write signal for last data chunk
    putc('\x01', fid);  // One byte in this chunk
    putc('\x81', fid);  // LZW table end
    putc('\x00', fid);
    
}

int comparefcn_pixel(const void* first, const void* second){
    return ((SortedPixel*)first)->pixel - ((SortedPixel*)second)->pixel;
}

int comparefcn_sortind(const void* first, const void* second){
    return ((SortedPixel*)first)->sortedindex - ((SortedPixel*)second)->sortedindex;
}

int comparefcn_frameind(const void* first, const void* second){
    return ((SortedPixel*)first)->frameindex - ((SortedPixel*)second)->frameindex;
}

int comparefcn_colorind(const void* first, const void* second){
    return ((SortedPixel*)first)->colorindex - ((SortedPixel*)second)->colorindex;
}

uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint8_t* lastframe, uint32_t width, uint32_t height, GIFOptStruct gifopts, int isFirstFrame){
    // lastframe contains the indices from the previous frame after it has been run through this code
    // For the first frame, lastframe has been initialized to zeros, so all indices are equal to the background color
    
    SortedPixel* buffer;// = malloc(sizeof(SortedPixel)*npixel);
    SortedPixel* unique;//[npixel];
    SortedPixel* bufferptr;
    uint8_t* frameptr;
    uint32_t npixel = width*height;
    
#if DEBUG
    printf("Writing gif local color table\n");
    printf("npixel=%d\n", npixel);
#endif
    
    // Set buffer array to zeros
    buffer = malloc(sizeof(SortedPixel)*npixel);
    unique = malloc(sizeof(SortedPixel)*npixel);
    memset(buffer, 0, sizeof(SortedPixel)*npixel);
    memset(unique, 0, sizeof(SortedPixel)*npixel);  // Necessary to prevent junk at the end of the color table if there are fewer unique colors than the length of the color table. Also for zeroing out residual for dithering.
    
    // Set pointers
    bufferptr = buffer;
    frameptr = frame;
    
    // Copy frame data into buffer array
    for(int i=0;i<npixel;i++){
        memcpy(&(bufferptr->pixel), frameptr, 3);  // Copy three RGB bytes into uint32_t pixel
        bufferptr->R = *frameptr++;
        bufferptr->G = *frameptr++;
        bufferptr->B = *frameptr++;
        bufferptr->frameindex = i;
        bufferptr++;
    }
    
    // Sort the buffer by the pixel color
    bufferptr = buffer;
    qsort((void*)bufferptr, npixel, sizeof(SortedPixel), comparefcn_pixel);

    // Find unique entries and number of each
    memcpy(&unique[0], &buffer[0], sizeof(SortedPixel));
    int nunique = 0;
#if DEBUG
    printf("unique #%i:RGB=%i,%i,%i\n",nunique,buffer[0].R,buffer[0].G,buffer[0].B);
#endif
    
    unique[nunique].npixel++;
    for(int i=1;i<npixel;i++){
        buffer[i].sortedindex = i;
        if(buffer[i].pixel == unique[nunique].pixel){
            unique[nunique].npixel++;
            continue;
        }else{
            nunique++;
            memcpy(&unique[nunique], &buffer[i], sizeof(SortedPixel));
            unique[nunique].npixel++;
#if DEBUG
            printf("unique #%i:RGB=%i,%i,%i\n",nunique,buffer[i].R,buffer[i].G,buffer[i].B);
#endif
        }
        buffer[i].colorindex = nunique;
        unique[i].colorindex = nunique;
    }
    nunique++;
    
    // Find the minimum table size
    // This can either be set externally or programmatically found by the number of unique entries
    // Certain color palettes will dictate what this value is
    int tablebitsize = _Palette_nbits[gifopts.colorpalette];
    int tablesize = 1 << tablebitsize;
    
    // Find size programatically if necessary
    if(tablebitsize == 0){
        // Size defined on command line
        if(gifopts.colortablebitsize > 0){
            if(gifopts.colortablebitsize > 8){
                printf("Error: too many defined colors using colortablebitsize=%i. Value must be 1 <= x <= 8. Exiting.\n", gifopts.colortablebitsize);
            }
            tablebitsize = gifopts.colortablebitsize;
        }else{
            // Find size programatically
            tablebitsize = 1;
            tablesize = 1 << tablebitsize;
            while(nunique > tablesize){
                if(tablebitsize >= 8){
                    break;
                }
                tablebitsize++;
                tablesize = 1 << tablebitsize;
            }
        }
    }
    tablesize = 1 << tablebitsize;
#if DEBUG
    printf("tablesize=%i\n",tablesize);
    printf("tablebitsize=%i\n",tablebitsize);
#endif
    
    // Write packed byte of the local image descriptor before writing the local color table
    // Need to do this here because we only just found the minimum size of the table
    // Note that the documentation at https://www.fileformat.info/format/gif/egff.htm is wrong and the packed byte for the local color table looks like the packed byte for the global color table
    // Only use a local color table is using Pmedian or Pgray, otherwise set size to zero
    uint8_t packedbyte;
    if(_Palette_nbits[gifopts.colorpalette] == 0){
        packedbyte = (1 << 7) + (tablebitsize-1);
    }else{
        packedbyte = 0x00;
    }
    fputc(packedbyte, fid);
    
    // Get the color palette if not yet defined (i.e., for Pmedian or Pgray)
    if(_Palette_nbits[gifopts.colorpalette] == 0){
        getColorPalette(gifopts.palette, unique, nunique, tablebitsize, gifopts);
    }
    
    // Palettize the unique colors (except for Pmedian and Pgray)
    if(_Palette_size[gifopts.colorpalette] != 0){
        palettizeColors(gifopts.palette, _Palette_size[gifopts.colorpalette], unique, nunique);
    }
    // Special palettizing handling for Pgray
    if(gifopts.colorpalette == Pgray){
        palettizeColors(gifopts.palette, tablesize, unique, nunique);
    }
    
    // Write the color palette (only if using Pmedian or Pgray)
    if(_Palette_nbits[gifopts.colorpalette] == 0){
        writeColorPalette(fid, gifopts.palette, tablesize);
    }
    
    // Re-sort unique into sorted state, same as buffer still is
    bufferptr = unique;
    qsort((void*)bufferptr, nunique, sizeof(SortedPixel), comparefcn_sortind);
    
    // Set buffer.colorindex based on unique entries
    // Only necessary if not doing dithering
    bufferptr = buffer;
    for(int i=0;i<nunique;i++){
        for(int j=0;j<unique[i].npixel;j++){
            bufferptr->colorindex = unique[i].colorindex;
            bufferptr++;
        }
    }
    
    // Sort the buffer to its original frame order
    bufferptr = buffer;
    qsort((void*)bufferptr, npixel, sizeof(SortedPixel), comparefcn_frameind);
    
    // Dither the image based on the smaller color palette
    if(gifopts.dither > 0){
        // Compress unique down to the color table size to speed up dithering
#if DEBUG
        printf("nunique=%i\n", nunique);
#endif
        // First sort unique by colorindex
        qsort((void*)unique, nunique, sizeof(SortedPixel), comparefcn_colorind);
        
        // Now compress unique
        int count = 0;
        for(int i=1;i<nunique;i++){
            if(unique[i].colorindex == unique[count].colorindex){
                continue;
            }else{
                count++;
                unique[count] = unique[i];
            }
        }
        nunique = count+1;
        
        // Make sure that the updated nunique is the size of the color table or less
#if DEBUG
        printf("nunique=%i\n", nunique);
#endif
        if(nunique > tablesize){
            printf("Error in preprocessing for dithering. Exiting.\n");
            exit(-1);
        }
        
        // Do the dithering
        printf("Dithering the frame\n");
        dither(unique, nunique, buffer, width, height);
    }
    
    // Store image indices in frame
    frameptr = frame;
    for(int i=0;i<npixel;i++){
        *frameptr++ = buffer[i].colorindex;
    }
    
    // If using a palette with a transparent index, replace indices that are equal to the last frame with the transparent index
    // Only do this if it is not the first frame
    if(isFirstFrame == 0){
        switch (gifopts.colorpalette){
            case P685g:
            case P676g:
            case Pweb:
            case PgrayT:
                setTransparent(frame, lastframe, npixel);
                break;
            default:
                // Do nothing
                break;
        }
    }
    
    // Free allocated variables
    free(unique);
    free(buffer);
    
    // Return the size of the color table in number of bits
    return tablebitsize;
}

uint32_t convert9to8(uint16_t* input, uint8_t* output, uint32_t length){
    // Convert 9-bit LZW codes to 8-bit bytes
    // Returns number of bytes in output
    // output is padded with zeros at the end to make full bytes
    
    uint32_t buffer = 0;
    uint32_t tmp;
    int shift = 0;
    uint32_t n = 0;
    
    buffer = (uint32_t) input[0];
#if DEBUG
    printf("length=%i\n",length);
    printf("input[%i]=%i\n",0,input[0]);
#endif
    for(int i=1;i<length;i++){
        // Grab next 9-bit code
        tmp = (uint32_t) input[i];
#if DEBUG
        printf("input[%i]=%i\n",i,input[i]);
#endif
        // Find how much to shift the input left
        shift++;
        if(shift >= 8){
            shift -= 8;
        }
        // Shift the input and add it to the buffer
        buffer += (tmp << (8+shift));
#if DEBUG
        printf("buffer=%i\n",buffer);
#endif
        // Copy the lowest buffer byte to the output
        *output++ = (uint8_t) buffer;
#if DEBUG
        printf("output=%i\n",*(output-1));
#endif
        n++;
        // Shift the buffer right to clear the copied byte
        buffer = buffer >> 8;
#if DEBUG
        printf("buffer=%i\n",buffer);
#endif
        // If shift == 8 then write another byte
        if(shift >= 7){
            *output++ = (uint8_t) buffer;
            n++;
            buffer = buffer >> 8;
#if DEBUG
            printf("output=%i\n",*(output-1));
            printf("buffer=%i\n",buffer);
#endif
        }
        
    }
    
    // Write out the last byte
    *output++ = (uint8_t) buffer;
    n++;
#if DEBUG
    printf("output=%i\n",*(output-1));
    printf("n=%i\n",n);
#endif
    return n;
}

uint32_t packLSB(uint16_t* input, uint8_t* output, uint32_t length, uint8_t startnbits, uint64_t* remain, int* startshift, uint32_t* widthjumps, int islast){
    // Convert LZW codes to 8-bit bytes
    // Starts with startnbits-bit codes, which increase by one at the intervals defined in widthjumps
    // Zero-pads end if islast != 0
    // Otherwise saves remaining data in remain and updates startshift with the remaining shift value
    // Returns number of bytes in output
    // output is padded with zeros at the end to make full bytes
    
    uint64_t buffer = 0;
    uint64_t tmp;
    int shift = *startshift;
    uint32_t n = 0;
    int nbits = startnbits;
    int jump = 0;
    int count = 1;  // For some reason it is necessary to start this at 1, perhaps because of the initial clear code?
    uint64_t clearcode = 1 << (startnbits-1);
    
    buffer = *remain;
#if DEBUG
    printf("length=%i\n",length);
    printf("shift=%i\n",shift);
    printf("buffer=0x%08llx\n",buffer);
#endif
    for(int i=0;i<length;i++){
        // Grab next nbit-bit code
        tmp = (uint64_t) input[i];
#if DEBUG
        printf("input[%i]=0x%04x\n",i,input[i]);
#endif
        
        // If a clear code is encountered, then reset the counter for checking jumps
        if(tmp == clearcode){
            count = 0;
        }
        
        // If at a width increase index then bump up nbits
        if(nbits < MAXCODESIZE && count > (widthjumps[jump]+1)){
#if DEBUG
            printf("widthjumps[jump]=%i\n",widthjumps[jump]);
            printf("nbits=%i\n",nbits+1);
#endif
            jump++;
            nbits++;
        }
        
        // Shift the input and add it to the buffer
        buffer += (tmp << shift);
#if DEBUG
        printf("shift=%i\n",shift);
        printf("buffer=0x%08llx\n",buffer);
#endif
        
        // Update the shift value
        shift += nbits;
        
        // Copy the lowest buffer byte to the output if there is one available
        while(shift >= 8){
            *output++ = (uint8_t) buffer;
#if DEBUG
            printf("output=0x%02x\n",*(output-1));
#endif
            n++;
            // Shift the buffer right to clear the copied byte
            buffer = buffer >> 8;
#if DEBUG
            printf("buffer=0x%08llx\n",buffer);
#endif
            
            // Update the shift
            shift -= 8;
        }
        
        count++;
        
    }
    
    // If is the last code to be written then zero-pad the last byte (unnecessary since right shifting already does this) and write out the final byte
    if(islast != 0){
        *output++ = (uint8_t) buffer;
#if DEBUG
        printf("output=0x%02x\n",*(output-1));
#endif
        n++;
        // Shift the buffer right to clear the copied byte
        buffer = buffer >> 8;
        shift = 0;
    }
    
    // Store what remains of the buffer for the next call
    *remain = buffer;
    *startshift = shift;
#if DEBUG
    printf("nbits=%i\n",nbits);
    printf("remain=0x%08llx\n",buffer);
    printf("startshift=%i\n",*startshift);
#endif
    
    return n;
}
