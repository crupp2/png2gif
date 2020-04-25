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

#define VERSION "0.1"

#include <string.h>
#include <getopt.h>
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#include <windows.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

#include "tinyfiledialogs.h"

#include "pngReader.h"
#include "gifWriter.h"

#define MAX_ARG 256
const char pathSeparator =
#if defined(_WIN32) || defined(_WIN64)
'\\';
#else
'/';
#endif


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

OptStruct argParser(int* argc, char ***argv);

int startGUI(char **argv);


int main (int argc, char **argv) {
    FILE *fid;
    FILE *fidgif;
    char giffilename[FILENAME_MAX];
    int pngfileind;
    uint8_t* frame0=NULL;
    uint8_t* frame1=NULL;
    uint8_t* curframeptr=NULL;
    uint8_t* lastframeptr=NULL;
    int frameselect = 0;
    int isFirstFrame = 1;
    PNGHeader header;
    
    // argParser will update where argc and argv point to, so need to pass in by reference
    OptStruct opts = argParser(&argc, &argv);
    
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
        // Do this here because we only now know the frame size
        // Always support RGBA size, just in case
        if(frame0==NULL){
            frame0 = malloc(sizeof(uint8_t)*4*header.Width*header.Height+header.Height);  // RBG bytes + png scanline filter bytes
            memset(frame0, 0, sizeof(uint8_t)*4*header.Width*header.Height);
            frame1 = malloc(sizeof(uint8_t)*4*header.Width*header.Height+header.Height);  // RBG bytes + png scanline filter bytes
            memset(frame1, 0, sizeof(uint8_t)*4*header.Width*header.Height);
        }
        
        // Figure out which frame is the current and previous frame
        if(frameselect == 0){
            curframeptr = frame0;
            lastframeptr = frame1;
            // Update frameselect
            frameselect = 1;
        }else{
            curframeptr = frame1;
            lastframeptr = frame0;
            // Update frameselect
            frameselect = 0;
        }
        
        // Get png frame in rgb raw format
        if(header.ColorType == 2){
            readPNGFrame(fid, header.Width, header.Height, curframeptr, 3);
        }else if(header.ColorType == 6){
            readPNGFrame(fid, header.Width, header.Height, curframeptr, 4);
        }else{
            printf("Error: Should never get here, unsupported PNG color type\n");
            return -1;
        }
        
        // If just starting then write the gif header
        if(ftell(fidgif) == 0){
            writeGIFHeader(fidgif, header.Width, header.Height, opts.gifopts);
            
            // If more than one frame then write the application extension to enable looping animations
            if((argc-pngfileind) > 1){
                writeGIFAppExtension(fidgif);
            }
        }
        
        // Write frame to gif
        writeGIFFrame(fidgif, curframeptr, lastframeptr, header.Width, header.Height, opts.gifopts, isFirstFrame);
        isFirstFrame = 0;
        
        fclose(fid);
    }
    
    // Write the gif end byte
    putc('\x3B', fidgif);
    
    // Close gif
    fclose(fidgif);
    
    // Free frame memory
    free(frame1);
    free(frame0);
    
    printf("Finished!\n\n");
    
    return(0);
}

void usage(char **argv){
    printf("\nPNG to GIF converter.\n\n");
    printf("Usage: %s [opts] [GIFfile] PNGfile1 [PNGfile2 ...]\n", argv[0]);
    printf(" If GIFfile is omitted then it will be inferred by the name of PNGfile1.\n");
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
    printf("      gray    Grayscale palette, no transparent, size determined by -n flag\n");
    printf("      grayT   Grayscale palette, with transparent, size determined by -n flag\n");
    printf("  -n, --ncolorbits <nbits>   Number of color bits to use in the color palette\n");
    printf("                              (default=8)\n");
    printf("  -f, --forcebw              Force black and white into color palette\n");
    printf("  -s, --silent               Silent mode\n");
    printf("  -v, --version              Print version number\n");
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
    }else if(strcmp("grayT", option) == 0){
        return PgrayT;
    }else{
        printf("Unknown color palette option %s. Exiting.\n", option);
        exit(-1);
    }
}

