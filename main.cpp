#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <Windows.h>
#include "mainwindow.h"

#define SERVER_NAME "PlanThrough_SingleInstance_Server"
static MainWindow *g_mainWindow = nullptr;

QString loadQss();

// 主入口点
int main(int argc, char *argv[])
{
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


