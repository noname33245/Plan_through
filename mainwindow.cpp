#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_currentDate(QDate::currentDate())
    , m_currentYear(QDate::currentDate().year())
    , m_currentMonth(QDate::currentDate().month())
{
    initSavePath();
    initConfigFile();
    loadDataFromFile();
    loadConfigFromFile();
    initUI();
    loadDateData(m_currentDate);
    updateDayViewStats();
    generateMonthCalendar(m_currentYear, m_currentMonth);
    this->setStyleSheet("QMainWindow{background-color: #F5F7FA;border: none;}");
}

MainWindow::~MainWindow()
{
    saveDataToFile();
    saveConfigToFile();
}

void MainWindow::initSavePath()
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

void MainWindow::initConfigFile()
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

void MainWindow::saveConfigToFile()
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

void MainWindow::loadConfigFromFile()
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

void MainWindow::saveDataToFile()
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

void MainWindow::loadDataFromFile()
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

void MainWindow::initUI()
{
    this->setWindowTitle("学习计划打卡");
    this->resize(800, 700);

    QWidget* centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->setSpacing(20);

    QHBoxLayout* topTabLayout = new QHBoxLayout;
    m_dayViewBtn = new QPushButton("日视图");
    m_monthViewBtn = new QPushButton("月视图");
    QString topBtnStyle =
        "QPushButton{font-size:18px; font-weight:bold; padding:12px 40px; margin-right:15px; border-radius:12px; border:none; background-color:#FFFFFF; color:#2D8CF0;}"
        "QPushButton:checked{background-color:#2D8CF0; color:#FFFFFF;}"
        "QPushButton:hover:!checked{background-color:#ECF5FF; color:#1D7AD9;}"
        "QPushButton:pressed{background-color:#1D7AD9; color:#FFFFFF;}";
    m_dayViewBtn->setStyleSheet(topBtnStyle);
    m_monthViewBtn->setStyleSheet(topBtnStyle);
    m_dayViewBtn->setCheckable(true);
    m_monthViewBtn->setCheckable(true);
    m_dayViewBtn->setChecked(true);

    topTabLayout->addWidget(m_dayViewBtn);
    topTabLayout->addWidget(m_monthViewBtn);
    topTabLayout->addStretch();
    mainLayout->addLayout(topTabLayout);

    m_mainStackedWidget = new QStackedWidget;
    m_mainStackedWidget->addWidget(createDayViewPage());
    m_mainStackedWidget->addWidget(createMonthViewPage());
    mainLayout->addWidget(m_mainStackedWidget);

    connect(m_dayViewBtn, &QPushButton::clicked, this, &MainWindow::switchToDayView);
    connect(m_monthViewBtn, &QPushButton::clicked, this, &MainWindow::switchToMonthView);
}

