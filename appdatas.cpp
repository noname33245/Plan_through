#include "appdatas.h"
#include <QSettings>

AppDatas appDatas;

// 构造函数，初始化应用数据管理
AppDatas::AppDatas() {
    m_appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Plan_through";
    m_appSettings = new QSettings(m_appDataPath + "/app_settings.ini", QSettings::IniFormat);

    initSavePath();
    initConfigFile();
    initSettings();

    cleanupOldLogs();

    loadDataFromFile();
    loadConfigFromFile();
}

// 析构函数，释放资源并保存数据
AppDatas::~AppDatas(){
    saveDataToFile();
    saveConfigToFile();
    saveSettings();
    
    if (m_appSettings) {
        delete m_appSettings;
        m_appSettings = nullptr;
    }
}

// 初始化存档路径
void AppDatas::initSavePath()
{
    QDir dir(m_appDataPath);
    if(!dir.exists())
    {
        if(!dir.mkpath(m_appDataPath)) {
            qCritical() << "无法创建应用数据目录：" << m_appDataPath;
        }
    }
    m_saveFilePath = m_appDataPath + "/study_data.json";
    m_logDirectory = m_appDataPath + "/logs";
    
    QDir logDir(m_logDirectory);
    if(!logDir.exists())
    {
        if(!logDir.mkpath(m_logDirectory)) {
            qCritical() << "无法创建日志目录：" << m_logDirectory;
        }
    }

    QString userName = QProcessEnvironment::systemEnvironment().value("USERNAME");
    qDebug() << "当前登录用户名：" << userName;
    qDebug() << "当前学习数据存档路径：" << m_saveFilePath;
    qDebug() << "当前日志目录：" << m_logDirectory;
}

// 初始化配置文件
void AppDatas::initConfigFile()
{
    QDir dir(m_appDataPath);
    if(!dir.exists())
    {
        if(!dir.mkpath(m_appDataPath)) {
            qCritical() << "无法创建应用数据目录：" << m_appDataPath;
        }
    }
    m_configFilePath = m_appDataPath + "/study_config.json";
    qDebug() << "当前配置文件存档路径：" << m_configFilePath;
}

// 保存配置到文件
void AppDatas::saveConfigToFile()
{
    QJsonObject rootObj;
    rootObj.insert("studyTargetHour", m_studyTargetHour);

    QFile file(m_configFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "无法打开配置文件进行写入：" << m_configFilePath << "，错误：" << file.errorString();
        return;
    }
    
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    qint64 bytesWritten = file.write(jsonData);
    
    if (bytesWritten != jsonData.size()) {
        qCritical() << "配置文件写入不完整，预期写入" << jsonData.size() << "字节，实际写入" << bytesWritten << "字节";
    }
    
    file.close();
    
    if (file.error() != QFile::NoError) {
        qCritical() << "关闭配置文件时发生错误：" << file.errorString();
    } else {
        qDebug() << "配置文件保存成功：" << m_configFilePath;
    }
}

// 从文件加载配置
void AppDatas::loadConfigFromFile()
{
    QFile file(m_configFilePath);
    if(!file.exists()) {
        qDebug() << "配置文件不存在，将使用默认配置：" << m_configFilePath;
        return;
    }
    
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "无法打开配置文件进行读取：" << m_configFilePath << "，错误：" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();
    
    if (data.isEmpty()) {
        qWarning() << "配置文件为空，将使用默认配置：" << m_configFilePath;
        return;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) {
        qCritical() << "配置文件解析失败：" << m_configFilePath << "，错误：" << error.errorString();
        return;
    }

    QJsonObject rootObj = doc.object();
    if(rootObj.contains("studyTargetHour"))
    {
        m_studyTargetHour = rootObj["studyTargetHour"].toInt();
        if(m_studyTargetHour <1 || m_studyTargetHour>8) {
            qWarning() << "配置文件中的学习目标小时数无效(" << m_studyTargetHour << ")，将使用默认值4";
            m_studyTargetHour =4;
        } else {
            qDebug() << "从配置文件加载学习目标小时数：" << m_studyTargetHour;
        }
    }
}

