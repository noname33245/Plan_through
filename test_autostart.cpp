#include <QApplication>
#include <QSettings>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    qDebug() << "Test program started";
    
    // 测试读取当前设置
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString currentValue = settings.value("PlanThrough").toString();
    qDebug() << "Current PlanThrough value in registry:" << currentValue;
    
    // 测试写入新值
    QString executablePath = QApplication::applicationFilePath().replace("/", "\\");
    if (!executablePath.startsWith('"') && !executablePath.endsWith('"')) {
        executablePath = '"' + executablePath + '"';
    }
    qDebug() << "Attempting to write path:" << executablePath;
    
    settings.setValue("PlanThrough", executablePath);
    qDebug() << "Registry write status:" << (settings.status() == QSettings::NoError ? "Success" : "Failure");
    
    // 重新读取验证
    QString newValue = settings.value("PlanThrough").toString();
    qDebug() << "New PlanThrough value in registry:" << newValue;
    
    qDebug() << "Test completed";
    
    return 0;
}
