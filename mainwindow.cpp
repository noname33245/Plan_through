#include "datastruct.h"
#include "utils/datehelper.h"
#include "utils/widgetcontainer.h"
#include "widgets/dayview.h"
#include "widgets/monthview.h"
#include "mainwindow.h"
#include "appdatas.h"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 去除默认标题栏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    
    // 设置窗口图标
    this->setWindowIcon(QIcon(":/16.ico"));
    
    widgetContainer("main", this);
    initUI();
    applyTheme(appDatas.themeType());
    initSystemTray();
    // 初始化日视图和月视图数据
    findChild<DayView*>("dayView")->loadDateData(DateHelper::currentDate());
    findChild<DayView*>("dayView")->updateDayViewStats();
    findChild<MonthView*>("monthView")->generateMonthCalendar();



    // 根据用户设置显示默认视图
    if (appDatas.defaultViewType() == 0) {
        switchToMonthView();
    } else {
        switchToDayView();
    }

    // 强制窗口显示
    this->showNormal();
    this->raise();
    this->activateWindow();

    // 设置窗口最小大小
    this->setMinimumSize(400, 500);
    
    // 强制设置窗口位置到屏幕中心
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    // 如果设置了开机自启，则隐藏窗口
    if (appDatas.isAutoStartup()) {
        this->hide();
    }
}

// 析构函数，释放所有动态分配的资源
MainWindow::~MainWindow()
{
    // 释放系统托盘相关资源
    if (m_systemTrayIcon) {
        m_systemTrayIcon->hide();
        delete m_systemTrayIcon;
        m_systemTrayIcon = nullptr;
    }
    if (m_trayMenu) {
        delete m_trayMenu;
        m_trayMenu = nullptr;
    }
}

// 应用主题样式
// @param themeType 主题类型，0表示默认主题，1表示深色主题
void MainWindow::applyTheme(int themeType)
{
    appDatas.setTheme(themeType);
    this->setStyleSheet(loadQss(themeType));
    findChild<DayView*>("dayView")->updateDayViewStats();
}

// 初始化系统托盘
void MainWindow::initSystemTray()
{
    m_systemTrayIcon = new QSystemTrayIcon(this);
    m_systemTrayIcon->setIcon(QIcon(":/16.ico"));
    m_systemTrayIcon->setToolTip("学习计划打卡");

    // 创建托盘菜单
    m_trayMenu = new QMenu(this);
    m_trayMenu->setStyleSheet(
        "QMenu{background-color:#FFFFFF; border:1px solid #EEEEEE; border-radius:6px; padding:3px 0px;}"
        "QMenu::item{color:#000000; font-size:12px; padding:4px 30px 4px 15px; margin:1px 3px; border-radius:3px;}"
        "QMenu::item:selected{background-color:#ECF5FF; color:#000000;}"
        "QMenu::separator{height:1px; background-color:#EEEEEE; margin:3px 0px;}"
    );

    // 添加托盘菜单选项
    QAction *showAct = new QAction("显示窗口", this);
    QAction *exitAct = new QAction("退出程序", this);

    connect(showAct, &QAction::triggered, this, &MainWindow::showWindowFromTray);
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    m_trayMenu->addAction(showAct);
    m_trayMenu->addAction(exitAct);

    m_systemTrayIcon->setContextMenu(m_trayMenu);
    m_systemTrayIcon->show();
    connect(m_systemTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconClicked);
}

// 系统托盘图标点击事件处理
// @param reason 激活原因
void MainWindow::onTrayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    // 双击托盘图标显示窗口
    if (reason == QSystemTrayIcon::DoubleClick) {
        this->showWindowFromTray();
    }
}

// 从系统托盘显示窗口
void MainWindow::showWindowFromTray()
{
    this->show();
    this->activateWindow();
    this->raise();
}

