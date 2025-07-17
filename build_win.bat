@echo off
echo GProlog Windows build script for WSL

pushd src
if "%PATH:Pl2Wam=%" == "%PATH%" call :set_path 
setlocal
if '%1' == '' goto usage
if not '%Platform%' == 'x64' goto usage

call ensurePythonInstalled.bat
call :installYasm
call :buildYasm

echo Building GProlog %1
if /I %1 == Debug (
	wsl ./install_win.sh debug %2 %3 %4 %5 %6 %7 %8 %9
) else if /I %1 == Release (
	wsl ./install_win.sh release %2 %3 %4 %5 %6 %7 %8 %9
) else goto usage

goto end

:set_path
set PATH=%cd%\Wam2Ma;%cd%\Ma2Asm;%cd%\Pl2Wam;%cd%\Fd2C;%cd%;%PATH%
exit /b

:installYasm
if exist ..\..\yasm\README exit /b
echo  Installing Yasm from github
pushd ..\..

git clone https://github.com/yasm/yasm.git
if not exist yasm\README (
  echo could not install Yasm
  pause
  goto :end_InstallYasm
)

cd yasm

:end_InstallYasm
popd
exit /b

:buildYasm
if exist ..\..\yasm\Mkfiles\vs\x64\Release\yasm.exe exit /b
echo  Building Yasm
pushd ..\..\Yasm\Mkfiles\vs
for /f "usebackq tokens=1*" %%f in (`reg query HKCR\Python.File\shell\open\command 2^>NUL`) do (set _my_=%%f %%g)
msbuild yasm.sln -p:Configuration=Release

:end_buildYasm
popd
exit /b


:usage
echo Usage: %0 (Debug^|Release) [extra args]
echo Note: Must be run from x64 Native Tools Command Prompt (Visual Studio)
echo example extra args: --disable-linedit
goto end

:end
endlocal
popd
