@echo off
if "%TEEM_DEST%" == "" goto system
set __bdest=%TEEM_DEST%\bin
set __hdest=%TEEM_DEST%\include
set __ldest=%TEEM_DEST%\lib
set __ddest=%TEEM_DEST%\lib
goto install
:system
if "%TEEM_INSTALL_SYSTEM%" == "" goto local
set __bdest=%SystemRoot%\system32
set __hdest=%1\include
set __ldest=%1\lib
set __ddest=%SystemRoot%\system32
goto install
:local
set __bdest=..\..\bin
set __hdest=..\..\include
set __ldest=..\..\lib
set __ddest=..\..\lib
:install
echo Installing ext headers into %__hdest%
mkdir "%__hdest%"
copy /y /b ..\ext\include\zconf.h "%__hdest%"
copy /y /b ..\ext\include\zlib.h "%__hdest%"
copy /y /b ..\ext\include\bzlib.h "%__hdest%"
copy /y /b ..\ext\include\png.h "%__hdest%"
copy /y /b ..\ext\include\pngconf.h "%__hdest%"
echo Installing ext libs into %__ldest%
mkdir "%__ldest%"
copy /y /b ..\ext\lib\zlib.lib "%__ldest%"
copy /y /b ..\ext\lib\zlib_s.lib "%__ldest%"
copy /y /b ..\ext\lib\libbz2.lib "%__ldest%"
copy /y /b ..\ext\lib\libbz2_s.lib "%__ldest%"
copy /y /b ..\ext\lib\libpng.lib "%__ldest%"
copy /y /b ..\ext\lib\libpng_s.lib "%__ldest%"
echo Installing ext dlls into %__ddest%
mkdir "%__ddest%"
copy /y /b ..\ext\lib\zlib.dll "%__ddest%"
copy /y /b ..\ext\lib\libbz2.dll "%__ddest%"
copy /y /b ..\ext\lib\libpng.dll "%__ddest%"
echo Installing ext bins into %__bdest%
mkdir "%__bdest%"
copy /y /b ..\ext\bin\pv.exe "%__bdest%"
:end
