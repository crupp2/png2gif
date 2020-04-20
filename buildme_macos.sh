#export CFLAGS=-g
export CFLAGS=-O3

clang++ $CFLAGS -c -o libLZWlib.o LZWlib.cpp

clang $CFLAGS -c -o pngReader.o pngReader.c

clang $CFLAGS -c -o gifWriter.o gifWriter.c

clang $CFLAGS -c -o png2gif.o png2gif.c -Itinyfiledialogs

clang $CFLAGS -c -o tinyfiledialogs.o tinyfiledialogs/tinyfiledialogs.c

clang $CFLAGS -c -o pixel.o pixel.c

clang $CFLAGS -c -o palette.o palette.c

clang $CFLAGS -c -o medianCut.o medianCut.c

clang $CFLAGS -c -o dither.o dither.c

clang++ $CFLAGS -o png2gif png2gif.o pngReader.o gifWriter.o pixel.o palette.o medianCut.o dither.o libLZWlib.o tinyfiledialogs.o -lz

# Make an app
rm -rf png2gif.app
mkdir -p png2gif.app/Contents/{MacOS,Resources}
cp png2gif png2gif.app/Contents/MacOS/
cp "/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/GenericApplicationIcon.icns" png2gif.app/Contents/Resources/png2gif.icns
cp png2gif_macos.command png2gif.app/Contents/MacOS/
chmod +x png2gif.app/Contents/MacOS
cp Info.plist png2gif.app/Contents/
