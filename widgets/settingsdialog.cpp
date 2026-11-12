#include "settingsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QSpinBox>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "../appdatas.h"
#include "../memreduct.h"

namespace {

// 秒数格式化成 HH:MM:SS
QString formatHMS(int totalSeconds)
{
    if (totalSeconds < 0) {
        totalSeconds = 0;
    }
    return QString("%1:%2:%3")
        .arg(totalSeconds / 3600, 2, 10, QChar('0'))
        .arg((totalSeconds % 3600) / 60, 2, 10, QChar('0'))
        .arg(totalSeconds % 60, 2, 10, QChar('0'));
}

const int kIntervals[] = { 1, 2, 5, 10, 15, 20, 25, 30 };
const int kThresholds[] = { 30, 40, 50, 60, 70, 80, 90 };

// QSpinBox 没有 setFormat，想显示成 "09" 只能自己重写取值转文本
class TwoDigitSpinBox : public QSpinBox
{
public:
    using QSpinBox::QSpinBox;

protected:
    QString textFromValue(int value) const override
    {
        return QString("%1").arg(value, 2, 10, QChar('0'));
    }
};

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("软件设置");
    setFixedSize(350, 610);
    setModal(true);

    // 禁用所有可能的窗口动画效果
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_DontShowOnScreen, false);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setWindowOpacity(1.0);

    // 颜色全部由统一主题表提供
    setObjectName("settingsDlg");

    buildUi();
}

void SettingsDialog::setRemainingSecondsProvider(std::function<int()> provider)
{
    m_remainingProvider = std::move(provider);
    refreshCountdown();
}