QWidget* MainWindow::createDayViewPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(20);

    QHBoxLayout* dateLayout = new QHBoxLayout;
    QPushButton* dateSelectBtn = new QPushButton("日期选择");
    QPushButton* todayBtn = new QPushButton("今日");
    QPushButton* setTargetBtn = new QPushButton("设置目标");
    QPushButton* clearBtn = new QPushButton("清除当日数据");

    m_selectedDateLabel = new QLabel();
    m_selectedDateLabel->setStyleSheet("font-size:15px; font-weight:bold; color:#2D8CF0; padding:0 15px;");
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(m_currentDate.toString("yyyy年MM月dd日")));

    m_targetHourShowLabel = new QLabel();
    m_targetHourShowLabel->setStyleSheet("font-size:15px; font-weight:bold; color:#27AE60; padding:0 10px;");
    m_targetHourShowLabel->setText(QString("每日学习目标：%1 小时").arg(m_studyTargetHour));

    QString funcBtnStyle =
        "QPushButton{font-size:14px; font-weight:bold; padding:8px 20px; border-radius:8px; border:none; background-color:#FFFFFF; color:#333333;}"
        "QPushButton:hover{background-color:#F0F0F0;}"
        "QPushButton:pressed{background-color:#E0E0E0;}";
    dateSelectBtn->setStyleSheet(funcBtnStyle);
    todayBtn->setStyleSheet(funcBtnStyle);
    clearBtn->setStyleSheet("QPushButton{font-size:14px; font-weight:bold; padding:8px 20px; border-radius:8px; border:none; background-color:#FF6B6B; color:#FFFFFF;}"
                            "QPushButton:hover{background-color:#FF5252;}"
                            "QPushButton:pressed{background-color:#FF3B3B;}");
    setTargetBtn->setStyleSheet("QPushButton{font-size:14px; font-weight:bold; padding:8px 20px; border-radius:8px; border:none; background-color:#27AE60; color:#FFFFFF;}"
                                "QPushButton:hover{background-color:#219653;}"
                                "QPushButton:pressed{background-color:#1E8845;}");

    dateLayout->addWidget(dateSelectBtn);
    dateLayout->addWidget(todayBtn);
    dateLayout->addWidget(m_selectedDateLabel);
    dateLayout->addStretch();
    dateLayout->addWidget(m_targetHourShowLabel);
    dateLayout->addWidget(setTargetBtn);
    dateLayout->addWidget(clearBtn);
    pageLayout->addLayout(dateLayout);

    QGroupBox* progressGroup = new QGroupBox("📚 学习进度");
    progressGroup->setStyleSheet("QGroupBox{font-size:16px; font-weight:bold; color:#2D8CF0; border:2px solid #ECF5FF; border-radius:10px; padding:15px;}");
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);
    m_todayStudyHourLabel = new QLabel(QString("今日学习时间：0小时 / 目标%1小时").arg(m_studyTargetHour));
    m_todayStudyHourLabel->setStyleSheet("font-size:15px; font-weight:bold; color:#333333; padding:8px 0;");

    m_dayProgressBar = new QProgressBar;
    m_dayProgressBar->setAlignment(Qt::AlignCenter);
    // ============ 进度条文字颜色改为黑色 ============
    m_dayProgressBar->setStyleSheet(
        "QProgressBar{border:none; border-radius:8px; height:26px; background-color:#ECF5FF; font-size:14px; font-weight:bold; color:black;}"
        "QProgressBar::chunk{background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2D8CF0,stop:1 #1D7AD9); border-radius:8px;}");
    m_dayProgressBar->setRange(0, m_studyTargetHour);
    m_dayProgressBar->setValue(0);
    progressLayout->addWidget(m_todayStudyHourLabel);
    progressLayout->addWidget(m_dayProgressBar);
    pageLayout->addWidget(progressGroup);

    QGroupBox* statsGroup = new QGroupBox("📊 打卡统计");
    statsGroup->setStyleSheet("QGroupBox{font-size:16px; font-weight:bold; color:#2D8CF0; border:2px solid #ECF5FF; border-radius:10px; padding:15px;}");
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    statsLayout->setSpacing(20);
    m_continuousDaysLabel = new QLabel("当前连续天数：0");
    m_maxContinuousDaysLabel = new QLabel("最长连续天数：0");
    m_completedProjectsLabel = new QLabel("已完成项目：0/0");
    m_studyCheckLabel = new QLabel(QString("学习打卡：0/%1").arg(m_studyTargetHour));
    QString statLabelStyle = "font-size:15px; font-weight:bold; color:#555555; padding:5px;";
    m_continuousDaysLabel->setStyleSheet(statLabelStyle);
    m_maxContinuousDaysLabel->setStyleSheet(statLabelStyle);
    m_completedProjectsLabel->setStyleSheet(statLabelStyle);
    m_studyCheckLabel->setStyleSheet(statLabelStyle);
    statsLayout->addWidget(m_continuousDaysLabel, 0, 0);
    statsLayout->addWidget(m_maxContinuousDaysLabel, 0, 1);
    statsLayout->addWidget(m_completedProjectsLabel, 1, 0);
    statsLayout->addWidget(m_studyCheckLabel, 1, 1);
    pageLayout->addWidget(statsGroup);

    QScrollArea* timeAxisScroll = new QScrollArea(this);
    timeAxisScroll->setWidgetResizable(true);
    timeAxisScroll->setStyleSheet("QScrollArea{border:none; background-color:transparent;}"
                                  "QScrollBar:vertical{width:8px; background-color:#F5F7FA; border-radius:4px;}"
                                  "QScrollBar::handle:vertical{background-color:#C0C4CC; border-radius:4px;}"
                                  "QScrollBar::handle:vertical:hover{background-color:#909399;}");
    m_timeAxisWidget = createTimeAxis();
    timeAxisScroll->setWidget(m_timeAxisWidget);
    pageLayout->addWidget(timeAxisScroll);

    connect(dateSelectBtn, &QPushButton::clicked, this, &MainWindow::showDateSelectDialog);
    connect(todayBtn, &QPushButton::clicked, this, &MainWindow::setToTodayDate);
    connect(setTargetBtn, &QPushButton::clicked, this, &MainWindow::showSetTargetDialog);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCurrentData);

    return page;
}

