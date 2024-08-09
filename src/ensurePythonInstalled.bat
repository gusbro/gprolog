@echo off

set pythonPath=none
for /F "tokens=*" %%F in ('where python.exe^|findstr /r /i /v WindowsApps') do set pythonPath=%%F
if not %pythonPath% == none (
echo Python installed in %pythonPath%
goto end
)

for /F "tokens=* USEBACKQ" %%F IN (`dir %LOCALAPPDATA%\Programs\Python\Python* /b 2^>NUL`) DO (SET pythonPath=%LOCALAPPDATA%\Programs\Python\%%F)
if %pythonPath% == none call :installPython

for /F "tokens=* USEBACKQ" %%F IN (`dir %LOCALAPPDATA%\Programs\Python\Python* /b 2^>NUL`) DO (SET pythonPath=%LOCALAPPDATA%\Programs\Python\%%F)
if pythonPath% == none goto abort

echo Updating path
set PATH=%pythonPath%;%PATH%

echo Python installed in %pythonPath%

goto end

:installPython
echo Installing Python from Microsoft Store
pause
winget install python3 --disable-interactivity --silent
winget uninstall --id=Python.Launcher  -e --disable-interactivity --silent
winget install --id=Python.Launcher  -e --disable-interactivity --silent

exit /b

:abort 
echo Could not find python in %LOCALAPPDATA%\Programs\Python\
echo Aborting...
pause
goto end

:end

