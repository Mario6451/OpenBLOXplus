@echo off
cd /d "%~dp0"

:menu
echo.
echo ====================================
echo   OpenBLOX 3DS Build - RAM Select
echo ====================================
echo.
echo  [1] 32 MB  - Old3DS  (Conservative)
echo  [2] 64 MB  - Old3DS  (Standard)
echo  [3] 72 MB  - Old3DS  (Extended)
echo  [4] 80 MB  - Old3DS  (Heavy Games)
echo  [5] 96 MB  - Old3DS  (Maximum)
echo  [6] 124 MB - New3DS  (Standard)
echo  [7] 178 MB - New3DS  (Maximum)
echo.
set /p ram="Choose RAM allocation (1-7, default=1): "

if "%ram%"=="" set ram=1
if "%ram%"=="1" set RAMMODE=32MB
if "%ram%"=="2" set RAMMODE=64MB
if "%ram%"=="3" set RAMMODE=72MB
if "%ram%"=="4" set RAMMODE=80MB
if "%ram%"=="5" set RAMMODE=96MB
if "%ram%"=="6" set RAMMODE=124MB
if "%ram%"=="7" set RAMMODE=178MB

echo.
echo Selected: %RAMMODE%

echo Removing all .o files...
for /r %%f in (*.o) do del "%%f"

echo Running make clean...
cd C_Engine
make 3ds-clean

echo Generating RAM variants...
make -C Makefiles -f makefile_3ds gen-ram-metas

echo Rebuilding 3DS CIA (RAM=%RAMMODE%)...
make -C Makefiles -f makefile_3ds RAM=%RAMMODE%

echo Done!
pause