// 保存数据到文件
void AppDatas::saveDataToFile()
{
    qDebug() << "开始保存学习数据...";
    
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

    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    if (jsonData.isEmpty()) {
        qCritical() << "数据序列化失败，跳过保存";
        return;
    }
    
    qDebug() << "数据序列化成功，数据大小：" << jsonData.size() << "字节，包含" << m_studyDataMap.size() << "天的学习数据";

    QString tempFilePath = m_saveFilePath + ".tmp";
    QFile tempFile(tempFilePath);
    if(!tempFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "临时文件打开失败：" << tempFilePath << "，错误：" << tempFile.errorString();
        return;
    }
    
    qint64 written = tempFile.write(jsonData);
    tempFile.close();

    if (tempFile.error() != QFile::NoError) {
        qCritical() << "临时文件写入过程中发生错误：" << tempFile.errorString();
        QFile::remove(tempFilePath);
        return;
    }

    if (written != jsonData.size()) {
        qCritical() << "临时文件写入不完整，预期写入" << jsonData.size() << "字节，实际写入" << written << "字节";
        QFile::remove(tempFilePath);
        return;
    }
    
    qDebug() << "临时文件写入成功：" << tempFilePath;

    // 备份原有文件
    QString backupFilePath = m_saveFilePath + ".bak";
    bool backupSuccess = true;
    
    if (QFile::exists(m_saveFilePath)) {
        // 删除旧备份
        QFile::remove(backupFilePath);
        
        if (!QFile::rename(m_saveFilePath, backupFilePath)) {
            qWarning() << "创建备份文件失败：" << backupFilePath;
            backupSuccess = false;
        } else {
            qDebug() << "原有文件备份成功：" << backupFilePath;
        }
    }
    
    // 替换原有文件
    if (!QFile::rename(tempFilePath, m_saveFilePath)) {
        qCritical() << "覆盖存档文件失败：" << m_saveFilePath;
        QFile::remove(tempFilePath);
        
        // 尝试恢复备份
        if (backupSuccess && QFile::exists(backupFilePath)) {
            if (QFile::rename(backupFilePath, m_saveFilePath)) {
                qDebug() << "从备份恢复存档文件成功";
            } else {
                qCritical() << "从备份恢复存档文件失败";
            }
        }
        
        return;
    }
    
    qDebug() << "学习数据保存成功：" << m_saveFilePath;
    
    // 删除备份文件
    QFile::remove(backupFilePath);
    
    // 保存数据后写入日志
    saveLog();
}

// 保存每日日志
void AppDatas::saveLog()
{
    QString logFileName = QDate::currentDate().toString("yyyy-MM-dd") + ".json";
    QString logFilePath = m_logDirectory + "/" + logFileName;
    
    QJsonObject rootObj;
    rootObj.insert("maxContinuousDays", m_maxContinuousDays);
    QJsonObject dateObj;
    
    QDate currentDate = QDate::currentDate();
    if (m_studyDataMap.contains(currentDate)) {
        const DateStudyData& data = m_studyDataMap[currentDate];
        QString dateStr = currentDate.toString("yyyy-MM-dd");
        
        QJsonObject studyObj;
        studyObj.insert("studyHours", data.studyHours);
        studyObj.insert("completedProjects", data.completedProjects);
        studyObj.insert("totalProjects", data.totalProjects);
        
        QJsonObject timeAxisObj;
        QMap<int, TimeAxisItem>::const_iterator timeIt = data.timeAxisData.constBegin();
        while(timeIt != data.timeAxisData.constEnd())
        {
            int hour = timeIt.key();
            const TimeAxisItem& item = timeIt.value();
            QJsonObject itemObj;
            itemObj.insert("type", item.type);
            itemObj.insert("isCompleted", item.isCompleted);
            timeAxisObj.insert(QString::number(hour), itemObj);
            ++timeIt;
        }
        studyObj.insert("timeAxisData", timeAxisObj);
        dateObj.insert(dateStr, studyObj);
    }
    
    rootObj.insert("studyData", dateObj);
    
    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QFile logFile(logFilePath);
    if(!logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "日志文件打开失败：" << logFilePath << "，错误：" << logFile.errorString();
        return;
    }
    
    qint64 bytesWritten = logFile.write(jsonData);
    logFile.close();
    
    if (logFile.error() != QFile::NoError) {
        qCritical() << "日志文件写入过程中发生错误：" << logFile.errorString();
        return;
    }
    
    if (bytesWritten != jsonData.size()) {
        qCritical() << "日志文件写入不完整，预期写入" << jsonData.size() << "字节，实际写入" << bytesWritten << "字节";
        return;
    }
    
    qDebug() << "日志保存成功：" << logFilePath;
    
    // 清理旧日志
    cleanupOldLogs();
}

