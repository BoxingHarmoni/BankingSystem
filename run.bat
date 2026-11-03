@echo off
chcp 65001 >nul
cd /d "%~dp0"

if not exist bin\BankFormaTailApp.exe (
    echo [!] Файли не скомпільовані. Запускаємо build.bat...
    call build.bat
    exit /b
)

:: Запуск красивого десктопного GUI вікна
start "" "bin\BankFormaTailApp.exe"
