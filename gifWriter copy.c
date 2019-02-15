
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>
#include <math.h>

#define BUFFSIZE 16284

typedef struct _PNGHeader {
    uint32_t Width;
    uint32_t Height;
    uint8_t BitDepth;
    uint8_t ColorType;
    uint8_t Compression;
    uint8_t Filter;
    uint8_t Interlace;
} PNGHeader;

typedef struct _PNGChunk
{
    uint8_t DataLength[4];   /* Size of Data field in bytes */
    uint32_t Length;
    char Type[5];         /* Code identifying the type of chunk */
    uint8_t Data[BUFFSIZE];       /* The actual data stored by the chunk */
    uint8_t CRC[4];          /* CRC-32 value of the Type and Data fields */
} PNGChunk;

typedef struct _SortedPixel {
//    uint32_t pixel;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint32_t frameindex;
    uint32_t sortedindex;
    uint8_t colorindex;
    uint32_t npixel;
} SortedPixel;

void readPNGHeader(FILE* fid, PNGHeader *header);
void readPNGFrame(FILE* fid, uint32_t width, uint32_t height, uint8_t* frame);
void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, uint16_t delay);
void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
uint32_t writeGIFLCT(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height);
uint32_t shrinkGIF(SortedPixel* buffer, uint32_t width, uint32_t height);
void readPNGChunk(FILE* fid, PNGChunk *chunk);
void defilterPNGFrame(uint32_t width, uint32_t height, uint8_t* frame);
uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c);
int comparefcn1(const void* first, const void* second);
int comparefcn2(const void* first, const void* second);
int byteswap(uint8_t* bytes);
uint32_t inflateChunk(z_stream *zstrm, uint8_t *source, uint32_t sourcelen, uint8_t *dest, uint32_t destlen);
void zerr(int ret);

// From LZWlib.cpp
int LZWcompress(uint8_t* input, uint32_t inlen, uint8_t* output);


int main () {
    FILE *fid;
    FILE *fidgif;
    char filename[] = "file1.png";
    uint8_t* frame=NULL;
    PNGHeader header;
    
    // Get PNG header to get frame size so we can write the gif header
//    fid = fopen(filename, "rb");
//    readPNGHeader(fid, &header);
//    fclose(fid);
    
    uint16_t delay = 25;  // Delay between frames in 1/100 sec
    
    // Open the gif file
    fidgif = fopen("file.gif", "wb");
//    writeGIFHeader(fidgif, header.Width, header.Height);
    
    for(int i=1; i<2; i++){
//        filename[4] = itoa(i);
        fid = fopen(filename, "rb");
        
        // Get png header and make sure the frame is the same size
        readPNGHeader(fid, &header);
        
        // Allocate memory for the frame if we are just starting
        if(frame==NULL){
            frame = malloc(sizeof(uint8_t)*3*header.Width*header.Height+header.Height);  // RBG bytes + png scanline filter bytes
            memset(frame, 0, sizeof(uint8_t)*3*header.Width*header.Height);
        }
        
        // Get png frame in gif frame format
        readPNGFrame(fid, header.Width, header.Height, frame);
        
        // If just starting then write the gif header
        if(ftell(fidgif) == 0){
            writeGIFHeader(fidgif, header.Width, header.Height, delay);
        }
        
        // Write frame to gif
        writeGIFFrame(fidgif, frame, header.Width, header.Height);
        
        fclose(fid);
    }
    
    // Write the gif end byte
    putc('\x3B', fidgif);
    
    // Close gif
    fclose(fidgif);
    
    // Free frame memory
    free(frame);
    
    return(0);
}
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
        printf("chunk.Type=%s len=%i cmp=%i\n", chunk.Type, chunk.Length, strncmp((char*)chunk.Type, "IHDR", 4));
        readPNGChunk(fid, &chunk);
    }
    
    // Read width and height
    header->Width = byteswap(&chunk.Data[0]);
    header->Height = byteswap(&chunk.Data[4]);
    
    // Read other header data
    // We'll be assuming that BitDepth=8, ColorType=2, and the rest are 0
    header->BitDepth = chunk.Data[8];
    header->ColorType = chunk.Data[9];
    header->Compression = chunk.Data[10];
    header->Filter = chunk.Data[11];
    header->Interlace = chunk.Data[12];
    
    printf("width=%i height=%i\n", header->Width, header->Height);
    
}

