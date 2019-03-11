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

// Code adapted from: http://rosettacode.org/wiki/LZW_compression
// Heavily modified to/by:
//  Use variable width codes
//  Consider max table size
//  Find code width jump locations
//  Built into a library with C interface
// Cory Rupp, January 7, 2019
// MIT License

#include <string>
#include <map>
#include <vector>

#define MAXINT 2147483647
#define MAXCODESIZE 12  // In bits
#define DEBUG 0

// Compress a string to a list of output symbols.
// The result will be written to the output iterator
// starting at "result"; the final iterator is returned.
template <typename Iterator>
int compress(const std::string &uncompressed, Iterator result, uint32_t* widthjumps, uint8_t initialcodesize) {
    int last = 0;
    int count = 0;  // Number of input bytes used
    int jump = 0;
    int tableMaxed = 0;
    int ncodes = 0;  // Number of codes written
    int nbits = initialcodesize;
    int maxDictSize = 1 << nbits;
    
    // Initialize widthjumps to ensure that unused jumps won't happen
    for(int i=0;i<10;i++){
        widthjumps[i] = MAXINT;
    }
    
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
#if DEBUG
                    printf("maxDictSize=%x, dictSize=%x\n",maxDictSize,dictSize);
                    printf("Table size increase to %i bits at input %i, code %i\n", nbits, count, ncodes);
#endif
                }
                if(nbits >= MAXCODESIZE){
                    tableMaxed++;
#if DEBUG
                    printf("maxDictSize=%x, dictSize=%x\n",maxDictSize,dictSize);
                    printf("Table size maxed out at nbits %i, input %i, code %i\n", nbits, count, ncodes);
#endif
                }
            }
            
            if(tableMaxed >= 1 && dictSize >= maxDictSize){// && dictionary[w] > maxDictSize){
                if(last > 0){
                // Clear table and return number of input bytes consumed
                    return count-1;
                }else{
                    last++;
                    // Can increment "tableMaxed" instead of "last" and keep the table size at the max, however this typically results in larger file sizes
//                    tableMaxed++;
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
    
#if DEBUG
    printf("dictSize: %i\n",dictSize);
    printf("count: %i, ncodes written: %i\n", count, ncodes);
#endif
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
#if DEBUG
    printf("inputsize=%lu\n", invec.size());
    printf("initialcodesize=%i\n", initialcodesize);
#endif
    int ninputused = compress(invec, std::back_inserter(compressed), widthjumps, initialcodesize);
#if DEBUG
    printf("ninputused=%i\n", ninputused);
#endif
    *inlen -= ninputused;
    *input += ninputused;
    copy(compressed.begin(), compressed.end(), *output);
    *output += compressed.size();
#if DEBUG
    printf("outputsize=%lu\n", compressed.size());
#endif
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
