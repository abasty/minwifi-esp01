@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "WEBSOCAT_URL=https://github.com/vi/websocat/releases/latest/download/websocat.x86_64-pc-windows-gnu.exe"
set "WEBSOCAT_LOCAL=%SCRIPT_DIR%websocat.exe"
set "WS_ADDR=127.0.0.1:1967"

where websocat.exe >nul 2>nul
if %errorlevel%==0 (
    set "WEBSOCAT=websocat.exe"
) else (
    if exist "%WEBSOCAT_LOCAL%" (
        set "WEBSOCAT=%WEBSOCAT_LOCAL%"
    ) else (
        echo websocat introuvable dans le PATH.
        set /p "REPLY=Le telecharger maintenant dans %SCRIPT_DIR% ? [o/N] "
        set "FIRSTCHAR=!REPLY:~0,1!"
        set "DOWNLOAD=0"
        if /i "!FIRSTCHAR!"=="o" set "DOWNLOAD=1"
        if /i "!FIRSTCHAR!"=="y" set "DOWNLOAD=1"
        if "!DOWNLOAD!"=="1" (
            echo Telechargement de websocat...
            curl.exe -fL -o "%WEBSOCAT_LOCAL%" "%WEBSOCAT_URL%"
            if errorlevel 1 (
                echo Echec du telechargement.
                exit /b 1
            )
            set "WEBSOCAT=%WEBSOCAT_LOCAL%"
        ) else (
            echo Annule. Installe websocat ou relance ce script.
            exit /b 1
        )
    )
)

rem The binary sits next to this script in a distributed archive, but in the
rem dev repo it's still under lib\basic\test\bin\.
if exist "%SCRIPT_DIR%bastos-windows-amd64.exe" (
    set "BASTOS_EXE=%SCRIPT_DIR%bastos-windows-amd64.exe"
) else (
    set "BASTOS_EXE=%SCRIPT_DIR%lib\basic\test\bin\bastos-windows-amd64.exe"
)

echo BASTOS lance sur ws://%WS_ADDR%
echo Connecte-toi avec minterm : https://abasty.github.io/minterm/?ws=ws%%3A%%2F%%2F127.0.0.1%%3A1967

"%WEBSOCAT%" -v -t -E --no-line ws-l:%WS_ADDR% exec:"%BASTOS_EXE%"
