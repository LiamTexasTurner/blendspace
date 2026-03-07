@echo off
make
if errorlevel 1 exit /b

cd build
main.exe
