
# PNG2GIF

png2gif is a lightweight PNG to GIF image conversion utility written with the intent to create animated GIF (hard-G, sorry Mr. Wilhite) from a series of PNG files. The only external dependency is zlib.

## License

png2gif is free software and its use is completely at your own risk;
it can be redistributed and/or modified under the terms of the [MIT License](http://www.opensource.org/licenses/MIT).

Copyright (c) 2019, Cory Rupp

This work uses the zlib library, which is released under the zlib license.
This work uses tinyfiledialogs, which is released under the zlib license.

## Development

This project started with the naive assumption that "Oh, it is just a matter of converting file formats. Surely the hardest part is figuring out the data compression, but handling that shouldn't be too bad." I very quickly realized how wrong I was as the depth and complexity of image files revealed themselves. This fascinating world introduced me to unexpected jewels such as PNG pre-compression filtering, color palette size reduction, dithering, LZW encoding, and little-Endian packing. This was a challenge I couldn't resist and an excuse to exercise my rusting C skills. I make no claims as to the quality of the code or the utility of the results, but if you find it useful then it makes this project all the more worthwhile. External contributions, bug reporting, and suggestions are encouraged and appreciated.

## Building

./buildme.sh

## Acknowledgements

This work uses the zlib library, which is released under the zlib license.
This work uses tinyfiledialogs, which is released under the zlib license.
