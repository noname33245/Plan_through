#include "datastruct.h"
#include "utils/dialoganimator.h"
#include "utils/theme.h"
#include "widgets/dayview.h"
#include "widgets/monthview.h"
#include "widgets/resizehandle.h"
#include "widgets/settingsdialog.h"
#include "mainwindow.h"
#include "appdatas.h"
#include "memreduct.h"
#include <synchapi.h>
#include <QShortcut>
#include <QKeySequence>
#include <QPainter>
#include <QPixmap>
#include <QTime>


MainWindow::MainWindow(bool isAutoStart, QWidget *parent)
    : QMainWindow(parent)
{
    // 去除默认标题栏
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    
    // 设置窗口图标
    this->setWindowIcon(QIcon(":/16.ico"));
    
    initUI();
    applyTheme(appDatas.themeType());
    initSystemTray();
    // 初始化日视图和月视图数据
    findChild<DayView*>("dayView")->loadDateData(QDate::currentDate());
    findChild<DayView*>("dayView")->updateDayViewStats();
    findChild<MonthView*>("monthView")->generateMonthCalendar();



    // 根据用户设置显示默认视图
    if (appDatas.defaultViewType() == 0) {
        switchToMonthView();
    } else {
        switchToDayView();
    }

    // 设置窗口最小大小
    this->setMinimumSize(400, 500);
    
    // 强制设置窗口位置到屏幕中心
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - this->width()) / 2;
    int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);

    // 如果是开机自启，则隐藏窗口到托盘
    if (isAutoStart) {
        // 开机自启时，隐藏窗口到托盘
        Sleep(10);
        this->hide();
    } else {
        // 手动打开时，显示窗口 + 淡入动画
        this->setWindowOpacity(0.0);
        this->showNormal();
        this->raise();
        this->activateWindow();
        QPropertyAnimation *fadeIn = new QPropertyAnimation(this, "windowOpacity");
        fadeIn->setDuration(280);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    }
    
    // 初始化自动内存清理定时器
    m_memoryCleanTimer = new QTimer(this);
    connect(m_memoryCleanTimer, &QTimer::timeout, this, &MainWindow::checkMemoryUsage);
    
    // 从appdatas加载自动内存清理设置
    m_isAutoMemoryCleanEnabled = appDatas.isAutoMemoryCleanEnabled();
    m_memoryCleanInterval = appDatas.memoryCleanInterval();
    m_memoryCleanThreshold = appDatas.memoryCleanThreshold();
    
    // 根据设置启动或停止定时器
    if (m_isAutoMemoryCleanEnabled) {
        m_memoryCleanTimer->start(m_memoryCleanInterval * 60 * 1000);
    } else {
        m_memoryCleanTimer->stop();
    }

    // 今日完成度变化时刷新托盘图标
    connect(m_dayView, &DayView::statsChanged, this, &MainWindow::updateTrayIcon);

    // 每日提醒：20 秒轮询一次，靠日期去重保证一天只提醒一次
    m_reminderTimer = new QTimer(this);
    connect(m_reminderTimer, &QTimer::timeout, this, &MainWindow::checkReminder);
    m_reminderTimer->start(20 * 1000);
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
// @param themeType 主题类型：0 纯净白（浅色），1 灰黑（深色），2 跟随系统
void MainWindow::applyTheme(int themeType)
{
    appDatas.setTheme(themeType);
    // 只切换配色方案，样式表与调色板统一由 Theme 下发到 QApplication，
    // 这样 parent 为空的对话框、弹出菜单也会一起变色
    Theme::setTheme(themeType);
    Theme::applyToApplication();
    findChild<DayView*>("dayView")->updateDayViewStats();
}

// 初始化系统托盘
void MainWindow::initSystemTray()
{
    m_systemTrayIcon = new QSystemTrayIcon(this);
    m_systemTrayIcon->setIcon(QIcon(":/16.ico"));
    m_systemTrayIcon->setToolTip("学习计划打卡");

    // 创建托盘菜单（外观由全局 QMenu 样式统一控制，不再单独写死颜色）
    m_trayMenu = new QMenu(this);

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

    updateTrayIcon();
}

