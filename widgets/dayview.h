#ifndef DAYVIEW_H
#define DAYVIEW_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QCalendarWidget>
#include <QMessageBox>
#include <QPropertyAnimation>
#include "timeaxis.h"
#include "roundprogressbar.h"

class DayView : public QWidget
{
    Q_OBJECT
public:
    explicit DayView(QWidget *parent = nullptr);
    ~DayView();

    // 当前查看的日期（可以是历史日期，不再依赖全局状态）
    QDate date() const { return m_date; }
    // 切换查看日期并刷新界面
    void setDate(const QDate &date);
    // 时间轴起止小时变化后重建时间轴
    void rebuildTimeAxis();

    void updateDayViewStats();
    void loadDateData(const QDate& date);

signals:
    // 统计数据变化（用于刷新托盘进度等外部显示）
    void statsChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void layoutProgressHeader();

    QDate m_date = QDate::currentDate();
    QWidget *m_progressHeader = nullptr;
    QLabel *m_selectedDateLabel = nullptr;
    QLabel *m_todayStudyHourLabel = nullptr;
    QLabel *m_continuousDaysLabel = nullptr;
    QLabel *m_maxContinuousDaysLabel = nullptr;
    QLabel *m_completedProjectsLabel = nullptr;
    QLabel *m_studyCheckLabel = nullptr;
    TimeAxis *m_timeAxisWidget = nullptr;
    QScrollArea *m_timeAxisScroll = nullptr;
    RoundProgressBar *m_dayProgressBar = nullptr;
    QPropertyAnimation *m_progressAnimation = nullptr;

    static const int PROGRESS_SCALE = 100;
    void animateProgress(int targetHours);

private slots:
    void showDateSelectDialog();
    void setToTodayDate();
    void showSetTargetDialog();
    void clearCurrentData();
};

#endif // DAYVIEW_H