void SettingsDialog::buildUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    // ---- 开机自启 ----
    auto *autoStartLayout = new QHBoxLayout;
    auto *autoStartCb = new QCheckBox("开机自动启动");
    {
        QSignalBlocker blocker(autoStartCb);
        autoStartCb->setChecked(appDatas.isAutoStartup());
    }
    connect(autoStartCb, &QCheckBox::checkStateChanged, this, [](Qt::CheckState state) {
        const bool enabled = (state == Qt::Checked);
        if (enabled != appDatas.isAutoStartup()) {
            appDatas.setAutoStartup(enabled);
        }
    });
    autoStartLayout->addWidget(autoStartCb);
    autoStartLayout->addStretch();

    // ---- 关闭后最小化到托盘 ----
    auto *minTrayLayout = new QHBoxLayout;
    auto *minTrayCb = new QCheckBox("关闭窗口后最小化到托盘");
    {
        QSignalBlocker blocker(minTrayCb);
        minTrayCb->setChecked(appDatas.isMinToTray());
    }
    connect(minTrayCb, &QCheckBox::checkStateChanged, this, [](Qt::CheckState state) {
        const bool enabled = (state == Qt::Checked);
        if (enabled != appDatas.isMinToTray()) {
            appDatas.setMinToTray(enabled);
        }
    });
    minTrayLayout->addWidget(minTrayCb);
    minTrayLayout->addStretch();

    // ---- 主题 ----
    auto *themeLayout = new QHBoxLayout;
    auto *themeLab = new QLabel("软件主题：");
    auto *themeCbx = new QComboBox;
    themeCbx->addItems({ "纯净白", "灰黑", "跟随系统" });
    {
        QSignalBlocker blocker(themeCbx);
        themeCbx->setCurrentIndex(appDatas.themeType());
    }
    connect(themeCbx, &QComboBox::currentIndexChanged, this, [this](int index) {
        emit themeChanged(index);
    });
    themeLayout->addWidget(themeLab);
    themeLayout->addWidget(themeCbx);
    themeLayout->addStretch();

    // ---- 默认视图 ----
    auto *defaultViewLayout = new QHBoxLayout;
    auto *defaultViewLab = new QLabel("默认视图：");
    auto *defaultViewCbx = new QComboBox;
    defaultViewCbx->addItems({ "月视图", "日视图" });
    defaultViewCbx->setCurrentIndex(appDatas.defaultViewType());
    connect(defaultViewCbx, &QComboBox::currentIndexChanged, [](int index) {
        appDatas.setDefaultViewType(index);
    });
    defaultViewLayout->addWidget(defaultViewLab);
    defaultViewLayout->addWidget(defaultViewCbx);
    defaultViewLayout->addStretch();

    // ---- 时间轴范围 ----
    auto *rangeLayout = new QHBoxLayout;
    auto *rangeLab = new QLabel("时间轴范围：");
    auto *startCbx = new QComboBox;
    auto *endCbx = new QComboBox;
    for (int h = 0; h <= 23; ++h) {
        const QString text = QString("%1:00").arg(h, 2, 10, QChar('0'));
        startCbx->addItem(text);
        endCbx->addItem(text);
    }
    startCbx->setMinimumWidth(78);
    endCbx->setMinimumWidth(78);
    {
        QSignalBlocker b1(startCbx), b2(endCbx);
        startCbx->setCurrentIndex(appDatas.dayStartHour());
        endCbx->setCurrentIndex(appDatas.dayEndHour());
    }
    // 保证起始 < 结束，避免出现空范围
    auto applyRange = [this, startCbx, endCbx]() {
        int s = startCbx->currentIndex();
        int e = endCbx->currentIndex();
        if (e <= s) {
            e = qMin(23, s + 1);
            if (e <= s) {
                s = qMax(0, e - 1);
            }
            QSignalBlocker b1(startCbx), b2(endCbx);
            startCbx->setCurrentIndex(s);
            endCbx->setCurrentIndex(e);
        }
        appDatas.setDayStartHour(s);
        appDatas.setDayEndHour(e);
        emit dayRangeChanged();
    };
    connect(startCbx, &QComboBox::currentIndexChanged, this, [applyRange](int) { applyRange(); });
    connect(endCbx, &QComboBox::currentIndexChanged, this, [applyRange](int) { applyRange(); });

    rangeLayout->addWidget(rangeLab);
    rangeLayout->addWidget(startCbx);
    rangeLayout->addWidget(new QLabel("—"));
    rangeLayout->addWidget(endCbx);
    rangeLayout->addStretch();

    // ---- 每日提醒 ----
    auto *reminderLayout = new QHBoxLayout;
    auto *reminderCb = new QCheckBox("每日提醒");

    // 小时、分钟各一个框，上下箭头分别调各自的值
    auto *hourSpin = new TwoDigitSpinBox;
    hourSpin->setRange(0, 23);
    hourSpin->setFixedWidth(58);

    auto *minuteSpin = new TwoDigitSpinBox;
    minuteSpin->setRange(0, 59);
    minuteSpin->setFixedWidth(58);

    {
        const QTime reminder = QTime::fromString(appDatas.reminderTime(), "HH:mm");
        QSignalBlocker b1(hourSpin), b2(minuteSpin);
        hourSpin->setValue(reminder.isValid() ? reminder.hour() : 21);
        minuteSpin->setValue(reminder.isValid() ? reminder.minute() : 0);
    }

    auto saveReminderTime = [hourSpin, minuteSpin]() {
        appDatas.setReminderTime(QString("%1:%2")
            .arg(hourSpin->value(), 2, 10, QChar('0'))
            .arg(minuteSpin->value(), 2, 10, QChar('0')));
    };
    connect(hourSpin, &QSpinBox::valueChanged, this, [saveReminderTime](int) { saveReminderTime(); });
    connect(minuteSpin, &QSpinBox::valueChanged, this, [saveReminderTime](int) { saveReminderTime(); });

    auto setReminderEnabled = [hourSpin, minuteSpin](bool enabled) {
        hourSpin->setEnabled(enabled);
        minuteSpin->setEnabled(enabled);
    };
    {
        QSignalBlocker blocker(reminderCb);
        reminderCb->setChecked(appDatas.isReminderEnabled());
    }
    setReminderEnabled(appDatas.isReminderEnabled());

    connect(reminderCb, &QCheckBox::checkStateChanged, this,
            [setReminderEnabled](Qt::CheckState state) {
        const bool enabled = (state == Qt::Checked);
        appDatas.setReminderEnabled(enabled);
        setReminderEnabled(enabled);
    });

    reminderLayout->addWidget(reminderCb);
    reminderLayout->addWidget(hourSpin);
    reminderLayout->addWidget(new QLabel(":"));
    reminderLayout->addWidget(minuteSpin);
    reminderLayout->addStretch();

    // ---- 打开存档 / 日志位置（并排一行） ----
    auto *pathLayout = new QHBoxLayout;
    pathLayout->setSpacing(8);

    auto *pathBtn = new QPushButton("打开存档文件位置");
    pathBtn->setObjectName("primaryBtn");

    auto *logBtn = new QPushButton("打开日志文件位置");
    logBtn->setObjectName("warningBtn");

    pathLayout->addWidget(pathBtn);
    pathLayout->addWidget(logBtn);
    pathLayout->addStretch();

    connect(pathBtn, &QPushButton::clicked, this, &SettingsDialog::openSavePath);
    connect(logBtn, &QPushButton::clicked, this, &SettingsDialog::openLogPath);

    // ---- 商店评分 / GitHub ----
    auto *rateContainer = new QWidget;
    rateContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    rateContainer->setFixedWidth(320);

    auto *rateLayout = new QVBoxLayout(rateContainer);
    rateLayout->setContentsMargins(0, 0, 0, 0);
    rateLayout->setSpacing(4);

    auto *buttonsLayout = new QHBoxLayout;
    buttonsLayout->setSpacing(8);
    auto *rateBtn = new QPushButton("微软商店好评支持一下吧 ❤️");
    rateBtn->setObjectName("successBtn");
    rateBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonsLayout->addWidget(rateBtn);

    auto *githubBtn = new QPushButton("GitHub 下载页面");
    githubBtn->setObjectName("cyanBtn");
    githubBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonsLayout->addWidget(githubBtn);

    auto *downloadNoteLabel = new QLabel("如果微软商城下载的版本无法使用内存清理功能，请先尝试使用管理员打开，如果失败则从github下载软件包");
    downloadNoteLabel->setObjectName("downloadNoteLabel");
    downloadNoteLabel->setWordWrap(true);
    downloadNoteLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    downloadNoteLabel->setMinimumWidth(320);
    downloadNoteLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    rateLayout->addLayout(buttonsLayout);
    rateLayout->addWidget(downloadNoteLabel);

    connect(rateBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://apps.microsoft.com/detail/9P7X9B7RKXDB?hl=neutral&gl=CN&ocid=pdpshare"));
    });
    connect(githubBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl("https://github.com/noname33245/Plan_through/releases"));
    });

    // ---- 自动内存清理 ----
    auto *autoMemoryCleanGroup = new QGroupBox("自动内存清理");
    autoMemoryCleanGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    autoMemoryCleanGroup->setMinimumHeight(165);

    auto *autoMemoryCleanLayout = new QVBoxLayout(autoMemoryCleanGroup);
    autoMemoryCleanLayout->setContentsMargins(12, 14, 12, 10);
    autoMemoryCleanLayout->setSpacing(6);

    auto *enableAutoCleanLayout = new QHBoxLayout;
    enableAutoCleanLayout->setSpacing(6);
    auto *enableAutoCleanCb = new QCheckBox("启用自动内存清理");
    enableAutoCleanCb->setChecked(appDatas.isAutoMemoryCleanEnabled());
    enableAutoCleanCb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    enableAutoCleanLayout->addWidget(enableAutoCleanCb);
    enableAutoCleanLayout->addStretch();

    auto *intervalLayout = new QHBoxLayout;
    intervalLayout->setSpacing(6);
    auto *intervalLab = new QLabel("检测间隔：");
    intervalLab->setMinimumWidth(60);
    auto *intervalCbx = new QComboBox;
    intervalCbx->addItems({ "1分钟", "2分钟", "5分钟", "10分钟", "15分钟", "20分钟", "25分钟", "30分钟" });
    intervalCbx->setMinimumWidth(120);
    {
        const int interval = appDatas.memoryCleanInterval();
        int index = 1;
        for (int i = 0; i < 8; ++i) {
            if (kIntervals[i] == interval) {
                index = i;
                break;
            }
        }
        QSignalBlocker blocker(intervalCbx);
        intervalCbx->setCurrentIndex(index);
    }
    intervalLayout->addWidget(intervalLab);
    intervalLayout->addWidget(intervalCbx);
    intervalLayout->addStretch();

    auto *thresholdLayout = new QHBoxLayout;
    thresholdLayout->setSpacing(6);
    auto *thresholdLab = new QLabel("内存阈值：");
    thresholdLab->setMinimumWidth(60);
    auto *thresholdCbx = new QComboBox;
    thresholdCbx->addItems({ "30%", "40%", "50%", "60%", "70%", "80%", "90%" });
    thresholdCbx->setMinimumWidth(120);
    {
        const int threshold = appDatas.memoryCleanThreshold();
        int index = 3;
        for (int i = 0; i < 7; ++i) {
            if (kThresholds[i] == threshold) {
                index = i;
                break;
            }
        }
        QSignalBlocker blocker(thresholdCbx);
        thresholdCbx->setCurrentIndex(index);
    }
    thresholdLayout->addWidget(thresholdLab);
    thresholdLayout->addWidget(thresholdCbx);
    thresholdLayout->addStretch();

    auto *nextScanLayout = new QHBoxLayout;
    nextScanLayout->setSpacing(6);
    auto *nextScanLab = new QLabel("下次扫描：");
    nextScanLab->setMinimumWidth(60);
    m_nextScanTimeLabel = new QLabel("--:--:--");
    m_nextScanTimeLabel->setMinimumWidth(60);
    m_nextScanTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_memoryUsageLabel = new QLabel("当前内存：--%");
    m_memoryUsageLabel->setMinimumWidth(90);
    nextScanLayout->addWidget(nextScanLab);
    nextScanLayout->addWidget(m_nextScanTimeLabel);
    nextScanLayout->addStretch();
    nextScanLayout->addWidget(m_memoryUsageLabel);

    autoMemoryCleanLayout->addLayout(enableAutoCleanLayout);
    autoMemoryCleanLayout->addLayout(intervalLayout);
    autoMemoryCleanLayout->addLayout(thresholdLayout);
    autoMemoryCleanLayout->addLayout(nextScanLayout);

    connect(enableAutoCleanCb, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        appDatas.setAutoMemoryCleanEnabled(state == Qt::Checked);
        emit autoMemoryCleanSettingsChanged();
        refreshCountdown();
    });

    connect(intervalCbx, &QComboBox::currentIndexChanged, this, [this](int index) {
        const int safeIndex = qBound(0, index, 7);
        appDatas.setMemoryCleanInterval(kIntervals[safeIndex]);
        emit autoMemoryCleanSettingsChanged();
        refreshCountdown();
    });

    connect(thresholdCbx, &QComboBox::currentIndexChanged, [](int index) {
        const int safeIndex = qBound(0, index, 6);
        appDatas.setMemoryCleanThreshold(kThresholds[safeIndex]);
    });

    // 每秒刷新倒计时；内存占用每两秒刷一次，避免频繁触发布局
    m_countdownTimer = new QTimer(this);
    {
        int tick = 0;
        connect(m_countdownTimer, &QTimer::timeout, this, [this, tick]() mutable {
            setUpdatesEnabled(false);
            refreshCountdown();
            if (++tick % 2 == 0 && m_memoryUsageLabel) {
                m_memoryUsageLabel->setText(QString("当前内存：%1%").arg(GetCurrentMemoryUsage()));
            }
            setUpdatesEnabled(true);
        });
    }
    m_countdownTimer->start(1000);

    m_memoryUsageLabel->setText(QString("当前内存：%1%").arg(GetCurrentMemoryUsage()));
    refreshCountdown();

    // ---- 备份 / 恢复 ----
    auto *backupLayout = new QHBoxLayout;
    auto *createBackupBtn = new QPushButton("创建数据备份");
    createBackupBtn->setObjectName("cyanBtn");
    auto *restoreBackupBtn = new QPushButton("从备份恢复");
    restoreBackupBtn->setObjectName("violetBtn");
    backupLayout->addWidget(createBackupBtn);
    backupLayout->addWidget(restoreBackupBtn);
    backupLayout->addStretch();

    connect(createBackupBtn, &QPushButton::clicked, [this]() {
        const QString fileName = "study_data_backup_"
            + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json";
        const QString path = QFileDialog::getSaveFileName(
            this, "保存数据备份",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + fileName,
            "JSON Files (*.json)");
        if (path.isEmpty()) {
            return;
        }
        if (appDatas.createBackup(path)) {
            QMessageBox::information(this, "成功", "数据备份创建成功！\n" + path);
        } else {
            QMessageBox::critical(this, "失败", "数据备份创建失败！");
        }
    });

    connect(restoreBackupBtn, &QPushButton::clicked, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, "选择数据备份文件",
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            "JSON Files (*.json)");
        if (path.isEmpty()) {
            return;
        }
        const auto reply = QMessageBox::warning(
            this, "警告", "从备份恢复将覆盖当前所有数据，是否继续？",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
        if (appDatas.restoreFromBackup(path)) {
            QMessageBox::information(this, "成功", "数据恢复成功！");
            emit dataRestored();
        } else {
            QMessageBox::critical(this, "失败", "数据恢复失败！");
        }
    });

    // ---- 一键清理 ----
    auto *memoryCleanLayout = new QHBoxLayout;
    auto *memoryCleanBtn = new QPushButton("一键内存清理");
    memoryCleanBtn->setObjectName("dangerBtn");
    memoryCleanLayout->addWidget(memoryCleanBtn);
    memoryCleanLayout->addStretch();
    connect(memoryCleanBtn, &QPushButton::clicked, this, [this]() {
        PerformMemoryClean(parentWidget());
        emit autoMemoryCleanSettingsChanged();
        refreshCountdown();
    });

    // ---- 组装 ----
    mainLayout->addLayout(autoStartLayout);
    mainLayout->addLayout(minTrayLayout);
    mainLayout->addLayout(themeLayout);
    mainLayout->addLayout(defaultViewLayout);
    mainLayout->addLayout(rangeLayout);
    mainLayout->addLayout(reminderLayout);
    mainLayout->addLayout(pathLayout);
    mainLayout->addLayout(backupLayout);
    mainLayout->addLayout(memoryCleanLayout);
    mainLayout->addWidget(autoMemoryCleanGroup, 0, Qt::AlignTop);
    mainLayout->addWidget(rateContainer, 0, Qt::AlignTop);
    mainLayout->addStretch();
}

void SettingsDialog::refreshCountdown()
{
    if (!m_nextScanTimeLabel) {
        return;
    }
    if (!appDatas.isAutoMemoryCleanEnabled()) {
        m_nextScanTimeLabel->setText("已禁用");
        return;
    }
    const int remaining = m_remainingProvider ? m_remainingProvider() : -1;
    m_nextScanTimeLabel->setText(remaining >= 0 ? formatHMS(remaining) : "已禁用");
}

void SettingsDialog::openSavePath()
{
    const QString savePath = appDatas.path("Save");
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(savePath).absolutePath()));
}

void SettingsDialog::openLogPath()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(appDatas.path("Log")));
}