// 把今日完成度画进托盘图标：外圈进度环 + 中心程序图标
void MainWindow::updateTrayIcon()
{
    if (!m_systemTrayIcon) {
        return;
    }

    const Theme::Colors &tc = Theme::colors();
    const QDate today = QDate::currentDate();
    const int hours = appDatas.value(today).studyHours;
    const int target = qMax(1, appDatas.targetHour());
    const double ratio = qBound(0.0, double(hours) / double(target), 1.0);

    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF ring(4.0, 4.0, 56.0, 56.0);

        QPen track(QColor(tc.borderSoft));
        track.setWidth(7);
        track.setCapStyle(Qt::RoundCap);
        p.setPen(track);
        p.drawEllipse(ring);

        if (ratio > 0.0) {
            QPen arc(QColor(ratio >= 1.0 ? tc.success : tc.accent));
            arc.setWidth(7);
            arc.setCapStyle(Qt::RoundCap);
            p.setPen(arc);
            p.drawArc(ring, 90 * 16, -int(ratio * 360 * 16));
        }

        const QPixmap icon(":/16.ico");
        if (!icon.isNull()) {
            p.drawPixmap(QRect(19, 19, 26, 26), icon);
        }
    }
    m_systemTrayIcon->setIcon(QIcon(pm));
    m_systemTrayIcon->setToolTip(
        QString("学习计划打卡\n今日 %1 / %2 小时").arg(hours).arg(target));
    m_lastTrayDate = today;
}

// 到点检查：今天还没达标就发一条托盘通知，一天只提醒一次
void MainWindow::checkReminder()
{
    // 跨天后托盘进度需要归零
    const QDate today = QDate::currentDate();
    if (m_lastTrayDate.isValid() && m_lastTrayDate != today) {
        updateTrayIcon();
    }

    if (!appDatas.isReminderEnabled() || !m_systemTrayIcon) {
        return;
    }

    const QTime target = QTime::fromString(appDatas.reminderTime(), "HH:mm");
    if (!target.isValid()) {
        return;
    }

    if (m_lastReminderDate == today) {
        return;
    }

    // 到点后 10 分钟内都算有效，避免定时器刚好错过那一分钟
    const int diff = target.secsTo(QTime::currentTime());
    if (diff < 0 || diff > 600) {
        return;
    }

    const int hours = appDatas.value(today).studyHours;
    const int goal = appDatas.targetHour();
    if (hours >= goal) {
        return;
    }

    m_lastReminderDate = today;
    m_systemTrayIcon->showMessage(
        "今日还没达标",
        QString("已学习 %1 小时 / 目标 %2 小时，还差 %3 小时")
            .arg(hours).arg(goal).arg(goal - hours),
        QSystemTrayIcon::Information, 10000);
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
    this->setWindowOpacity(0.0);
    this->show();
    this->activateWindow();
    this->raise();
    QPropertyAnimation *fadeIn = new QPropertyAnimation(this, "windowOpacity");
    fadeIn->setDuration(220);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
}

