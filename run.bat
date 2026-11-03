@echo off
chcp 65001 >nul
cd /d "%~dp0"

if not exist bin\BankingSystem.exe (
    echo [!] Виконуваний файл не знайдено. Спочатку запускаємо build.bat...
    call build.bat
)

if exist bin\BankingSystem.exe (
    bin\BankingSystem.exe
)
