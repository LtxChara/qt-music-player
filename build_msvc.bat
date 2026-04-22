@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
D:\Software\Qt5\5.14.2\msvc2017_64\bin\qmake.exe MusicPlayer4.pro -spec win32-msvc
if %ERRORLEVEL% NEQ 0 (
    echo qmake failed with errorlevel %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
nmake
if %ERRORLEVEL% NEQ 0 (
    echo nmake failed with errorlevel %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Build completed successfully.
