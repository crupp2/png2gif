
#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE 16284

// From LZWlib.cpp
int LZWcompress(uint8_t* input, uint32_t inlen, uint16_t* output);

int main() {
    
    uint16_t* buffer = malloc(sizeof(uint16_t)*BUFFSIZE);
//    uint8_t frame[] = "TOBEORNOTTOBEORTOBEORNOT";
    uint8_t frame[] = "\x28\xFF\xFF\xFF\x28\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
    
    
    int n = LZWcompress(frame, sizeof(frame)-1, buffer);
    printf("n=%d\n", n);
    printf("output=x");
    for(int i=0;i<n;i++){
        printf(" %03x", buffer[i]);
    }
    printf("\n");
    printf("output=d");
    for(int i=0;i<n;i++){
        printf(" %03i", buffer[i]);
    }
    printf("\n");

    return 0;
}



//std::vector<int> compressed;
//compress("TOBEORNOTTOBEORTOBEORNOT", std::back_inserter(compressed));
//copy(compressed.begin(), compressed.end(), std::ostream_iterator<int>(std::cout, ", "));
//std::cout << std::endl;
//std::string decompressed = decompress(compressed.begin(), compressed.end());
//std::cout << decompressed << std::endl;


// Expecting
// (dec) 84, 79, 66, 69, 79, 82, 78, 79, 84, 256, 258, 260, 265, 259, 261, 263
// (hex) 054 04f 042 045 04f 052 04e 04f 054 100 102 104 109 103 105 107
//
// (hex) 028 0ff 103 102 103 106 107
