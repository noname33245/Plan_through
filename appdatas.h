#ifndef APPDATAS_H
#define APPDATAS_H

#include "datastruct.h"
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QString>
#include <QProcessEnvironment>
#include <QSettings>
#include <QApplication>
#include <QStandardPaths>



// 应用数据管理类，负责用户数据读取与存储
class AppDatas
{
public:
    AppDatas();
    ~AppDatas();
    
    void initSavePath();
    void initConfigFile();
    void initSettings();
    void loadConfigFromFile();
    void loadDataFromFile();
    void saveConfigToFile();
    void saveDataToFile();
    void saveSettings();

public:
    void setAutoStartup(bool isAuto);
    void setMinToTray(bool isMinToTray){m_isMinToTray = isMinToTray;}
    void setTheme(int themeType){m_themeType = themeType;}
    void setTargetHour(int targetHour){m_studyTargetHour = targetHour;}
    void setAutoCleanMemoryThreshold(int threshold){m_autoCleanMemoryThreshold = threshold;}
    void setAutoCleanMemoryEnabled(bool enabled){m_isAutoCleanMemoryEnabled = enabled;}
    void setMemoryCleanInterval(int interval){m_memoryCleanInterval = interval;}
    void setMemoryCleanThreshold(int threshold){m_memoryCleanThreshold = threshold;}
    void setAutoMemoryCleanEnabled(bool enabled){m_isAutoMemoryCleanEnabled = enabled;}
    
    int memoryCleanInterval(){return m_memoryCleanInterval;}
    int memoryCleanThreshold(){return m_memoryCleanThreshold;}
    bool isAutoMemoryCleanEnabled(){return m_isAutoMemoryCleanEnabled;}
    
    void setMaxContinDays(int continDays){m_maxContinuousDays = continDays;}
    void setDefaultViewType(int viewType){m_defaultViewType = viewType;}
    int defaultViewType(){return m_defaultViewType;}
    
    // 重载[]运算符，用于访问指定日期的学习数据
    DateStudyData& operator[](const QDate& key){return m_studyDataMap[key];}

public:
    // 获取指定类型的路径
    const QString& path(QString type = "Root");
    
    bool isAutoStartup(){return m_isAutoStartup;}
    bool isMinToTray(){return m_isMinToTray;}
    int themeType(){return m_themeType;}
    int targetHour(){return m_studyTargetHour;}
    int maxContinDays(){return m_maxContinuousDays;}
    int autoCleanMemoryThreshold(){return m_autoCleanMemoryThreshold;}
    bool isAutoCleanMemoryEnabled(){return m_isAutoCleanMemoryEnabled;}
    
    DateStudyData value(const QDate& key){return m_studyDataMap.value(key);}
    bool contains(const QDate& key){return m_studyDataMap.contains(key);}
    
    int calculateContinuousDays();
    bool createBackup(const QString& backupPath);
    bool restoreFromBackup(const QString& backupPath);
    
    int getTotalStudyDays() const;
    int getTotalStudyHours() const;
    double getAverageStudyHoursPerDay() const;
    double getRecentMonthAverageStudyHoursPerDay() const;
    int getTotalProjects() const;
    int getCompletedProjects() const;
    double getProjectCompletionRate() const;
    
    // 获取最近N天的学习数据
    QMap<QDate, DateStudyData> getRecentStudyData(int days) const;
    
    // 获取指定月份的总学习时长
    int getMonthStudyHours(int year, int month) const;

private:
    QString m_appDataPath;
    QString m_saveFilePath;
    QString m_configFilePath;
    QString m_logDirectory;

    QMap<QDate, DateStudyData> m_studyDataMap;
    int m_studyTargetHour = 4;
    int m_maxContinuousDays = 0;

    QSettings *m_appSettings;
    bool m_isAutoStartup = false;
    bool m_isMinToTray = false;
    int m_themeType = 0;
    
    // 自动清理内存相关设置
    bool m_isAutoCleanMemoryEnabled = true; // 默认启用自动清理
    int m_autoCleanMemoryThreshold = 80; // 默认内存使用率超过80%时清理
    
    // 自动内存清理相关设置
    bool m_isAutoMemoryCleanEnabled = false; // 默认禁用自动内存清理
    int m_memoryCleanInterval = 10; // 默认10分钟
    int m_memoryCleanThreshold = 60; // 默认60%
    
    // 默认视图设置
    int m_defaultViewType = 0; // 0: 月视图, 1: 日视图

private:
    void saveLog();
    void cleanupOldLogs();
    bool loadDataFromLogs();
};

extern AppDatas appDatas;

#endif // APPDATAS_H