QWidget* MainWindow::createMonthViewPage()
{
    QWidget* page = new QWidget;
    QVBoxLayout* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0,0,0,0);
    pageLayout->setSpacing(20);

    QHBoxLayout* monthLayout = new QHBoxLayout;
    QPushButton* prevMonthBtn = new QPushButton("◀ 上一月");
    QPushButton* nextMonthBtn = new QPushButton("下一月 ▶");
    QPushButton* currentMonthBtn = new QPushButton("当月");
    m_monthTitleLabel = new QLabel(QString("%1年%2月").arg(m_currentYear).arg(m_currentMonth));
    m_monthTitleLabel->setAlignment(Qt::AlignCenter);
    m_monthTitleLabel->setStyleSheet("font-size:18px; font-weight:bold; color:#2D8CF0; padding:0 20px;");
    QString monthBtnStyle =
        "QPushButton{font-size:14px; font-weight:bold; padding:8px 15px; border-radius:8px; border:none; background-color:#FFFFFF; color:#333333;}"
        "QPushButton:hover{background-color:#F0F0F0;}"
        "QPushButton:pressed{background-color:#E0E0E0;}";
    prevMonthBtn->setStyleSheet(monthBtnStyle);
    nextMonthBtn->setStyleSheet(monthBtnStyle);
    currentMonthBtn->setStyleSheet("QPushButton{font-size:14px; font-weight:bold; padding:8px 15px; border-radius:8px; border:none; background-color:#2D8CF0; color:#FFFFFF;}"
                                   "QPushButton:hover{background-color:#1D7AD9;}");
    monthLayout->addWidget(prevMonthBtn);
    monthLayout->addWidget(m_monthTitleLabel);
    monthLayout->addWidget(nextMonthBtn);
    monthLayout->addStretch();
    monthLayout->addWidget(currentMonthBtn);
    pageLayout->addLayout(monthLayout);

    QGroupBox* calendarGroup = new QGroupBox("📅 月度学习记录");
    calendarGroup->setStyleSheet("QGroupBox{font-size:16px; font-weight:bold; color:#2D8CF0; border:2px solid #ECF5FF; border-radius:10px; padding:15px;}");
    m_monthCalendarLayout = new QGridLayout(calendarGroup);
    m_monthCalendarLayout->setSpacing(8);
    QStringList weeks = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel* weekLab = new QLabel(weeks[i]);
        weekLab->setStyleSheet("font-size:14px; font-weight:bold; color:#2D8CF0; text-align:center;");
        weekLab->setAlignment(Qt::AlignCenter);
        m_monthCalendarLayout->addWidget(weekLab, 0, i, Qt::AlignCenter);
    }
    calendarGroup->setLayout(m_monthCalendarLayout);
    pageLayout->addWidget(calendarGroup);

    connect(prevMonthBtn, &QPushButton::clicked, [=](){ switchMonth(-1); });
    connect(nextMonthBtn, &QPushButton::clicked, [=](){ switchMonth(1); });
    connect(currentMonthBtn, &QPushButton::clicked, this, &MainWindow::setToCurrentMonth);

    return page;
}

