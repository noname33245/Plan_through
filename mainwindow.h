#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>
#include <QScrollArea>
#include <QMap>
#include <QDate>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDialog>
#include <QCalendarWidget>
#include <QGridLayout>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QProcessEnvironment>
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <QColor>
#include <QPalette>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QFileDialog>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QAbstractAnimation>
#include <QMouseEvent>
#include "widgets/dayview.h"
#include "widgets/monthview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 构造函数
    // 参数1：父窗口指针
    MainWindow(QWidget *parent = nullptr);
    
    // 析构函数，释放所有动态分配的资源
    ~MainWindow();
    
    // 应用主题
    // 参数1：主题类型
    void applyTheme(int themeType);

    QString loadQss(int type);
    
    // 从系统托盘显示窗口
    void showWindowFromTray();

private slots:
    // 切换到日视图
    void switchToDayView();
    
    // 切换到月视图
    void switchToMonthView();
    
    // 显示设置窗口
    void showSettingsWindow();
    
    // 系统托盘图标点击事件处理
    // 参数1：激活原因
    void onTrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    
    // 自动启动设置改变事件处理
    // 参数1：复选框状态
    void onAutoStartupChanged(Qt::CheckState state);
    
    // 最小化到托盘设置改变事件处理
    // 参数1：复选框状态
    void onMinToTrayChanged(Qt::CheckState state);
    
    // 主题设置改变事件处理
    // 参数1：主题索引
    void onThemeChanged(int index);
    
    // 打开存档文件位置
    void openSavePath();
    
    // 打开日志文件位置
    void openLogPath();
    
    // 跳转到微软商店评分页面
    void goToMsStoreRate();

protected:
    // 窗口关闭事件处理
    // 参数1：关闭事件对象
    void closeEvent(QCloseEvent *event) override;
    
    // 窗口大小改变事件处理
    void resizeEvent(QResizeEvent *event) override;
    
    // 鼠标按下事件处理
    void mousePressEvent(QMouseEvent *event) override;
    
    // 鼠标移动事件处理
    void mouseMoveEvent(QMouseEvent *event) override;
    
    // 鼠标释放事件处理
    void mouseReleaseEvent(QMouseEvent *event) override;
    
    // 事件处理函数，拦截所有事件
    bool event(QEvent *event) override;

private:
    // 初始化用户界面
    void initUI();
    
    // 初始化系统托盘
    void initSystemTray();
    


private:
    QPushButton *m_dayViewBtn = nullptr;
    QPushButton *m_monthViewBtn = nullptr;
    QPushButton *m_settingsBtn = nullptr;
    QPushButton *m_minimizeBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QStackedWidget *m_mainStackedWidget = nullptr;

    QSystemTrayIcon *m_systemTrayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;

    DayView* m_dayView = nullptr;
    MonthView* m_monthView = nullptr;
    
    // 用于防止连点的标志
    bool m_isAnimating = false;
    
    // 窗口拖动相关
    bool m_isDragging = false;
    QPoint m_dragStartPos;
    
    // 窗口大小调整相关
    QWidget *m_resizeHandle = nullptr;
    bool m_isResizing = false;
    QPoint m_resizeStartPos;
};

#endif
