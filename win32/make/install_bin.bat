@echo off
if "%TEEM_DEST%" == "" goto local
set __bdest=%TEEM_DEST%\bin
goto install
:local
set __bdest=..\..\bin
:install
echo Installing %1.exe into %__bdest%
mkdir "%__bdest%"
copy /y /b ..\bin\%1.exe "%__bdest%"
:end
