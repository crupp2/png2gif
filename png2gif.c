
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
    
    printf("Starting conversion of PNG file(s) to GIF file.\n");
    
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
    printf("\nPNG to GIF converter.\n\n Usage: %s [-t <delay>] PNGfile1 [PNGfile2 ...]\n\n", argv[0]);
}

OptStruct argParser(int argc, char **argv){
    
    int ch;
    OptStruct opts = newOptStructInst();
    
    static struct option longopts[] = {
        {"timedelay",  required_argument, NULL, 't'},
        {"dither",     no_argument,       NULL, 'd'},
        {"ncolorbits", required_argument, NULL, 'n'},
        {"help",       no_argument,       NULL, 'h'},
        {NULL,         0,                 NULL, 0  }
    };
    
    printf("Options selected:\n");
    while ((ch = getopt_long(argc, argv, "t:dn:h" ,longopts, NULL)) != -1){
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
            case 'n':
                opts.gifopts.colortablebitsize = atoi(optarg);
                int ncolors = 1 << opts.gifopts.colortablebitsize;
                printf(" Color table size will be %i colors.\n", ncolors);
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
