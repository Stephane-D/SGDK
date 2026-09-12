@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: SGDK Command Line Interface (Windows)
:: Compatible with SGDK build workflow (Maven / Gradle / NPM style)
:: ============================================================================

:: Resolve SGDK root directory (GDK)
if "%GDK%"=="" (
    set "GDK_DIR=%~dp0.."
    for %%I in ("!GDK_DIR!") do set "GDK=%%~fI"
)

:: Convert backslashes to forward slashes for Makefile compatibility
set "GDK_POSIX=%GDK:\=/%"
set "GDK=%GDK_POSIX%"
set "MAKEFILE_GEN=%GDK%/makefile.gen"
set "MAKELIB_GEN=%GDK%/makelib.gen"

:: Ensure SGDK bin directory is in PATH
set "PATH=%GDK%\bin;%PATH%"

:: Read primary command
set "CMD=%~1"

if "%CMD%"=="" goto help
if /i "%CMD%"=="help" goto help
if /i "%CMD%"=="-h" goto help
if /i "%CMD%"=="--help" goto help
if /i "%CMD%"=="version" goto version
if /i "%CMD%"=="-v" goto version
if /i "%CMD%"=="--version" goto version
if /i "%CMD%"=="build" goto build
if /i "%CMD%"=="compile" goto build
if /i "%CMD%"=="release" goto release
if /i "%CMD%"=="debug" goto debug
if /i "%CMD%"=="asm" goto asm
if /i "%CMD%"=="clean" goto clean
if /i "%CMD%"=="rebuild" goto rebuild
if /i "%CMD%"=="lib" goto lib
if /i "%CMD%"=="build-lib" goto lib
if /i "%CMD%"=="deps" goto install_deps
if /i "%CMD%"=="dependencies" goto install_deps
if /i "%CMD%"=="install" goto install_deps
if /i "%CMD%"=="run" goto run
if /i "%CMD%"=="test" goto run

echo [ERROR] Unknown command: '%CMD%'
echo Run 'sgdk help' for available commands.
exit /b 1

:: ----------------------------------------------------------------------------
:: Helper: Read dependencies from sgdk.yml
:: ----------------------------------------------------------------------------
:get_yml_deps
set "YML_DEPS="

if not exist "sgdk.yml" goto :eof

set "IN_DEPS=0"
for /f "tokens=1* delims=:" %%A in ('findstr /n "^" "sgdk.yml"') do (
    set "LINE=%%B"
    call :parse_dep_line "%%A"
)

if not "%YML_DEPS%"=="" (
    echo [SGDK] Found dependencies in sgdk.yml: %YML_DEPS%
    if "%DEPENDENCIES%"=="" (
        set "DEPENDENCIES=%YML_DEPS%"
    ) else (
        set "DEPENDENCIES=%DEPENDENCIES% %YML_DEPS%"
    )
)
goto :eof

:parse_dep_line
if not defined LINE goto :eof
if "!LINE!"=="dependencies:" (
    set "IN_DEPS=1"
    goto :eof
)
if "%IN_DEPS%"=="1" (
    set "TRIM_L=!LINE!"
    :strip_dep_space
    if "!TRIM_L:~0,1!"==" " (
        set "TRIM_L=!TRIM_L:~1!"
        goto strip_dep_space
    )
    if "!TRIM_L:~0,1!"=="	" (
        set "TRIM_L=!TRIM_L:~1!"
        goto strip_dep_space
    )
    
    if "!TRIM_L:~0,1!"=="-" (
        set "DEP=!TRIM_L:~1!"
        :strip_dep_val
        if "!DEP:~0,1!"==" " (
            set "DEP=!DEP:~1!"
            goto strip_dep_val
        )
        set "DEP=!DEP:"=!"
        set "DEP=!DEP:'=!"
        if defined DEP set "YML_DEPS=%YML_DEPS% !DEP!"
    ) else if not "!TRIM_L:~0,1!"=="#" (
        if not "!LINE:~0,1!"==" " if not "!LINE:~0,1!"=="	" set "IN_DEPS=0"
    )
)
goto :eof

:: ----------------------------------------------------------------------------
:: Helper: Parse builds and sgdkConfigs from sgdk.yml
:: ----------------------------------------------------------------------------
:parse_sgdk_yml_builds
set "IN_BUILDS=0"
set "IN_CONFIGS=0"
set "BUILD_COUNT=0"

if not exist "sgdk.yml" goto :eof