QWidget* MainWindow::createTimeAxis()
{
    QWidget* widget = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setSpacing(10);
    layout->setContentsMargins(10,10,10,10);

    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for (int hour : hours) {
        QHBoxLayout* hourLayout = new QHBoxLayout;
        hourLayout->setSpacing(12);

        QLabel* timeLabel = new QLabel(QString("%1:00").arg(hour));
        timeLabel->setFixedWidth(70);
        timeLabel->setStyleSheet("font-size:14px; font-weight:bold; color:#2D8CF0; text-align:center;");
        timeLabel->setAlignment(Qt::AlignCenter);

        QPushButton* axisBtn = new QPushButton("未安排");
        axisBtn->setObjectName(QString::number(hour));
        axisBtn->setEnabled(true);
        axisBtn->setStyleSheet(
            "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#FFFFFF; color:#909399;}"
            "QPushButton:hover{background-color:#F8F9FA; color:#606266;}"
            "QPushButton:pressed{background-color:#F0F0F0;}"
            "QPushButton[text!=\"未安排\"]{background-color:#ECF5FF; color:#2D8CF0; font-weight:bold;}");
        axisBtn->setMinimumHeight(40);
        axisBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_timeAxisBtnMap.insert(hour, axisBtn);

        hourLayout->addWidget(timeLabel);
        hourLayout->addWidget(axisBtn);

        layout->addLayout(hourLayout);

        connect(axisBtn, &QPushButton::clicked, [=](){ onTimeAxisBtnClicked(hour); });
    }
    return widget;
}

void MainWindow::switchToDayView()
{
    m_mainStackedWidget->setCurrentIndex(0);
    m_dayViewBtn->setChecked(true);
    m_monthViewBtn->setChecked(false);
    updateDayViewStats();
}

void MainWindow::switchToMonthView()
{
    m_mainStackedWidget->setCurrentIndex(1);
    m_monthViewBtn->setChecked(true);
    m_dayViewBtn->setChecked(false);
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::onTimeAxisBtnClicked(int hour)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("选择事项");
    dialog->setModal(true);
    dialog->resize(300, 320);
    dialog->setStyleSheet("QDialog{background-color:#F5F7FA; border-radius:12px; border:none;}"
                          "QLabel{font-size:15px; font-weight:bold; color:#2D8CF0; padding:10px 0; text-align:center;}");

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20,20,20,20);
    layout->setSpacing(12);

    QLabel* titleLabel = new QLabel("请选择事项类型");
    layout->addWidget(titleLabel);

    QStringList types = {"学习", "吃饭", "睡觉", "洗澡", "游戏", "杂事"};
    for (const QString& type : types) {
        QPushButton* typeBtn = new QPushButton(type);
        typeBtn->setStyleSheet(
            "QPushButton{font-size:14px; padding:10px 0; border-radius:8px; border:none; background-color:#FFFFFF; color:#333333;}"
            "QPushButton:hover{background-color:#ECF5FF; color:#2D8CF0;}"
            "QPushButton:pressed{background-color:#2D8CF0; color:#FFFFFF;}");
        layout->addWidget(typeBtn);

        connect(typeBtn, &QPushButton::clicked, [=](){
            confirmTimeAxisItem(hour, type);
            dialog->close();
        });
    }

    QHBoxLayout* btnGroupLayout = new QHBoxLayout;
    btnGroupLayout->setSpacing(10);
    QPushButton* clearBtn = new QPushButton("清除");
    QPushButton* cancelBtn = new QPushButton("取消");
    clearBtn->setStyleSheet(
        "QPushButton{font-size:14px; font-weight:bold; padding:8px 0; border-radius:8px; border:none; background-color:#FF6B6B; color:#FFFFFF; width:100px;}"
        "QPushButton:hover{background-color:#FF5252;}"
        "QPushButton:pressed{background-color:#FF3B3B;}");
    cancelBtn->setStyleSheet(
        "QPushButton{font-size:14px; font-weight:bold; padding:8px 0; border-radius:8px; border:none; background-color:#C0C4CC; color:#FFFFFF; width:100px;}"
        "QPushButton:hover{background-color:#909399;}"
        "QPushButton:pressed{background-color:#606266;}");

    btnGroupLayout->addStretch();
    btnGroupLayout->addWidget(clearBtn);
    btnGroupLayout->addWidget(cancelBtn);
    btnGroupLayout->addStretch();
    layout->addLayout(btnGroupLayout);

    connect(clearBtn, &QPushButton::clicked, [=](){
        clearCurrentHourItem(hour);
        dialog->close();
    });
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::close);

    dialog->exec();
}

