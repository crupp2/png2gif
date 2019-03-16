#export CFLAGS=-g
export CFLAGS=-O3

clang++ $CFLAGS -c -o libLZWlib.o LZWlib.cpp

clang $CFLAGS -c -o pngReader.o pngReader.c

clang $CFLAGS -c -o gifWriter.o gifWriter.c

clang $CFLAGS -c -o png2gif.o png2gif.c

clang $CFLAGS -c -o medianCut.o medianCut.c

clang $CFLAGS -c -o palette.o palette.c

clang $CFLAGS -c -o dither.o dither.c

clang++ $CFLAGS -o png2gif png2gif.o pngReader.o gifWriter.o palette.o medianCut.o dither.o libLZWlib.o -lz

clang -g -c -o LZWcompressTest.o LZWcompressTest.c
clang++ -o lzwtest LZWcompressTest.o libLZWlib.o -lz
