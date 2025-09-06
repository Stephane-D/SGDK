SET "GDK=%cd:\=/%"
SET "GDK_WIN=%cd%"

@ECHO Building SGDK samples...
@ECHO -------------------------------------------

@ECHO Building advanced\sprites-sharing-tiles...
@cd %GDK_WIN%\sample\advanced\sprites-sharing-tiles
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building advanced\tile-animation...
@cd %GDK_WIN%\sample\advanced\tile-animation
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building basics\hello-world...
@cd %GDK_WIN%\sample\basics\hello-world
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building basics\image...
@cd %GDK_WIN%\sample\basics\image
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building basics\pools...
@cd %GDK_WIN%\sample\basics\pools
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building basics\tmx-objects...
@cd %GDK_WIN%\sample\basics\tmx-objects
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building basics\tmx-map...
@cd %GDK_WIN%\sample\basics\tmx-map
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building benchmark...
@cd %GDK_WIN%\sample\benchmark
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building bitmap\cube-3D...
@cd %GDK_WIN%\sample\bitmap\cube-3D
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building bitmap\partic...
@cd %GDK_WIN%\sample\bitmap\partic
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building demo\bad-apple...
@cd %GDK_WIN%\sample\demo\bad-apple
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building demo\starfield-donut...
@cd %GDK_WIN%\sample\demo\starfield-donut
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building flash-save...
@cd %GDK_WIN%\sample\flash-save
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\hilight-shadow...
@cd %GDK_WIN%\sample\fx\hilight-shadow
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\h-int\scaling...
@cd %GDK_WIN%\sample\fx\h-int\scaling
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\h-int\wobble...
@cd %GDK_WIN%\sample\fx\h-int\wobble
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\scroll\linescroll...
@cd %GDK_WIN%\sample\fx\scroll\linescroll
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\sprite-masking...
@cd %GDK_WIN%\sample\fx\sprite-masking
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building fx\silhouette...
@cd %GDK_WIN%\sample\fx\silhouette
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building game\platformer...
@cd %GDK_WIN%\sample\game\platformer
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building game\sonic...
@cd %GDK_WIN%\sample\game\sonic
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building joy-test...
@cd %GDK_WIN%\sample\joy-test
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building snd\sound-test...
@cd %GDK_WIN%\sample\snd\sound-test
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building snd\xgm-player...
@cd %GDK_WIN%\sample\snd\xgm-player
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building sys\console...
@cd %GDK_WIN%\sample\sys\console
%GDK_WIN%\bin\make -f %GDK%/makefile.gen

@ECHO Building sys\multitasking...
@cd %GDK_WIN%\sample\sys\multitasking
%GDK_WIN%\bin\make -f %GDK%/makefile.gen


@ECHO.
@ECHO.
@ECHO DONE !
@ECHO.
@ECHO PRESS ANY KEY TO CLOSE
@PAUSE >nul