void readPNGFrame(FILE* fid, uint32_t width, uint32_t height, uint8_t* frame){
    
    PNGChunk chunk;
    uint32_t frameloc = 0;
    uint32_t framesize = sizeof(uint8_t)*3*width*height+height;
    
    int ret;
    z_stream zstrm;
    //    unsigned char in[CHUNK];
    //    unsigned char out[CHUNK];
    
    /* allocate inflate state */
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
    
    printf("Reading PNG frame\n");
    
    memset(chunk.Type, '\0', 5);
    while(!feof(fid) && (strncmp((char*)chunk.Type, "IEND", 4) != 0)){
        readPNGChunk(fid, &chunk);
        printf("chunk.Type=%s len=%i cmp=%i\n", chunk.Type, chunk.Length, strncmp((char*)chunk.Type, "IHDR", 4));
        
        if(strncmp((char*)chunk.Type, "IDAT", 4)!=0){
            continue;
        }
        
        // Inflate the chunk to the current frame position
        printf("data=%s\n",chunk.Data);
//        frameloc += inflateChunk(&frame[frameloc], framesize-frameloc, chunk.Data, chunk.Length);
        frameloc += inflateChunk(&zstrm, chunk.Data, chunk.Length, &frame[frameloc], framesize-frameloc);
        printf("frame=%s\n",&frame[frameloc]);
        printf("frameloc=%d\n",frameloc);
    }
    
    /* done when inflate() says it's done */
    (void)inflateEnd(&zstrm);
    
    // Defilter the frame
    defilterPNGFrame(width, height, frame);
}

void readPNGChunk(FILE* fid, PNGChunk *chunk){
    
    printf("Reading chunk\n");
    printf("pos=%ld\n",ftell(fid));
    // Read length
    if(fread(chunk->DataLength, 1, 4, fid) < 1){
        // At the end of the file
        printf("Reached the end of the file\n");
        return;
    }
    
    // Convert length from big to little endian and convert to int
    chunk->Length = byteswap(chunk->DataLength);
    
    // Read chunk->type
    fread(chunk->Type, 1, 4, fid);
    printf("chunk->type=%s\n", chunk->Type);
    
    // Read data
    fread(chunk->Data, 1, chunk->Length, fid);
    
    // Read CRC
    fread(chunk->CRC, 1, 4, fid);
    
    printf("chunk->length=%i\n", chunk->Length);
    
    // Check CRC
    // TODO
}

void defilterPNGFrame(uint32_t width, uint32_t height, uint8_t* frame){
    // Use the frame buffer and temporary scanline workspaces to process the data mostly in place
//    uint8_t* abovescanline = malloc(sizeof(uint8_t)*3*width);
//    uint8_t* scanline = malloc(sizeof(uint8_t)*3*width);
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
//    memset(abovescanline, 0, sizeof(uint8_t)*3*width);
//    memset(scanline, 0, sizeof(uint8_t)*3*width);
    // pixel before first pixel is always zeros
//    memset(a, 0, sizeof(uint8_t)*3);
//    b = abovescanline;
//    memset(c, 0, sizeof(uint8_t)*3);
    framereadptr = frame;
    framewriteptr = frame;
    nextb = framewriteptr;
    nextc = framewriteptr;
    
    // Loop through scanlines (i.e., rows)
    for(i=0;i<height;i++){
        // Read filter type byte
        filtertype = *framereadptr++;
//        framereadptr++;
        
        // copy above scanline and scanline
//        memcpy(abovescanline, scanline, sizeof(uint8_t)*3*width);
//        memcpy(scanline, framereadptr, sizeof(uint8_t)*3*width);
        b = nextb;
        c = nextc;
        nextb = framewriteptr;
        nextc = framewriteptr;
        
        // Do the unfiltering
        switch(filtertype){
            case 0:  // None: Recon(x) = Filt(x)
                memcpy(framewriteptr, framereadptr, sizeof(uint8_t)*3*width);
                framereadptr += 3*width;
                framewriteptr += 3*width;
                break;
            case 1:  // Sub:  Recon(x) = Filt(x) + Recon(a)
                // pixel before first pixel is always zeros
                // First pixel is special
                for(k=0;k<3;k++){
                    (*framewriteptr++) = (*framereadptr++);
                }
                a = framewriteptr-3;
                // Continue with the rest of the line
                for(j=1;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + (*a++);
                    }
                }
                break;
            case 2:  // Up:   Filt(x) + Recon(b)
                
                // Process the line
                for(j=0;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + (*b++);
                    }
                }
                break;
            case 3:  // Average: Filt(x) + floor((Recon(a) + Recon(b)) / 2)
                // pixel before first pixel is always zeros
                // First pixel is special
                for(k=0;k<3;k++){
                    (*framewriteptr++) = (*framereadptr++) + floor((*b++)/2);
                }
                a = framewriteptr-3;
                // Continue with the rest of the line
                for(j=1;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + floor(((*a++)+(*b++))/2);
                    }
                }
                break;
            case 4:  // Paeth: Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))