void MainWindow::showSetTargetDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("设置每日学习目标");
    dialog->setModal(true);
    dialog->resize(300, 360);
    dialog->setStyleSheet("QDialog{background-color:#F5F7FA; border-radius:12px; border:none;}"
                          "QLabel{font-size:15px; font-weight:bold; color:#27AE60; padding:10px 0; text-align:center;}");

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20,20,20,20);
    layout->setSpacing(12);

    QLabel* titleLabel = new QLabel("请选择每日学习小时数");
    layout->addWidget(titleLabel);

    QList<int> targetHours = {1,2,3,4,5,6,7,8};
    for (int hour : targetHours) {
        QPushButton* hourBtn = new QPushButton(QString("%1 小时").arg(hour));
        hourBtn->setStyleSheet(
            "QPushButton{font-size:14px; padding:10px 0; border-radius:8px; border:none; background-color:#FFFFFF; color:#333333;}"
            "QPushButton:hover{background-color:#F0F9F0; color:#27AE60;}"
            "QPushButton:pressed{background-color:#27AE60; color:#FFFFFF; font-weight:bold;}");
        layout->addWidget(hourBtn);

        connect(hourBtn, &QPushButton::clicked, [=](){
            setStudyTargetHour(hour);
            dialog->close();
        });
    }

    QPushButton* cancelBtn = new QPushButton("取消");
    cancelBtn->setStyleSheet(
        "QPushButton{font-size:14px; font-weight:bold; padding:8px 0; border-radius:8px; border:none; background-color:#C0C4CC; color:#FFFFFF; width:100px; margin-top:5px;}"
        "QPushButton:hover{background-color:#909399;}"
        "QPushButton:pressed{background-color:#606266;}");
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::close);

    dialog->exec();
}

void MainWindow::setStudyTargetHour(int targetHour)
{
    m_studyTargetHour = targetHour;
    m_targetHourShowLabel->setText(QString("每日学习目标：%1 小时").arg(m_studyTargetHour));
    m_dayProgressBar->setRange(0, m_studyTargetHour);
    saveConfigToFile();
    updateDayViewStats();
    QMessageBox::information(this, "设置成功", QString("每日学习目标已设置为 %1 小时！").arg(m_studyTargetHour));
}

