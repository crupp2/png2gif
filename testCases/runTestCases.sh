# MacOS
export PNG2GIF="../png2gif.app"
# Linux
#export PNG2GIF="../png2gif"
# Windows
#set PNG2GIF="../png2gif.exe"

# Convert single png frame to a single gif frame
$PNG2GIF -c gray file1b.png > testcases.log
$PNG2GIF file1c.png --colorpalette median >> testcases.log
$PNG2GIF file1d.png -c median >> testcases.log
$PNG2GIF -t 0.25 file1e.png -c median >> testcases.log
$PNG2GIF -c gray file1f.png >> testcases.log
$PNG2GIF -c gray file1g.gif file1g.png >> testcases.log

# Test dithering
$PNG2GIF file1c_dither.gif -c median file1c.png -d >> testcases.log

# Test ncolorbits and forcebw
$PNG2GIF -c gray file1f_full_dither.gif -n 1 file1f_full.png -d -f >> testcases.log

# Test various color palettes
$PNG2GIF file1e_685g.gif file1e.png >> testcases.log
$PNG2GIF file1h.png >> testcases.log
$PNG2GIF file1i_685g.gif file1i.png -c 685g >> testcases.log
$PNG2GIF file1i_676g.gif file1i.png -c 676g >> testcases.log
$PNG2GIF file1i_844.gif file1i.png -c 884 >> testcases.log
$PNG2GIF file1i_web.gif file1i.png -c web >> testcases.log
$PNG2GIF file1i_median.gif file1i.png -c median >> testcases.log
$PNG2GIF file1i_gray.gif file1i.png -c gray >> testcases.log
$PNG2GIF file1i_grayT.gif file1i.png -c grayT >> testcases.log

# Test animation
cd movie
../$PNG2GIF -t 0.02 -c median file1a.gif file1a_f-01.png file1a_f-02.png file1a_f-03.png file1a_f-04.png file1a_f-05.png file1a_f-06.png file1a_f-07.png file1a_f-08.png file1a_f-09.png file1a_f-10.png file1a_f-11.png file1a_f-12.png file1a_f-13.png file1a_f-14.png file1a_f-15.png file1a_f-16.png file1a_f-17.png file1a_f-18.png file1a_f-19.png file1a_f-20.png file1a_f-21.png file1a_f-22.png file1a_f-23.png file1a_f-24.png file1a_f-25.png file1a_f-26.png file1a_f-27.png file1a_f-28.png file1a_f-29.png file1a_f-30.png file1a_f-31.png file1a_f-32.png file1a_f-33.png file1a_f-34.png file1a_f-35.png file1a_f-36.png >> ../testcases.log
../$PNG2GIF -t 0.02 -c grayT file1a_gray.gif file1a_f-01.png file1a_f-02.png file1a_f-03.png file1a_f-04.png file1a_f-05.png file1a_f-06.png file1a_f-07.png file1a_f-08.png file1a_f-09.png file1a_f-10.png file1a_f-11.png file1a_f-12.png file1a_f-13.png file1a_f-14.png file1a_f-15.png file1a_f-16.png file1a_f-17.png file1a_f-18.png file1a_f-19.png file1a_f-20.png file1a_f-21.png file1a_f-22.png file1a_f-23.png file1a_f-24.png file1a_f-25.png file1a_f-26.png file1a_f-27.png file1a_f-28.png file1a_f-29.png file1a_f-30.png file1a_f-31.png file1a_f-32.png file1a_f-33.png file1a_f-34.png file1a_f-35.png file1a_f-36.png >> ../testcases.log
cd ..
