#!/bin/bash

# This script will be executed at docker build.
# See Dockerfile

BIN_PATH="/sgdk/bin"

# Create a bash script for each .exe file in BIN_PATH
for binary in $BIN_PATH/*.exe; do
    BASENAME=$(basename $binary .exe)
    FILENAME=$BIN_PATH/$BASENAME
    echo "#!/bin/bash" > $FILENAME
    echo "WINEDEBUG=-all WINEPREFIX=/sgdk/bin/.wineconf wine /sgdk/bin/$BASENAME.exe \"\$@\"" >> $FILENAME
    chmod +x $FILENAME
done