// 切换到日视图
void MainWindow::switchToDayView()
{
    if (m_isAnimating) {
        return;
    }
    
    if (m_mainStackedWidget->currentIndex() == 0) {
        // 已经在日视图，确保按钮状态正确
        m_dayViewBtn->setChecked(true);
        m_monthViewBtn->setChecked(false);
        findChild<DayView*>("dayView")->updateDayViewStats();
        return;
    }
    
    m_isAnimating = true;
    
    QWidget *currentWidget = m_mainStackedWidget->currentWidget();
    if (!currentWidget) {
        m_isAnimating = false;
        return;
    }
    
    QPoint originalPos = currentWidget->pos();
    
    // 创建退出动画 - 只执行一半（快→慢阶段）
    QParallelAnimationGroup *exitGroup = new QParallelAnimationGroup(this);
    
    QPropertyAnimation *fadeOutAnim = new QPropertyAnimation(currentWidget, "windowOpacity");
        fadeOutAnim->setDuration(250); // 总动画时长的一半
        fadeOutAnim->setEasingCurve(QEasingCurve::InCubic); // 慢→快的缓动曲线
        fadeOutAnim->setStartValue(1.0);
        fadeOutAnim->setEndValue(0.5); // 只淡出到半透明
        
        QPropertyAnimation *slideOutAnim = new QPropertyAnimation(currentWidget, "pos");
        slideOutAnim->setDuration(250);
        slideOutAnim->setEasingCurve(QEasingCurve::InCubic); // 慢→快的缓动曲线
        slideOutAnim->setStartValue(originalPos);
        slideOutAnim->setEndValue(QPoint(originalPos.x() + 30, originalPos.y())); // 只滑动一半距离
    
    exitGroup->addAnimation(fadeOutAnim);
    exitGroup->addAnimation(slideOutAnim);
    
    connect(exitGroup, &QParallelAnimationGroup::finished, this, [=]() {
        // 在速度最快时（动画中点）切换页面
        m_mainStackedWidget->setCurrentIndex(0);
        
        // 恢复当前控件状态
        currentWidget->move(originalPos);
        currentWidget->setWindowOpacity(1.0);
        
        QWidget *newWidget = m_mainStackedWidget->currentWidget();
        if (!newWidget) {
            m_isAnimating = false;
            return;
        }
        
        QPoint newOriginalPos = newWidget->pos();
        
        // 月视图→日视图：日视图从左滑入
        QPoint newStartPos = newOriginalPos - QPoint(30, 0);
        
        newWidget->setWindowOpacity(0.5); // 从半透明开始
        newWidget->move(newStartPos);
        
        // 执行进入动画的后半段（慢→快阶段）
        QParallelAnimationGroup *enterGroup = new QParallelAnimationGroup(this);
        
        QPropertyAnimation *fadeInAnim = new QPropertyAnimation(newWidget, "windowOpacity");
        fadeInAnim->setDuration(250);
        fadeInAnim->setEasingCurve(QEasingCurve::OutCubic); // 快→慢的缓动曲线
        fadeInAnim->setStartValue(0.5);
        fadeInAnim->setEndValue(1.0);
        
        QPropertyAnimation *slideInAnim = new QPropertyAnimation(newWidget, "pos");
        slideInAnim->setDuration(250);
        slideInAnim->setEasingCurve(QEasingCurve::OutCubic); // 快→慢的缓动曲线
        slideInAnim->setStartValue(newStartPos);
        slideInAnim->setEndValue(newOriginalPos);
        
        enterGroup->addAnimation(fadeInAnim);
        enterGroup->addAnimation(slideInAnim);
        
        connect(enterGroup, &QParallelAnimationGroup::finished, this, [=]() {
            newWidget->move(newOriginalPos);
            newWidget->setWindowOpacity(1.0);
            
            // 最终保障：强制设置正确的按钮状态
            // 日视图显示 → 日视图按钮必须为checked，月视图按钮必须为unchecked
            m_dayViewBtn->setChecked(true);
            m_monthViewBtn->setChecked(false);
            findChild<DayView*>("dayView")->updateDayViewStats();
            
            m_isAnimating = false;
        });
        
        enterGroup->start(QAbstractAnimation::DeleteWhenStopped);
        exitGroup->deleteLater();
    });
    
    exitGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

// 切换到月视图
void MainWindow::switchToMonthView()
{
    if (m_isAnimating) {
        return;
    }
    
    if (m_mainStackedWidget->currentIndex() == 1) {
        // 已经在月视图，确保按钮状态正确
        m_monthViewBtn->setChecked(true);
        m_dayViewBtn->setChecked(false);
        findChild<MonthView*>("monthView")->generateMonthCalendar();
        return;
    }
    
    m_isAnimating = true;
    
    QWidget *currentWidget = m_mainStackedWidget->currentWidget();
    if (!currentWidget) {
        m_isAnimating = false;
        return;
    }
    
    QPoint originalPos = currentWidget->pos();
    
    // 创建退出动画 - 只执行一半（快→慢阶段）
    QParallelAnimationGroup *exitGroup = new QParallelAnimationGroup(this);
    
    QPropertyAnimation *fadeOutAnim = new QPropertyAnimation(currentWidget, "windowOpacity");
        fadeOutAnim->setDuration(250); // 总动画时长的一半
        fadeOutAnim->setEasingCurve(QEasingCurve::InCubic); // 慢→快的缓动曲线
        fadeOutAnim->setStartValue(1.0);
        fadeOutAnim->setEndValue(0.5); // 只淡出到半透明
        
        QPropertyAnimation *slideOutAnim = new QPropertyAnimation(currentWidget, "pos");
        slideOutAnim->setDuration(250);
        slideOutAnim->setEasingCurve(QEasingCurve::InCubic); // 慢→快的缓动曲线
        slideOutAnim->setStartValue(originalPos);
        slideOutAnim->setEndValue(QPoint(originalPos.x() - 30, originalPos.y())); // 只滑动一半距离
    
    exitGroup->addAnimation(fadeOutAnim);
    exitGroup->addAnimation(slideOutAnim);
    
    connect(exitGroup, &QParallelAnimationGroup::finished, this, [=]() {
        // 在速度最快时（动画中点）切换页面
        m_mainStackedWidget->setCurrentIndex(1);
        
        // 恢复当前控件状态
        currentWidget->move(originalPos);
        currentWidget->setWindowOpacity(1.0);
        
        QWidget *newWidget = m_mainStackedWidget->currentWidget();
        if (!newWidget) {
            m_isAnimating = false;
            return;
        }
        
        QPoint newOriginalPos = newWidget->pos();
        
        // 日视图→月视图：月视图从右滑入
        QPoint newStartPos = newOriginalPos + QPoint(30, 0);
        
        newWidget->setWindowOpacity(0.5); // 从半透明开始
        newWidget->move(newStartPos);
        
        // 执行进入动画的后半段（慢→快阶段）
        QParallelAnimationGroup *enterGroup = new QParallelAnimationGroup(this);
        
        QPropertyAnimation *fadeInAnim = new QPropertyAnimation(newWidget, "windowOpacity");
        fadeInAnim->setDuration(250);
        fadeInAnim->setEasingCurve(QEasingCurve::OutCubic); // 快→慢的缓动曲线
        fadeInAnim->setStartValue(0.5);
        fadeInAnim->setEndValue(1.0);
        
        QPropertyAnimation *slideInAnim = new QPropertyAnimation(newWidget, "pos");
        slideInAnim->setDuration(250);
        slideInAnim->setEasingCurve(QEasingCurve::OutCubic); // 快→慢的缓动曲线
        slideInAnim->setStartValue(newStartPos);
        slideInAnim->setEndValue(newOriginalPos);
        
        enterGroup->addAnimation(fadeInAnim);
        enterGroup->addAnimation(slideInAnim);
        
        connect(enterGroup, &QParallelAnimationGroup::finished, this, [=]() {
            newWidget->move(newOriginalPos);
            newWidget->setWindowOpacity(1.0);
            
            // 最终保障：强制设置正确的按钮状态
            // 月视图显示 → 月视图按钮必须为checked，日视图按钮必须为unchecked
            m_monthViewBtn->setChecked(true);
            m_dayViewBtn->setChecked(false);
            findChild<MonthView*>("monthView")->generateMonthCalendar();
            
            m_isAnimating = false;
        });
        
        enterGroup->start(QAbstractAnimation::DeleteWhenStopped);
        exitGroup->deleteLater();
    });
    
    exitGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

// 显示设置窗口
void MainWindow::showSettingsWindow()
{
    QDialog *settingsDlg = new QDialog(this);
    settingsDlg->setWindowTitle("软件设置");
    settingsDlg->setFixedSize(350, 300);
    settingsDlg->setModal(true);
    
    // 禁用所有可能的窗口动画效果
    settingsDlg->setAttribute(Qt::WA_NoSystemBackground, false);
    settingsDlg->setAttribute(Qt::WA_DontShowOnScreen, false);
    settingsDlg->setAttribute(Qt::WA_TranslucentBackground, false);
    settingsDlg->setWindowOpacity(1.0);

    // 设置对话框样式
    settingsDlg->setStyleSheet(
        "QDialog{background-color:#FFFFFF; border-radius:10px; border:1px solid #EEEEEE;}"
        "QLabel{font-size:12px; color:#000000; font-weight:normal;}"
        "QCheckBox{font-size:12px; color:#000000; padding:3px; background-color:transparent;}"
        "QCheckBox::indicator{width:14px;height:14px;border:1px solid #CCCCCC;border-radius:2px;background-color:#FFFFFF;}"
        "QCheckBox::indicator:checked{background-color:#2D8CF0;border-color:#2D8CF0;}"
        "QComboBox{font-size:12px; color:#000000; height:26px; padding:0 6px; border:1px solid #DDDDDD; border-radius:4px; background-color:#FFFFFF;}"
        "QComboBox::drop-down{border:none;}"
        "QComboBox::down-arrow{width:10px;height:10px;}"
        "QComboBox QAbstractItemView{background-color:#FFFFFF; color:#000000; border:1px solid #DDDDDD; selection-background-color:#ECF5FF; selection-color:#000000;}"
        "QPushButton{font-size:12px; padding:4px 10px; border-radius:4px; border:none; color:#FFFFFF;}"
        "QPushButton:hover{opacity:0.9;}"
        "QPushButton:pressed{opacity:0.8;}"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(settingsDlg);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // 开机自启设置
    QHBoxLayout *autoStartLayout = new QHBoxLayout;
    QCheckBox *autoStartCb = new QCheckBox("开机自动启动");
    autoStartCb->setChecked(appDatas.isAutoStartup());
    autoStartLayout->addWidget(autoStartCb);
    autoStartLayout->addStretch();
    connect(autoStartCb, &QCheckBox::checkStateChanged, this, &MainWindow::onAutoStartupChanged);

    // 关闭窗口后最小化到托盘设置
    QHBoxLayout *minTrayLayout = new QHBoxLayout;
    QCheckBox *minTrayCb = new QCheckBox("关闭窗口后最小化到托盘");
    minTrayCb->setChecked(appDatas.isMinToTray());
    minTrayLayout->addWidget(minTrayCb);
    minTrayLayout->addStretch();
    connect(minTrayCb, &QCheckBox::checkStateChanged, this, &MainWindow::onMinToTrayChanged);

    // 主题设置
    QHBoxLayout *themeLayout = new QHBoxLayout;
    QLabel *themeLab = new QLabel("软件主题：");
    QComboBox *themeCbx = new QComboBox;
    themeCbx->addItems({"简约灰", "纯净白"});
    themeCbx->setCurrentIndex(appDatas.themeType());
    themeLayout->addWidget(themeLab);
    themeLayout->addWidget(themeCbx);
    themeLayout->addStretch();
    connect(themeCbx, &QComboBox::currentIndexChanged, this, &MainWindow::onThemeChanged);
    
    // 默认视图设置
    QHBoxLayout *defaultViewLayout = new QHBoxLayout;
    QLabel *defaultViewLab = new QLabel("默认视图：");
    QComboBox *defaultViewCbx = new QComboBox;
    defaultViewCbx->addItems({"月视图", "日视图"});
    defaultViewCbx->setCurrentIndex(appDatas.defaultViewType());
    defaultViewLayout->addWidget(defaultViewLab);
    defaultViewLayout->addWidget(defaultViewCbx);
    defaultViewLayout->addStretch();
    connect(defaultViewCbx, &QComboBox::currentIndexChanged, [=](int index) {
        appDatas.setDefaultViewType(index);
    });

    // 打开存档文件位置
    QHBoxLayout *pathLayout = new QHBoxLayout;
    QPushButton *pathBtn = new QPushButton("打开存档文件位置");
    pathBtn->setStyleSheet("background-color:#2D8CF0;");
    pathLayout->addWidget(pathBtn);
    pathLayout->addStretch();
    connect(pathBtn, &QPushButton::clicked, this, &MainWindow::openSavePath);

    // 打开日志文件位置
    QHBoxLayout *logLayout = new QHBoxLayout;
    QPushButton *logBtn = new QPushButton("打开日志文件位置");
    logBtn->setStyleSheet("background-color:#F59E0B;");
    logLayout->addWidget(logBtn);
    logLayout->addStretch();
    connect(logBtn, &QPushButton::clicked, this, &MainWindow::openLogPath);

    // 微软商店评分
    QHBoxLayout *rateLayout = new QHBoxLayout;
    QPushButton *rateBtn = new QPushButton("微软商店好评支持一下吧 ❤️");
    rateBtn->setStyleSheet("background-color:#27AE60;");
    rateLayout->addWidget(rateBtn);
    rateLayout->addStretch();
    connect(rateBtn, &QPushButton::clicked, this, &MainWindow::goToMsStoreRate);

    // 数据备份和恢复
    QHBoxLayout *backupLayout = new QHBoxLayout;
    QPushButton *createBackupBtn = new QPushButton("创建数据备份");
    createBackupBtn->setStyleSheet("background-color:#34B7F1;");
    QPushButton *restoreBackupBtn = new QPushButton("从备份恢复");
    restoreBackupBtn->setStyleSheet("background-color:#9370DB;");

    backupLayout->addWidget(createBackupBtn);
    backupLayout->addWidget(restoreBackupBtn);
    backupLayout->addStretch();

    // 连接备份和恢复按钮的信号槽
    connect(createBackupBtn, &QPushButton::clicked, [=]() {
        // 获取当前日期时间作为备份文件名
        QString backupFileName = "study_data_backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";
        QString backupPath = QFileDialog::getSaveFileName(settingsDlg, "保存数据备份", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + backupFileName, "JSON Files (*.json)");

        if (!backupPath.isEmpty()) {
            if (appDatas.createBackup(backupPath)) {
                QMessageBox::information(settingsDlg, "成功", "数据备份创建成功！\n" + backupPath);
            } else {
                QMessageBox::critical(settingsDlg, "失败", "数据备份创建失败！");
            }
        }
    });

    connect(restoreBackupBtn, &QPushButton::clicked, [=]() {
        QString backupPath = QFileDialog::getOpenFileName(settingsDlg, "选择数据备份文件", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "JSON Files (*.json)");

        if (!backupPath.isEmpty()) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(settingsDlg, "警告", "从备份恢复将覆盖当前所有数据，是否继续？",
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                if (appDatas.restoreFromBackup(backupPath)) {
                    QMessageBox::information(settingsDlg, "成功", "数据恢复成功！");
                    // 刷新界面数据
                    switchToDayView();
                } else {
                    QMessageBox::critical(settingsDlg, "失败", "数据恢复失败！");
                }
            }
        }
    });

    // 添加所有布局到主布局
    mainLayout->addLayout(autoStartLayout);
    mainLayout->addLayout(minTrayLayout);
    mainLayout->addLayout(themeLayout);
    mainLayout->addLayout(defaultViewLayout);
    mainLayout->addLayout(pathLayout);
    mainLayout->addLayout(logLayout);
    mainLayout->addLayout(backupLayout);
    mainLayout->addLayout(rateLayout);
    mainLayout->addStretch();

    settingsDlg->exec();
    appDatas.saveSettings();
}

// 自动启动设置改变事件处理
// @param state 复选框状态
void MainWindow::onAutoStartupChanged(Qt::CheckState state)
{
    appDatas.setAutoStartup(state == Qt::Checked);
}

// 最小化到托盘设置改变事件处理
// @param state 复选框状态
void MainWindow::onMinToTrayChanged(Qt::CheckState state)
{
    appDatas.setMinToTray(state == Qt::Checked);
}

// 主题设置改变事件处理
// @param index 主题索引
void MainWindow::onThemeChanged(int index)
{
    applyTheme(index);
}

// 打开存档文件位置
void MainWindow::openSavePath()
{
    QString savePath = appDatas.path("Save");
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(savePath).absolutePath()));
}

