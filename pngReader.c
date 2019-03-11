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

#include <string.h>
#include <assert.h>
#include <math.h>

#include "pngReader.h"

#define DEBUG 0


void readPNGHeader(FILE* fid, PNGHeader *header){
    uint8_t buffer[BUFFSIZE];
    PNGChunk chunk;
    uint8_t head[]="\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";
    // Read first 8 bytes to make sure they match 89504E47 0D0A1A0A
    fread(buffer, 1, 8, fid);
    buffer[8] = 0;
    
    if( strncmp((char*)buffer, (char*)head, 8) != 0 ){
        printf("Error: Input file is not a .png file\n");
        printf("Test header is \"%s\"\n", head);
        printf("File header is \"%s\"\n", buffer);
    }
    
    memset(chunk.Type, '\0', 5);
    while(!feof(fid) && (strncmp((char*)chunk.Type, "IHDR", 4) != 0)){
#if DEBUG
        printf("chunk.Type=%s len=%i cmp=%i\n", chunk.Type, chunk.Length, strncmp((char*)chunk.Type, "IHDR", 4));
#endif
        readPNGChunk(fid, &chunk);
    }
    
    // Read width and height
    header->Width = byteswap(&chunk.Data[0]);
    header->Height = byteswap(&chunk.Data[4]);
    
    // Read other header data
    // We'll be assuming that BitDepth=8, ColorType=2 or 6, and the rest are 0
    header->BitDepth = chunk.Data[8];
    header->ColorType = chunk.Data[9];
    header->Compression = chunk.Data[10];
    header->Filter = chunk.Data[11];
    header->Interlace = chunk.Data[12];
    
#if DEBUG
    printf("width=%i height=%i\n", header->Width, header->Height);
#endif
    
}

void readPNGFrame(FILE* fid, uint32_t width, uint32_t height, uint8_t* frame, uint8_t bytesPerPixel){
    
    PNGChunk chunk;
    uint32_t framesize = height + bytesPerPixel*width*height+1000;  // Includes height bytes of filter codes
    uint8_t* buffer = malloc(framesize);
    uint32_t bufferloc = 0;
    
    int ret;
    
    printf("Reading PNG frame\n");
    
    memset(chunk.Type, '\0', 5);
    while(!feof(fid) && (strncmp((char*)chunk.Type, "IEND", 4) != 0)){
        readPNGChunk(fid, &chunk);
#if DEBUG
        printf("chunk.Type=%s len=%i cmp=%i\n", chunk.Type, chunk.Length, strncmp((char*)chunk.Type, "IHDR", 4));
#endif
        
        if(strncmp((char*)chunk.Type, "IDAT", 4)!=0){
            continue;
        }
        
        // Copy the chunk data to the current buffer position
        memcpy(&buffer[bufferloc], chunk.Data, sizeof(uint8_t)*chunk.Length);
        bufferloc += sizeof(uint8_t)*chunk.Length;
#if DEBUG
        printf("data=%s\n",chunk.Data);
        printf("bufferloc=%d\n",bufferloc);
#endif
    }
    
    // Inflate the data
    /* allocate inflate state */
    z_stream zstrm;
    zstrm.zalloc = Z_NULL;
    zstrm.zfree = Z_NULL;
    zstrm.opaque = Z_NULL;
    zstrm.avail_in = 0;
    zstrm.next_in = Z_NULL;
    ret = inflateInit(&zstrm);
    if (ret != Z_OK){
        zerr(ret);
        return;
    }
    
    inflateData(&zstrm, buffer, bufferloc, frame, framesize);
#if DEBUG
    printf("frame=%x\n",frame);
    printf("framesize=%i\n",framesize);
#endif
    
    /* done when inflate() says it's done */
    (void)inflateEnd(&zstrm);
    
    // Defilter the frame
    defilterPNGFrame(width, height, frame, bytesPerPixel);
}

