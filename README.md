# *Stunt Island* tools

Various side projects dedicated to this underappreciated game.

## `si-8x`: 8x Detail Patch

(aka “Ace got glasses”)

A mod enhancing the detail level to an equivalent of 800%. The “Detail” slider
in the preferences is ignored; all other preferences work as normal.

Version 1.1 (17.03.2026).

### To use

* Copy the built executable into the directory where `STUNT.EXE` and
  `MAKEONE.EXE` are located.
* Launch it.
* The files `STUNT1.EXE`, `MAKEONE1.EXE` and `PLAYONE1.EXE` should appear.

Alternatively, you can start the program from a terminal, in which case you can
provide the location of your Stunt Island directory as parameter.

### Supported versions

You can check whether your version is supported by looking at the size of
`STUNT.EXE`. The mod supports currently these versions:

* 137240 Bytes (GOG version; the one by Steam should be identical)
* 134784 Bytes (created by the “P3 - Final Patch Set” provided by
                www.planetmic.com/stunt)

### Making of

In [this blog post](https://marnetto.net/2024/11/20/tweaking-stunt-island).

### License

MIT (see below)

-------------------------------------------------------------------------------

## `si-free-flight-2`: *SI Free Flight: Take 2*

A remake of “SI Free Flight”, a micro flight simulator created in 2000 by an
unknown developer.

### Instructions

All flight controls are displayed in the game HUD.

### License
* The lzexe decompression code is derived from UNLZEXE by Mitugu Kurizono, and
  assumed to be in public domain. 
* The data in `terrain_mapbin.cpp` are taken from the original “SI Free Flight”
  and assumed to be in public domain. 
* All the remaining code is released under MIT license (see below)

-------------------------------------------------------------------------------

## `sod-decoder`: Decoder for the `SOD` format

A decoder allowing to export *Stunt Island*'s assets in Wavefront `.obj`
format. With respect to
[*SODParse*](https://armknechted.com/sicentral/newpage/hacksi.html) it seems to
be able to export more plane parts, but is less precise on all other assets.
The hope is that by comparing the algorithms someone will one day be able to
come up with an even better decoder.

### Instructions

Run with `--help` to see the syntax.

### License

MIT (see below)

-------------------------------------------------------------------------------

## MIT Software License

Copyright (c) 2026 Alberto Marnetto (except where otherwise stated)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

