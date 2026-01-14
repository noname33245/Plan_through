#include "appdatas.h"

AppDatas* appDatas = new AppDatas();

AppDatas::AppDatas() {
    initSavePath();
    initConfigFile();
    loadConfigFromFile();
    loadDataFromFile();
}

AppDatas::~AppDatas(){
    saveDataToFile();
    saveConfigToFile();
}

void AppDatas::initSavePath()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if(!dir.exists())
    {
        dir.mkpath(appDataPath);
    }
    m_saveFilePath = appDataPath + "/study_data.json";

    QString userName = QProcessEnvironment::systemEnvironment().value("USERNAME");
    qDebug() << "当前登录用户名：" << userName;
    qDebug() << "当前学习数据存档路径：" << m_saveFilePath;
}

void AppDatas::initConfigFile()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if(!dir.exists())
    {
        dir.mkpath(appDataPath);
    }
    m_configFilePath = appDataPath + "/study_config.json";
    qDebug() << "当前配置文件存档路径：" << m_configFilePath;
}

void AppDatas::saveConfigToFile()
{
    QJsonObject rootObj;
    rootObj.insert("studyTargetHour", m_studyTargetHour);

    QFile file(m_configFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    QApplication::processEvents();
}

void AppDatas::loadConfigFromFile()
{
    QFile file(m_configFilePath);
    if(!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) return;

    QJsonObject rootObj = doc.object();
    if(rootObj.contains("studyTargetHour"))
    {
        m_studyTargetHour = rootObj["studyTargetHour"].toInt();
        if(m_studyTargetHour <1 || m_studyTargetHour>8) m_studyTargetHour =4;
    }
}

void AppDatas::saveDataToFile()
{
    QJsonObject rootObj;
    rootObj.insert("maxContinuousDays", m_maxContinuousDays);
    QJsonObject dateObj;

    QMap<QDate, DateStudyData>::const_iterator dateIt = m_studyDataMap.constBegin();
    while(dateIt != m_studyDataMap.constEnd())
    {
        QDate date = dateIt.key();
        DateStudyData data = dateIt.value();
        QString dateStr = date.toString("yyyy-MM-dd");

        QJsonObject studyObj;
        studyObj.insert("studyHours", data.studyHours);
        studyObj.insert("completedProjects", data.completedProjects);
        studyObj.insert("totalProjects", data.totalProjects);

        QJsonObject timeAxisObj;
        QMap<int, TimeAxisItem>::const_iterator timeIt = data.timeAxisData.constBegin();
        while(timeIt != data.timeAxisData.constEnd())
        {
            int hour = timeIt.key();
            TimeAxisItem item = timeIt.value();
            QJsonObject itemObj;
            itemObj.insert("type", item.type);
            itemObj.insert("isCompleted", item.isCompleted);
            timeAxisObj.insert(QString::number(hour), itemObj);
            ++timeIt;
        }
        studyObj.insert("timeAxisData", timeAxisObj);
        dateObj.insert(dateStr, studyObj);
        ++dateIt;
    }
    rootObj.insert("studyData", dateObj);

    QFile file(m_saveFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Compact));
    file.close();

    QApplication::processEvents();
}

void AppDatas::loadDataFromFile()
{
    QFile file(m_saveFilePath);
    if(!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QByteArray data = file.readAll();
    file.close();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) return;

    QJsonObject rootObj = doc.object();
    if(rootObj.contains("maxContinuousDays")) m_maxContinuousDays = rootObj["maxContinuousDays"].toInt();

    if(rootObj.contains("studyData"))
    {
        QJsonObject dateObj = rootObj["studyData"].toObject();
        QStringList dateList = dateObj.keys();
        foreach (QString dateStr, dateList)
        {
            QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
            if(!date.isValid()) continue;

            QJsonObject studyObj = dateObj[dateStr].toObject();
            DateStudyData studyData;
            studyData.studyHours = studyObj["studyHours"].toInt();
            studyData.completedProjects = studyObj["completedProjects"].toInt();
            studyData.totalProjects = studyObj["totalProjects"].toInt();

            QJsonObject timeAxisObj = studyObj["timeAxisData"].toObject();
            QStringList hourList = timeAxisObj.keys();
            foreach (QString hourStr, hourList)
            {
                bool ok;
                int hour = hourStr.toInt(&ok);
                if(!ok) continue;

                QJsonObject itemObj = timeAxisObj[hourStr].toObject();
                TimeAxisItem item;
                item.type = itemObj["type"].toString();
                item.isCompleted = itemObj["isCompleted"].toBool();
                studyData.timeAxisData.insert(hour, item);
            }
            m_studyDataMap.insert(date, studyData);
        }
    }
}
