@echo OFF

	echo.                         
	echo " _____ _____ ____  _____ "
	echo "|   __|   __|    \|  |  |"
	echo "|__   |  |  |  |  |    -|"
	echo "|_____|_____|____/|__|__|"
	echo.
	echo.                        
	echo "This batch script helps to build SGDK if PATH environmental variables "
	echo "    are not setup for GDK already.                                    "
	echo.
	echo "(1) When run the GDK and GDK_WIN EnvVars will be configured           "
	echo "    temporarily for the local directory.                              "
	echo "(2) The User will be asked whether to temporarily empty PATH. If yes, "
	echo "    PATH will be cleared for the the local script's execution only.   "
	echo "(3) SGDK clean script will be launched and then SGDK will be rebuild. "

:START
    echo.
    echo.
    echo.
    echo 1) Starting:
    echo ------------
    set "str_ScriptName=%~f0"
    echo Now running %str_ScriptName%
    echo.
 
    setlocal

:CREATEENVVARS
    echo.
    echo 2) Creating EnvVars:
    echo --------------------
    set "str_MyCurrentDir=%~dp0"

    set "GDK=%str_MyCurrentDir%"
    set "GDK_WIN=%str_MyCurrentDir%"
    set "GDK=%GDK:\=/%"

    if %GDK:~-1%==/ set "GDK=%GDK:~0,-1%"
    if %GDK_WIN:~-1%==\ set "GDK_WIN=%GDK_WIN:~0,-1%"

    echo GDK is %GDK%
    echo GDK_WIN is %GDK_WIN%

:CHECKINGPATH
    echo.
    echo 3) Checking PATH:
    echo --------------------

    set "PATHREAL=%PATH%"
    echo PATH = %PATH%
    echo.

:PATHCLEARQUESTION 
    echo Conflicting GCC setups in PATH can cause issues. 
    set /p "EMPTYPATH=Should PATH be temporarily cleared? (Y/[N])?"
    if /i "%EMPTYPATH:~,1%" EQU "Y" goto YCLEARPATH
    if /i "%EMPTYPATH:~,1%" EQU "N" goto NCLEARPATH
    echo Please type Y for Yes or N for No
    goto PATHCLEARQUESTION

:YCLEARPATH
    echo Setting PATH to just GDK stuff so that other CC1's do not conflict.

    REM Assuming that the current directory exists since that is where this file is.
    set "PATH=%GDK_WIN%"
    if EXIST %GDK_WIN%\bin set "PATH=%PATH%;%GDK_WIN%\bin"
    echo PATH = %PATH%
    goto CONTINUEAFTERCLEARQUESTION

:NCLEARPATH
    set "TPATH=%GDK_WIN%"
    if EXIST %GDK_WIN%\bin set "TPATH=%TPATH%;%GDK_WIN%\bin"
    set "PATH=%TPATH%;%PATH%"
    goto CONTINUEAFTERCLEARQUESTION


:CONTINUEAFTERCLEARQUESTION
    echo %GDK_WIN%\bin added to PATH
    echo.
    echo PATH = %PATH%

:LAUNCHING
    echo.
    echo 4) Launching:
    echo ------------------

    %GDK_WIN%\bin\make -f %GDK_WIN%\makelib.gen clean
    %GDK_WIN%\bin\make -f %GDK_WIN%\makelib.gen

:MAKECARTBAT
	echo.
	echo 5) Creating a make_cart.bat file for this machine.
	echo -------------------------------------------------
	
	echo.
	echo Batch for your machine:
	echo =======================
	echo set GDK=%GDK%
    echo GDK_WIN=%GDK_WIN%
    echo PATH=%PATH%
    echo %GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen
    echo.
    echo ============================================
   
	echo set GDK=%GDK%> make_cart.bat
	echo set GDK_WIN=%GDK_WIN%>> make_cart.bat
	echo set PATH=%PATH%>> make_cart.bat
	echo %GDK_WIN%\bin\make -f %GDK_WIN%\makefile.gen>> make_cart.bat
	
      
:CLEANUP
	echo.
	echo 6) Done. Cleaning up.
	echo ---------------------
	    
    echo.
    echo Reseting PATH.
    set "PATH=%PATHREAL%"
    echo PATH = %PATH%

:END
    endlocal
    @echo ON
    exit /b