// 清理旧日志
void AppDatas::cleanupOldLogs()
{
    QDir logDir(m_logDirectory);
    QFileInfoList logFiles = logDir.entryInfoList(QStringList() << "*.json", QDir::Files);
    
    QDate currentDate = QDate::currentDate();
    int daysToKeep = 30;
    
    foreach (const QFileInfo& fileInfo, logFiles) {
        QString fileName = fileInfo.fileName();
        QString dateStr = fileName.left(10);
        QDate logDate = QDate::fromString(dateStr, "yyyy-MM-dd");
        
        if (logDate.isValid()) {
            int daysDiff = logDate.daysTo(currentDate);
            if (daysDiff > daysToKeep) {
                QString logFilePath = fileInfo.absoluteFilePath();
                if (QFile::remove(logFilePath)) {
                    qDebug() << "删除旧日志文件：" << logFilePath;
                } else {
                    qWarning() << "删除旧日志文件失败：" << logFilePath;
                }
            }
        }
    }
}

// 从日志读取数据
// 返回：是否成功
bool AppDatas::loadDataFromLogs()
{
    QDir logDir(m_logDirectory);
    QFileInfoList logFiles = logDir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Time); // 按时间排序，最新的日志优先
    
    if (logFiles.isEmpty()) {
        qDebug() << "没有找到日志文件";
        return false;
    }
    
    bool loaded = false;
    int maxContinuous = 0;
    int loadedDays = 0;
    int processedLogs = 0;
    
    qDebug() << "找到" << logFiles.size() << "个日志文件，开始从日志加载数据...";
    
    foreach (const QFileInfo& fileInfo, logFiles) {
        QString logFilePath = fileInfo.absoluteFilePath();
        processedLogs++;
        
        QFile logFile(logFilePath);
        
        if(!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "日志文件打开失败：" << logFilePath << "，错误：" << logFile.errorString();
            continue;
        }
        
        QByteArray data = logFile.readAll();
        logFile.close();
        
        if (logFile.error() != QFile::NoError) {
            qCritical() << "关闭日志文件时发生错误：" << logFilePath << "，错误：" << logFile.errorString();
            continue;
        }
        
        if (data.isEmpty()) {
            qWarning() << "日志文件为空：" << logFilePath;
            continue;
        }
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if(error.error != QJsonParseError::NoError) {
            qWarning() << "日志JSON解析失败：" << logFilePath << "，错误：" << error.errorString();
            continue;
        }
        
        QJsonObject rootObj = doc.object();
        
        // 更新最大连续天数
        if(rootObj.contains("maxContinuousDays")) {
            int logMaxContinuous = rootObj["maxContinuousDays"].toInt();
            if (logMaxContinuous > maxContinuous) {
                maxContinuous = logMaxContinuous;
            }
        }
        
        // 加载学习数据
        if(rootObj.contains("studyData"))
        {
            QJsonObject dateObj = rootObj["studyData"].toObject();
            QStringList dateList = dateObj.keys();
            
            foreach (QString dateStr, dateList)
            {
                QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
                if(!date.isValid()) {
                    qWarning() << "日志文件" << logFilePath << "中存在无效的日期格式：" << dateStr;
                    continue;
                }
                
                // 如果已经存在该日期的数据，则跳过（避免覆盖）
                if (m_studyDataMap.contains(date)) {
                    continue;
                }
                
                QJsonObject studyObj = dateObj[dateStr].toObject();
                DateStudyData studyData;
                studyData.studyHours = studyObj["studyHours"].toInt();
                studyData.completedProjects = studyObj["completedProjects"].toInt();
                studyData.totalProjects = studyObj["totalProjects"].toInt();
                
                // 加载时间轴数据
                if (studyObj.contains("timeAxisData")) {
                    QJsonObject timeAxisObj = studyObj["timeAxisData"].toObject();
                    QStringList hourList = timeAxisObj.keys();
                    
                    foreach (QString hourStr, hourList)
                    {
                        bool ok;
                        int hour = hourStr.toInt(&ok);
                        if(!ok) {
                            qWarning() << "日志文件" << logFilePath << "中存在无效的小时格式：" << hourStr;
                            continue;
                        }
                        
                        QJsonObject itemObj = timeAxisObj[hourStr].toObject();
                        TimeAxisItem item;
                        item.type = itemObj["type"].toString();
                        item.isCompleted = itemObj["isCompleted"].toBool();
                        studyData.timeAxisData.insert(hour, item);
                    }
                }
                
                m_studyDataMap.insert(date, studyData);
                loadedDays++;
                loaded = true;
            }
        }
    }
    
    if (loaded) {
        m_maxContinuousDays = maxContinuous;
        qDebug() << "从" << processedLogs << "个日志文件中成功加载" << loadedDays << "天的学习数据，最大连续天数：" << maxContinuous;
    } else {
        qDebug() << "处理了" << processedLogs << "个日志文件，但没有加载到有效数据";
    }
    
    return loaded;
}