// 打开日志文件位置
void MainWindow::openLogPath()
{
    QString logPath = appDatas.path("Log");
    QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
}

// 跳转到微软商店评分页面
void MainWindow::goToMsStoreRate()
{
    QDesktopServices::openUrl(QUrl("https://apps.microsoft.com/detail/9P7X9B7RKXDB?hl=neutral&gl=CN&ocid=pdpshare"));
}

// 窗口关闭事件处理
// @param event 关闭事件对象
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 如果设置了最小化到托盘，则隐藏窗口而不关闭
    if (appDatas.isMinToTray()) {
        event->ignore();
        this->hide();
    } else {
        event->accept();
    }
}

// 鼠标按下事件处理
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击了调整大小手柄
        if (m_resizeHandle && m_resizeHandle->geometry().contains(event->pos())) {
            m_isResizing = true;
            m_resizeStartPos = event->globalPosition().toPoint();
            this->setCursor(Qt::SizeFDiagCursor);
        }
        // 检查是否在标题栏区域（顶部标签栏）
        else if (event->pos().y() <= 60) { // 假设标题栏高度为60
            m_isDragging = true;
            m_dragStartPos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
            this->setCursor(Qt::ClosedHandCursor);
        }
    }
    QMainWindow::mousePressEvent(event);
}

