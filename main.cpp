#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <Windows.h>
#include "mainwindow.h"

#define SERVER_NAME "PlanThrough_SingleInstance_Server"
static MainWindow *g_mainWindow = nullptr;

// 服务相关变量
SERVICE_STATUS g_ServiceStatus;
SERVICE_STATUS_HANDLE g_ServiceStatusHandle = nullptr;
HANDLE g_ServiceStopEvent = nullptr;

QString loadQss();

// 服务主函数
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
// 服务控制处理函数
void WINAPI ServiceControlHandler(DWORD dwControl);
// 服务工作线程
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

// 主入口点
int main(int argc, char *argv[])
{
    // 检查是否以服务方式启动
    if (argc > 1 && QString(argv[1]) == "--service") {
        qDebug() << "Starting as service";
        
        // 服务入口点
    SERVICE_TABLE_ENTRYW ServiceTable[] = {
        {L"PlanThroughService", ServiceMain},
        {NULL, NULL}
    };
        
        if (!StartServiceCtrlDispatcherW(ServiceTable)) {
            qDebug() << "StartServiceCtrlDispatcherW failed with error:" << GetLastError();
            return 1;
        }
        
        return 0;
    }
    
    // 普通GUI应用程序启动
    try {
        // 设置高DPI策略，防止窗口在不同显示器间拖动时大小改变
        QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
        QApplication a(argc, argv);

        qDebug() << "Application started";

        QLocalSocket socket;
        socket.connectToServer(SERVER_NAME);
        if(socket.waitForConnected(200))
        {
            qDebug() << "Another instance already running";
            socket.close();
            return 0;
        }

        qDebug() << "Creating server";

        QLocalServer* server = new QLocalServer(&a);
        QObject::connect(server, &QLocalServer::newConnection, [=](){
            QLocalSocket *clientSocket = server->nextPendingConnection();
            clientSocket->close();
            clientSocket->deleteLater();
            if(g_mainWindow)
            {
                g_mainWindow->showWindowFromTray();
            }
        });
        server->listen(SERVER_NAME);

        qDebug() << "Creating main window";

        MainWindow w;
        g_mainWindow = &w;
        qDebug() << "Showing main window";
        w.show();

        qDebug() << "Entering event loop";

        return a.exec();
    } catch (const std::exception &e) {
        qCritical() << "Exception caught:" << e.what();
        return 1;
    } catch (...) {
        qCritical() << "Unknown exception caught";
        return 1;
    }
}

// 服务主函数
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    qDebug() << "ServiceMain called";
    
    // 初始化服务状态
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    
    // 注册服务控制处理函数
    g_ServiceStatusHandle = RegisterServiceCtrlHandlerW(L"PlanThroughService", ServiceControlHandler);
    if (g_ServiceStatusHandle == nullptr) {
        qDebug() << "RegisterServiceCtrlHandlerW failed with error:" << GetLastError();
        return;
    }
    
    // 创建停止事件
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == nullptr) {
        qDebug() << "CreateEvent failed with error:" << GetLastError();
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
        return;
    }
    
    // 更新服务状态为运行中
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    if (!SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus)) {
        qDebug() << "SetServiceStatus failed with error:" << GetLastError();
        return;
    }
    
    // 创建工作线程
    HANDLE hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if (hThread == nullptr) {
        qDebug() << "CreateThread failed with error:" << GetLastError();
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
        return;
    }
    
    // 等待停止事件
    WaitForSingleObject(g_ServiceStopEvent, INFINITE);
    
    // 清理
    CloseHandle(hThread);
    CloseHandle(g_ServiceStopEvent);
    
    // 更新服务状态为停止
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwCheckPoint = 0;
    g_ServiceStatus.dwWaitHint = 0;
    SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
    
    qDebug() << "Service stopped";
}

// 服务控制处理函数
void WINAPI ServiceControlHandler(DWORD dwControl)
{
    switch (dwControl) {
        case SERVICE_CONTROL_STOP:
            qDebug() << "Service stop requested";
            if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
                break;
            }
            
            // 更新服务状态为停止中
            g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            g_ServiceStatus.dwCheckPoint = 1;
            g_ServiceStatus.dwWaitHint = 5000;
            if (!SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus)) {
                qDebug() << "SetServiceStatus failed with error:" << GetLastError();
            }
            
            // 触发停止事件
            SetEvent(g_ServiceStopEvent);
            break;
            
        case SERVICE_CONTROL_INTERROGATE:
            // 响应查询请求
            SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
            break;
            
        default:
            break;
    }
}

// 服务工作线程
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    Q_UNUSED(lpParam);
    qDebug() << "ServiceWorkerThread started";
    
    // 启动应用程序
    QString appPath = QApplication::applicationFilePath();
    appPath = appPath.replace("/", "\\");
    
    qDebug() << "Starting application:" << appPath;
    
    // 使用ShellExecute来启动应用程序，这样可以处理UAC提权
    SHELLEXECUTEINFOW sei;
    ZeroMemory(&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = L"runas"; // 请求管理员权限
    sei.lpFile = (LPCWSTR)appPath.utf16();
    sei.lpParameters = NULL;
    sei.lpDirectory = NULL;
    sei.nShow = SW_NORMAL;
    
    if (!ShellExecuteExW(&sei)) {
        qDebug() << "ShellExecuteEx failed with error:" << GetLastError();
    } else {
        qDebug() << "Application started successfully";
        // 等待应用程序退出
        if (sei.hProcess != NULL) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
    }
    
    qDebug() << "ServiceWorkerThread exiting";
    return 0;
}