OptStruct argParser(int* argc, char ***argv){
    
    int narg = *argc;
    char** args = *argv;
    int ch;
    int printStartText = 1;
    int useGUI = 0;
    OptStruct opts = newOptStructInst();

    // Because Windows is Windows, need to check to see if we are executing from the command line or from a double-click (see https://devblogs.microsoft.com/oldnewthing/20160125-00/?p=92922 except that you can't pass in nullptr or 0 to GetConsoleProcessList)
    // For Windows if double-clicked then start from the GUI, otherwise start without the GUI unless requested
#if defined(_WIN32) || defined(_WIN64)
    LPDWORD lpdwProcessList[8];
    DWORD nproc = GetConsoleProcessList(lpdwProcessList[0], 8);
    if(nproc == 1){
        // Started by double-click
        useGUI = 1;
    }
#endif
    
    static struct option longopts[] = {
        {"timedelay",    required_argument, NULL, 't'},
        {"dither",       no_argument,       NULL, 'd'},
        {"colorpalette", required_argument, NULL, 'c'},
        {"ncolorbits",   required_argument, NULL, 'n'},
        {"forcebw",      no_argument,       NULL, 'f'},
        {"silent",       no_argument,       NULL, 's'},
        {"usegui",       no_argument,       NULL, 'g'},
        {"version",      no_argument,       NULL, 'v'},
        {"help",         no_argument,       NULL, 'h'},
        {NULL,           0,                 NULL, 0  }
    };
    
    // First check for silent mode to ensure that we are indeed silent
    // Also check for -v or -h to avoid startup and option string printing
    // Check for GUI flag as well
    while ((ch = getopt_long(narg, args, "t:dc:n:fsgvh" ,longopts, NULL)) != -1){
        switch(ch){
            case 's':
                // Set up silent mode
                freopen("/dev/null", "w" ,stdout);
                break;
            case 'v':
            case 'h':
                printStartText = 0;
                break;
            case 'g':
                useGUI = 1;
                break;
            default:
                break;
        }
    }

    // Update argv if using a GUI
    if(useGUI > 0){
        // Use the GUI for file selection
        // Need to allocate args here (this will leak, but we need it to since args can't be deallocated until sometime after exiting this function)
        args = malloc(MAX_ARG * sizeof(char *)); // Allocate row pointers
        for(int i = 0; i < MAX_ARG; i++){
            args[i] = malloc(FILENAME_MAX * sizeof(char));
        }
        narg = startGUI(args);
        *argc = narg;
        *argv = args;
    }

    if(printStartText > 0 && narg > 1){
        printf("Starting conversion of PNG file(s) to GIF file.\n");
        printf("Options selected:\n");
    }
    
    // Reset optind for getopt
    optind = 0;
    while ((ch = getopt_long(narg, args, "t:dc:n:fsgvh" ,longopts, NULL)) != -1){
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
            case 'v':
                printf("\n png2gif version %s\n\n", VERSION);
                exit(0);
            case 'h':
            case '?':
            default:
                usage(*argv);
                exit(0);
        }
    }
    
    // If forcing black and white colors then we also force the medianCut palette to be used
    if(opts.gifopts.forcebw == 1){
        opts.gifopts.colorpalette = checkPaletteOption("median");
        printf(" Forcing usage of \"median\" color palette due to forcebw flag.\n");
    }
    
    opts.fileind = optind;
    opts.nfile = narg - optind;
    
    if(opts.nfile < 1){
        usage(*argv);
        exit(0);
    }
    
    // Make sure files exist and can be opened
    // If the gif output file then it doesn't have to exist
    for(int i=opts.fileind; i<narg; i++){
        FILE* fid = fopen(args[i], "r");
        
        if(fid == 0){
            // If first file and there is more than one file then it is a gif and can be missing
            if(i == opts.fileind && opts.nfile > 1){
                continue;
            }
            printf("%s: Cannot open file %s. Exiting\n", (*argv)[0], args[i]);
            exit(-1);
        }
        
        fclose(fid);
    }

    return opts;
}

int startGUI(char **args){
    
    char const * PNGfilt[2] = { "*.png", "*.*" };
    char const * GIFfilt[2] = { "*.gif", "*.*" };
    int count = 1;
    
    // Get cwd and add separator
    char cwd[FILENAME_MAX];
    GetCurrentDir(cwd, FILENAME_MAX);
    sprintf(cwd, "%s%c", cwd, pathSeparator);
    
    // Open dialog for selecting PNG files
    char const *pngfilenames = tinyfd_openFileDialog("Select PNG files to convert", cwd, 1, PNGfilt, "PNG files", 1);
    if(pngfilenames == NULL){
        // User cancel
        exit(0);
    }

    // Get GIF filename
    char const *giffilename = tinyfd_saveFileDialog("Save GIF file as...", cwd, 1, GIFfilt, "GIF files");
    if(giffilename == NULL){
        // User cancel
        exit(0);
    }
    
    // Display message box with argument details
    // First, build the message string, building up argv along the way
    char message[16384];
    char buffer[16384];
    strcpy(message, "Starting conversion of PNG file(s) to GIF file.\n\n");
    
    sprintf(message, "%sOutput file:\n  %s\n", message, giffilename);
    strcpy(args[count], giffilename);
    count++;
    
    strcat(message, "\nInput file(s):\n");
    strcpy(buffer, pngfilenames);
    char* tok = strtok(buffer, "|");
    while(tok != NULL){
        sprintf(message, "%s  %s\n", message, tok);
        strcpy(args[count], tok);
        tok = strtok(NULL, "|");
        count++;
    }
    
    // Query time delay between frames (only if more than one input file)
    if(count > 3){
        char const *timedelay = tinyfd_inputBox("Enter time delay", "Enter time delay between frames (in seconds)", "0.25");
        if(timedelay == NULL){
            // User cancel
            exit(0);
        }
        sprintf(message, "%s\nTime delay: %s seconds\n", message, timedelay);
        strcpy(args[count], "-t");
        count++;
        strcpy(args[count], timedelay);
        count++;
    }
    
    strcat(message, "\nOk to proceed?");
    
    int ret = tinyfd_messageBox("Ok to proceed?", message, "okcancel", "question", 1);

    if(ret == 0){
        // User canceled
        exit(0);
    }

    // Print out equivalent command line options
    for(int i=1;i<count;i++){
        printf("%s\n",args[i]);
    }
    
    // Set up silent mode when using the GUI
#if defined(_WIN32) || defined(_WIN64)
    freopen("NUL:", "w", stdout);
#else
    freopen("/dev/null", "w", stdout);
#endif
    
    // Return narg
    return count;
    
}