// 从文件加载数据
void AppDatas::loadDataFromFile()
{
    qDebug() << "开始加载学习数据...";
    
    QFile file(m_saveFilePath);
    if(file.exists()) {
        qDebug() << "找到存档文件：" << m_saveFilePath;
        
        if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray data = file.readAll();
            file.close();
            
            if (file.error() != QFile::NoError) {
                qCritical() << "关闭存档文件时发生错误：" << file.errorString();
            }
            
            if (!data.isEmpty()) {
                QJsonParseError error;
                QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                if(error.error == QJsonParseError::NoError) {
                    QJsonObject rootObj = doc.object();
                    
                    // 加载最大连续天数
                    if(rootObj.contains("maxContinuousDays")) {
                        m_maxContinuousDays = rootObj["maxContinuousDays"].toInt();
                        qDebug() << "加载最大连续天数：" << m_maxContinuousDays;
                    }
                    
                    // 加载学习数据
                    if(rootObj.contains("studyData"))
                    {
                        QJsonObject dateObj = rootObj["studyData"].toObject();
                        QStringList dateList = dateObj.keys();
                        int loadedDays = 0;
                        
                        foreach (QString dateStr, dateList)
                        {
                            QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
                            if(!date.isValid()) {
                                qWarning() << "无效的日期格式：" << dateStr;
                                continue;
                            }
                            
                            QJsonObject studyObj = dateObj[dateStr].toObject();
                            DateStudyData studyData;
                            studyData.studyHours = studyObj["studyHours"].toInt();
                            studyData.completedProjects = studyObj["completedProjects"].toInt();
                            studyData.totalProjects = studyObj["totalProjects"].toInt();
                            
                            // 加载时间轴数据
                            if (studyObj.contains("timeAxisData")) {
                                QJsonObject timeAxisObj = studyObj["timeAxisData"].toObject();
                                QStringList hourList = timeAxisObj.keys();
                                
                                foreach (QString hourStr, hourList)
                                {
                                    bool ok;
                                    int hour = hourStr.toInt(&ok);
                                    if(!ok) {
                                        qWarning() << "无效的小时格式：" << hourStr;
                                        continue;
                                    }
                                    
                                    QJsonObject itemObj = timeAxisObj[hourStr].toObject();
                                    TimeAxisItem item;
                                    item.type = itemObj["type"].toString();
                                    item.isCompleted = itemObj["isCompleted"].toBool();
                                    studyData.timeAxisData.insert(hour, item);
                                }
                            }
                            
                            m_studyDataMap.insert(date, studyData);
                            loadedDays++;
                        }
                        
                        qDebug() << "成功从存档文件加载" << loadedDays << "天的学习数据";
                        return;
                    } else {
                        qWarning() << "存档文件中没有studyData字段";
                    }
                } else {
                    qCritical() << "存档JSON解析失败：" << error.errorString();
                }
            } else {
                qWarning() << "存档文件为空";
            }
        } else {
            qCritical() << "无法打开存档文件进行读取：" << m_saveFilePath << "，错误：" << file.errorString();
        }
    } else {
        qDebug() << "存档文件不存在：" << m_saveFilePath;
    }
    
    qWarning() << "存档文件读取失败，尝试从日志恢复数据...";
    
    // 存档文件读取失败，尝试从日志读取
    if (loadDataFromLogs()) {
        qDebug() << "从日志恢复数据成功";
    } else {
        // 日志读取失败，按照老办法创建（即保持m_studyDataMap为空，后续会自动创建）
        qDebug() << "日志读取失败，将使用空数据集";
    }
}

