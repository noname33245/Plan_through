#include "appdatas.h"
#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <Windows.h>
#include <shellapi.h>
#include <winreg.h>

AppDatas appDatas;

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

AppDatas::~AppDatas(){
    saveDataToFile();
    saveConfigToFile();
    saveSettings();
    
    if (m_appSettings) {
        delete m_appSettings;
        m_appSettings = nullptr;
    }
}

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
    // 注意：此处不再直接调用 setAutoStartup()，因为 AppDatas 是全局对象，
    // 构造发生在 main() 之前，此时尚无事件循环，ShellExecuteExW 提权对话框
    // 无法正常显示。延迟到 main() 中事件循环启动后再执行（通过 QTimer::singleShot）。

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

// 检查当前进程是否拥有管理员权限
static BOOL IsElevated()
{
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, cbSize, &cbSize))
        {
            bRet = Elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return bRet;
}

// 辅助：注册表路径
static const wchar_t kRunSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t kAutoStartEntryName[] = L"PlanThroughStartup";

// 辅助：写入 HKCU\...\Run 注册表（使用 Windows API，无需 QSettings）
static bool writeRunRegistry(const QString &entryName, const QString &command)
{
    HKEY hKey = NULL;
    LONG lRes = RegCreateKeyExW(HKEY_CURRENT_USER, kRunSubKey, 0, NULL, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, NULL, &hKey, NULL);
    if (lRes != ERROR_SUCCESS) {
        qDebug() << QString("[AutoStartup] RegCreateKeyExW 失败，错误码: %1").arg(lRes);
        return false;
    }

    std::wstring wName = entryName.toStdWString();
    std::wstring wValue = command.toStdWString();
    DWORD valueSize = (DWORD)((wValue.size() + 1) * sizeof(wchar_t));

    lRes = RegSetValueExW(hKey, wName.c_str(), 0, REG_SZ, (BYTE*)wValue.c_str(), valueSize);
    if (lRes != ERROR_SUCCESS) {
        qDebug() << QString("[AutoStartup] RegSetValueExW 失败，错误码: %1").arg(lRes);
        RegCloseKey(hKey);
        return false;
    }

    // 验证
    wchar_t buffer[2048] = {};
    DWORD bufferSize = sizeof(buffer);
    DWORD lType = REG_SZ;
    lRes = RegQueryValueExW(hKey, wName.c_str(), NULL, &lType, (BYTE*)buffer, &bufferSize);
    if (lRes == ERROR_SUCCESS) {
        qDebug() << QString("[AutoStartup] 注册表写入验证通过，值: %1").arg(QString::fromWCharArray(buffer));
        RegCloseKey(hKey);
        return true;
    } else {
        qDebug() << QString("[AutoStartup] 注册表写入验证失败，错误码: %1").arg(lRes);
    }

    RegCloseKey(hKey);
    return false;
}

// 辅助：从 HKCU\...\Run 注册表删除条目
static bool deleteRunRegistry(const QString &entryName)
{
    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubKey, 0, KEY_SET_VALUE, &hKey);
    if (lRes != ERROR_SUCCESS) {
        qDebug() << QString("[AutoStartup] RegOpenKeyExW 失败，错误码: %1").arg(lRes);
        return false;
    }

    std::wstring wName = entryName.toStdWString();
    lRes = RegDeleteValueW(hKey, wName.c_str());
    if (lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND) {
        qDebug() << QString("[AutoStartup] RegDeleteValueW 失败，错误码: %1").arg(lRes);
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    return true;
}

// 辅助：检查注册表中是否存在条目
static bool existsRunRegistryEntry(const QString &entryName)
{
    HKEY hKey = NULL;
    LONG lRes = RegOpenKeyExW(HKEY_CURRENT_USER, kRunSubKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRes != ERROR_SUCCESS) return false;

    std::wstring wName = entryName.toStdWString();
    wchar_t buffer[2048] = {};
    DWORD bufferSize = sizeof(buffer);
    DWORD lType = REG_SZ;
    lRes = RegQueryValueExW(hKey, wName.c_str(), NULL, &lType, (BYTE*)buffer, &bufferSize);
    RegCloseKey(hKey);
    return (lRes == ERROR_SUCCESS);
}

