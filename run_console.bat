@echo off
chcp 65001 >nul
cd /d "%~dp0"

if not exist bin\BankingSystem.exe (
    call build.bat
)

bin\BankingSystem.exe
