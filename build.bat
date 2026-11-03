@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo       Building Bank Forma Tail (Desktop GUI + C++)
echo ========================================================
echo.

if not exist bin mkdir bin
if not exist data mkdir data

echo [1/3] Compiling Desktop GUI Launcher (bin\BankFormaTailApp.exe)...
g++ -std=c++20 -O2 -mwindows src/launcher.cpp -o bin/BankFormaTailApp.exe
if %ERRORLEVEL% NEQ 0 (
    echo [!] Launcher compilation failed!
    exit /b %ERRORLEVEL%
)

echo [2/3] Compiling Native Win32 GUI (bin\BankingGUI.exe)...
g++ -std=c++20 -O2 -Wall -mwindows -Iinclude src/Utils.cpp src/Transaction.cpp src/Account.cpp src/StorageManager.cpp src/BankService.cpp src/gui_main.cpp -lcomctl32 -lgdi32 -o bin/BankingGUI.exe
if %ERRORLEVEL% NEQ 0 (
    echo [!] Native GUI compilation failed!
    exit /b %ERRORLEVEL%
)

echo [3/3] Compiling Console Client (bin\BankingSystem.exe)...
g++ -std=c++20 -O2 -Wall -Iinclude src/Utils.cpp src/Transaction.cpp src/Account.cpp src/StorageManager.cpp src/BankService.cpp src/ConsoleUI.cpp src/main.cpp -o bin/BankingSystem.exe
if %ERRORLEVEL% NEQ 0 (
    echo [!] Console build failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================================
echo [OK] Build SUCCESSFUL!
echo Opening Desktop GUI Window...
echo ========================================================

:: Launch Desktop GUI window immediately
start "" "bin\BankFormaTailApp.exe"

endlocal
