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
#include "timeaxis.h"

class DayView : public QWidget
{
    Q_OBJECT
public:
    explicit DayView(QWidget *parent = nullptr);
    ~DayView();

    void updateDayViewStats();
    void loadDateData(const QDate& date);

    void setProgress(int hour);
    void setProgressStyle(QString style);

private:
    QLabel *m_selectedDateLabel = nullptr;
    QLabel *m_todayStudyHourLabel = nullptr;
    QLabel *m_continuousDaysLabel = nullptr;
    QLabel *m_maxContinuousDaysLabel = nullptr;
    QLabel *m_completedProjectsLabel = nullptr;
    QLabel *m_studyCheckLabel = nullptr;
    TimeAxis *m_timeAxisWidget = nullptr;
    QProgressBar *m_dayProgressBar = nullptr;


//signals:
private slots:
    void showDateSelectDialog();
    void setToTodayDate();
    void showSetTargetDialog();
    void clearCurrentData();
};

#endif // DAYVIEW_H
