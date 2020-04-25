SGDK 1.51 (april 2020)
Copyright 2020 Stephane Dallongeville
Patreon: https://www.patreon.com/SGDK
Github: https://github.com/Stephane-D/SGDK


SGDK is a free and open development kit for the Sega Megadrive.
It contains the development library itself (with the code sources) and some custom tools used to compile resources.
SGDK uses the GCC compiler (m68k-elf target) and the libgcc to generate ROM image. Binaries (GCC 6.3) are provided for Windows OS for convenience but you need to install it by yourself for others operating systems.
Note that SGDK also requires Java (custom tools need it) so you need to have Java JRE installed on your system:
https://www.oracle.com/technetwork/java/javase/downloads/jre8-downloads-2133155.html

SGDK library and custom tools are distributed under the MIT license (see license.txt file).
GCC compiler and libgcc are under GNU license (GPL3) and any software build from it (as the SGDK library) is under the GCC runtime library exception license (see COPYING.RUNTIME file)


GET STARTED
-----------

First, you need to know that SGDK uses C language (assembly is also possible) so it's highly recommended to be familiar with C programming before trying to develop with SGDK. Learning C language at same time than learning 'MegaDrive' programming is (imo) definitely too difficult and you will end nowhere.
It's also important to understand how the Sega Megadrive works internally (specifically the video system) so i recommend you to read documents about the Sega Megadrive hardware and development.
For that you can visit Mega Cat Studios Sega Megadrive graphics guide page:
https://megacatstudios.com/blogs/press/sega-genesis-mega-drive-vdp-graphics-guide-v1-2a-03-14-17
You can also check Sik's web site which is more dedicated to assembly programming but explain a lot (and nicely) about the Sega Megadrive hardware:
https://plutiedev.com/

Then when you feel ready you can go further and finally install SGDK :)

You can find installation instructions about how use SGDK on this page but be careful, some of these tutorials are incomplete or outdated:
https://github.com/Stephane-D/SGDK/wiki

I plan to complete tutorials in future but in the meantime i strongly suggest you to have a look on the available samples instead in the 'sample' directory of SGDK. The 'sprite' example is particularly useful as it show the basics in a small example.

You can also follow up-to-date and more complete online tutorials as this one (thanks to Ohsat for making them):
https://www.ohsat.com/tutorial/
You also have the great ones from Danibus (spanish only):
https://danibus.wordpress.com/

SGDK comes with a doxygen documentation that you can find in the 'doc' directory: doc/html/files.html
This documentation is generated from header files (.h) which provides information about SGDK structures and functions description.


HELP AND SUPPORT
----------------

If you need help or support with SGDK, you can go to the Spritesmind forum which is dedicated to Sega Megadrive development and has a specific section for SGDK:
http://gendev.spritesmind.net/forum/

You can also join the SGDK Discord server to get support:
https://discord.gg/xmnBWQS


MACOSX / LINUX
--------------

Unix/Linux users should give a try to the Gendev project from Kubilus:
https://github.com/kubilus1/gendev
It allows to quickly setup SGDK on a Unix environment.

MacOSX users also have access to SGDK with Gendev for MacOS from Sonic3D:
https://github.com/SONIC3D/gendev-macos

There is also the new and nice solution proposed by Daniel Valdivieso to use SGDK under any OS using Wine:
https://github.com/v4ld3r5/sgdk_vscode_template


VISUAL STUDIO
-------------

You can find a Visual Studio template into the 'vstudio' folder to facilate SGDK integration with VS.
To go even further you can also install the VS extension made by zerasul:
https://marketplace.visualstudio.com/items?itemName=zerasul.genesis-code


THANKS
------

- Chilly Willy for making almost all the JOY / controller support in SGDK (and the joy test sample ^^).
- Astrofa for the starfield donut sample ;)
- LIZARDRIVE for making the new SGDK logo.
- Kubilus for the Linux port of SGDK.
- Sonic3D for the OSX port of SGDK.
- Daniel Valdivieso for the SGDK multi-OS solution using Wine.
- Vladimir Kryvian for Visual Studio support and template.
- Tusario for resource manager tools.
- clbr for various contributions.
- jgyllinsky for providing / improving build batches.
- nolddor for fixes / contributions.
- starling13 for fixes.
- Ohsat for making nice tutorials.
- all those i forgot and generally all people helping by providing support, reporting bugs and supporting SGDK in any way !


SUPPORT SGDK
------------

SGDK is completly free but you can support it on Patreon or using the Paypal donation link.

Patreon: https://www.patreon.com/SGDK
Paypal donation link: https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2SCWVXYDEEBUU

Thanks =) I wish you a great and happy coding time !
