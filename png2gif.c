
#include <string.h>

#include "pngReader.h"
#include "gifWriter.h"


int main () {
    FILE *fid;
    FILE *fidgif;
    char filename[] = "file1.png";
    uint8_t* frame=NULL;
    PNGHeader header;
    
    uint16_t delay = 25;  // Delay between frames in 1/100 sec
    
    // Open the gif file
    fidgif = fopen("file.gif", "wb");
    
    for(int i=1; i<2; i++){
        fid = fopen(filename, "rb");
        
        // Get png header and make sure the frame is the same size
        readPNGHeader(fid, &header);
        
        // Check for supported PNG formats
        if(header.ColorType != 2 && header.ColorType != 6){
            printf("Error: PNG reader only supports 24-bit or 32-bit Truecolor images (colorType=%i)\n", header.ColorType);
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
