:: Build all C++ samples
:: The library must be compiled with ENABLE_NEWLIB and ENABLE_CPLUSPLUS set to 1

ECHO off
SET "GDK=%cd:\=/%"
SET "GDK_WIN=%cd%"
SET "XGCC_WIN=%GDK_WIN%\x68k-gcc\bin"
SET "PATH=%GDK_WIN%\bin;%XGCC_WIN%;%PATH%"

set DIRS=sample\cplusplus\hello-world*

for /D %%i in (%DIRS%) do (
    echo "%%i"
    %GDK_WIN%\bin\make -C "%%i" release -f %GDK_WIN%\makefilecpp.gen
    %GDK_WIN%\bin\make -C "%%i" distclean -f %GDK_WIN%\makefilecpp.gen 
)  
