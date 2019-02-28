// Code adapted from: http://rosettacode.org/wiki/LZW_compression
// Modified to use vector<uint8_t> rather than strings to allow for \x0
// Keep max table size to 9 bits
// Built into a library
// Cory Rupp, January 7, 2019
// MIT License

#include <string>
#include <map>
#include <vector>

#define MAXTABLESIZE 12  // In bits

// Compress a string to a list of output symbols.
// The result will be written to the output iterator
// starting at "result"; the final iterator is returned.
template <typename Iterator>
int compress(const std::string &uncompressed, Iterator result, uint32_t* widthjumps, uint8_t initialcodesize) {
    int junk = 0;
    int count = 0;  // Number of input bytes used
    int jump = 0;
    int tableMaxed = 0;
    int ncodes = 0;  // Number of codes written
    int nbits = initialcodesize;
    int maxDictSize = 1 << nbits;
    // Build the dictionary.
    int dictSize = 1 << (nbits-1);
    std::map<std::string,int> dictionary;
    for (int i = 0; i < dictSize; i++)
        dictionary[std::string(1, i)] = i;
    
    // increment dictSize by two for clear (0x100) and stop (0x101) codes
    dictSize += 2;
    
    std::string w;
    for (std::string::const_iterator it = uncompressed.begin();
         it != uncompressed.end(); ++it) {
        char c = *it;
        std::string wc = w + c;
        count++;
        // If wc is in the dictionary then add another character and continue
        if (dictionary.count(wc)) {
            w = wc;
        } else {
            // If wc is not in the dictionary then, add the code for w to the output
            *result++ = dictionary[w];
            ncodes++;
            // Add wc to the dictionary
            if(tableMaxed < 2){
                dictionary[wc] = dictSize++;
            }
            // If the table is full, then increase its size
            if(tableMaxed < 1 && dictSize >= maxDictSize){
                if(tableMaxed < 1){
                    widthjumps[jump] = ncodes;
                    jump++;
                    nbits++;
                    maxDictSize = 1 << nbits;
                    printf("maxDictSize=%x, dictSize=%x\n",maxDictSize,dictSize);
                    printf("Table size increase to %i bits at input %i, code %i\n", nbits, count, ncodes);
                }
                if(nbits >= MAXTABLESIZE){
                    tableMaxed++;
                    printf("maxDictSize=%x, dictSize=%x\n",maxDictSize,dictSize);
                    printf("Table size maxed out at nbits %i, input %i, code %i\n", nbits, count, ncodes);
                }
            }
            
            if(tableMaxed >= 1 && dictSize >= maxDictSize){// && dictionary[w] > maxDictSize){
                if(junk > 0){
                // Clear table and return number of input bytes consumed
                return count;
                }else{
                    junk++;
                }
            }
            
            w = c;
        }
    }
    
    // Output the code for w.
    if (!w.empty()){
        *result++ = dictionary[w];
        ncodes++;
    }
    
    // End result with stop code
//    *result++ = 0x101;
//    ncodes++;
    
    printf("dictSize: %i\n",dictSize);
    printf("count: %i, ncodes written: %i\n", count, ncodes);
//    return result;
    return count;
}

// Compress a string to a list of output symbols.
// The result will be written to the output iterator
// starting at "result"; the final iterator is returned.
template <typename Iterator>
Iterator compress9bit(const std::string &uncompressed, Iterator result) {
    int once = 1;
    int count = 0;
    int maxDictSize = 1 << 9;
    // Build the dictionary.
    int dictSize = 256;
    std::map<std::string,int> dictionary;
    for (int i = 0; i < 256; i++)
        dictionary[std::string(1, i)] = i;
    
    // increment dictSize by two for clear (0x100) and stop codes (0x101)
    dictSize += 2;
    
    std::string w;
    for (std::string::const_iterator it = uncompressed.begin();
         it != uncompressed.end(); ++it) {
        char c = *it;
        std::string wc = w + c;
        count++;
        // If wc is in the dictionary then add another character and continue
        if (dictionary.count(wc)) {
            w = wc;
        } else {
            // If wc is not in the dictionary then, add the code for w to the output
            *result++ = dictionary[w];
            // Add wc to the dictionary if it is not full
            if(dictSize < maxDictSize){
                dictionary[wc] = dictSize++;
            }else{
                if(once){
                    printf("Table exhausted at %i\n",count);
                    once = 0;
                }
            }
            //            w = std::string(1, *it);
            w = c;
        }
    }
    
    // Output the code for w.
    if (!w.empty())
        *result++ = dictionary[w];
    return result;
}

// Decompress a list of output ks to a string.
// "begin" and "end" must form a valid range of ints
template <typename Iterator>
std::string decompress(Iterator begin, Iterator end) {
    // Build the dictionary.
    int dictSize = 256;
    std::map<int,std::string> dictionary;
    for (int i = 0; i < 256; i++)
        dictionary[i] = std::string(1, i);
    
    std::string w(1, *begin++);
    std::string result = w;
    std::string entry;
    for ( ; begin != end; begin++) {
        int k = *begin;
        if (dictionary.count(k))
            entry = dictionary[k];
        else if (k == dictSize)
            entry = w + w[0];
        else
            throw "Bad compressed k";
        
        result += entry;
        
        // Add w+entry[0] to the dictionary.
        dictionary[dictSize++] = w + entry[0];
        
        w = entry;
    }
    return result;
}

#include <iostream>
#include <iterator>

//int main() {
//    std::vector<int> compressed;
//    compress("TOBEORNOTTOBEORTOBEORNOT", std::back_inserter(compressed));
//    copy(compressed.begin(), compressed.end(), std::ostream_iterator<int>(std::cout, ", "));
//    std::cout << std::endl;
//    std::string decompressed = decompress(compressed.begin(), compressed.end());
//    std::cout << decompressed << std::endl;
//
//    return 0;
//}

extern "C" int LZWcompress(uint8_t** input, uint32_t *inlen, uint16_t** output, uint32_t* widthjumps, uint8_t initialcodesize) {
    std::string invec = std::string(*input, *input + sizeof(uint8_t)*(*inlen));
    std::vector<uint16_t> compressed;
    printf("inputsize=%lu\n", invec.size());
    printf("initialcodesize=%i\n", initialcodesize);
    int ninputused = compress(invec, std::back_inserter(compressed), widthjumps, initialcodesize);
    printf("ninputused=%i\n", ninputused);
    *inlen -= ninputused;
    *input += ninputused;
    copy(compressed.begin(), compressed.end(), *output);
    *output += compressed.size();
    printf("outputsize=%lu\n", compressed.size());
    return compressed.size();
}

extern "C" int LZWcompress9bit(uint8_t* input, uint32_t inlen, uint16_t* output) {
    std::string invec = std::string(input, input + sizeof(uint8_t)*inlen);
    std::vector<uint16_t> compressed;
    printf("inputsize=%lu\n", invec.size());
    compress9bit(invec, std::back_inserter(compressed));
    copy(compressed.begin(), compressed.end(), output);
    return compressed.size();
}