// 辅助：清理系统中可能残留的 PlanThrough 开机自启项
static void cleanupAllAutoStartResidue()
{
    qDebug() << "[AutoStartup] 开始清理所有残留项...";

    // 1) 清理旧版 schtasks 创建的计划任务
    QStringList queryArgs = {"/query", "/fo", "CSV", "/v"};
    QProcess queryProc;
    queryProc.setProgram("schtasks");
    queryProc.setArguments(queryArgs);
    queryProc.setProcessChannelMode(QProcess::SeparateChannels);
    queryProc.start();
    if (queryProc.waitForFinished(5000)) {
        QString output = QString::fromLocal8Bit(queryProc.readAllStandardOutput());
        QStringList lines = output.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.contains("PlanThrough", Qt::CaseInsensitive)
                || line.contains("Plan_through", Qt::CaseInsensitive)) {
                int firstQuote = line.indexOf('"');
                int secondQuote = line.indexOf('"', firstQuote + 1);
                if (firstQuote >= 0 && secondQuote > firstQuote) {
                    QString name = line.mid(firstQuote + 1, secondQuote - firstQuote - 1);
                    qDebug() << "[AutoStartup] 发现残留计划任务:" << name << "尝试删除";
                    QProcess delProc;
                    delProc.setProgram("schtasks");
                    delProc.setArguments({"/delete", "/tn", name, "/f"});
                    delProc.setProcessChannelMode(QProcess::SeparateChannels);
                    delProc.start();
                    if (delProc.waitForFinished(3000)) {
                        int code = delProc.exitCode();
                        if (code == 0) {
                            qDebug() << "[AutoStartup] 已删除残留计划任务:" << name;
                        } else {
                            qDebug() << "[AutoStartup] 删除计划任务失败:" << name
                                     << QString::fromLocal8Bit(delProc.readAllStandardError()).trimmed();
                        }
                    }
                }
            }
        }
    }

    // 2) 清理注册表残留（使用 Windows API）
    QStringList candidates = {"PlanThroughStartup", "PlanThrough", "Plan_through", "Plan_through_Startup"};
    for (const QString &cand : candidates) {
        if (existsRunRegistryEntry(cand)) {
            deleteRunRegistry(cand);
            qDebug() << "[AutoStartup] 已清理注册表残留项:" << cand;
        }
    }

    // 3) 清理启动文件夹残留
    QString startupDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
                         + "/Startup";
    QDir dir(startupDir);
    if (dir.exists()) {
        QStringList filters = {"*.lnk", "*.url", "*.exe", "*.cmd", "*.bat"};
        QStringList entries = dir.entryList(filters, QDir::Files | QDir::NoSymLinks);
        for (const QString &entry : entries) {
            QString lower = entry.toLower();
            if (lower.contains("planthrough") || lower.contains("plan_through")) {
                QString path = dir.absoluteFilePath(entry);
                if (QFile::remove(path)) {
                    qDebug() << "[AutoStartup] 已清理启动文件夹残留项:" << path;
                }
            }
        }
    }

    qDebug() << "[AutoStartup] 残留清理完成";
}

// 辅助：获取并验证当前程序的 exe 路径
// 返回值：{ 是否成功, exe完整路径, exe所在目录 }
struct ExePathInfo {
    bool valid;
    QString exePath;
    QString dirPath;
};