//                printf("Error: Paeth filter type currently unsupported, used in scanline %d\n", i);
                // pixel before first pixel is always zeros
                // First pixel is special
                for(k=0;k<3;k++){
                    (*framewriteptr++) = (*framereadptr++) + PaethPredictor(0, *b++, 0);
                }
                a = framewriteptr-3;
                // Continue with the rest of the line
                for(j=1;j<width;j++){
                    for(k=0;k<3;k++){
                        (*framewriteptr++) = (*framereadptr++) + PaethPredictor(*a++, *b++, *c++);
                    }
                }
                break;
            default:
                printf("Error: unknown filter type %d\n", filtertype);
                return;
        }
        
        // Increment the framereadptr to the next scanline
//        framereadptr += 3*width;
    }
    
}

uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c){
    uint8_t p, pa, pb, pc;
    p = a + b - c;
    pa = abs(p - a);
    pb = abs(p - b);
    pc = abs(p - c);
    if(pa <= pb && pa <= pc){
        return a;
    } else if(pb <= pc){
        return b;
    } else {
        return c;
    }
}

void writeGIFHeader(FILE* fid, uint32_t width, uint32_t height, uint16_t delay){

    uint8_t head[] = "\x47\x49\x46\x38\x39\x61";
//    uint8_t appext[] = "\x21\xFF\x0B\x4E\x45\x54\x53\x43\x41\x50\x45\x32\x2E\x30\x03\x01\x00\x00\x00";
    
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
    fputc('\xFF', fid);  // White background
    fputc('\x00', fid);  // No pixel aspect ratio
    
    // Write global color table
    fwrite("\xFF\xFF\xFF\x00\x00\x00", 6, 1, fid);
    
    // Write graphics controls extension block
    fwrite("\x21\xF9\x04\x04", 4, 1, fid);  // Using a transparent background
    // Write delay time
    fwrite(&delay, sizeof(uint16_t), 1, fid);
    // Finish off the block
    fwrite("\x02\x00", 2, 1, fid);
    
    // Write application extension block
//    fwrite(appext, 19, 1, fid);
    
    //
    
}

void writeGIFFrame(FILE* fid, uint8_t* frame, uint32_t width, uint32_t height){
    
    uint8_t* buffer = malloc(sizeof(uint8_t)*3*width*height);  // frame size bytes

    printf("Writing gif local image descriptor\n");
    printf("width=%d, height=%d\n", width, height);
    
    // Write local image descriptor
    fputc('\x2C', fid);
    fwrite("\x00\x00\x00\x00", 4, 1, fid);
    // Write width and height as uint16
    uint16_t w = (uint16_t) width;
    fwrite(&w, 2, 1, fid);
    uint16_t h = (uint16_t) height;
    fwrite(&h, 2, 1, fid);
    // Write packed byte with 256-byte local color table
    fputc('\xE1', fid);
    
    // Write local color table
    writeGIFLCT(fid, frame, width, height);
    
    // Write image data
    printf("Writing gif frame data\n");
    
    // Compress the image with LZW
    printf("indices=%s\n",frame);
//    int n = LZWcompress(frame, sizeof(uint8_t)*width*height, buffer);
//    printf("n=%i\n", n);
//    printf("frame[n]=%d\n", frame[n]);
//    printf("buffer=%s\n",buffer);
    
    // Remove, this is only for debugging
    memcpy(buffer, frame, sizeof(uint8_t)*width*height);
    int n = width*height;
    
    // Write the LZW minimum code size byte
    putc('\x07', fid);  //FIXME
    
    // Write the buffer in 255 byte chunks
    int pos = 0;
    while(n > 255){
        putc('\xFF', fid);  // Number of bytes in data chunk
        fwrite(&buffer[pos], 1, 255, fid);
        pos += 255;
        n -= 255;
    }
    
    // Write the remainder, truncating n to a uint8_t
    uint8_t rem = (uint8_t) n;
    fwrite(&rem, 1, 1, fid);  // Number of bytes in data chunk
    fwrite(&buffer[pos], 1, rem, fid);
    
    // Write signal for last data chunk
    putc('\x00', fid);
    
    // Free the allocated buffer
    free(buffer);
    
}

//int comparefcn1(const void* first, const void* second){
//    return ((SortedPixel*)first)->pixel - ((SortedPixel*)second)->pixel;
//}
//
//int comparefcn2(const void* first, const void* second){
//    return ((SortedPixel*)first)->frameindex - ((SortedPixel*)second)->frameindex;
//}

