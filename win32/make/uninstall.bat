@echo off
if "%TEEM_DEST%" == "" goto system
set __hdest=%TEEM_DEST%\include
set __ldest=%TEEM_DEST%\lib
set __ddest=%TEEM_DEST%\lib
goto install
:system
if "%TEEM_INSTALL_SYSTEM%" == "" goto local
set __hdest=%2\include
set __ldest=%2\lib
set __ddest=%SystemRoot%\system32
goto install
:local
set __hdest=..\..\include
set __ldest=..\..\lib
set __ddest=..\..\lib
:install
echo Deleting teem headers from %__hdest%
del "%__hdest%\air.h"
del "%__hdest%\bane.h"
del "%__hdest%\biff.h"
del "%__hdest%\dye.h"
del "%__hdest%\echo.h"
del "%__hdest%\ell.h"
del "%__hdest%\ellMacros.h"
del "%__hdest%\gage.h"
del "%__hdest%\hest.h"
del "%__hdest%\hoover.h"
del "%__hdest%\limn.h"
del "%__hdest%\mite.h"
del "%__hdest%\moss.h"
del "%__hdest%\nrrd.h"
del "%__hdest%\nrrdDefines.h"
del "%__hdest%\nrrdEnums.h"
del "%__hdest%\nrrdMacros.h"
del "%__hdest%\ten.h" 
del "%__hdest%\tenMacros.h"
del "%__hdest%\unrrdu.h"
echo Deleting %1.lib from %__ldest% 
del "%__ldest%\%1.lib"
echo Deleting %1.dll from %__ddest% 
del "%__ddest%\%1.dll"
:end
exit 0
