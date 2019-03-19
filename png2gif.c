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
#include <getopt.h>

#include "pngReader.h"
#include "gifWriter.h"

#define FILENAMESIZE 256


typedef struct _OptStruct {
    int fileind;
    int nfile;
    GIFOptStruct gifopts;
} OptStruct;

OptStruct newOptStructInst(){
    // Set defaults
    OptStruct opts;
    
    opts.fileind = 0;
    opts.nfile = 0;
    opts.gifopts = newGIFOptStructInst();
    
    return opts;
}

OptStruct argParser(int argc, char **argv);


int main (int argc, char **argv) {
    FILE *fid;
    FILE *fidgif;
    char giffilename[FILENAMESIZE];
    int pngfileind;
    uint8_t* frame=NULL;
    PNGHeader header;
    
    if(argc > 1){
        printf("Starting conversion of PNG file(s) to GIF file.\n");
    }
    
    OptStruct opts = argParser(argc, argv);
    
    // If only one file then use same basename for .gif
    strcpy(giffilename, argv[opts.fileind]);
    if(opts.nfile == 1){
        pngfileind = opts.fileind;
        // Replace extension
        int namelen = strlen(giffilename);
        strcpy(giffilename+namelen-4, ".gif");
    }else{
        pngfileind = opts.fileind+1;
    }
    
    printf("giffilename=%s\n", giffilename);
    
    // Open the gif file
    fidgif = fopen(giffilename, "wb");
    
    for(int i=pngfileind; i<argc; i++){
        printf("pngfilename=%s\n", argv[i]);
        fid = fopen(argv[i], "rb");
        
        // Get png header and make sure the frame is the same size
        readPNGHeader(fid, &header);
        
        // Check for supported PNG formats
        if(header.ColorType != 2 && header.ColorType != 6){
            printf("Error: PNG reader only supports 24-bit or 32-bit Truecolor images (this image colorType=%i)\n", header.ColorType);
            return -1;
        }
        if(header.Interlace != 0){
            printf("Error: PNG reader does not support interlaced images\n");
            return -1;
        }
        
        // Allocate memory for the frame if we are just starting
        // Always support RGBA size, just in case
        if(frame==NULL){
            frame = malloc(sizeof(uint8_t)*4*header.Width*header.Height+header.Height);  // RBG bytes + png scanline filter bytes
            memset(frame, 0, sizeof(uint8_t)*4*header.Width*header.Height);
        }
        
        // Get png frame in gif frame format
        if(header.ColorType == 2){
            readPNGFrame(fid, header.Width, header.Height, frame, 3);
        }else if(header.ColorType == 6){
            readPNGFrame(fid, header.Width, header.Height, frame, 4);
        }else{
            printf("Error: Should never get here, unsupported PNG color type\n");
            return -1;
        }
        
        // If just starting then write the gif header
        if(ftell(fidgif) == 0){
            writeGIFHeader(fidgif, header.Width, header.Height, opts.gifopts);
        }
        
        // Write frame to gif
        writeGIFFrame(fidgif, frame, header.Width, header.Height, opts.gifopts);
        
        fclose(fid);
    }
    
    // Write the gif end byte
    putc('\x3B', fidgif);
    
    // Close gif
    fclose(fidgif);
    
    // Free frame memory
    free(frame);
    
    printf("Finished!\n\n");
    
    return(0);
}

