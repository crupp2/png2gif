export CXX=clang++
export CC=clang

#export CFLAGS="-g -fsanitize=address -O1 -fno-omit-frame-pointer -Wall"
export CFLAGS=-O3

$CXX $CFLAGS -c -o libLZWlib.o LZWlib.cpp

$CC $CFLAGS -c -o pngReader.o pngReader.c

$CC $CFLAGS -c -o gifWriter.o gifWriter.c

$CC $CFLAGS -c -o png2gif.o png2gif.c -Itinyfiledialogs

$CC $CFLAGS -c -o tinyfiledialogs.o tinyfiledialogs/tinyfiledialogs.c

$CC $CFLAGS -c -o pixel.o pixel.c

$CC $CFLAGS -c -o palette.o palette.c

$CC $CFLAGS -c -o medianCut.o medianCut.c

$CC $CFLAGS -c -o dither.o dither.c

$CXX $CFLAGS -o png2gif png2gif.o pngReader.o gifWriter.o pixel.o palette.o medianCut.o dither.o libLZWlib.o tinyfiledialogs.o -lz

# Make an app
rm -rf png2gif.app
mkdir -p png2gif.app/Contents/{MacOS,Resources}
cp png2gif png2gif.app/Contents/MacOS/
cp "/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/GenericApplicationIcon.icns" png2gif.app/Contents/Resources/png2gif.icns
cp png2gif_macos.command png2gif.app/Contents/MacOS/
chmod +x png2gif.app/Contents/MacOS
cp Info.plist png2gif.app/Contents/

# Clean up (turn off for debugging)
rm *.o
