#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <functional>

class QLabel;
class QTimer;

// 软件设置窗口
// ---------------------------------------------------------------------------
// 从 MainWindow 里剥出来的独立对话框：只负责读写 appDatas 和界面交互，
// 与主窗口的耦合通过三个信号 + 一个倒计时回调完成。
// ---------------------------------------------------------------------------
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // 注入自动清理定时器的剩余秒数（-1 表示定时器没在跑）
    void setRemainingSecondsProvider(std::function<int()> provider);

signals:
    // 自动内存清理的开关/间隔/阈值发生变化，主窗口据此重启定时器
    void autoMemoryCleanSettingsChanged();
    // 主题选择变化（0 纯净白 / 1 灰黑 / 2 跟随系统）
    void themeChanged(int index);
    // 从备份恢复完成，主窗口需要切回日视图刷新
    void dataRestored();
    // 时间轴起止小时变化，主窗口需要重建时间轴
    void dayRangeChanged();

private:
    void buildUi();
    void refreshCountdown();
    void openSavePath();
    void openLogPath();

    std::function<int()> m_remainingProvider;
    QLabel *m_nextScanTimeLabel = nullptr;
    QLabel *m_memoryUsageLabel = nullptr;
    QTimer *m_countdownTimer = nullptr;
};

#endif // SETTINGSDIALOG_H
