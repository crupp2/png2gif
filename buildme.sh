clang++ -g -c -o libLZWlib.o LZWlib.cpp

clang -g -c -o pngReader.o pngReader.c

clang -g -c -o gifWriter.o gifWriter.c

clang -g -c -o png2gif.o png2gif.c

clang -g -c -o medianCut.o medianCut.c

clang -g -c -o dither.o dither.c

clang++ -o png2gif png2gif.o pngReader.o gifWriter.o medianCut.o dither.o libLZWlib.o -lz

clang -g -c -o LZWcompressTest.o LZWcompressTest.c
clang++ -o lzwtest LZWcompressTest.o libLZWlib.o -lz