void MainWindow::clearCurrentHourItem(int hour)
{
    DateStudyData& data = m_studyDataMap[m_currentDate];
    if (data.timeAxisData.contains(hour))
    {
        TimeAxisItem oldItem = data.timeAxisData[hour];
        if (oldItem.type == "学习" && oldItem.isCompleted) data.studyHours -= 1;
        if (oldItem.isCompleted) data.completedProjects -= 1;
        data.timeAxisData.remove(hour);
        data.totalProjects = data.timeAxisData.count();
    }

    QPushButton* btn = m_timeAxisBtnMap[hour];
    btn->setText("未安排");
    btn->setStyleSheet(
        "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#FFFFFF; color:#909399;}"
        "QPushButton:hover{background-color:#F8F9FA; color:#606266;}"
        "QPushButton:pressed{background-color:#F0F0F0;}");

    saveDataToFile();
    updateDayViewStats();
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::confirmTimeAxisItem(int hour, QString type)
{
    bool isCompleted = true;
    DateStudyData& data = m_studyDataMap[m_currentDate];

    if (data.timeAxisData.contains(hour))
    {
        TimeAxisItem oldItem = data.timeAxisData[hour];
        if (oldItem.type == "学习" && oldItem.isCompleted) data.studyHours -= 1;
        if (oldItem.isCompleted) data.completedProjects -= 1;
    }

    data.timeAxisData[hour] = {type, isCompleted};

    if(m_timeAxisBtnMap.contains(hour)){
        QPushButton* btn = m_timeAxisBtnMap[hour];
        btn->setText(type);
        btn->setStyleSheet(
            "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#ECF5FF; color:#2D8CF0; font-weight:bold;}"
            "QPushButton:hover{background-color:#E6F0FF;}"
            "QPushButton:pressed{background-color:#D9E8FF;}");
    }

    data.totalProjects = data.timeAxisData.count();
    if (type == "学习" && isCompleted) data.studyHours += 1;
    if (isCompleted) data.completedProjects += 1;

    saveDataToFile();
    updateDayViewStats();
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::showDateSelectDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("选择日期");
    dialog->setModal(true);
    dialog->resize(320, 240);
    dialog->setStyleSheet("QDialog{background-color:#F5F7FA; border-radius:12px; border:none;}"
                          "QCalendarWidget{background-color:#FFFFFF; border-radius:8px; border:1px solid #ECF5FF;}"
                          "QPushButton{font-size:14px; font-weight:bold; padding:8px 20px; border-radius:8px; border:none; background-color:#2D8CF0; color:#FFFFFF; margin-top:15px;}"
                          "QPushButton:hover{background-color:#1D7AD9;}");
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20,20,20,20);
    layout->setSpacing(15);

    QCalendarWidget* calendar = new QCalendarWidget;
    calendar->setSelectedDate(m_currentDate);
    calendar->setStyleSheet("QCalendarWidget{font-size:12px;}");
    layout->addWidget(calendar);

    QPushButton* confirmBtn = new QPushButton("确定");
    layout->addWidget(confirmBtn, 0, Qt::AlignCenter);
    connect(confirmBtn, &QPushButton::clicked, [=](){
        m_currentDate = calendar->selectedDate();
        m_currentYear = m_currentDate.year();
        m_currentMonth = m_currentDate.month();
        m_selectedDateLabel->setText(QString("当前日期：%1").arg(m_currentDate.toString("yyyy年MM月dd日")));
        loadDateData(m_currentDate);
        updateDayViewStats();
        generateMonthCalendar(m_currentYear, m_currentMonth);
        dialog->close();
    });

    dialog->exec();
}

void MainWindow::setToTodayDate()
{
    m_currentDate = QDate::currentDate();
    m_currentYear = m_currentDate.year();
    m_currentMonth = m_currentDate.month();
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(m_currentDate.toString("yyyy年MM月dd日")));
    loadDateData(m_currentDate);
    updateDayViewStats();
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::clearCurrentData()
{
    m_studyDataMap[m_currentDate] = DateStudyData();
    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for(int hour : hours)
    {
        QPushButton* btn = m_timeAxisBtnMap[hour];
        btn->setText("未安排");
        btn->setStyleSheet(
            "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#FFFFFF; color:#909399;}"
            "QPushButton:hover{background-color:#F8F9FA; color:#606266;}"
            "QPushButton:pressed{background-color:#F0F0F0;}");
    }
    saveDataToFile();
    loadDateData(m_currentDate);
    updateDayViewStats();
    generateMonthCalendar(m_currentYear, m_currentMonth);
    QMessageBox::information(this, "提示", "当日数据已清除！");
}

