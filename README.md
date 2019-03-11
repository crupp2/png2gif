
PNG2GIF - A lightweight PNG to GIF image conversion utility (and movie creator too!)

png2gif is a simple image conversion utility written with the intent to create animated GIF (hard-G) from a series of PNG files. The only external dependency is zlib.

This project started with the naive assumption that "Oh, it is just a matter of converting file formats. Surely the hardest part is figuring out the data compression, but handling that shouldn't be too bad." I very quickly realized how wrong I was as the depth and complexity of image files revealed themselves. This fascinating world introduced me to unexpected jewels such as PNG pre-compression filtering, color palette size reduction, dithering, LZW encoding, and little-Endian packing.