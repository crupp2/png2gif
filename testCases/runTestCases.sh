# Convert single png frame to a single gif frame
../png2gif -c gray file1b.png
../png2gif file1c.png --colorpalette median
../png2gif file1d.png -c median
../png2gif -t 0.25 file1e.png -c median
../png2gif -c gray file1f.png
../png2gif -c gray file1g.gif file1g.png

# Test dithering
../png2gif file1c_dither.gif -c median file1c.png -d

# Test ncolorbits and forcebw
../png2gif -c gray file1f_full_dither.gif -n 1 file1f_full.png -d -f

# Test various color palettes
../png2gif file1e_685g.gif file1e.png

# Test animation
cd movie
../../png2gif -t 0.02 -c median file1a.gif file1a_f-01.png file1a_f-02.png file1a_f-03.png file1a_f-04.png file1a_f-05.png file1a_f-06.png file1a_f-07.png file1a_f-08.png file1a_f-09.png file1a_f-10.png file1a_f-11.png file1a_f-12.png file1a_f-13.png file1a_f-14.png file1a_f-15.png file1a_f-16.png file1a_f-17.png file1a_f-18.png file1a_f-19.png file1a_f-20.png file1a_f-21.png file1a_f-22.png file1a_f-23.png file1a_f-24.png file1a_f-25.png file1a_f-26.png file1a_f-27.png file1a_f-28.png file1a_f-29.png file1a_f-30.png file1a_f-31.png file1a_f-32.png file1a_f-33.png file1a_f-34.png file1a_f-35.png file1a_f-36.png
../../png2gif -t 0.02 -c gray file1a_gray.gif file1a_f-01.png file1a_f-02.png file1a_f-03.png file1a_f-04.png file1a_f-05.png file1a_f-06.png file1a_f-07.png file1a_f-08.png file1a_f-09.png file1a_f-10.png file1a_f-11.png file1a_f-12.png file1a_f-13.png file1a_f-14.png file1a_f-15.png file1a_f-16.png file1a_f-17.png file1a_f-18.png file1a_f-19.png file1a_f-20.png file1a_f-21.png file1a_f-22.png file1a_f-23.png file1a_f-24.png file1a_f-25.png file1a_f-26.png file1a_f-27.png file1a_f-28.png file1a_f-29.png file1a_f-30.png file1a_f-31.png file1a_f-32.png file1a_f-33.png file1a_f-34.png file1a_f-35.png file1a_f-36.png
cd ..
