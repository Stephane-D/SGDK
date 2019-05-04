SGDK 1.34 (december 2017)
Copyright 2019 Stephane Dallongeville
https://stephane-d.github.io/SGDK/

SGDK is an open and free development kit for the Sega Megadrive.
It contains the development library itself (sources included) and some custom tools used to compile resources.
SGDK uses the GCC compiler (m68k-elf target) and libgcc to generate ROM image. Binaries (GCC 6.3) are provided for Windows OS for convenience but you need to install it by yourself for others operating systems.
Note that SGDK also requires Java (custom tools use it) so you need to have Java JRE installed on your system:
https://www.oracle.com/technetwork/java/javase/downloads/jre8-downloads-2133155.html

SGDK library and custom tools are distributed under the MIT license (see license.txt file).
GCC compiler and libgcc are under GNU license (GPL3) and any software build from it (as the SGDK library) is under the GCC runtime library exception license (see COPYING.RUNTIME file)

You can find the library doxygen documentation in the 'doc' directory: doc/html/files.html

SGDK GitHub page: https://github.com/Stephane-D/SGDK
SGDK Wiki page: https://github.com/Stephane-D/SGDK/wiki

You can find basics tutorials about how install and use SGDK on this page:
https://github.com/Stephane-D/SGDK/wiki/Tuto-Intro

Unix/Linux users should give a try to the Gendev project from Kubilus:
https://github.com/kubilus1/gendev
It allows to quickly setup SGDK on a Unix environment.

MacOSX users also have access to SGDK with Gendev for MacOS from Sonic3D:
https://github.com/SONIC3D/gendev-macos

There is also the new and nice solution proposed by Daniel Valdivieso tu use SGDK under any OS using Wine: 
https://github.com/v4ld3r5/sgdk_vscode_template

If you enjoy SGDK you can support it on Patreon or using the Paypal donation link:
https://www.patreon.com/StephaneD
https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=2SCWVXYDEEBUU

Thanks =) I wish you a happy coding time !