
#include <string.h>

#include "gifWriter.h"

#define N_LCT 8  // # of local color table entries = 2^N_LCT
#define MAXCODESIZE 12
#define DEBUG 1


void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, uint16_t delay){

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
    
    // Write graphics controls extension block
    fwrite("\x21\xF9\x04\x04", 4, 1, fid);  // Not using a transparent background
    // Write delay time
    fwrite(&delay, sizeof(uint16_t), 1, fid);
    // Finish off the block
    fwrite("\x00\x00", 2, 1, fid);
    
    // Write application extension block
//    fwrite(appext, 19, 1, fid);
    
    //
    
}

void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height){

#if DEBUG
    printf("Writing gif local image descriptor\n");
    printf("width=%d, height=%d\n", width, height);
#endif
    
    // Write local image descriptor
    fputc('\x2C', fid);
    fwrite("\x00\x00\x00\x00", 4, 1, fid);
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, 2, 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, 2, 1, fid);
    // Write packed byte with 256-byte local color table
    // Note that the documentation at https://www.fileformat.info/format/gif/egff.htm is wrong and the packed byte for the local color table looks like the packed byte for the global color table
    uint8_t packedbyte = (1 << 7) + (N_LCT-1);
    fputc(packedbyte, fid);
    
    // Write local color table
    writeGIFLCT(fid, frame, width, height);
    
    // Write image data
    printf("Writing gif frame data\n");
    
    writeGIFImageCompressed(fid, frame, width, height);
//    writeGIFImageCompressed9bit(fid, frame, width, height);
//    writeGIFImageUncompressed256(fid, frame, width*height);
//    writeGIFImageUncompressed128(fid, frame, width*height);
    
}

void writeGIFImageCompressed(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height){
    
    printf("Writing compressed frame\n");
    
    uint32_t length = width*height;
    
    uint8_t startnbits = 9;  // Start the LZW table at 9 bit codes
    
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
    int ret = 1;
    int startshift = 0;
    uint64_t remain = 0;
    int islast = 0;
    uint32_t inlen = sizeof(uint8_t)*width*height;
    
    // Start the compressed codes with a clear code
    buffer[0] = 0x100;
    ncodes++;
    bufferptr = buffer+1;
    
    while(islast == 0){
        printf("bufferptr=0x%x\n", bufferptr);
        ret += LZWcompress(&frameptr, &inlen, &bufferptr, widthjumps);
        ncodes += ret;
        printf("frameptr=0x%x\n", frameptr);
        printf("inlen=%i\n", inlen);
        printf("ncodes=%i\n", ncodes);
        printf("widthjumps={%i,%i,%i,%i}\n",widthjumps[0],widthjumps[1],widthjumps[2],widthjumps[3]);
        
        if(ret <= 0 && islast == 0){
            // End table with stop code
            *bufferptr = 0x101;
            ret = 1;
            ncodes++;
            islast = 1;
        }else{
            // Reset table with clear code
            printf("*bufferptr=0x%x\n", *bufferptr);
            *bufferptr = 0x100;
            printf("bufferptr=0x%x\n", bufferptr);
            ret++;
            ncodes++;
        }
                   
        bufferptr = buffer;
        
        // Pack all codes into bytes
        packedn += packLSB(bufferptr, &output[packedn], ret, startnbits, &remain, &startshift, widthjumps);
        
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
        printf("packedn=%i\n", packedn);
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
//
//    // Copy the buffer into ?? 9-bit codes that will fit into chunksize byte chunks
//    int chunksize = 101;  // Using 252 here because otherwise ncodes is not an integer number and will have trailing zeros, which is not correct.
//    int ncodes = (8*chunksize)/9;  // 8*chunksize bits available will be enough space for ncodes 9-bit codes.
//    while(n > ncodes){
//        putc((uint8_t) chunksize, fid);  // Number of bytes in data chunk
//        // Convert 9-bit codes into bytes
//        convert9to8(bufferptr, output, ncodes);
//        bufferptr += ncodes;
//
//        // Write bytes to file
//        fwrite(output, 1, chunksize, fid);
//        n -= (ncodes);
//    }
    
    // Write the remainder, truncating n to a uint8_t
    // Include signal for last data chunk
//    uint8_t rem = (uint8_t) (n+1);
    // Convert 9-bit codes into bytes
//    n = convert9to8(bufferptr, output, rem);
//    n = packLSB(bufferptr, output, rem, 8);
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

int comparefcnR(const void* first, const void* second){
    return (int)((SortedPixel*)first)->R - (int)((SortedPixel*)second)->R;
}

int comparefcnG(const void* first, const void* second){
    return ((SortedPixel*)first)->G - ((SortedPixel*)second)->G;
}

int comparefcnB(const void* first, const void* second){
    return ((SortedPixel*)first)->B - ((SortedPixel*)second)->B;
}

uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height){
    
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
    memset(unique, 0, sizeof(SortedPixel)*npixel);  // Strictly not necessary, but otherwise we may get junk at the end of the color table if there are fewer unique colors than the length of the color table. The way, the extra entries will be zero.
    
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
//        printf("pixel #%i:RGB=%i,%i,%i\n",i,bufferptr->R,bufferptr->G,bufferptr->B);
        bufferptr++;
//        frameptr += 3;
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
    
    // Shrink the color pallete to an optimal set via median cut
    medianCut(unique, nunique);
    
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
            printf("#colors in bin: %i(%i)\n",unique[i].npixel,i);
#endif
            memcpy(frameptr, &(unique[i].pixel), 3);
            frameptr += 3;
        }
    }
    fwrite(frame, sizeof(uint8_t), 3*(1 << N_LCT), fid);
    
    // Re-sort unique into sorted state, same as buffer still is
    bufferptr = unique;
    qsort((void*)bufferptr, nunique, sizeof(SortedPixel), comparefcn_sortind);
    
    // Set buffer.colorindex based on unique entries
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
    dither();
    