static ExePathInfo getCurrentExePath()
{
    ExePathInfo info;
    info.valid = false;
    info.exePath.clear();
    info.dirPath.clear();

    QString exePath = QApplication::applicationFilePath();
    if (exePath.isEmpty()) {
        qDebug() << "[AutoStartup] 错误：无法获取程序路径";
        return info;
    }

    exePath = exePath.replace("/", "\\");
    QFileInfo fi(exePath);
    if (!fi.exists()) {
        qDebug() << QString("[AutoStartup] 错误：exe 不存在: %1").arg(exePath);
        return info;
    }

    if (!fi.isFile()) {
        qDebug() << QString("[AutoStartup] 错误：路径不是文件: %1").arg(exePath);
        return info;
    }

    info.valid = true;
    info.exePath = fi.absoluteFilePath();
    info.dirPath = fi.absolutePath();
    qDebug() << QString("[AutoStartup] 获取到 exe 路径: %1").arg(info.exePath);
    qDebug() << QString("[AutoStartup] exe 所在目录: %1").arg(info.dirPath);

    // 检查是否是开发环境（Qt Creator 启动）
    // 如果路径包含 "build" 或 "Qt Creator"，可能不是最终发布版本
    if (info.exePath.contains("/build/", Qt::CaseInsensitive)
        || info.exePath.contains("\\build\\", Qt::CaseInsensitive)) {
        qDebug() << "[AutoStartup] 警告：当前在开发环境中，设置开机自启的是构建目录中的 exe";
        qDebug() << "[AutoStartup] 发布到生产环境后请重新设置开机自启";
    }

    return info;
}

// 辅助：确保启动脚本存在
// 使用 wscript.exe 执行 VBS 脚本，完全隐藏命令行窗口
static bool ensureStartupScript()
{
    ExePathInfo exeInfo = getCurrentExePath();
    if (!exeInfo.valid) {
        qDebug() << "[AutoStartup] 无法创建启动脚本：exe 路径无效";
        return false;
    }

    // 创建 VBS 脚本
    QString vbsPath = QDir(exeInfo.dirPath).absoluteFilePath("autostart.vbs");
    QString batPath = QDir(exeInfo.dirPath).absoluteFilePath("autostart.bat");

    // 获取路径并替换斜杠
    QString dirPathStr = exeInfo.dirPath;
    dirPathStr.replace("/", "\\");
    QString exePathStr = exeInfo.exePath;
    exePathStr.replace("/", "\\");

    // 创建 VBS 脚本（用于开机自启，完全无窗口）
    {
        QFile file(vbsPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // VBS 中的字符串用双引号括起来，反斜杠不需要转义
            QString vbsContent;
            vbsContent += "Set ws = CreateObject(\"WScript.Shell\")\r\n";
            vbsContent += "ws.CurrentDirectory = \"" + dirPathStr + "\"\r\n";
            vbsContent += "ws.Run \"\"\"" + exePathStr + "\"\" --autostart\", 0, False\r\n";

            file.write(vbsContent.toUtf8());
            file.close();
            qDebug() << QString("[AutoStartup] 已创建 VBS 启动脚本: %1").arg(vbsPath);
        } else {
            qDebug() << QString("[AutoStartup] 创建 VBS 脚本失败: %1").arg(vbsPath);
            return false;
        }
    }

    // 同时创建 bat 脚本作为手动测试用（会显示窗口）
    {
        QFile file(batPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString batContent;
            batContent += "@echo off\r\n";
            batContent += "cd /d \"" + dirPathStr + "\"\r\n";
            batContent += "start \"\" /min \"" + exePathStr + "\" --autostart\r\n";

            file.write(batContent.toUtf8());
            file.close();
            qDebug() << QString("[AutoStartup] 已创建 BAT 测试脚本: %1").arg(batPath);
        }
    }

    return true;
}

