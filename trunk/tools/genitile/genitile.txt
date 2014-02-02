-------------------------------------------------------------------------------
--<< GeNiTiLe >>---------------------------------------------------------------
-------------------------------------------------------------------------------

VeRsIoN	: 1.2
AuThoR	: PaScAl BoSqUet
EmAiL	: PaStOrAmA@hOTmAiL.cOm
WeB	: WwW.pAsCaLoRaMa.Com

--<< InTrO >>------------------------------------------------------------------

Genitile is a command line tool for converting pictures to megadrive/genesis
tile format. It's versatile because the tool uses plugins for reading and 
outputing data.

Genitile is able to:
- ouput	tiles
	sprites
	maps
	optimised tilesets (unused tiles,duplicates,flipped tiles removing)
	palettes

Currents plugins allow to:
- read pcx and bmp (256 colors only file)
- output the data in the following format
	Asm (supporting snasm68k and xgcc format)
	C header file
	Binary

--<< SwItChEs >>---------------------------------------------------------------

legend:	<mandatory>
	[optional]

-tileset [image filename]
	specify that we want to optimize the tiles and generate a new tileset.
	shortcut "-t"

-out <output type> 
	define the kind of data output you want.
	the default plugins are:
		asm	: snasm68k asm format
		asgcc	: xgcc asm format
		bin	: binary
		pcx	: pcx picture file (only combined with -tileset)
		bmp	: bmp picture file (only combined with -tileset)
		h	: C Header
	shortcut "-o"

-pal [palette index]
	specify that you want to output the palette.The palette index argument
	allow you to specify the index of the first 16 colors palettes that you
	want to extract.
	shortcut "-p"

-map	
	specify that you want to generate the map.This switch can only be
	combine with the "-tileset".
	shortcut "-m"

-noecho 
	no console output for the pictures informations
	shortcut "-ne"

-outdir <path of the output directory>
	specify the path where you want to store the outputed data.
	shortcut "-od"

-sprw	[sprite width in tiles]
	specify the width of the sprite generation exprimed in tiles.
	valid value is from 1 to 4
	shortcut "-sw"

-sprh	[sprite height in tiles]
	specify the height of the sprite generation exprimed in tiles.
	valid value is from 1 to 4
	shortcut "-sw"

-notile	
	specify that you don't want the program to output the tiles.
	shorcut "-nt"

-nbppal [amount of 16 colors palettes]
	specify the amount of palettes to be extracted (max 15).
	shorcut "-np"

-shadow	[shadow value (0,1)]
	Set the value of the shadow bit when doing a map generation
	For the shadow to be detected you need to add a copy of your palette
	in the palette 4,5,6,7 position [64->128] but with colors/2 and be sure
	that ur tile point to that palette.
	shortcut "-s"

-mapoffset [index of the first tile map]
	define the offset of the first map tile entry in vram
	shorcut "-mo"

-mappal	[index of a palette]
	force the palette number to be generated during the map entry generation
	shortcut "-mp"

-mapprio
	Set the value of the priority bit when doing a map generation
	shortcut "-mpr"

-crpx	Crop Top left X coord (in pixels)
-crpy	Crop Top left Y coord (in pixels)
-crpw	Cropping width in tile
-crph	Cropping height in tile


--<< SaMpLes >>----------------------------------------------------------------

genitile rick.pcx -sw 3 -sh 3 -pal 1 -o asm

	generate a 24x24 pixels sprites generation outputed in asm format
	and will output the palette 1

genitile spk.pcx -o bin -pal

	generate 8x8 tiles in binary format and will output palette 0

genitile spk.pcx -tileset -map -o h -od data -mo 54 -mp 1 -pal
	
	generate:
	output optimized tileset for spk.pcx
	output map of spk.pcx using the generated tileset
	output are in c header h format
	output the palette 0
	output will be stored in "data" directory
	first map index is 54
	force the palette 1 for the map entries

genitile spk.pcx -tileset mytiles.pcx -map -o asgcc -pal
	
	won't optimized the tileset mytiles.pcx but will generate the map
	of spk.pcx using the tileset mytiles.pcx
	output the tileset mytiles
	output map of spk.pcx using the specified tileset mytiles.pcx
	output are in xgcc asm format
	output the palette 0

genitile -tileset spk.pcx -o pcx

	ouput the optimized tileset in pcx format

genitile *.pcx -crpx 8 -crpy 0 -crpw 16 -crph 1 -out pcx
	crop all the files matching the wildcard *.pcx 
	 to the square extracted at [8,0] [24,8] and output a pcx with

--<< HiStOrY >>----------------------------------------------------------------

- 11/07/2006
	+ add picture cropping features
	+ add wildcard processing

- 19/05/2006
	add the switch -mapprio so you can force the priority bit to 1 during
	map generation

- 12/05/2005
	first release

--<< ThAnkS >>-----------------------------------------------------------------

Special thanks to fonzie for comments and beta testing.
Thanks to kaneda and steph for genskmod and all the genesis/megadrive

HaVe FuN ;)
		
	