void usage(char **argv){
    printf("\nPNG to GIF converter.\n\n");
    printf("Usage: %s [opts] PNGfile1 [PNGfile2 ...]\n", argv[0]);
    printf(" opts:\n");
    printf("  -t, --timedelay <delay>    Time delay between frames in seconds (float)\n");
    printf("                              (default=0.25)\n");
    printf("  -d, --dither               Turn on dithering\n");
    printf("  -c, --colorpalette <name>  Set a specific color palette to be used.\n");
    printf("     Color palette options for <name>:\n");
    printf("      685g    6-8-5 level RGB with 15 gray and 1 transparent (default)\n");
    printf("      676g    6-7-6 level RGB with 3 gray and 1 transparent\n");
    printf("      884     8-8-4 level RGB with 0 gray and 0 transparent\n");
    printf("      web     6-6-6 level RGB, also known as the web palette, no transparent\n");
    printf("      median  Adaptive palette using the median cut algorithm, no transparent\n");
    printf("      gray    Grayscale palette, size determined by -n flag\n");
    printf("  -n, --ncolorbits <nbits>   Number of color bits to use in the color palette\n");
    printf("                              (default=8)\n");
    printf("  -f, --forcebw              Force black and white into color palette\n");
    printf("  -h, --help                 Print this help\n\n");
}

int checkPaletteOption(char* option){
    
    if(strcmp("685g", option) == 0){
        return P685g;
    }else if(strcmp("676g", option) == 0){
        return P676g;
    }else if(strcmp("884", option) == 0){
        return P884;
    }else if(strcmp("web", option) == 0){
        return Pweb;
    }else if(strcmp("median", option) == 0){
        return Pmedian;
    }else if(strcmp("gray", option) == 0){
        return Pgray;
    }else{
        printf("Unknown color palette option %s. Exiting.\n", option);
        exit(-1);
    }
}

OptStruct argParser(int argc, char **argv){
    
    int ch;
    OptStruct opts = newOptStructInst();
    
    static struct option longopts[] = {
        {"timedelay",    required_argument, NULL, 't'},
        {"dither",       no_argument,       NULL, 'd'},
        {"colorpalette", required_argument, NULL, 'c'},
        {"ncolorbits",   required_argument, NULL, 'n'},
        {"forcebw",      no_argument,       NULL, 'f'},
        {"help",         no_argument,       NULL, 'h'},
        {NULL,           0,                 NULL, 0  }
    };
    
    if(argc > 1){
        printf("Options selected:\n");
    }
    
    while ((ch = getopt_long(argc, argv, "t:dc:n:fh" ,longopts, NULL)) != -1){
        switch(ch){
            case 't':
                // Delay between frames in 1/100 sec
                opts.gifopts.delay = (uint16_t) (100*atof(optarg));
                printf(" Using %i ms between frames\n", opts.gifopts.delay);
                break;
            case 'd':
                opts.gifopts.dither = 1;
                printf(" Dithering will be performed.\n");
                break;
            case 'c':
                opts.gifopts.colorpalette = checkPaletteOption(optarg);
                printf(" Color palette \"%s\" will be used.\n", optarg);
                break;
            case 'n':
                opts.gifopts.colortablebitsize = atoi(optarg);
                int ncolors = 1 << opts.gifopts.colortablebitsize;
                printf(" Color table size will be %i colors.\n", ncolors);
                break;
            case 'f':
                opts.gifopts.forcebw = 1;
                printf(" Black and white colors will be forced.\n");
                break;
            case 'h':
            case '?':
            default:
                usage(argv);
                exit(0);
        }
    }
    
    // If forcing black and white colors then we also force the medianCut palette to be used
    if(opts.gifopts.forcebw == 1){
        opts.gifopts.colorpalette = checkPaletteOption("median");
        printf(" Forcing usage of \"median\" color palette due to forcebw flag.\n");
    }
    
    opts.fileind = optind;
    opts.nfile = argc - optind;
    
    if(opts.nfile < 1){
        usage(argv);
        exit(0);
    }
    
    // Make sure files exist and can be opened
    // If the gif output file then it doesn't have to exist
    for(int i=opts.fileind; i<argc; i++){
        FILE* fid = fopen(argv[i], "r");
        
        if(fid == 0){
            // If first file and there is more than one file then it is a gif and can be missing
            if(i == opts.fileind && opts.nfile > 1){
                continue;
            }
            printf("%s: Cannot open file %s. Exiting\n", argv[0], argv[i]);
            exit(-1);
        }
        
        fclose(fid);
    }

    return opts;
}
