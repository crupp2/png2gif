# Convert single png frame to a single gif frame
../png2gif file1b.png
../png2gif file1c.png
../png2gif file1d.png
../png2gif -t 0.25 file1e.png
../png2gif file1f.png
../png2gif file1g.gif file1g.png

# Test dithering
../png2gif file1c_dither.gif file1c.png -d

# Test ncolorbits
../png2gif file1f_full_dither.gif -n 1 file1f_full.png -d