// 鼠标移动事件处理
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 处理调整大小
    if (m_isResizing) {
        QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        
        // 分别计算新的宽度和高度，确保不小于最小值
        int newWidth = this->width() + delta.x();
        int newHeight = this->height() + delta.y();
        
        // 确保宽度不小于最小值
        newWidth = qMax(newWidth, this->minimumWidth());
        // 确保高度不小于最小值
        newHeight = qMax(newHeight, this->minimumHeight());
        
        // 调整窗口大小
        this->resize(newWidth, newHeight);
        m_resizeStartPos = event->globalPosition().toPoint();
    }
    // 处理拖动
    else if (m_isDragging) {
        this->move(event->globalPosition().toPoint() - m_dragStartPos);
    }
    // 处理鼠标悬停时的光标变化
    else {
        // 检查是否在调整大小手柄上
        if (m_resizeHandle && m_resizeHandle->geometry().contains(event->pos())) {
            this->setCursor(Qt::SizeFDiagCursor);
        }
        // 检查是否在标题栏区域（顶部标签栏）
        else if (event->pos().y() <= 60) {
            this->setCursor(Qt::OpenHandCursor);
        }
        else {
            this->setCursor(Qt::ArrowCursor);
        }
    }
    QMainWindow::mouseMoveEvent(event);
}