//
//    // count needs to be size 2^n up to 256 (n=8)
//    // TODO
//
//    // Sort the buffer back into frame order
//    bufferptr = buffer;
//    qsort((void*)bufferptr, npixel, sizeof(SortedPixel), comparefcn2);
//
//    // Write the color table
//    // Copy the data to the frame variable first since it is otherwise just sitting around, then write as one chunk
//    frameptr = frame;
//    for(int i=0;i<count;i++){
//        memcpy(frameptr, &(unique[i].pixel), 3);  // Copy three RGB bytes into uint8_t*
//        frameptr += 3;
//    }
//    frameptr = frame;
//    fwrite(frameptr, sizeof(uint8_t), 3*npixel, fid);
    
    // Store image indices in frame
    frameptr = frame;
    for(int i=0;i<npixel;i++){
        *frameptr++ = buffer[i].colorindex;
    }
    
    // Return the size of the color table
#if DEBUG
    printf("nunique=%d\n", nunique);
#endif
    return nunique;
}

//uint32_t shrinkGIF(SortedPixel* buffer, uint32_t length){
//    // First, do a median cut process to find the optimal colorspace for 256 colors
//
//    // Find colorspace
//    medianCut(buffer, length);
//
//
//
//    return 0;
//}


void medianCut(SortedPixel* unique, uint32_t length){
    // Find optimal 256 color set using the median cut algorithm
    // We will be hard-coding 0x00 and 0xFF since they are so common, so really we want 254 colors
    // This requires first finding 256 colors, then figuring out which two to get rid of
    // Once finished, find the mean for each bin and set the pixels accordingly
    
    int tablebitsize = N_LCT;
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
//    bin.minR = bin.buffer[0].R;
//    bin.maxR = bin.buffer[bin.length-1].R;
//    bin.minG = bin.buffer[0].G;
//    bin.maxG = bin.buffer[bin.length-1].G;
//    bin.minB = bin.buffer[0].B;
//    bin.maxB = bin.buffer[bin.length-1].B;
    
    // Find the largest range (bias against B with R over G in tiebreaker)
//    printf("bin.maxR, bin.minR: %i, %i\n", bin->maxR,bin->minR);
//    printf("bin.maxG, bin.minG: %i, %i\n", bin->maxG,bin->minG);
//    printf("bin.maxB, bin.minB: %i, %i\n", bin->maxB,bin->minB);
//    printf("bin.maxR-bin.minR: %i\n", bin->maxR-bin->minR);
//    printf("bin.maxG-bin.minG: %i\n", bin->maxG-bin->minG);
//    printf("bin.maxB-bin.minB: %i\n", bin->maxB-bin->minB);
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

void getRange(CutBin* bin){
    
    // For each color, sort and then get its range
    
    sortR(bin->buffer, bin->length);
    bin->minR = bin->buffer[0].R;
    bin->maxR = bin->buffer[bin->length-1].R;
//    printf("1Sorted R (max, min): %i, %i\n", bin->buffer[bin->length-1].R, bin->buffer[0].R);
//    printf("2Sorted R (max, min): %i, %i\n", bin->maxR, bin->minR);
    
    sortG(bin->buffer, bin->length);
    bin->minG = bin->buffer[0].G;
    bin->maxG = bin->buffer[bin->length-1].G;
    
    sortB(bin->buffer, bin->length);
    bin->minB = bin->buffer[0].B;
    bin->maxB = bin->buffer[bin->length-1].B;
    
//    switch(color){
//        case 0:  // R
//            sortR(bin.buffer, bin.length);
//            return bin.buffer[length-1].R - bin.buffer[0].R;
//        case 1:  // G
//            sortG(bin.buffer, bin.length);
//            return bin.buffer[length-1].G - bin.buffer[0].G;
//        case 2:  // B
//            sortB(bin.buffer, bin.length);
//            return bin.buffer[length-1].B - bin.buffer[0].B;
//        default:
//            printf("getRange color %d is out of range\n", color);
//            return 0;
//    }
    
    
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

void dither(){
    
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

uint32_t packLSB(uint16_t* input, uint8_t* output, uint32_t length, uint8_t startnbits, uint64_t* remain, int* startshift, uint32_t* widthjumps){
    // Convert LZW codes to 8-bit bytes
    // Starts with startnbits-bit codes, which increase by one at the intervals defined in widthjumps
    // Returns number of bytes in output
    // output is padded with zeros at the end to make full bytes
    
    uint64_t buffer = 0;
    uint64_t tmp;
    int shift = (*startshift)-startnbits;
    uint32_t n = 0;
    int nbits = startnbits - 1;
    int jump = 0;
    
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
        
        // If at a width increase index then bump up nbits
        if(nbits < (MAXCODESIZE-1) && i > (widthjumps[jump]+1)){
            printf("widthjumps[jump]=%i\n",widthjumps[jump]);
            printf("nbits=%i\n",nbits+1);
            jump++;
            
            // Find how much to shift the input left
            shift += (nbits-7);
            if(shift >= nbits){
                //            shift -= nbits;
                shift -= 8;
            }
            shift--;
            nbits++;
        }else{
        
            // Find how much to shift the input left
            shift += (nbits-7);
            if(shift >= nbits){
                shift -= 8;
            }
        }
            
        // Shift the input and add it to the buffer
        buffer += (tmp << (nbits+shift));
#if DEBUG
        printf("shift=%i\n",shift);
        printf("buffer=0x%08llx\n",buffer);
#endif
        // Copy the lowest buffer byte to the output
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
        
        // If shift == 8 then write another byte
        if(shift >= 7){
            *output++ = (uint8_t) buffer;
            n++;
            buffer = buffer >> 8;
#if DEBUG
            printf("output=0x%02x\n",*(output-1));
            printf("buffer=0x%08llx\n",buffer);
#endif
        }
        
    }
    
    // Write out the last byte
    *output++ = (uint8_t) buffer;
    n++;
    buffer = buffer >> 8;
#if DEBUG
    printf("output=0x%02x\n",*(output-1));
    printf("n=%i\n",n);
#endif
    
    // Save remaining bits
    *remain = buffer;
    shift += (nbits-7);
    if(shift >= nbits){
        shift -= 8;
    }
    *startshift = nbits+1;  // FIXME: Not sure what this should be!
#if DEBUG
    printf("nbits=%i\n",nbits);
    printf("remain=0x%08llx\n",buffer);
    printf("startshift=%i\n",startshift);
#endif
    
    return n;
}
