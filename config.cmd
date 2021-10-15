@echo off
setlocal enabledelayedexpansion
title OneNET_AIO  Config
echo Load Configuration:
set id=1
for /r build\configs %%i in (*.config) do (
    @echo !id!. %%~ni
    set /a id+=1
    set config_list=!config_list! %%i
)
echo %id%. Custom Configuration
set input=%id%
set /p input=Input your choice:

if %input% neq %id% (
    set /a input-=1
    for /f "tokens=%input% delims= " %%i in ("%config_list%") do (
        python.exe build\Kconfiglib\defconfig.py --kconfig Config.menu %%i
    )
)

python.exe build\Kconfiglib\menuconfig.py Config.menu
python.exe build\Kconfiglib\genconfig.py Config.menu

pause & exit