@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo       Building Banking System (C++20)
echo ========================================================
echo.

if not exist bin mkdir bin
if not exist data mkdir data

echo [1/2] Compiling C++ source files...
g++ -std=c++20 -O2 -Wall -Iinclude src/Utils.cpp src/Transaction.cpp src/Account.cpp src/StorageManager.cpp src/BankService.cpp src/ConsoleUI.cpp src/main.cpp -o bin/BankingSystem.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [!] Build FAILED! Please check the compiler errors above.
    exit /b %ERRORLEVEL%
)

echo [2/2] Build SUCCESSFUL! Output binary: bin\BankingSystem.exe
echo.
echo You can run the application with: run.bat or bin\BankingSystem.exe
echo ========================================================
endlocal