// 视图切换（日/月），带淡出 + 滑入动画
// @param targetIndex 0 = 日视图，1 = 月视图
void MainWindow::switchToView(int targetIndex)
{
    if (m_isAnimating) {
        return;
    }

    const bool toDay = (targetIndex == 0);

    // 已经在目标视图：只对齐按钮状态并刷新数据
    if (m_mainStackedWidget->currentIndex() == targetIndex) {
        m_dayViewBtn->setChecked(toDay);
        m_monthViewBtn->setChecked(!toDay);
        if (toDay) {
            findChild<DayView*>("dayView")->updateDayViewStats();
        } else {
            findChild<MonthView*>("monthView")->generateMonthCalendar();
        }
        return;
    }

    QWidget *currentWidget = m_mainStackedWidget->currentWidget();
    if (!currentWidget) {
        return;
    }

    m_isAnimating = true;

    // 日视图在左、月视图在右：新页面从对应一侧滑入，旧页面往反方向滑出
    const int enterDir = toDay ? -1 : 1;
    const QPoint originalPos = currentWidget->pos();

    // 退出动画：只做前半段，在速度最快时切换页面
    auto *exitGroup = new QParallelAnimationGroup(this);

    auto *fadeOut = new QPropertyAnimation(currentWidget, "windowOpacity");
    fadeOut->setDuration(250);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.5);

    auto *slideOut = new QPropertyAnimation(currentWidget, "pos");
    slideOut->setDuration(250);
    slideOut->setEasingCurve(QEasingCurve::InCubic);
    slideOut->setStartValue(originalPos);
    slideOut->setEndValue(originalPos - QPoint(enterDir * 30, 0));

    exitGroup->addAnimation(fadeOut);
    exitGroup->addAnimation(slideOut);

    connect(exitGroup, &QParallelAnimationGroup::finished, this, [=]() {
        m_mainStackedWidget->setCurrentIndex(targetIndex);

        // 还原旧页面
        currentWidget->move(originalPos);
        currentWidget->setWindowOpacity(1.0);

        QWidget *newWidget = m_mainStackedWidget->currentWidget();
        if (!newWidget) {
            m_isAnimating = false;
            return;
        }

        const QPoint newOriginalPos = newWidget->pos();
        const QPoint newStartPos = newOriginalPos + QPoint(enterDir * 30, 0);

        // 进入动画：后半段
        newWidget->setWindowOpacity(0.5);
        newWidget->move(newStartPos);

        auto *enterGroup = new QParallelAnimationGroup(this);

        auto *fadeIn = new QPropertyAnimation(newWidget, "windowOpacity");
        fadeIn->setDuration(250);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);
        fadeIn->setStartValue(0.5);
        fadeIn->setEndValue(1.0);

        auto *slideIn = new QPropertyAnimation(newWidget, "pos");
        slideIn->setDuration(250);
        slideIn->setEasingCurve(QEasingCurve::OutCubic);
        slideIn->setStartValue(newStartPos);
        slideIn->setEndValue(newOriginalPos);

        enterGroup->addAnimation(fadeIn);
        enterGroup->addAnimation(slideIn);

        connect(enterGroup, &QParallelAnimationGroup::finished, this, [=]() {
            newWidget->move(newOriginalPos);
            newWidget->setWindowOpacity(1.0);

            // 最终保障：按钮状态与当前页面保持一致
            m_dayViewBtn->setChecked(toDay);
            m_monthViewBtn->setChecked(!toDay);
            if (toDay) {
                findChild<DayView*>("dayView")->updateDayViewStats();
            } else {
                findChild<MonthView*>("monthView")->generateMonthCalendar();
            }

            m_isAnimating = false;
        });

        enterGroup->start(QAbstractAnimation::DeleteWhenStopped);
    });

    exitGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

// 切换到日视图
void MainWindow::switchToDayView()
{
    switchToView(0);
}

// 切换到月视图
void MainWindow::switchToMonthView()
{
    switchToView(1);
}

// 显示设置窗口
void MainWindow::showSettingsWindow()
{
    SettingsDialog dlg(this);

    // 倒计时显示需要读取主窗口的清理定时器
    dlg.setRemainingSecondsProvider([this]() -> int {
        if (!m_memoryCleanTimer || !m_memoryCleanTimer->isActive()) {
            return -1;
        }
        return m_memoryCleanTimer->remainingTime() / 1000;
    });

    connect(&dlg, &SettingsDialog::autoMemoryCleanSettingsChanged,
            this, &MainWindow::onAutoMemoryCleanSettingsChanged);
    connect(&dlg, &SettingsDialog::themeChanged,
            this, &MainWindow::onThemeChanged);
    connect(&dlg, &SettingsDialog::dataRestored,
            this, &MainWindow::switchToDayView);
    connect(&dlg, &SettingsDialog::dayRangeChanged, this, [this]() {
        if (auto *dayView = findChild<DayView*>("dayView")) {
            dayView->rebuildTimeAxis();
        }
    });

    DialogAnimator::applyFadeSlideIn(&dlg);
    dlg.exec();
    appDatas.saveSettings();
}

