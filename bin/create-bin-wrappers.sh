#!/bin/bash

# List of binaries to wrap (without ".exe" extension)
BINARIES="ar as bintos cc1 cpp gcc gdb ld lto1 mac68k make nm objcopy objdump size sizebnd sjasm xgmtool"

# Output path
BIN_PATH=$(dirname "$0")

# Create a bash script for each binary file
for binary in $(echo $BINARIES); do
    FILENAME = "$BIN_PATH/$binary";
    echo "#!/bin/bash" > FILENAME
    echo "WINEDEBUG=-all WINEPREFIX=/sgdk/bin/.wineconf wine /sgdk/bin/$binary.exe \"\$@\"" >> FILENAME
done
