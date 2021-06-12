#!/bin/bash

# This script will be executed at docker build.
# See Dockerfile

BIN_PATH="/sgdk/bin"

# Create a bash script for each .exe file in BIN_PATH
for binary in $BIN_PATH/*.exe; do
    BASENAME=$(basename $binary .exe)
    FILENAME=$BIN_PATH/$BASENAME
    echo "#!/bin/bash" > $FILENAME
    # create a wineconf folder owned by us, otherwise it will refuse to execute for current user
    echo "mkdir -p /tmp/wine" >> $FILENAME
    echo "WINEARCH=win32 WINEDEBUG=-all WINEPREFIX=/tmp/wine/.wineconf wine /sgdk/bin/$BASENAME.exe \"\$@\"" >> $FILENAME
    chmod +x $FILENAME
done