int comparefcnR(const void* first, const void* second){
    return ((SortedPixel*)first)->R - ((SortedPixel*)second)->R;
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
    uint8_t npixel = width*height;
    
    printf("Writing gif local color table\n");
    printf("npixel=%d\n", npixel);
    
    // Set buffer array to zeros
    buffer = malloc(sizeof(SortedPixel)*npixel);
    unique = malloc(sizeof(SortedPixel)*npixel);
    memset(buffer, 0, sizeof(SortedPixel)*npixel);
    
    // Set pointers
    bufferptr = buffer;
    frameptr = frame;
    
    // Copy frame data into buffer array
    for(int i=0;i<npixel;i++){
//        memcpy(&(bufferptr->pixel), frameptr, 3);  // Copy three RGB bytes into uint32_t
        bufferptr->R = *frameptr++;
        bufferptr->G = *frameptr++;
        bufferptr->B = *frameptr++;
        bufferptr->frameindex = i;
        bufferptr++;
        frameptr += 3;
    }
    
    // Shrink the color pallete to an optimal set and dither
    int count = 0;
    count = shrinkGIF(buffer, width, height);
    
    
    
//    // Sort the buffer by the pixel color
//    bufferptr = buffer;
//    qsort((void*)bufferptr, npixel, sizeof(SortedPixel), comparefcn1);
//
//    // Find unique entries and number of each
//    memcpy(&unique[0], &buffer[0], sizeof(SortedPixel));
////    unique[0] = buffer[0];
//    int count = 0;
//    unique[count].npixel++;
//    for(int i=1;i<npixel;i++){
//        buffer[i].sortedindex = i;
//        if(buffer[i].pixel == unique[count].pixel){
//            unique[count].npixel++;
//            continue;
//        }else{
//            count++;
//            memcpy(&unique[count], &buffer[i], sizeof(SortedPixel));
////            unique[count] = buffer[i];
//            unique[count].npixel++;
//        }
//        buffer[i].colorindex = count;
//        unique[i].colorindex = count;
//    }
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
    printf("count=%d\n", count);
    return count;
}

uint32_t shrinkGIF(SortedPixel* buffer, uint32_t width, uint32_t height){
    return 0;
}

int byteswap(uint8_t* bytes){
    // Convert big to little endian
    return bytes[3] + (bytes[2] << 8) + (bytes[1] << 16) + (bytes[0] << 24);
}

uint16_t dec2hex(uint8_t input){
    return 0;
}


// This code taken from: https://www.zlib.net/zpipe.c
// Adapted to use input/output uint8_t arrays and to work with this program

/* Decompress from file source to file dest until stream ends or EOF.
 inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
 allocated for processing, Z_DATA_ERROR if the deflate data is
 invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
 the version of the library linked do not match, or Z_ERRNO if there
 is an error reading or writing the files. */
uint32_t inflateChunk(z_stream *zstrm, uint8_t *source, uint32_t sourcelen, uint8_t *dest, uint32_t destlen)
{
    int ret;
    uint32_t have;
//    z_stream strm;
////    unsigned char in[CHUNK];
////    unsigned char out[CHUNK];
//
//    /* allocate inflate state */
//    strm.zalloc = Z_NULL;
//    strm.zfree = Z_NULL;
//    strm.opaque = Z_NULL;
//    strm.avail_in = 0;
//    strm.next_in = Z_NULL;
//    ret = inflateInit(&strm);
//    if (ret != Z_OK)
//        return ret;
    
    /* decompress until deflate stream ends or end of file */
//    do {
//        strm.avail_in = fread(in, 1, CHUNK, source);
//        if (ferror(source)) {
//            (void)inflateEnd(&strm);
//            return Z_ERRNO;
//        }
//        if (strm.avail_in == 0)
//            break;
//        strm.next_in = in;
        
        zstrm->avail_in = sourcelen;
        zstrm->next_in = source;
        
        /* run inflate() on input until output buffer not full */
        do {
//            strm.avail_out = CHUNK;
//            strm.next_out = out;
            printf("destlen=%d\n", destlen);
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
//                    (void)inflateEnd(&strm);
                    return 0;
                case Z_STREAM_END:
                    printf("Z_STREAM_END reached\n");
                    return have;
            }
//            have = CHUNK - strm.avail_out;
//            have = destlen - zstrm->avail_out;
//            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
//                (void)inflateEnd(&strm);
//                return Z_ERRNO;
//            }
            printf("strm.avail_in=%d\n", zstrm->avail_in);
            printf("strm.avail_out=%d\n", zstrm->avail_out);
            printf("strm.total_out=%lu\n", zstrm->total_out);
        } while (zstrm->avail_out == 0);
        
        /* done when inflate() says it's done */
//    } while (ret != Z_STREAM_END);
    
    /* clean up and return */
//    (void)inflateEnd(&strm);
//    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
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
