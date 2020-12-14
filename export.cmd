@echo off
SET PATH=%CD%\build\msys2;%PATH%
if not exist tmp md tmp
bash.exe --norc --noprofile export.sh
if exist tmp rd /s /q tmp
pause