#ifndef APPDATAS_H
#define APPDATAS_H

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QJsonObject>
#include <QApplication>
#include <QMap>

struct TimeAxisItem
{
    QString type;
    bool isCompleted;
};

struct DateStudyData
{
    int studyHours = 0;
    int completedProjects = 0;
    int totalProjects = 0;
    QMap<int, TimeAxisItem> timeAxisData;
};

class AppDatas
{
public:
    AppDatas();
    ~AppDatas();
    void initSavePath();
    void initConfigFile();
    void saveConfigToFile();
    void loadConfigFromFile();
    void saveDataToFile();
    void loadDataFromFile();

    int getStudyTargetHour(){return m_studyTargetHour;}
    int getMaxContinDays(){return m_maxContinuousDays;}
    QMap<QDate,DateStudyData>& getStudyDataMap(){return m_studyDataMap;}
    QString getSaveFilePath(){return m_saveFilePath;}
    QString getConfigFilePath(){return m_configFilePath;}

    void setStudyTargetHour(int newTargetHour){m_studyTargetHour = newTargetHour;}
    void setMaxContinDays(int newMaxContinDays){m_maxContinuousDays = newMaxContinDays;}

private:
    QString m_saveFilePath;
    QString m_configFilePath;
    int m_studyTargetHour = 4;
    int m_maxContinuousDays = 0;
    QMap<QDate, DateStudyData> m_studyDataMap;
};

extern AppDatas* appDatas;

#endif // APPDATAS_H
