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

// 包含服务管理类
#include "windowservice/service.h"

// 应用数据管理类，负责用户数据读取与存储
class AppDatas
{
public:
    // 构造函数，初始化应用数据管理
    AppDatas();
    
    // 析构函数，释放资源并保存数据
    ~AppDatas();
    
    // 初始化存档路径
    void initSavePath();
    
    // 初始化配置文件
    void initConfigFile();
    
    // 初始化设置
    void initSettings();
    
    // 从文件加载配置
    void loadConfigFromFile();
    
    // 从文件加载数据
    void loadDataFromFile();
    
    // 保存配置到文件
    void saveConfigToFile();
    
    // 保存数据到文件
    void saveDataToFile();
    
    // 保存设置
    void saveSettings();

public:
    // 设置是否自动启动
    // 参数1：是否自动启动
    void setAutoStartup(bool isAuto);
    
    // 设置是否最小化到托盘
    // 参数1：是否最小化到托盘
    void setMinToTray(bool isMinToTray){m_isMinToTray = isMinToTray;}
    
    // 设置主题类型
    // 参数1：主题类型
    void setTheme(int themeType){m_themeType = themeType;}
    
    // 设置学习目标小时数
    // 参数1：学习目标小时数
    void setTargetHour(int targetHour){m_studyTargetHour = targetHour;}
    
    // 设置自动清理内存阈值
    // 参数1：内存阈值百分比
    void setAutoCleanMemoryThreshold(int threshold){m_autoCleanMemoryThreshold = threshold;}
    
    // 设置是否启用自动清理内存
    // 参数1：是否启用自动清理
    void setAutoCleanMemoryEnabled(bool enabled){m_isAutoCleanMemoryEnabled = enabled;}
    
    // 设置自动内存清理间隔
    // 参数1：清理间隔（分钟）
    void setMemoryCleanInterval(int interval){m_memoryCleanInterval = interval;}
    
    // 设置自动内存清理阈值
    // 参数1：内存阈值百分比
    void setMemoryCleanThreshold(int threshold){m_memoryCleanThreshold = threshold;}
    
    // 设置是否启用自动内存清理
    // 参数1：是否启用自动清理
    void setAutoMemoryCleanEnabled(bool enabled){m_isAutoMemoryCleanEnabled = enabled;}
    
    // 获取自动内存清理间隔
    // 返回：清理间隔（分钟）
    int memoryCleanInterval(){return m_memoryCleanInterval;}
    
    // 获取自动内存清理阈值
    // 返回：内存阈值百分比
    int memoryCleanThreshold(){return m_memoryCleanThreshold;}
    
    // 获取是否启用自动内存清理
    // 返回：是否启用自动清理
    bool isAutoMemoryCleanEnabled(){return m_isAutoMemoryCleanEnabled;}
    
    // 设置最大连续天数
    // 参数1：最大连续天数
    void setMaxContinDays(int continDays){m_maxContinuousDays = continDays;}
    
    // 设置默认视图类型
    // 参数1：视图类型（0: 月视图, 1: 日视图）
    void setDefaultViewType(int viewType){m_defaultViewType = viewType;}
    
    // 获取默认视图类型
    // 返回：视图类型（0: 月视图, 1: 日视图）
    int defaultViewType(){return m_defaultViewType;}
    
    // 重载[]运算符，用于访问指定日期的学习数据
    // 参数1：日期键
    // 返回：学习数据引用
    DateStudyData& operator[](const QDate& key){return m_studyDataMap[key];}

public:
    // 获取指定类型的路径
    // 参数1：路径类型，支持"Root"、"Save"、"Config"、"Log"
    // 返回：路径字符串
    const QString& path(QString type = "Root");
    
    // 获取是否自动启动
    // 返回：是否自动启动
    bool isAutoStartup(){return m_isAutoStartup;}
    
    // 获取是否最小化到托盘
    // 返回：是否最小化到托盘
    bool isMinToTray(){return m_isMinToTray;}
    
    // 获取主题类型
    // 返回：主题类型
    int themeType(){return m_themeType;}
    
    // 获取学习目标小时数
    // 返回：学习目标小时数
    int targetHour(){return m_studyTargetHour;}
    
    // 获取最大连续天数
    // 返回：最大连续天数
    int maxContinDays(){return m_maxContinuousDays;}
    
    // 获取自动清理内存阈值
    // 返回：内存阈值百分比
    int autoCleanMemoryThreshold(){return m_autoCleanMemoryThreshold;}
    
    // 获取是否启用自动清理内存
    // 返回：是否启用自动清理
    bool isAutoCleanMemoryEnabled(){return m_isAutoCleanMemoryEnabled;}
    
    // 获取指定日期的学习数据
    // 参数1：日期键
    // 返回：学习数据
    DateStudyData value(const QDate& key){return m_studyDataMap.value(key);}
    
    // 检查是否包含指定日期的数据
    // 参数1：日期键
    // 返回：是否包含
    bool contains(const QDate& key){return m_studyDataMap.contains(key);}
    
    // 计算连续学习天数
    // 返回：连续学习天数
    int calculateContinuousDays();
    
    // 创建数据备份
    // 参数1：备份文件路径
    // 返回：是否成功
    bool createBackup(const QString& backupPath);
    
    // 从备份恢复数据
    // 参数1：备份文件路径
    // 返回：是否成功
    bool restoreFromBackup(const QString& backupPath);
    
    // 获取总学习天数
    // 返回：总学习天数
    int getTotalStudyDays() const;
    
    // 获取总学习时长（小时）
    // 返回：总学习时长
    int getTotalStudyHours() const;
    
    // 获取平均每天学习时长（小时）
    // 返回：平均每天学习时长
    double getAverageStudyHoursPerDay() const;
    
    // 获取总项目数
    // 返回：总项目数
    int getTotalProjects() const;
    
    // 获取完成项目数
    // 返回：完成项目数
    int getCompletedProjects() const;
    
    // 获取项目完成率（百分比）
    // 返回：项目完成率
    double getProjectCompletionRate() const;
    
    // 获取最近N天的学习数据
    // 参数1：天数
    // 返回：最近N天的学习数据，键为日期，值为学习数据
    QMap<QDate, DateStudyData> getRecentStudyData(int days) const;
    
    // 获取指定月份的总学习时长
    // 参数1：年份
    // 参数2：月份
    // 返回：总学习时长（小时）
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
    // 保存每日日志
    void saveLog();
    
    // 清理旧日志
    void cleanupOldLogs();
    
    // 从日志读取数据
    // 返回：是否成功
    bool loadDataFromLogs();
};

extern AppDatas appDatas;

#endif // APPDATAS_H
