@ECHO off
set "GDK=F:/github/SGDK"
set "GDK_WIN=F:\github\SGDK"
set "XGCC_WIN=%GDK_WIN%\x68k-gcc\bin"
set "PATH=%GDK_WIN%\bin;%XGCC_WIN%;%PATH%"

F:\github\SGDK\bin\make %1 -f F:\github\SGDK\makefilecpp.gen 