// 自动内存清理设置变化后，按最新配置重启定时器
void MainWindow::onAutoMemoryCleanSettingsChanged()
{
    m_isAutoMemoryCleanEnabled = appDatas.isAutoMemoryCleanEnabled();
    m_memoryCleanInterval = appDatas.memoryCleanInterval();
    m_memoryCleanThreshold = appDatas.memoryCleanThreshold();

    if (m_isAutoMemoryCleanEnabled) {
        m_memoryCleanTimer->start(m_memoryCleanInterval * 60 * 1000);
    } else {
        m_memoryCleanTimer->stop();
    }
}

// 主题设置改变事件处理
// @param index 主题索引
void MainWindow::onThemeChanged(int index)
{
    if (index == appDatas.themeType()) {
        qDebug() << "主题未变化，跳过";
        return;
    }
    applyTheme(index);
    appDatas.saveSettings();
    updateTrayIcon();
}

// 检查内存使用情况并在需要时执行清理
void MainWindow::checkMemoryUsage()
{
    if (m_isAutoMemoryCleanEnabled) {
        // 调用内存清理检测函数
        CheckAndCleanMemory(m_memoryCleanThreshold);
    }
}

// 显示内存清理失败提示
void MainWindow::showMemoryCleanError()
{
    // 查找设置窗口
    QDialog *settingsDlg = nullptr;
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    foreach (QDialog *dialog, dialogs) {
        if (dialog->windowTitle() == "软件设置") {
            settingsDlg = dialog;
            break;
        }
    }
    
    if (settingsDlg) {
        // 查找内存清理按钮
        QPushButton *memoryCleanBtn = nullptr;
        QList<QPushButton*> buttons = settingsDlg->findChildren<QPushButton*>();
        foreach (QPushButton *button, buttons) {
            if (button->text() == "一键内存清理") {
                memoryCleanBtn = button;
                break;
            }
        }
        
        if (memoryCleanBtn) {
            // 查找按钮所在的布局
            QLayout *layout = memoryCleanBtn->parentWidget()->layout();
            if (layout) {
                // 移除已有的失败提示标签
                QList<QLabel*> existingLabels = memoryCleanBtn->parentWidget()->findChildren<QLabel*>();
                foreach (QLabel *label, existingLabels) {
                    if (label->objectName() == "memoryCleanErrorLabel") {
                        layout->removeWidget(label);
                        label->deleteLater();
                        break;
                    }
                }
                
                // 创建失败提示标签
                QLabel *errorLabel = new QLabel("失败，请使用管理员运行");
                errorLabel->setObjectName("memoryCleanErrorLabel");
                
                // 添加到布局中，放在按钮旁边
                layout->addWidget(errorLabel);
                
                // 5秒后隐藏提示
                QTimer::singleShot(5000, errorLabel, [=]() {
                    layout->removeWidget(errorLabel);
                    errorLabel->deleteLater();
                });
            }
        }
    }
}

// 窗口关闭事件处理
// @param event 关闭事件对象
void MainWindow::closeEvent(QCloseEvent *event)
{
    // 如果设置了最小化到托盘，则隐藏窗口而不关闭
    if (appDatas.isMinToTray()) {
        event->ignore();
        // 淡出后再隐藏到托盘
        QPropertyAnimation *fadeOut = new QPropertyAnimation(this, "windowOpacity");
        fadeOut->setDuration(180);
        fadeOut->setStartValue(this->windowOpacity());
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        connect(fadeOut, &QPropertyAnimation::finished, this, [=]() {
            this->hide();
            this->setWindowOpacity(1.0);
            fadeOut->deleteLater();
        });
        fadeOut->start();
    } else {
        event->accept();
    }
}

