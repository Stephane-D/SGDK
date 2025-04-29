SET "GDK=%cd:\=/%"
SET "GDK_WIN=%cd%"
SET PATH=%GDK_WIN%\bin;%PATH%

make -f %GDK_WIN%\makelib.gen clean-release
make -f %GDK_WIN%\makelib.gen release
make -f %GDK_WIN%\makelib.gen clean-debug
make -f %GDK_WIN%\makelib.gen debug

@ECHO.
@ECHO.
@ECHO -------------------------------------------
@ECHO SGDK is now ready! Press any key to exit...
@PAUSE >nul