// 初始化设置
void AppDatas::initSettings()
{
    m_isAutoStartup = m_appSettings->value("auto_startup", false).toBool();
    // 不要在初始化时调用setAutoStartup，避免覆盖注册表中的路径
    // setAutoStartup(m_isAutoStartup);

    m_isMinToTray = m_appSettings->value("min_to_tray", false).toBool();
    m_themeType = m_appSettings->value("theme", 0).toInt();
    
    // 加载自动清理内存设置
    m_isAutoCleanMemoryEnabled = m_appSettings->value("auto_clean_memory_enabled", true).toBool();
    m_autoCleanMemoryThreshold = m_appSettings->value("auto_clean_memory_threshold", 80).toInt();
    
    // 加载自动内存清理设置
    m_isAutoMemoryCleanEnabled = m_appSettings->value("auto_memory_clean_enabled", false).toBool();
    m_memoryCleanInterval = m_appSettings->value("memory_clean_interval", 10).toInt();
    m_memoryCleanThreshold = m_appSettings->value("memory_clean_threshold", 60).toInt();
    
    // 加载默认视图设置
    m_defaultViewType = m_appSettings->value("default_view_type", 0).toInt();
}

// 保存设置
void AppDatas::saveSettings()
{
    m_appSettings->setValue("auto_startup", m_isAutoStartup);
    m_appSettings->setValue("min_to_tray", m_isMinToTray);
    m_appSettings->setValue("theme", m_themeType);
    
    // 保存自动清理内存设置
    m_appSettings->setValue("auto_clean_memory_enabled", m_isAutoCleanMemoryEnabled);
    m_appSettings->setValue("auto_clean_memory_threshold", m_autoCleanMemoryThreshold);
    
    // 保存自动内存清理设置
    m_appSettings->setValue("auto_memory_clean_enabled", m_isAutoMemoryCleanEnabled);
    m_appSettings->setValue("memory_clean_interval", m_memoryCleanInterval);
    m_appSettings->setValue("memory_clean_threshold", m_memoryCleanThreshold);
    
    // 保存默认视图设置
    m_appSettings->setValue("default_view_type", m_defaultViewType);
    
    m_appSettings->sync();
}

// 设置是否自动启动
// 参数1：是否自动启动
void AppDatas::setAutoStartup(bool isAuto)
{
    m_isAutoStartup = isAuto;
    
    QString taskName = "PlanThroughStartup";
    QString executablePath = QApplication::applicationFilePath();
    executablePath = executablePath.replace("/", "\\");
    
    if(isAuto) {
        // 使用任务计划程序添加开机自启
        QString command = QString("schtasks /create /tn %1 /tr \"%2\" /sc onlogon /rl highest /f").arg(taskName).arg(executablePath);
        int result = system(command.toLocal8Bit().constData());
        if(result == 0) {
            qDebug() << "添加到开机自启成功（任务计划程序）：" << executablePath;
        } else {
            qDebug() << "添加到开机自启失败（任务计划程序）";
        }
    } else {
        // 使用任务计划程序移除开机自启
        QString command = QString("schtasks /delete /tn %1 /f").arg(taskName);
        int result = system(command.toLocal8Bit().constData());
        if(result == 0) {
            qDebug() << "从开机自启中移除成功（任务计划程序）";
        } else {
            qDebug() << "从开机自启中移除失败（任务计划程序）";
        }
    }
}

// 获取指定类型的路径
// 参数1：路径类型，支持"Root"、"Save"、"Config"、"Log"
// 返回：路径字符串
const QString& AppDatas::path(QString type){
    return type=="Root"?m_appDataPath:
               type=="Save"?m_saveFilePath:
               type=="Config"?m_configFilePath:
               type=="Log"?m_logDirectory:
               m_appDataPath;
}

// 计算连续学习天数
// 返回：连续学习天数
int AppDatas::calculateContinuousDays()
{
    int days = 0;
    QDate current = QDate::currentDate();
    while (contains(current)) {
        days++;
        current = current.addDays(-1);
    }
    return days;
}