void MainWindow::switchMonth(int offset)
{
    m_currentMonth += offset;
    if (m_currentMonth < 1) {
        m_currentMonth = 12;
        m_currentYear -= 1;
    } else if (m_currentMonth > 12) {
        m_currentMonth = 1;
        m_currentYear += 1;
    }
    m_monthTitleLabel->setText(QString("%1年%2月").arg(m_currentYear).arg(m_currentMonth));
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::setToCurrentMonth()
{
    m_currentYear = QDate::currentDate().year();
    m_currentMonth = QDate::currentDate().month();
    m_monthTitleLabel->setText(QString("%1年%2月").arg(m_currentYear).arg(m_currentMonth));
    generateMonthCalendar(m_currentYear, m_currentMonth);
}

void MainWindow::generateMonthCalendar(int year, int month)
{
    QLayoutItem* item;
    while ((item = m_monthCalendarLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QStringList weeks = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel* weekLab = new QLabel(weeks[i]);
        weekLab->setStyleSheet("font-size:14px; font-weight:bold; color:#2D8CF0; text-align:center;");
        weekLab->setAlignment(Qt::AlignCenter);
        m_monthCalendarLayout->addWidget(weekLab, 0, i, Qt::AlignCenter);
    }

    QDate firstDay(year, month, 1);
    int startWeek = firstDay.dayOfWeek();
    startWeek = (startWeek == 7) ? 0 : startWeek;

    int daysInMonth = firstDay.daysInMonth();
    int row = 1;
    int col = startWeek;

    for (int day = 1; day <= daysInMonth; ++day) {
        QDate currentDate(year, month, day);
        DateStudyData data = m_studyDataMap.value(currentDate);

        QLabel* dayLabel = new QLabel(QString("%1\n%2h").arg(day).arg(data.studyHours));
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setFixedSize(65, 65);
        if (data.studyHours == 0) {
            dayLabel->setStyleSheet("background-color:#FFFFFF; border:1px solid #F0F0F0; border-radius:10px; font-size:13px; color:#909399;");
        } else if (data.studyHours >= m_studyTargetHour) {
            dayLabel->setStyleSheet("background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #27AE60,stop:1 #219653); color:white; border-radius:10px; font-size:13px; font-weight:bold;");
        } else {
            dayLabel->setStyleSheet("background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2D8CF0,stop:1 #1D7AD9); color:white; border-radius:10px; font-size:13px; font-weight:bold;");
        }

        m_monthCalendarLayout->addWidget(dayLabel, row, col, Qt::AlignCenter);
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
}

void MainWindow::loadDateData(const QDate& date)
{
    if (!m_studyDataMap.contains(date)) {
        m_studyDataMap[date] = DateStudyData();
    }
    DateStudyData data = m_studyDataMap[date];

    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for(int hour : hours)
    {
        QPushButton* btn = m_timeAxisBtnMap[hour];
        if(data.timeAxisData.contains(hour))
        {
            TimeAxisItem item = data.timeAxisData[hour];
            btn->setText(item.type);
            btn->setStyleSheet(
                "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#ECF5FF; color:#2D8CF0; font-weight:bold;}"
                "QPushButton:hover{background-color:#E6F0FF;}"
                "QPushButton:pressed{background-color:#D9E8FF;}");
        }
        else
        {
            btn->setText("未安排");
            btn->setStyleSheet(
                "QPushButton{font-size:14px; padding:10px 0; border-radius:15px; border:none; background-color:#FFFFFF; color:#909399;}"
                "QPushButton:hover{background-color:#F8F9FA; color:#606266;}"
                "QPushButton:pressed{background-color:#F0F0F0;}");
        }
    }
}

void MainWindow::updateDayViewStats()
{
    DateStudyData data = m_studyDataMap[m_currentDate];
    int continuousDays = calculateContinuousDays();
    m_maxContinuousDays = qMax(m_maxContinuousDays, continuousDays);

    m_todayStudyHourLabel->setText(QString("今日学习时间：%1小时 / 目标%2小时").arg(data.studyHours).arg(m_studyTargetHour));
    if(data.studyHours >= m_studyTargetHour)
    {
        m_dayProgressBar->setValue(m_studyTargetHour);
    }
    else
    {
        m_dayProgressBar->setValue(data.studyHours);
    }

    m_continuousDaysLabel->setText(QString("当前连续天数：%1").arg(continuousDays));
    m_maxContinuousDaysLabel->setText(QString("最长连续天数：%1").arg(m_maxContinuousDays));
    m_completedProjectsLabel->setText(QString("已完成项目：%1/%2").arg(data.completedProjects).arg(data.totalProjects));
    m_studyCheckLabel->setText(QString("学习打卡：%1/%2").arg(data.studyHours).arg(m_studyTargetHour));
}

int MainWindow::calculateContinuousDays()
{
    int days = 0;
    QDate current = QDate::currentDate();
    while (m_studyDataMap.contains(current) && m_studyDataMap[current].studyHours > 0) {
        days++;
        current = current.addDays(-1);
    }
    return days;
}