// 鼠标按下事件处理
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 只在标题栏区域启动拖窗；缩放手柄由 ResizeHandle 自己处理
    if (event->button() == Qt::LeftButton && event->pos().y() <= 60) {
        m_isDragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - this->frameGeometry().topLeft();
        this->setCursor(Qt::ClosedHandCursor);
    }
    QMainWindow::mousePressEvent(event);
}

// 鼠标移动事件处理
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 兜底：如果左键已经松开却没收到 release（例如拖出了窗口），
    // 立刻把拖动状态和光标清掉，避免光标卡在“抓手/缩放”样式
    if (event->buttons() == Qt::NoButton && m_isDragging) {
        m_isDragging = false;
        this->unsetCursor();
    }

    if (m_isDragging) {
        this->move(event->globalPosition().toPoint() - m_dragStartPos);
    } else {
        // 悬停光标只跟随标题栏区域，其余区域交回系统默认
        if (event->pos().y() <= 60) {
            this->setCursor(Qt::OpenHandCursor);
        } else {
            this->unsetCursor();
        }
    }
    QMainWindow::mouseMoveEvent(event);
}

// 鼠标释放事件处理
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        // unsetCursor 而不是 setCursor(Arrow)：清掉自定义光标，
        // 让子控件（含缩放手柄）恢复各自的默认光标
        this->unsetCursor();
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

    // 主题切换（含跟随系统时系统明暗变化）后，
    // 内嵌 <font color> 的富文本不会自动跟着变色，需要重新生成一次
    if (event->type() == QEvent::ThemeChange ||
        event->type() == QEvent::ApplicationPaletteChange) {
        if (DayView *dayView = findChild<DayView*>("dayView")) {
            QTimer::singleShot(0, dayView, [dayView]() { dayView->updateDayViewStats(); });
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
    
    // 按钮样式：全部交给统一主题表，这里只标记用途
    m_dayViewBtn->setObjectName("dayViewBtn");
    m_monthViewBtn->setObjectName("monthViewBtn");
    m_settingsBtn->setObjectName("settingsBtn");
    m_minimizeBtn->setObjectName("minimizeBtn");
    m_closeBtn->setObjectName("closeBtn");
    
    // 设置按钮为可检查状态
    m_dayViewBtn->setCheckable(true);
    m_monthViewBtn->setCheckable(true);
    m_dayViewBtn->setChecked(true);

    // 连接按钮信号槽
    connect(m_minimizeBtn, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(m_closeBtn, &QPushButton::clicked, this, [=]() {
        if (appDatas.isMinToTray()) {
            // 淡出后隐藏到托盘
            QPropertyAnimation *fadeOut = new QPropertyAnimation(this, "windowOpacity");
            fadeOut->setDuration(180);
            fadeOut->setStartValue(this->windowOpacity());
            fadeOut->setEndValue(0.0);
            fadeOut->setEasingCurve(QEasingCurve::InCubic);
            connect(fadeOut, &QPropertyAnimation::finished, this, [=]() {
                this->hide();
                this->setWindowOpacity(1.0);
                fadeOut->deleteLater();
            });
            fadeOut->start();
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
    
    // 创建调整大小手柄（缩放逻辑全部由 ResizeHandle 自己处理）
    int handleSize = 12;
    m_resizeHandle = new ResizeHandle(this);
    m_resizeHandle->move(this->width() - handleSize, this->height() - handleSize);

    // 快捷键
    new QShortcut(QKeySequence("Ctrl+1"), this, [this]() { switchToDayView(); });
    new QShortcut(QKeySequence("Ctrl+2"), this, [this]() { switchToMonthView(); });
    new QShortcut(QKeySequence("Ctrl+,"), this, [this]() { showSettingsWindow(); });
}
