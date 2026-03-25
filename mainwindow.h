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
    MainWindow(bool isAutoStart = false, QWidget *parent = nullptr);
    ~MainWindow();
    
    void applyTheme(int themeType);
    QString loadQss(int type);
    
    void showWindowFromTray();
    void showMemoryCleanError();

private slots:
    void switchToDayView();
    void switchToMonthView();
    void showSettingsWindow();
    void onTrayIconClicked(QSystemTrayIcon::ActivationReason reason);
    void onAutoStartupChanged(Qt::CheckState state);
    void onMinToTrayChanged(Qt::CheckState state);
    void onThemeChanged(int index);
    void openSavePath();
    void openLogPath();
    void goToMsStoreRate();
    void goToGithubReleases();
    void checkMemoryUsage();

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

private:
    void initUI();
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
    
    // 自动内存清理相关
    QTimer *m_memoryCleanTimer = nullptr;
    int m_memoryCleanInterval = 10; // 默认10分钟
    int m_memoryCleanThreshold = 60; // 默认60%
    bool m_isAutoMemoryCleanEnabled = false;
};

#endif
