copy /Y out\rom.bin %GDK_WIN%\emu\rom.bin
copy /Y out\rom.wch %GDK_WIN%\emu\watchers\rom.wch
cd %GDK_WIN%\emu
call GensKMod.exe rom.bin