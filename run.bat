@echo off
make -B
if errorlevel 1 exit /b

cd build
main.exe
