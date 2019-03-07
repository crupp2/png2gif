
#include <string.h>
#include <getopt.h>

#include "pngReader.h"
#include "gifWriter.h"

#define FILENAMESIZE 256


typedef struct _OptStruct {
    int fileind;
    int nfile;
    float delay;
    int dither;
} OptStruct;

OptStruct argParser(int argc, char **argv);


int main (int argc, char **argv) {
    FILE *fid;
    FILE *fidgif;
    char giffilename[FILENAMESIZE];
    int pngfileind;
    uint8_t* frame=NULL;
    PNGHeader header;
    GIFOptStruct gifopts;
    
    OptStruct opts = argParser(argc, argv);
    
    gifopts.delay = (uint16_t) (opts.delay*100);  // Delay between frames in 1/100 sec
    gifopts.dither = opts.dither;
    
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
            writeGIFHeader(fidgif, header.Width, header.Height, gifopts);
        }
        
        // Write frame to gif
        writeGIFFrame(fidgif, frame, header.Width, header.Height, gifopts);
        
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

void usage(char **argv){
    printf("\nPNG to GIF converter.\n\n Usage: %s [-t <delay>] PNGfile1 [PNGfile2 ...]\n\n", argv[0]);
}

OptStruct argParser(int argc, char **argv){
    
    int ch;
    OptStruct opts;
    
    // Set defaults
    opts.delay = 0.25;  // Delay in seconds
    
    static struct option longopts[] = {
        {"delay",  required_argument, NULL, 't'},
        {"dither", no_argument,       NULL, 'd'},
        {"help",   no_argument,       NULL, 'h'}
    };

    while ((ch = getopt_long(argc, argv, "tdh:" ,longopts, NULL)) != -1){
        switch(ch){
            case 't':
                opts.delay = atof(optarg);
                break;
            case 'd':
                opts.dither = 1;
                break;
            case 'h':
            case '?':
            default:
                usage(argv);
                exit(0);
        }
    }
    
    opts.fileind = optind;
    opts.nfile = argc - optind;
    printf("fileInd=%i, nfile=%d, delay=%f\n", opts.fileind, opts.nfile, opts.delay);
    
    if(opts.nfile < 1){
        printf("%s: At least one input file required. Exiting.\n", argv[0]);
        exit(-1);
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
