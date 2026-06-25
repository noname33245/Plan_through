@echo off
REM 自动以管理员权限启动 Plan_through.exe
REM 放在 release 目录下，与 Plan_through.exe 同级，便于 Qt Creator 调用

setlocal
set "APP_DIR=%~dp0"
set "APP_EXE=%APP_DIR%Plan_through.exe"

if not exist "%APP_EXE%" (
    echo [PlanThrough] 未找到程序：%APP_EXE%
    pause
    exit /b 1
)

REM 用 PowerShell 的 Start-Process -Verb RunAs 触发 UAC 提权
powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%APP_EXE%' -Verb RunAs"
endlocal
exit /b 0
