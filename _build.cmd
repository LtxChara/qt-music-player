@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=D:\Software\Qt5\5.14.2\msvc2017_64\bin;%PATH%
cd /d d:\Workspace\qt_codes\MusicPlayer4
nmake -f Makefile.Release > d:\build_log.txt 2>&1
echo BUILD_EXIT=%ERRORLEVEL% >> d:\build_log.txt
