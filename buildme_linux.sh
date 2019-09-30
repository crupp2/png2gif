#export CFLAGS=-ansi -g
export CFLAGS="-Wno-unused-result -O3"
export CXXFLAGS="-std=c++0x -O3"

g++ $CXXFLAGS -c -o libLZWlib.o LZWlib.cpp

gcc $CFLAGS -c -o pngReader.o pngReader.c

gcc $CFLAGS -c -o gifWriter.o gifWriter.c

gcc $CFLAGS -c -o png2gif.o png2gif.c -I./tinyfiledialogs

gcc $CFLAGS -c -o tinyfiledialogs.o tinyfiledialogs/tinyfiledialogs.c

gcc $CFLAGS -c -o pixel.o pixel.c

gcc $CFLAGS -c -o palette.o palette.c

gcc $CFLAGS -c -o medianCut.o medianCut.c

gcc $CFLAGS -c -o dither.o dither.c

g++ $CXXFLAGS -o png2gif png2gif.o pngReader.o gifWriter.o pixel.o palette.o medianCut.o dither.o libLZWlib.o tinyfiledialogs.o -lz
