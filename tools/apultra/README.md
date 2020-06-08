apultra -- a new, opensource optimal compressor for the apLib format
====================================================================

apultra is a command-line tool and a library that compresses bitstreams in the apLib format. 

The tool produces files that are 5 to 7% smaller on average than appack, the apLib compressor. Unlike the similar [cap](https://github.com/svendahl/cap) compressor, apultra can compress files larger than 64K (for files smaller than 64K, cap compresses 0.01% better on average).

apultra is written in portable C. It is fully open-source under a liberal license. You can continue to use the regular apLib decompression libraries for your target environment. You can do whatever you like with it.

    Example compression with vmlinux-5.3.0-1-amd64

    original       27923676 (100,00%)
    appack         7370129 (26,39%)
    gzip 1.8       7166179 (25,66%)
    apultra 1.2.2  6910729 (24,75%)


The output is fully compatible with the original [aPLib](http://ibsensoftware.com/products_aPLib.html) by Jørgen Ibsen.

Inspirations:

 * [cap](https://github.com/svendahl/cap) by Sven-Åke Dahl. 
 * [Charles Bloom](http://cbloomrants.blogspot.com/)'s compression blog. 
 * [LZ4](https://github.com/lz4/lz4) by Yann Collet. 
 * spke for help and support

Also of interest:
 * [oapack](https://gitlab.com/eugene77/oapack) by Eugene Larchenko, a brute-force (exhaustive) optimal packer for the aPLib format. 
 * Check out [Kitsune's Curse](https://www.usebox.net/jjm/kitsunes-curse/), a new game for the Amstrad CPC 464/6128 by usebox.net that uses apultra for compression.

License:

* The apultra code is available under the Zlib license.
* The match finder (matchfinder.c) is available under the CC0 license due to using portions of code from Eric Bigger's Wimlib in the suffix array-based matchfinder.
