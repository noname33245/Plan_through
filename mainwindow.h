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

#include "appdatas.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchToDayView();
    void switchToMonthView();
    void onTimeAxisBtnClicked(int hour);
    void confirmTimeAxisItem(int hour, QString type);
    void clearCurrentHourItem(int hour);
    void showDateSelectDialog();
    void setToTodayDate();
    void clearCurrentData();
    void switchMonth(int offset);
    void setToCurrentMonth();
    void showSetTargetDialog();
    void setStudyTargetHour(int targetHour);

private:
    void initUI();
    QWidget* createDayViewPage();
    QWidget* createMonthViewPage();
    QWidget* createTimeAxis();
    void loadDateData(const QDate& date);
    void updateDayViewStats();
    void generateMonthCalendar(int year, int month);
    int calculateContinuousDays();

    QPushButton *m_dayViewBtn = nullptr;
    QPushButton *m_monthViewBtn = nullptr;
    QStackedWidget *m_mainStackedWidget = nullptr;
    QLabel *m_todayStudyHourLabel = nullptr;
    QProgressBar *m_dayProgressBar = nullptr;
    QLabel *m_continuousDaysLabel = nullptr;
    QLabel *m_maxContinuousDaysLabel = nullptr;
    QLabel *m_completedProjectsLabel = nullptr;
    QLabel *m_studyCheckLabel = nullptr;
    QWidget *m_timeAxisWidget = nullptr;
    QMap<int, QPushButton*> m_timeAxisBtnMap;
    QLabel *m_monthTitleLabel = nullptr;
    QGridLayout *m_monthCalendarLayout = nullptr;
    QLabel *m_selectedDateLabel = nullptr;
    QLabel *m_targetHourShowLabel = nullptr;

    QDate m_currentDate;
    int m_currentYear;
    int m_currentMonth;
};

#endif // MAINWINDOW_H
