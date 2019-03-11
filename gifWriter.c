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
#include "medianCut.h"
#include "dither.h"

#define MAXCODESIZE 12
#define DEBUG 0


GIFOptStruct newGIFOptStructInst(){
    // Set defaults
    GIFOptStruct gifopts;
    
    gifopts.delay = 25;
    gifopts.dither = 0;
    gifopts.colortablebitsize = 0;
    gifopts.forcebw = 0;
    
    return gifopts;
}

void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, GIFOptStruct gifopts){

    uint8_t head[] = "\x47\x49\x46\x38\x39\x61";
    
    printf("Writing gif header\n");
    
    // Write gif header
    fwrite(head, 6, 1, fid);
    
    // Write Logical screen descriptor
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, sizeof(uint16_t), 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, sizeof(uint16_t), 1, fid);
    
    fputc('\xF0', fid);  // global color table is size 6 (2 RGB colors)
    fputc('\x00', fid);  // White background
    fputc('\x00', fid);  // No pixel aspect ratio
    
    // Write global color table
    fwrite("\xFF\xFF\xFF\x00\x00\x00", 6, 1, fid);
    
}

void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, GIFOptStruct gifopts){

#if DEBUG
    printf("Writing gif local image descriptor\n");
    printf("width=%d, height=%d\n", width, height);
#endif
    
    // Write graphics control extension block
    fwrite("\x21\xF9\x04\x04", 4, 1, fid);  // Not using a transparent background
    // Write delay time
    fwrite(&gifopts.delay, sizeof(uint16_t), 1, fid);
    // Finish off the block
    fwrite("\x00\x00", 2, 1, fid);
    
    // Write local image descriptor
    fputc('\x2C', fid);
    fwrite("\x00\x00\x00\x00", 4, 1, fid);
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, 2, 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, 2, 1, fid);
    // Wait to write the packed byte until after we know the minimum table size
    
    // Write local color table
    int tablebitsize = writeGIFLCT(fid, frame, width, height, gifopts);
    
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
    uint8_t* output = malloc(sizeof(uint8_t)*length);    // frame size
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

uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height, GIFOptStruct gifopts){
    
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
    int tablebitsize = 1;
    int tablesize = 1 << tablebitsize;
    if(gifopts.colortablebitsize > 0){
        if(gifopts.colortablebitsize > 8){
            printf("Error: too many defined colors using colortablebitsize=%i. Value must be 1 <= x <= 8. Exiting.\n", gifopts.colortablebitsize);
        }
        tablebitsize = gifopts.colortablebitsize;
    }else{
        while(nunique > tablesize){
            if(tablebitsize >= 8){
                break;
            }
            tablebitsize++;
            tablesize = 1 << tablebitsize;
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
    uint8_t packedbyte = (1 << 7) + (tablebitsize-1);
    fputc(packedbyte, fid);
    
    // Shrink the color pallete to an optimal set via median cut
    medianCut(unique, nunique, tablebitsize);
    
    // If requested then force black and white to the color table
    // Find the closest colors overwrite them
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
        
        // Need to resort unique into colorindex order in case findClosetColor messed it up
        qsort((void*)unique, nunique, sizeof(SortedPixel), comparefcn_colorind);
    }
    
    // Write the color table
    // Copy the data to the frame variable first since it is otherwise just sitting around, then write as one chunk
    frameptr = frame;
    // Write first color
    memcpy(frameptr, &(unique[0].pixel), 3);
    frameptr += 3;
    for(int i=1;i<nunique;i++){
        if(unique[i].colorindex == unique[i-1].colorindex){
            continue;
        }else{
#if DEBUG
            printf("#pixels(#colors)[color] in bin: %i(%i)[%i]\n",unique[i].npixel,i,unique[i].pixel);
#endif
            memcpy(frameptr, &(unique[i].pixel), 3);
            frameptr += 3;
        }
    }
#if DEBUG
    printf("#pixels(#colors)[color]  in bin: %i(%i)[%i]\n",unique[nunique-1].npixel,nunique,unique[nunique].pixel);
#endif
    fwrite(frame, sizeof(uint8_t), 3*(1 << tablebitsize), fid);
    
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
    
    // Dither the image based on the smaller color pallete
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