void readPNGChunk(FILE* fid, PNGChunk *chunk){
    
#if DEBUG
    printf("Reading chunk\n");
    printf("pos=%ld\n",ftell(fid));
#endif
    // Read length
    if(fread(chunk->DataLength, 1, 4, fid) < 1){
        // At the end of the file
#if DEBUG
        printf("Reached the end of the file\n");
#endif
        return;
    }
    
    // Convert length from big to little endian and convert to int
    chunk->Length = byteswap(chunk->DataLength);
    
    // Read chunk->type
    fread(chunk->Type, 1, 4, fid);
#if DEBUG
    printf("chunk->type=%s\n", chunk->Type);
#endif
    
    // Read data
    fread(chunk->Data, 1, chunk->Length, fid);
    
    // Read CRC
    fread(chunk->CRC, 1, 4, fid);
    
#if DEBUG
    printf("chunk->length=%i\n", chunk->Length);
#endif
    
    // Check CRC
    // TODO
}

void defilterPNGFrame(uint32_t width, uint32_t height, uint8_t* frame, uint8_t bytesPerPixel){
    // Use the frame buffer and temporary scanline workspaces to process the data mostly in place
    // If bytesPerPixel == 4 then just drop the alpha here since we won't use it later for GIFs (if we ever want to support outputing a different file format then this would need to change)
    uint8_t* a;  // pixel before current index
    uint8_t* b;  // pixel above current index
    uint8_t* c;  // pixel before b
    uint8_t* nextb;  // pixel above current index for next row
    uint8_t* nextc;  // pixel before b for next row
    uint32_t i = 0;  // Row index
    uint32_t j = 0;  // Column index
    uint32_t k = 0;
    uint8_t *framereadptr;  // Pointer to current reading location in frame
    uint8_t *framewriteptr;  // Pointer to current writing location in frame
    uint8_t filtertype;
    
    printf("Defiltering png frame\n");
    
    // scanline above image is always zeros
    // pixel before first pixel is always zeros
    framereadptr = frame;
    framewriteptr = frame;
    nextb = framewriteptr;
    nextc = framewriteptr;
    
    // Loop through scanlines (i.e., rows)
    for(i=0;i<height;i++){
        // Read filter type byte
        filtertype = *framereadptr++;
#if DEBUG
        printf("Filter %i used on line %i\n", filtertype, i);
#endif
        
        // copy above scanline and scanline
        a = framewriteptr;
        b = nextb;
        c = nextc;
        nextb = framewriteptr;
        nextc = framewriteptr;
        
        // Do the unfiltering
        switch(filtertype){
            case 0:  // None: Recon(x) = Filt(x)
                if(bytesPerPixel == 3){
                    memcpy(framewriteptr, framereadptr, sizeof(uint8_t)*3*width);
                    framereadptr += 3*width;
                    framewriteptr += 3*width;
                }else if(bytesPerPixel == 4){
                    // Process the line
                    for(j=0;j<width;j++){
                        for(k=0;k<3;k++){
                            (*framewriteptr++) = (*framereadptr++);
                        }
                        // Skip alpha byte
                        framereadptr++;
                    }
                }else{
                    printf("Error: should not get here, unsupported number of bytes per pixel\n");
                }
                break;
            case 1:  // Sub:  Recon(x) = Filt(x) + Recon(a)
                // pixel before first pixel is always zeros
                // First pixel is special
                for(k=0;k<3;k++){
                    (*framewriteptr++) = (*framereadptr++);
                }
                
                // Skip alpha byte if necessary
                if(bytesPerPixel > 3){framereadptr++;}
                
                // Continue with the rest of the line
                for(j=1;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + (*a++);
                    }
                    // Skip alpha byte if necessary
                    if(bytesPerPixel > 3){framereadptr++;}
                }
                break;
            case 2:  // Up:   Recon(x) = Filt(x) + Recon(b)
                
                // Process the line
                for(j=0;j<width;j++){
                    for(k=0;k<3;k++){
                        if(i==0){
                            (*framewriteptr++) = (*framereadptr++);
                        }else{
                            (*framewriteptr++) = (*framereadptr++) + (*b++);
                        }
                    }
                    // Skip alpha byte if necessary
                    if(bytesPerPixel > 3){framereadptr++;}
                }
                break;
            case 3:  // Average: Filt(x) + floor((Recon(a) + Recon(b)) / 2)
                // pixel before first pixel is always zeros
                // First pixel is special
                for(k=0;k<3;k++){
                    (*framewriteptr++) = (*framereadptr++) + floor((*b++)/2);
                }
                // Skip alpha byte if necessary
                if(bytesPerPixel > 3){framereadptr++;}
                // Continue with the rest of the line
                for(j=1;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + floor(((*a++)+(*b++))/2);
                    }
                    // Skip alpha byte if necessary
                    if(bytesPerPixel > 3){framereadptr++;}
                }
                break;
            case 4:  // Paeth: Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))
                // pixel before first pixel is always zeros
                // First pixel is special, as is first row
                for(k=0;k<3;k++){
                    if(i==0){
                        (*framewriteptr++) = (*framereadptr++) + PaethPredictor(0, 0, 0);
                    }else{
                        (*framewriteptr++) = (*framereadptr++) + PaethPredictor(0, *b++, 0);
                    }
                }
                // Skip alpha byte if necessary
                if(bytesPerPixel > 3){framereadptr++;}
                // Continue with the rest of the line
                for(j=1;j<width;j++){
#if DEBUG
                    printf("px(%i):",j);
#endif
                    for(k=0;k<3;k++){
#if DEBUG
                        printf("(%i,",*framereadptr);
#endif
                        if(i==0){
                            (*framewriteptr++) = (*framereadptr++) + PaethPredictor(*a++, 0, 0);
                        }else{
                            (*framewriteptr++) = (*framereadptr++) + PaethPredictor(*a++, *b++, *c++);
                        }
#if DEBUG
                        printf("%i,a=%i,b=%i,c=%i)",*(framewriteptr-1), *(a-1), *(b-1), *(c-1));
#endif
                    }
#if DEBUG
                    printf(", ");
#endif
                    // Skip alpha byte if necessary
                    if(bytesPerPixel > 3){framereadptr++;}
                }
#if DEBUG
                printf("\n		");
#endif
                break;
            default:
#if DEBUG
                printf("Error: unknown filter type %d\n", filtertype);
#endif
                return;
        }
        
    }
    
}

uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c){
    int p, pa, pb, pc;  // Needs signed value to prevent overflow
    p = a + b - c;
    pa = abs(p - a);
    pb = abs(p - b);
    pc = abs(p - c);
    if((pa <= pb) && (pa <= pc)){
        return a;
    } else if(pb <= pc){
        return b;
    } else {
        return c;
    }
}

int byteswap(uint8_t* bytes){
    // Convert big to little endian
    return bytes[3] + (bytes[2] << 8) + (bytes[1] << 16) + (bytes[0] << 24);
}


// This code adapted from: https://www.zlib.net/zpipe.c
// Adapted to use input/output uint8_t arrays and to work with this program

/* Decompress from file source to file dest until stream ends or EOF.
 inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 allocated for processing, Z_DATA_ERROR if the deflate data is
 invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
 the version of the library linked do not match, or Z_ERRNO if there
 is an error reading or writing the files. */
uint32_t inflateData(z_stream *zstrm, uint8_t *source, uint32_t sourcelen, uint8_t *dest, uint32_t destlen)
{
    int ret;
    uint32_t have;
        
        zstrm->avail_in = sourcelen;
        zstrm->next_in = source;
        
        /* run inflate() on input until output buffer not full */
        do {
#if DEBUG
            printf("destlen=%d\n", destlen);
#endif
            zstrm->avail_out = destlen;
            zstrm->next_out = dest;
            ret = inflate(zstrm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            
            have = destlen - zstrm->avail_out;
            
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;     /* and fall through */
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    zerr(ret);
                    return 0;
                case Z_STREAM_END:
#if DEBUG
                    printf("Z_STREAM_END reached\n");
#endif
                    return have;
            }
#if DEBUG
            printf("strm.avail_in=%d\n", zstrm->avail_in);
            printf("strm.avail_out=%d\n", zstrm->avail_out);
            printf("strm.total_out=%lu\n", zstrm->total_out);
#endif
        } while (zstrm->avail_out == 0);
    
    return have;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
        case Z_ERRNO:
            if (ferror(stdin))
                fputs("error reading stdin\n", stderr);
            if (ferror(stdout))
                fputs("error writing stdout\n", stderr);
            break;
        case Z_STREAM_ERROR:
            fputs("invalid compression level\n", stderr);
            break;
        case Z_DATA_ERROR:
            fputs("invalid or incomplete deflate data\n", stderr);
            break;
        case Z_MEM_ERROR:
            fputs("out of memory\n", stderr);
            break;
        case Z_VERSION_ERROR:
            fputs("zlib version mismatch!\n", stderr);
    }
}
