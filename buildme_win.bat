rem Windows build script uses MinGW
rem Easiest is to use cygwin and install these packages:
rem   mingw64-x86_64-gcc-core
rem   mingw64-x86_64-g++
rem   mingw64-x86_64-zlib
rem Make sure cygwin in on your path

set CC=x86_64-w64-mingw32-gcc
set CPP=x86_64-w64-mingw32-g++
set CFLAGS=-Wno-unused-result -O3
set CXXFLAGS=-std=c++0x -O3

%CPP% %CXXFLAGS% -c -o libLZWlib.o LZWlib.cpp

%CC% %CFLAGS% -c -o pngReader.o pngReader.c

%CC% %CFLAGS% -c -o gifWriter.o gifWriter.c

%CC% %CFLAGS% -c -o png2gif.o png2gif.c -I./tinyfiledialogs

%CC% %CFLAGS% -c -o tinyfiledialogs.o tinyfiledialogs/tinyfiledialogs.c

%CC% %CFLAGS% -c -o pixel.o pixel.c

%CC% %CFLAGS% -c -o palette.o palette.c

%CC% %CFLAGS% -c -o medianCut.o medianCut.c

%CC% %CFLAGS% -c -o dither.o dither.c

%CPP% %CXXFLAGS% -o png2gif.exe png2gif.o pngReader.o gifWriter.o pixel.o palette.o medianCut.o dither.o libLZWlib.o tinyfiledialogs.o -lz -lComdlg32 -lOle32 -static