for /f "tokens=1* delims=:" %%A in ('findstr /n "^" "sgdk.yml"') do (
    set "RAW_LINE=%%B"
    call :parse_yml_line "%%A"
)
goto :eof

:parse_yml_line
if not defined RAW_LINE goto :eof

set "TRIM_LINE=!RAW_LINE!"
:strip_leading
if "!TRIM_LINE:~0,1!"==" " (
    set "TRIM_LINE=!TRIM_LINE:~1!"
    goto strip_leading
)
if "!TRIM_LINE:~0,1!"=="	" (
    set "TRIM_LINE=!TRIM_LINE:~1!"
    goto strip_leading
)

if "!TRIM_LINE:~0,1!"=="#" goto :eof

if "!TRIM_LINE!"=="builds:" (
    set "IN_BUILDS=1"
    set "IN_CONFIGS=0"
    goto :eof
)

if "%IN_BUILDS%"=="1" (
    set "RAW_FIRST=!RAW_LINE:~0,1!"
    set "TRIM_FIRST=!TRIM_LINE:~0,1!"
    if "!RAW_FIRST!"=="!TRIM_FIRST!" if not "!TRIM_FIRST!"=="-" (
        set "IN_BUILDS=0"
        set "IN_CONFIGS=0"
        goto :eof
    )
    
    if "!TRIM_LINE:~0,7!"=="- name:" (
        set /a BUILD_COUNT+=1
        set "IN_CONFIGS=0"
        set "B_NAME=!TRIM_LINE:~7!"
        :strip_bname
        if "!B_NAME:~0,1!"==" " (
            set "B_NAME=!B_NAME:~1!"
            goto strip_bname
        )
        call set "BUILD_NAME_%BUILD_COUNT%=!B_NAME!"
        call set "BUILD_CFG_COUNT_%BUILD_COUNT%=0"
        goto :eof
    )
    
    if "!TRIM_LINE!"=="sgdkConfigs:" (
        set "IN_CONFIGS=1"
        goto :eof
    )
    
    if "%IN_CONFIGS%"=="1" (
        if "!TRIM_FIRST!"=="-" (
            set "CFG_ENTRY=!TRIM_LINE:~1!"
            :strip_cfg
            if "!CFG_ENTRY:~0,1!"==" " (
                set "CFG_ENTRY=!CFG_ENTRY:~1!"
                goto strip_cfg
            )
            
            for /f "tokens=1* delims=:" %%K in ("!CFG_ENTRY!") do (
                set "K_NAME=%%K"
                set "V_VAL=%%L"
                call :clean_kv
            )
        )
    )
)
goto :eof

:clean_kv
:strip_kv
if "!K_NAME:~-1!"==" " (
    set "K_NAME=!K_NAME:~0,-1!"
    goto strip_kv
)
:strip_vv
if "!V_VAL:~0,1!"==" " (
    set "V_VAL=!V_VAL:~1!"
    goto strip_vv
)
call :add_config_entry "%BUILD_COUNT%"
goto :eof

:add_config_entry
set "B_I=%~1"
call set "OLD_CNT=%%BUILD_CFG_COUNT_%B_I%%%"
if not defined OLD_CNT set "OLD_CNT=0"
set /a NEW_CNT=%OLD_CNT% + 1
set "BUILD_CFG_COUNT_%B_I%=%NEW_CNT%"
call set "BUILD_CFG_KEY_%B_I%_%NEW_CNT%=!K_NAME!"
call set "BUILD_CFG_VAL_%B_I%_%NEW_CNT%=!V_VAL!"
goto :eof

:: ----------------------------------------------------------------------------
:: Helper: Apply sgdkConfigs for a specific build index to config.h
:: ----------------------------------------------------------------------------
:apply_build_configs
set "B_IDX=%~1"
if not exist "%GDK%\inc\config.h" goto :eof

call set "CFG_CNT=%%BUILD_CFG_COUNT_%B_IDX%%%"
if not defined CFG_CNT goto :eof
if !CFG_CNT! lss 1 goto :eof

call :loop_configs "%B_IDX%" "%CFG_CNT%"
goto :eof

:loop_configs
set "B_I=%~1"
set "C_MAX=%~2"
set "K_NUM=1"

:cfg_loop_start
if %K_NUM% gtr %C_MAX% goto cfg_loop_end

call :replace_single_config_key "%B_I%" "%K_NUM%"

set /a K_NUM+=1
goto cfg_loop_start

:cfg_loop_end
goto :eof

