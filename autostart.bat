@echo off
chcp 65001 >nul
cd /d "%~dp0"
start "" /min "%~dp0Plan_through.exe" --autostart
