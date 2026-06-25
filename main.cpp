#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <Windows.h>
#include "mainwindow.h"
#include "appdatas.h"

#define SERVER_NAME "PlanThrough_SingleInstance_Server"
static MainWindow *g_mainWindow = nullptr;

QString loadQss();

int main(int argc, char *argv[])
{
    try {
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

        // 检查命令行参数，判断是否是开机自启
        bool isAutoStart = false;
        for (int i = 1; i < argc; ++i) {
            if (QString(argv[i]) == "--autostart") {
                isAutoStart = true;
                break;
            }
        }
        qDebug() << "Is auto start:" << isAutoStart;

        MainWindow w(isAutoStart);
        g_mainWindow = &w;

        // 延迟到事件循环启动后同步开机自启状态
        QTimer::singleShot(0, &a, [&]() {
            bool enabled = appDatas.isAutoStartup();
            qDebug() << "========== 启动时同步开机自启状态 ==========";
            qDebug() << "配置中的 auto_startup 值:" << enabled;
            if (enabled) {
                qDebug() << "尝试创建/更新计划任务...";
                appDatas.setAutoStartup(true);
            } else {
                qDebug() << "清理可能的残留计划任务...";
                appDatas.setAutoStartup(false);
            }
        });

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