// 鼠标释放事件处理
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isResizing) {
            m_isResizing = false;
            this->setCursor(Qt::ArrowCursor);
        }
        if (m_isDragging) {
            m_isDragging = false;
            this->setCursor(Qt::ArrowCursor);
        }
    }
    QMainWindow::mouseReleaseEvent(event);
}

// 窗口大小改变事件处理
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // 更新调整大小手柄的位置
    if (m_resizeHandle) {
        int handleSize = 12;
        m_resizeHandle->move(this->width() - handleSize, this->height() - handleSize);
    }
    QMainWindow::resizeEvent(event);
}

// 事件处理函数，拦截所有事件
bool MainWindow::event(QEvent *event)
{
    // 拦截窗口显示事件，确保调整大小手柄位置正确
    if (event->type() == QEvent::Show) {
        // 更新调整大小手柄的位置
        if (m_resizeHandle) {
            int handleSize = 12;
            m_resizeHandle->move(this->width() - handleSize, this->height() - handleSize);
        }
    }
    
    return QMainWindow::event(event);
}



// 初始化用户界面
void MainWindow::initUI()
{
    this->setMouseTracking(true); // 启用鼠标跟踪，确保鼠标移动时触发mouseMoveEvent

    QWidget* centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    // 顶部标签栏
    QHBoxLayout* topTabLayout = new QHBoxLayout;
    m_dayViewBtn = new QPushButton("日视图");
    m_monthViewBtn = new QPushButton("月视图");
    m_settingsBtn = new QPushButton("设置");
    
    // 自定义最小化和关闭按钮
    m_minimizeBtn = new QPushButton("-");
    m_closeBtn = new QPushButton("×");
    
    // 按钮样式
    QString topBtnStyle = 
        "QPushButton{font-size:15px; font-weight:bold; padding:8px 25px; margin-right:8px; border-radius:8px; border:none; background-color:#FFFFFF; color:#2D8CF0;}"
        "QPushButton:checked{background-color:#2D8CF0; color:#FFFFFF;}"
        "QPushButton:hover{background-color:#ECF5FF; color:#1D7AD9;}"
        "QPushButton:pressed{background-color:#1D7AD9; color:#FFFFFF;}";
    QString settingBtnStyle = 
        "QPushButton{font-size:12px; padding:6px 12px; border-radius:6px; border:none; background-color:#2D8CF0; color:#FFFFFF; margin-left:8px;}"
        "QPushButton:hover{background-color:#1D7AD9;}";
    QString windowBtnStyle = 
        "QPushButton{font-size:16px; font-weight:bold; padding:4px 10px; border-radius:6px; border:none; background-color:#FFFFFF; color:#333333; margin-left:4px;}"
        "QPushButton:hover{background-color:#ECF5FF; color:#1D7AD9;}"
        "QPushButton:pressed{background-color:#1D7AD9; color:#FFFFFF;}";
    
    m_dayViewBtn->setStyleSheet(topBtnStyle);
    m_monthViewBtn->setStyleSheet(topBtnStyle);
    m_settingsBtn->setStyleSheet(settingBtnStyle);
    m_minimizeBtn->setStyleSheet(windowBtnStyle);
    m_closeBtn->setStyleSheet(windowBtnStyle);
    
    // 设置按钮为可检查状态
    m_dayViewBtn->setCheckable(true);
    m_monthViewBtn->setCheckable(true);
    m_dayViewBtn->setChecked(true);

    // 连接按钮信号槽
    connect(m_minimizeBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_closeBtn, &QPushButton::clicked, this, [=]() {
        if (appDatas.isMinToTray()) {
            this->hide();
        } else {
            qApp->quit();
        }
    });

    topTabLayout->addWidget(m_dayViewBtn);
    topTabLayout->addWidget(m_monthViewBtn);
    topTabLayout->addStretch();
    topTabLayout->addWidget(m_settingsBtn);
    topTabLayout->addWidget(m_minimizeBtn);
    topTabLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(topTabLayout);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettingsWindow);

    // 堆叠窗口，用于切换日视图和月视图
    m_mainStackedWidget = new QStackedWidget;
    m_dayView = new DayView(this);
    m_monthView = new MonthView(this);
    m_mainStackedWidget->addWidget(m_dayView);
    m_mainStackedWidget->addWidget(m_monthView);
    mainLayout->addWidget(m_mainStackedWidget);

    // 连接视图切换按钮的信号槽
    // 彻底修复：完全控制按钮状态，禁止自动切换
    connect(m_dayViewBtn, &QPushButton::clicked, this, [=]() {
        // 1. 首先阻止按钮的自动checked状态变化
        // 2. 只有在动画允许时才执行切换
        if (!m_isAnimating) {
            switchToDayView();
        } else {
            // 动画进行时，保持当前正确的状态
            if (m_mainStackedWidget->currentIndex() == 0) {
                // 当前是日视图，保持日视图按钮为checked
                m_dayViewBtn->setChecked(true);
                m_monthViewBtn->setChecked(false);
            } else {
                // 当前是月视图，保持月视图按钮为checked
                m_monthViewBtn->setChecked(true);
                m_dayViewBtn->setChecked(false);
            }
        }
    });
    connect(m_monthViewBtn, &QPushButton::clicked, this, [=]() {
        // 1. 首先阻止按钮的自动checked状态变化
        // 2. 只有在动画允许时才执行切换
        if (!m_isAnimating) {
            switchToMonthView();
        } else {
            // 动画进行时，保持当前正确的状态
            if (m_mainStackedWidget->currentIndex() == 1) {
                // 当前是月视图，保持月视图按钮为checked
                m_monthViewBtn->setChecked(true);
                m_dayViewBtn->setChecked(false);
            } else {
                // 当前是日视图，保持日视图按钮为checked
                m_dayViewBtn->setChecked(true);
                m_monthViewBtn->setChecked(false);
            }
        }
    });
    
    // 创建调整大小手柄
    int handleSize = 12;
    m_resizeHandle = new QWidget(this);
    m_resizeHandle->setFixedSize(handleSize, handleSize);
    m_resizeHandle->setStyleSheet(
        "QWidget{background-color:#2D8CF0; border-radius:4px;}"
        "QWidget:hover{background-color:#1D7AD9; cursor:se-resize;}"
    );
    m_resizeHandle->move(this->width() - handleSize, this->height() - handleSize);
    m_resizeHandle->setMouseTracking(true);
}

QString MainWindow::loadQss(int type)
{
    const QString path = QString(":/style%1.qss").arg(type);
    QFile file(path);
    //打开文件
    file.open(QFile::ReadOnly);
    if(!file.isOpen())
    {
        qDebug()<<"Error in open "+path;
        return "";
    }
    QString style = QString::fromUtf8(file.readAll());
    file.close();
    return style;
}