// 创建数据备份
// 参数1：备份文件路径
// 返回：是否成功
bool AppDatas::createBackup(const QString& backupPath)
{
    qDebug() << "开始创建数据备份：" << backupPath;
    
    // 构建备份数据
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

    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    if (jsonData.isEmpty()) {
        qCritical() << "备份数据序列化失败";
        return false;
    }
    
    qDebug() << "备份数据序列化成功，数据大小：" << jsonData.size() << "字节，包含" << m_studyDataMap.size() << "天的学习数据";

    // 创建临时文件，确保写入完整
    QString tempBackupPath = backupPath + ".tmp";
    QFile tempFile(tempBackupPath);
    if(!tempFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qCritical() << "无法打开备份临时文件进行写入：" << tempBackupPath << "，错误：" << tempFile.errorString();
        return false;
    }
    
    qint64 written = tempFile.write(jsonData);
    tempFile.close();

    if (tempFile.error() != QFile::NoError) {
        qCritical() << "备份临时文件写入过程中发生错误：" << tempFile.errorString();
        QFile::remove(tempBackupPath);
        return false;
    }

    if (written != jsonData.size()) {
        qCritical() << "备份临时文件写入不完整，预期写入" << jsonData.size() << "字节，实际写入" << written << "字节";
        QFile::remove(tempBackupPath);
        return false;
    }
    
    qDebug() << "备份临时文件写入成功：" << tempBackupPath;

    // 替换最终备份文件
    if (!QFile::rename(tempBackupPath, backupPath)) {
        qCritical() << "替换备份文件失败：" << backupPath;
        QFile::remove(tempBackupPath);
        return false;
    }
    
    qDebug() << "数据备份创建成功：" << backupPath;
    return true;
}