// 设置或取消开机自启（通过注册表 HKCU Run 键实现，无需管理员权限）
static bool performAutoStartupOperation(const QString &operation)
{
    // 首先获取并验证 exe 路径
    ExePathInfo exeInfo = getCurrentExePath();
    if (!exeInfo.valid) {
        qDebug() << "[AutoStartup] 无法设置开机自启：exe 路径无效";
        return false;
    }

    QString entryName = "PlanThroughStartup";

    qDebug() << QString("[AutoStartup] 操作: %1").arg(operation);
    qDebug() << QString("[AutoStartup] exe 路径: %1").arg(exeInfo.exePath);
    qDebug() << QString("[AutoStartup] exe 目录: %1").arg(exeInfo.dirPath);

    if (operation == "disable") {
        cleanupAllAutoStartResidue();
        if (existsRunRegistryEntry(entryName)) {
            deleteRunRegistry(entryName);
            qDebug() << QString("[AutoStartup] 已删除注册表项: %1").arg(entryName);
        }
        qDebug() << "[AutoStartup] 开机自启已禁用";
        return true;
    }

    // operation == "enable"
    cleanupAllAutoStartResidue();

    // 确保启动脚本存在且路径正确
    if (!ensureStartupScript()) {
        qDebug() << "[AutoStartup] 无法设置开机自启：启动脚本创建失败";
        return false;
    }

    QString vbsPath = QDir(exeInfo.dirPath).absoluteFilePath("autostart.vbs");
    vbsPath = vbsPath.replace("/", "\\");

    // 先验证 VBS 脚本是否存在
    if (!QFile::exists(vbsPath)) {
        qDebug() << QString("[AutoStartup] 错误：VBS 脚本不存在: %1").arg(vbsPath);
        return false;
    }

    // 使用 wscript.exe 执行 VBS 脚本，完全隐藏窗口
    QString command = QString("wscript.exe \"%1\"").arg(vbsPath);
    qDebug() << QString("[AutoStartup] 注册表启动命令: %1").arg(command);
    qDebug() << QString("[AutoStartup] VBS 脚本路径: %1").arg(vbsPath);

    // 写入注册表
    bool ok = writeRunRegistry(entryName, command);
    if (ok) {
        qDebug() << "[AutoStartup] 开机自启已启用（注册表 Run 键 + VBS 无窗口启动）";
        qDebug() << "[AutoStartup] 下次启动将使用此 exe: " << exeInfo.exePath;
        return true;
    }

    qDebug() << "[AutoStartup] 开机自启启用失败";
    return false;
}

// 设置是否自动启动
void AppDatas::setAutoStartup(bool isAuto)
{
    qDebug() << "========== setAutoStartup 开始 ==========";
    m_isAutoStartup = isAuto;
    // 立即保存到配置文件
    m_appSettings->setValue("auto_startup", isAuto);
    m_appSettings->sync();
    qDebug() << "已保存 auto_startup =" << isAuto << "到 app_settings.ini";

    if (isAuto) {
        bool ok = performAutoStartupOperation("enable");
        if (ok) {
            qDebug() << "========== setAutoStartup 完成: 成功启用 ==========";
        } else {
            // 创建失败时回滚配置
            m_isAutoStartup = false;
            m_appSettings->setValue("auto_startup", false);
            m_appSettings->sync();
            qDebug() << "创建失败，已回滚配置为 auto_startup = false";
            qDebug() << "========== setAutoStartup 完成: 启用失败 ==========";
        }
    } else {
        performAutoStartupOperation("disable");
        qDebug() << "========== setAutoStartup 完成: 已禁用 ==========";
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

double AppDatas::getAverageStudyHoursPerDay() const
{
    int totalDays = getTotalStudyDays();
    if (totalDays == 0) {
        return 0.0;
    }
    return static_cast<double>(getTotalStudyHours()) / totalDays;
}

double AppDatas::getRecentMonthAverageStudyHoursPerDay() const
{
    QMap<QDate, DateStudyData> recentData = getRecentStudyData(30);
    int totalDays = 0;
    int totalHours = 0;
    
    QMap<QDate, DateStudyData>::const_iterator it = recentData.constBegin();
    while (it != recentData.constEnd()) {
        if (it.value().studyHours > 0) {
            totalDays++;
            totalHours += it.value().studyHours;
        }
        ++it;
    }
    
    if (totalDays == 0) {
        return 0.0;
    }
    return static_cast<double>(totalHours) / totalDays;
}

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

double AppDatas::getProjectCompletionRate() const
{
    int totalProjects = getTotalProjects();
    if (totalProjects == 0) {
        return 0.0;
    }
    return static_cast<double>(getCompletedProjects()) / totalProjects * 100.0;
}

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
