#include "service.h"
#include <QDebug>
#include <QCoreApplication>
#include <Windows.h>
#include <shellapi.h>

// 检查是否拥有管理员权限
static BOOL IsElevated()
{
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &Elevation, cbSize, &cbSize))
        {
            bRet = Elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return bRet;
}

// 自动UAC提权重启程序
static BOOL RunElevated()
{
    QString appPath = QCoreApplication::applicationFilePath();
    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(SHELLEXECUTEINFOW));

    sei.cbSize = sizeof(SHELLEXECUTEINFOW);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = L"runas";
    sei.lpFile = appPath.toStdWString().c_str();
    sei.lpParameters = NULL;
    sei.lpDirectory = NULL;
    sei.nShow = SW_NORMAL;

    return ShellExecuteExW(&sei);
}

bool ServiceManager::installService(const QString& serviceName, const QString& displayName, const QString& executablePath)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service installation will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot install service without administrator privileges";
            return false;
        }
    }

    // 构建带参数的可执行路径
    QString serviceExecutablePath = executablePath + " --service";
    
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = CreateService(
        schSCManager,
        (LPCWSTR)serviceName.utf16(),
        (LPCWSTR)displayName.utf16(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        (LPCWSTR)serviceExecutablePath.utf16(),
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (schService == NULL) {
        qDebug() << "CreateService failed with error:" << GetLastError();
        CloseServiceHandle(schSCManager);
        return false;
    }

    qDebug() << "Service installed successfully!";
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool ServiceManager::uninstallService(const QString& serviceName)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service uninstallation will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot uninstall service without administrator privileges";
            return false;
        }
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, (LPCWSTR)serviceName.utf16(), DELETE | SERVICE_STOP);
    if (schService == NULL) {
        qDebug() << "OpenService failed with error:" << GetLastError();
        CloseServiceHandle(schSCManager);
        return false;
    }

    // Stop the service if it's running
    SERVICE_STATUS ssStatus;
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
        qDebug() << "Stopping service...";
        Sleep(1000);

        while (QueryServiceStatus(schService, &ssStatus)) {
            if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                qDebug() << "Service is stopping...";
                Sleep(1000);
            } else {
                break;
            }
        }

        if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
            qDebug() << "Service stopped successfully!";
        } else {
            qDebug() << "Service could not be stopped!";
        }
    }

    if (!DeleteService(schService)) {
        qDebug() << "DeleteService failed with error:" << GetLastError();
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    qDebug() << "Service uninstalled successfully!";
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool ServiceManager::startService(const QString& serviceName)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service start will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot start service without administrator privileges";
            return false;
        }
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, (LPCWSTR)serviceName.utf16(), SERVICE_START);
    if (schService == NULL) {
        qDebug() << "OpenService failed with error:" << GetLastError();
        CloseServiceHandle(schSCManager);
        return false;
    }

    if (!StartService(schService, 0, NULL)) {
        qDebug() << "StartService failed with error:" << GetLastError();
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    qDebug() << "Service started successfully!";
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return true;
}

bool ServiceManager::stopService(const QString& serviceName)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service stop will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot stop service without administrator privileges";
            return false;
        }
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, (LPCWSTR)serviceName.utf16(), SERVICE_STOP);
    if (schService == NULL) {
        qDebug() << "OpenService failed with error:" << GetLastError();
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_STATUS ssStatus;
    if (!ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
        qDebug() << "ControlService failed with error:" << GetLastError();
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }

    qDebug() << "Service stop pending...";
    while (QueryServiceStatus(schService, &ssStatus)) {
        if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
            Sleep(1000);
        } else {
            break;
        }
    }

    if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
        qDebug() << "Service stopped successfully!";
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return true;
    } else {
        qDebug() << "Service could not be stopped!";
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return false;
    }
}

bool ServiceManager::isServiceInstalled(const QString& serviceName)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service status check will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot check service status without administrator privileges";
            return false;
        }
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, (LPCWSTR)serviceName.utf16(), SERVICE_QUERY_STATUS);
    bool result = (schService != NULL);

    if (schService) {
        CloseServiceHandle(schService);
    }

    CloseServiceHandle(schSCManager);
    return result;
}

bool ServiceManager::isServiceRunning(const QString& serviceName)
{
    // 检查是否有管理员权限
    if (!IsElevated()) {
        qDebug() << "No administrator privileges, attempting to elevate...";
        // 尝试提权
        if (RunElevated()) {
            qDebug() << "Elevation successful, service status check will continue in elevated process";
            return true;
        } else {
            qDebug() << "Elevation failed, cannot check service status without administrator privileges";
            return false;
        }
    }

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (schSCManager == NULL) {
        qDebug() << "OpenSCManager failed with error:" << GetLastError();
        return false;
    }

    SC_HANDLE schService = OpenService(schSCManager, (LPCWSTR)serviceName.utf16(), SERVICE_QUERY_STATUS);
    if (schService == NULL) {
        qDebug() << "OpenService failed with error:" << GetLastError();
        CloseServiceHandle(schSCManager);
        return false;
    }

    SERVICE_STATUS ssStatus;
    bool result = false;

    if (QueryServiceStatus(schService, &ssStatus)) {
        result = (ssStatus.dwCurrentState == SERVICE_RUNNING);
    } else {
        qDebug() << "QueryServiceStatus failed with error:" << GetLastError();
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return result;
}
