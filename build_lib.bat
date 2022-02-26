@ECHO off
SET "GDK=%cd:\=/%"
SET "GDK_WIN=%cd%"
SET "XGCC_WIN=%GDK_WIN%\x68k-gcc\bin"
SET "PATH=%GDK_WIN%\bin;%XGCC_WIN%;%PATH%"

@ECHO on
make -f %GDK_WIN%\makelib.gen cleanrelease
make -f %GDK_WIN%\makelib.gen release
make -f %GDK_WIN%\makelib.gen cleandebug
make -f %GDK_WIN%\makelib.gen debug
make -f %GDK_WIN%\makelib.gen distclean

@ECHO.
@ECHO.
@ECHO -------------------------------------------
@ECHO SGDK is now ready! Press any key to exit...
@PAUSE >nul