// 从备份恢复数据
// 参数1：备份文件路径
// 返回：是否成功
bool AppDatas::restoreFromBackup(const QString& backupPath)
{
    qDebug() << "开始从备份恢复数据：" << backupPath;
    
    QFile backupFile(backupPath);
    if(!backupFile.exists()) {
        qCritical() << "备份文件不存在：" << backupPath;
        return false;
    }
    
    if(!backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "无法打开备份文件进行读取：" << backupPath << "，错误：" << backupFile.errorString();
        return false;
    }

    QByteArray data = backupFile.readAll();
    backupFile.close();
    
    if (backupFile.error() != QFile::NoError) {
        qCritical() << "关闭备份文件时发生错误：" << backupFile.errorString();
        return false;
    }
    
    if (data.isEmpty()) {
        qCritical() << "备份文件为空：" << backupPath;
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if(error.error != QJsonParseError::NoError) {
        qCritical() << "备份文件JSON解析失败：" << backupPath << "，错误：" << error.errorString();
        return false;
    }
    
    // 临时保存恢复的数据，确保完整解析后再替换
    QMap<QDate, DateStudyData> tempStudyDataMap;
    int tempMaxContinuousDays = m_maxContinuousDays;
    
    QJsonObject rootObj = doc.object();
    
    // 加载最大连续天数
    if(rootObj.contains("maxContinuousDays")) {
        tempMaxContinuousDays = rootObj["maxContinuousDays"].toInt();
        qDebug() << "从备份加载最大连续天数：" << tempMaxContinuousDays;
    }
    
    // 加载学习数据
    if(rootObj.contains("studyData"))
    {
        QJsonObject dateObj = rootObj["studyData"].toObject();
        QStringList dateList = dateObj.keys();
        int loadedDays = 0;
        
        foreach (QString dateStr, dateList)
        {
            QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
            if(!date.isValid()) {
                qWarning() << "备份文件中存在无效的日期格式：" << dateStr;
                continue;
            }
            
            QJsonObject studyObj = dateObj[dateStr].toObject();
            DateStudyData studyData;
            studyData.studyHours = studyObj["studyHours"].toInt();
            studyData.completedProjects = studyObj["completedProjects"].toInt();
            studyData.totalProjects = studyObj["totalProjects"].toInt();
            
            // 加载时间轴数据
            if (studyObj.contains("timeAxisData")) {
                QJsonObject timeAxisObj = studyObj["timeAxisData"].toObject();
                QStringList hourList = timeAxisObj.keys();
                
                foreach (QString hourStr, hourList)
                {
                    bool ok;
                    int hour = hourStr.toInt(&ok);
                    if(!ok) {
                        qWarning() << "备份文件中存在无效的小时格式：" << hourStr;
                        continue;
                    }
                    
                    QJsonObject itemObj = timeAxisObj[hourStr].toObject();
                    TimeAxisItem item;
                    item.type = itemObj["type"].toString();
                    item.isCompleted = itemObj["isCompleted"].toBool();
                    studyData.timeAxisData.insert(hour, item);
                }
            }
            
            tempStudyDataMap.insert(date, studyData);
            loadedDays++;
        }
        
        qDebug() << "成功从备份文件加载" << loadedDays << "天的学习数据";
    } else {
        qWarning() << "备份文件中没有studyData字段";
        return false;
    }
    
    // 备份当前数据，以便恢复失败时可以回滚
    QString currentBackupPath = m_saveFilePath + ".restore.bak";
    if (createBackup(currentBackupPath)) {
        qDebug() << "恢复前已创建当前数据的临时备份：" << currentBackupPath;
    } else {
        qWarning() << "无法创建恢复前的临时备份，将继续恢复操作";
    }
    
    // 替换当前数据
    m_studyDataMap = tempStudyDataMap;
    m_maxContinuousDays = tempMaxContinuousDays;
    
    // 保存恢复后的数据到主文件
    saveDataToFile();
    
    qDebug() << "数据恢复成功，共恢复" << m_studyDataMap.size() << "天的学习数据";
    return true;
}

// 获取总学习天数（仅统计有学习记录的天数）
// 返回：总学习天数
int AppDatas::getTotalStudyDays() const
{
    int totalDays = 0;
    QMap<QDate, DateStudyData>::const_iterator it = m_studyDataMap.constBegin();
    while (it != m_studyDataMap.constEnd()) {
        if (it.value().studyHours > 0) {
            totalDays++;
        }
        ++it;
    }
    return totalDays;
}

// 获取总学习时长（小时）
// 返回：总学习时长
int AppDatas::getTotalStudyHours() const
{
    int totalHours = 0;
    QMap<QDate, DateStudyData>::const_iterator it = m_studyDataMap.constBegin();
    while (it != m_studyDataMap.constEnd()) {
        totalHours += it.value().studyHours;
        ++it;
    }
    return totalHours;
}

// 获取平均每天学习时长（小时）
// 返回：平均每天学习时长
double AppDatas::getAverageStudyHoursPerDay() const
{
    int totalDays = getTotalStudyDays();
    if (totalDays == 0) {
        return 0.0;
    }
    return static_cast<double>(getTotalStudyHours()) / totalDays;
}

// 获取总项目数
// 返回：总项目数
int AppDatas::getTotalProjects() const
{
    int totalProjects = 0;
    QMap<QDate, DateStudyData>::const_iterator it = m_studyDataMap.constBegin();
    while (it != m_studyDataMap.constEnd()) {
        totalProjects += it.value().totalProjects;
        ++it;
    }
    return totalProjects;
}

// 获取完成项目数
// 返回：完成项目数
int AppDatas::getCompletedProjects() const
{
    int completedProjects = 0;
    QMap<QDate, DateStudyData>::const_iterator it = m_studyDataMap.constBegin();
    while (it != m_studyDataMap.constEnd()) {
        completedProjects += it.value().completedProjects;
        ++it;
    }
    return completedProjects;
}

// 获取项目完成率（百分比）
// 返回：项目完成率
double AppDatas::getProjectCompletionRate() const
{
    int totalProjects = getTotalProjects();
    if (totalProjects == 0) {
        return 0.0;
    }
    return static_cast<double>(getCompletedProjects()) / totalProjects * 100.0;
}

// 获取最近N天的学习数据
// 参数1：天数
// 返回：最近N天的学习数据，键为日期，值为学习数据
QMap<QDate, DateStudyData> AppDatas::getRecentStudyData(int days) const
{
    QMap<QDate, DateStudyData> recentData;
    QDate currentDate = QDate::currentDate();
    
    for (int i = 0; i < days; ++i) {
        QDate date = currentDate.addDays(-i);
        if (m_studyDataMap.contains(date)) {
            recentData.insert(date, m_studyDataMap[date]);
        }
    }
    
    return recentData;
}

// 获取指定月份的总学习时长
// 参数1：年份
// 参数2：月份
// 返回：总学习时长（小时）
int AppDatas::getMonthStudyHours(int year, int month) const
{
    int totalHours = 0;
    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();
    
    for (int day = 1; day <= daysInMonth; ++day) {
        QDate currentDate(year, month, day);
        if (m_studyDataMap.contains(currentDate)) {
            totalHours += m_studyDataMap[currentDate].studyHours;
        }
    }
    
    return totalHours;
}