:replace_single_config_key
set "B_I=%~1"
set "K_I=%~2"
call set "TARGET_KEY=%%BUILD_CFG_KEY_%B_I%_%K_I%%%"
call set "TARGET_VAL=%%BUILD_CFG_VAL_%B_I%_%K_I%%%"

if not defined TARGET_KEY goto :eof

type nul > "%GDK%\inc\config.h_tmp"
setlocal disabledelayedexpansion
for /f "tokens=1* delims=:" %%A in ('findstr /n "^" "%GDK%\inc\config.h"') do (
    set "C_LINE=%%B"
    call :process_single_line
)
endlocal
move /y "%GDK%\inc\config.h_tmp" "%GDK%\inc\config.h" >nul
goto :eof

:process_single_line
if not defined C_LINE (
    >>"%GDK%\inc\config.h_tmp" echo(
    goto :eof
)

setlocal enabledelayedexpansion
if "!C_LINE:~0,8!"=="#define " (
    set "WORD2=!C_LINE:~8!"
    for /f "tokens=1" %%Y in ("!WORD2!") do (
        if "%%Y"=="!TARGET_KEY!" (
            >>"%GDK%\inc\config.h_tmp" echo #define !TARGET_KEY!         !TARGET_VAL!
            endlocal
            goto :eof
        )
    )
)

>>"%GDK%\inc\config.h_tmp" echo(!C_LINE!
endlocal
goto :eof

:: ----------------------------------------------------------------------------
:: Helper: Restore original config.h
:: ----------------------------------------------------------------------------
:restore_config_h
if exist "%GDK%\inc\config.h_original" (
    copy /y "%GDK%\inc\config.h_original" "%GDK%\inc\config.h" >nul
)
goto :eof

:: ----------------------------------------------------------------------------
:: Command: deps / dependencies / install
:: ----------------------------------------------------------------------------
:install_deps
shift
call :get_yml_deps
if not exist "%MAKEFILE_GEN%" (
    echo [ERROR] Cannot find SGDK makefile.gen at: %MAKEFILE_GEN%
    exit /b 1
)
echo [SGDK] Installing dependencies...
if "%DEPENDENCIES%"=="" (
    echo [SGDK] No dependencies specified in sgdk.yml or DEPENDENCIES variable.
    exit /b 0
)
make -f "%MAKEFILE_GEN%" install DEPENDENCIES="%DEPENDENCIES%"
exit /b %ERRORLEVEL%

:: ----------------------------------------------------------------------------
:: Command: build / compile [target] [options]
:: ----------------------------------------------------------------------------
:build
shift
set "TARGET=release"
if /i "%~1"=="release" (
    set "TARGET=release"
    shift
) else if /i "%~1"=="debug" (
    set "TARGET=debug"
    shift
) else if /i "%~1"=="asm" (
    set "TARGET=asm"
    shift
)
goto exec_make

:release
shift
set "TARGET=release"
goto exec_make

:debug
shift
set "TARGET=debug"
goto exec_make

:asm
shift
set "TARGET=asm"
goto exec_make

:exec_make
call :get_yml_deps
set "EXTRA_ARGS="
:collect_build_args
if "%~1"=="" goto run_make
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift
goto collect_build_args

:run_make
if not exist "%MAKEFILE_GEN%" (
    echo [ERROR] Cannot find SGDK makefile.gen at: %MAKEFILE_GEN%
    exit /b 1
)

call :parse_sgdk_yml_builds

if %BUILD_COUNT% equ 0 goto run_single_make

echo [SGDK] Found %BUILD_COUNT% build configuration(s) in sgdk.yml.

if not exist "%GDK%\inc\config.h_original" (
    if exist "%GDK%\inc\config.h" (
        copy "%GDK%\inc\config.h" "%GDK%\inc\config.h_original" >nul
    )
)

set "LIB_TARGET=release"
if /i "%TARGET%"=="debug" set "LIB_TARGET=debug"

call :loop_builds "%BUILD_COUNT%"
if !errorlevel! neq 0 (
    call :restore_config_h
    exit /b !errorlevel!
)

call :restore_config_h
exit /b 0

:loop_builds
set "B_NUM=1"

:build_loop_start
if %B_NUM% gtr %BUILD_COUNT% goto build_loop_end

call :build_single_config %B_NUM%
if !errorlevel! neq 0 exit /b 1

set /a B_NUM+=1
goto build_loop_start

:build_loop_end
goto :eof

:build_single_config
set "B_NUM=%~1"
call set "CURR_BNAME=%%BUILD_NAME_%B_NUM%%%"
call set "CURR_CFG_CNT=%%BUILD_CFG_COUNT_%B_NUM%%%"

echo.
echo ============================================================================
echo [SGDK] Building configuration %B_NUM% of %BUILD_COUNT%: %CURR_BNAME%
echo ============================================================================

call :restore_config_h

if not defined CURR_CFG_CNT goto skip_lib_rebuild
if %CURR_CFG_CNT% leq 0 goto skip_lib_rebuild

call :apply_build_configs %B_NUM%

echo [SGDK] Rebuilding SGDK library (%LIB_TARGET%) for build '%CURR_BNAME%'...
pushd "%GDK%"
make -f makelib.gen clean-%LIB_TARGET%
make -f makelib.gen %LIB_TARGET%
popd
if !errorlevel! neq 0 (
    echo [ERROR] Failed to compile SGDK library for build '%CURR_BNAME%'.
    exit /b 1
)

:skip_lib_rebuild
echo [SGDK] Executing project build target '%TARGET%'...
make -f "%MAKEFILE_GEN%" clean %EXTRA_ARGS%
if not "%DEPENDENCIES%"=="" (
    make -f "%MAKEFILE_GEN%" %TARGET% DEPENDENCIES="%DEPENDENCIES%" %EXTRA_ARGS%
) else (
    make -f "%MAKEFILE_GEN%" %TARGET% %EXTRA_ARGS%
)
if !errorlevel! neq 0 (
    echo [ERROR] Build failed for configuration '%CURR_BNAME%'.
    exit /b 1
)

if not exist "output" mkdir output
if exist "out\rom.bin" (
    copy /y "out\rom.bin" "output\%CURR_BNAME%.bin" >nul
    copy /y "out\rom.bin" "out\rom-%CURR_BNAME%.bin" >nul
    echo [SGDK] Created output artifact: output\%CURR_BNAME%.bin
) else if exist "out\%TARGET%\rom.bin" (
    copy /y "out\%TARGET%\rom.bin" "output\%CURR_BNAME%.bin" >nul
    copy /y "out\%TARGET%\rom.bin" "out\%TARGET%\rom-%CURR_BNAME%.bin" >nul
    echo [SGDK] Created output artifact: output\%CURR_BNAME%.bin
)
exit /b 0

:run_single_make
call :restore_config_h
echo [SGDK] Executing build target '%TARGET%'...
if not "%DEPENDENCIES%"=="" (
    make -f "%MAKEFILE_GEN%" %TARGET% DEPENDENCIES="%DEPENDENCIES%" %EXTRA_ARGS%
) else (
    make -f "%MAKEFILE_GEN%" %TARGET% %EXTRA_ARGS%
)
exit /b %ERRORLEVEL%

:: ----------------------------------------------------------------------------
:: Command: clean [target] [options]
:: ----------------------------------------------------------------------------
:clean
shift
set "CLEAN_TARGET=clean"
if /i "%~1"=="release" (
    set "CLEAN_TARGET=clean-release"
    shift
) else if /i "%~1"=="debug" (
    set "CLEAN_TARGET=clean-debug"
    shift
) else if /i "%~1"=="asm" (
    set "CLEAN_TARGET=clean-asm"
    shift
) else if /i "%~1"=="all" (
    set "CLEAN_TARGET=clean-all"
    shift
)

set "EXTRA_ARGS="
:collect_clean_args
if "%~1"=="" goto run_clean
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift
goto collect_clean_args

:run_clean
if not exist "%MAKEFILE_GEN%" (
    echo [ERROR] Cannot find SGDK makefile.gen at: %MAKEFILE_GEN%
    exit /b 1
)
call :restore_config_h
echo [SGDK] Executing clean target '%CLEAN_TARGET%'...
make -f "%MAKEFILE_GEN%" %CLEAN_TARGET% %EXTRA_ARGS%
exit /b %ERRORLEVEL%

:: ----------------------------------------------------------------------------
:: Command: rebuild [target] [options]
:: ----------------------------------------------------------------------------
:rebuild
shift
set "TARGET=release"
if /i "%~1"=="release" (
    set "TARGET=release"
    shift
) else if /i "%~1"=="debug" (
    set "TARGET=debug"
    shift
) else if /i "%~1"=="asm" (
    set "TARGET=asm"
    shift
)

call :get_yml_deps
set "EXTRA_ARGS="
:collect_rebuild_args
if "%~1"=="" goto run_rebuild
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift
goto collect_rebuild_args

:run_rebuild
goto run_make

:: ----------------------------------------------------------------------------
:: Command: lib / build-lib [target] [options]
:: ----------------------------------------------------------------------------
:lib
shift
set "LIB_TARGET=release"
if /i "%~1"=="release" (
    set "LIB_TARGET=release"
    shift
) else if /i "%~1"=="debug" (
    set "LIB_TARGET=debug"
    shift
) else if /i "%~1"=="clean" (
    set "LIB_TARGET=clean"
    shift
)

set "EXTRA_ARGS="
:collect_lib_args
if "%~1"=="" goto run_lib
set "EXTRA_ARGS=%EXTRA_ARGS% %1"
shift
goto collect_lib_args

:run_lib
if not exist "%MAKELIB_GEN%" (
    echo [ERROR] Cannot find SGDK makelib.gen at: %MAKELIB_GEN%
    exit /b 1
)
echo [SGDK] Building library target '%LIB_TARGET%'...
make -f "%MAKELIB_GEN%" %LIB_TARGET% %EXTRA_ARGS%
exit /b %ERRORLEVEL%

:: ----------------------------------------------------------------------------
:: Command: run / test [rom_path]
:: ----------------------------------------------------------------------------
:run
shift
set "ROM_PATH=%~1"

if "%ROM_PATH%"=="" (
    if exist "out\rom.bin" (
        set "ROM_PATH=out\rom.bin"
    ) else if exist "out\release\rom.bin" (
        set "ROM_PATH=out\release\rom.bin"
    ) else if exist "out\debug\rom.bin" (
        set "ROM_PATH=out\debug\rom.bin"
    ) else (
        echo [ERROR] No ROM file found in out\ or out\release\. Please build first or specify ROM path.
        exit /b 1
    )
)

if not exist "%ROM_PATH%" (
    echo [ERROR] Specified ROM file does not exist: %ROM_PATH%
    exit /b 1
)

:: Find emulator executable
set "EMU=%EMULATOR%"
if "%EMU%"=="" (
    where blastem >nul 2>&1
    if !errorlevel! equ 0 (
        set "EMU=blastem"
    ) else (
        where gens >nul 2>&1
        if !errorlevel! equ 0 (
            set "EMU=gens"
        ) else (
            where fusion >nul 2>&1
            if !errorlevel! equ 0 set "EMU=fusion"
        )
    )
)

if "%EMU%"=="" (
    echo [ERROR] No Sega Genesis emulator found in PATH.
    echo Please set the EMULATOR environment variable or add blastem/gens/fusion to PATH.
    exit /b 1
)

echo [SGDK] Running ROM with %EMU%: %ROM_PATH%
"%EMU%" "%ROM_PATH%"
exit /b %ERRORLEVEL%

:: ----------------------------------------------------------------------------
:: Command: version / -v / --version
:: ----------------------------------------------------------------------------
:version
echo SGDK CLI Version 2.11
echo GDK Directory: %GDK%
where make >nul 2>&1
if %errorlevel% equ 0 (
    echo Make executable: found
) else (
    echo Make executable: NOT found
)
where gcc >nul 2>&1
if %errorlevel% equ 0 (
    echo GCC compiler: found
) else (
    where m68k-elf-gcc >nul 2>&1
    if !errorlevel! equ 0 (
        echo GCC compiler: m68k-elf-gcc found
    ) else (
        echo GCC compiler: NOT found
    )
)
exit /b 0

:: ----------------------------------------------------------------------------
:: Command: help / -h / --help
:: ----------------------------------------------------------------------------
:help
echo.
echo SGDK Command Line Interface (CLI)
echo.
echo Usage: sgdk ^<command^> [target] [options]
echo.
echo Commands:
echo   build, compile [target]   Build project (default target: release)
echo   release                   Build project in release mode
echo   debug                     Build project in debug mode (with symbols)
echo   asm                       Generate assembly output
echo   clean [target]            Clean build output (targets: all, release, debug, asm)
echo   rebuild [target]          Clean and rebuild project
echo   deps, install             Fetch and clone dependencies defined in sgdk.yml
echo   lib, build-lib [target]   Build SGDK library itself
echo   run, test [rom_path]      Launch ROM in emulator
echo   version, -v, --version    Display SGDK and toolchain version information
echo   help, -h, --help          Display this help message
echo.
echo Targets:
echo   release                   Optimized release build (default)
echo   debug                     Debug build with symbol injection
echo   asm                       Assembly listing target
echo.
echo Examples:
echo   sgdk build
echo   sgdk build debug -j4
echo   sgdk clean
echo   sgdk rebuild release
echo   sgdk run
echo   sgdk lib
echo.
exit /b 0
