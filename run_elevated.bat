@echo off
REM 自动以管理员权限启动 Plan_through.exe
REM 由 Qt Creator 调用，或手动双击均可

setlocal
set "EXE_PATH=%~dp0release\Plan_through.exe"

if not exist "%EXE_PATH%" (
    echo [错误] 找不到目标程序: %EXE_PATH%
    pause
    exit /b 1
)

REM 使用 runas 动词触发 UAC 提权
powershell -Command "Start-Process -FilePath '%EXE_PATH%' -Verb RunAs -Wait"
endlocal
