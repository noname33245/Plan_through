#include "monthview.h"
#include "./utils/datehelper.h"
#include "./appdatas.h"
#include "./utils/widgetcontainer.h"
#include "dayview.h"

MonthView::MonthView(QWidget *parent)
    : QWidget{parent}
{
    widgetContainer("monthView", this);
    this->setObjectName("monthView");
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(10);  // 月视图间距紧凑

    // 月份切换和标题布局
    QHBoxLayout* monthLayout = new QHBoxLayout;
    QPushButton* prevMonthBtn = new QPushButton("◀ 上月");
    QPushButton* nextMonthBtn = new QPushButton("下月 ▶");
    QPushButton* currentMonthBtn = new QPushButton("当月");
    QPushButton* statisticsBtn = new QPushButton("学习统计");
    m_monthTitleLabel = new QLabel(QString("%1年%2月").arg(DateHelper::currentYear()).arg(DateHelper::currentMonth()));
    m_monthTitleLabel->setObjectName("monthTitleLabel");
    m_monthTitleLabel->setAlignment(Qt::AlignCenter);
    
    // 按钮样式
    prevMonthBtn->setObjectName("monthBtn");
    nextMonthBtn->setObjectName("monthBtn");
    currentMonthBtn->setObjectName("currentMonthBtn");
    statisticsBtn->setObjectName("statisticsBtn");
    
    monthLayout->addWidget(prevMonthBtn);
    monthLayout->addWidget(m_monthTitleLabel);
    monthLayout->addWidget(nextMonthBtn);
    monthLayout->addStretch();
    monthLayout->addWidget(currentMonthBtn);
    monthLayout->addWidget(statisticsBtn);
    
    // 学习统计对话框
    connect(statisticsBtn, &QPushButton::clicked, [=]() {
        QDialog *statsDlg = new QDialog(this);
        statsDlg->setWindowTitle("学习统计");
        statsDlg->setFixedSize(800, 800);
        statsDlg->setModal(true);
        // 禁用所有可能的窗口动画效果
        statsDlg->setAttribute(Qt::WA_NoSystemBackground, false);
        statsDlg->setAttribute(Qt::WA_DontShowOnScreen, false);
        statsDlg->setAttribute(Qt::WA_TranslucentBackground, false);
        statsDlg->setWindowOpacity(1.0);

        // 设置统计对话框样式
        statsDlg->setObjectName("statsDlg");

        // 创建滚动区域
        QScrollArea *scrollArea = new QScrollArea(statsDlg);
        scrollArea->setWidgetResizable(true);
        
        // 创建容器widget
        QWidget *scrollContent = new QWidget();
        QVBoxLayout *statsLayout = new QVBoxLayout(scrollContent);
        statsLayout->setSpacing(10);
        statsLayout->setContentsMargins(15, 15, 15, 15);
        
        // 设置对话框布局
        QVBoxLayout *dlgLayout = new QVBoxLayout(statsDlg);
        dlgLayout->setContentsMargins(0, 0, 0, 0);
        dlgLayout->addWidget(scrollArea);

        // 学习时长统计
        QGroupBox *studyHoursGroup = new QGroupBox("学习统计");
        QGridLayout *studyHoursLayout = new QGridLayout(studyHoursGroup);
        studyHoursLayout->setSpacing(8);
        studyHoursLayout->setContentsMargins(10, 10, 10, 10);

        studyHoursLayout->addWidget(new QLabel("总学习天数："), 0, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getTotalStudyDays()) + " 天"), 0, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("总学习时长："), 1, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getTotalStudyHours()) + " 小时"), 1, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("平均每天学习时长："), 2, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getAverageStudyHoursPerDay(), 'f', 1) + " 小时（仅统计当日有学习记录的天数）"), 2, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("完成项目数："), 3, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getCompletedProjects()) + " 个"), 3, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("最大连续学习天数："), 4, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.maxContinDays()) + " 天"), 4, 1, 1, 1, Qt::AlignLeft);

        // 最近30天学习趋势折线图
        QGroupBox *lineChartGroup = new QGroupBox("最近30天学习趋势");
        QVBoxLayout *lineChartLayout = new QVBoxLayout(lineChartGroup);
        lineChartLayout->setContentsMargins(10, 10, 10, 10);
        
        QChart *lineChart = new QChart();
        lineChart->setTitle("学习时长趋势（小时）");
        lineChart->setAnimationOptions(QChart::SeriesAnimations);
        
        QLineSeries *lineSeries = new QLineSeries();
        lineSeries->setName("学习时长");
        
        QDateTimeAxis *lineAxisX = new QDateTimeAxis();
        lineAxisX->setFormat("MM-dd");
        lineAxisX->setTitleText("日期");
        
        QValueAxis *lineAxisY = new QValueAxis();
        lineAxisY->setTitleText("小时");
        lineAxisY->setMin(0);
        lineAxisY->setMax(8);
        
        // 获取最近30天的学习数据
        QMap<QDate, DateStudyData> monthData = appDatas.getRecentStudyData(30);
        QList<QDate> monthDates = monthData.keys();
        std::sort(monthDates.begin(), monthDates.end());
        
        for (const QDate &date : monthDates) {
            QDateTime dateTime;
            dateTime.setDate(date);
            int hours = monthData[date].studyHours;
            lineSeries->append(dateTime.toMSecsSinceEpoch(), hours);
        }
        
        lineChart->addSeries(lineSeries);
        lineChart->addAxis(lineAxisX, Qt::AlignBottom);
        lineChart->addAxis(lineAxisY, Qt::AlignLeft);
        lineSeries->attachAxis(lineAxisX);
        lineSeries->attachAxis(lineAxisY);
        
        QChartView *lineChartView = new QChartView(lineChart);
        lineChartView->setRenderHint(QPainter::Antialiasing);
        lineChartView->setMinimumHeight(150);
        lineChartLayout->addWidget(lineChartView);

        statsLayout->addWidget(studyHoursGroup);
        statsLayout->addWidget(lineChartGroup);

        // 关闭按钮
        QHBoxLayout *closeLayout = new QHBoxLayout;
        QPushButton *closeBtn = new QPushButton("关闭");
        closeBtn->setObjectName("closeBtn");
        closeLayout->addStretch();
        closeLayout->addWidget(closeBtn);
        closeLayout->addStretch();

        statsLayout->addLayout(closeLayout);

        // 设置滚动区域内容
        scrollArea->setWidget(scrollContent);

        connect(closeBtn, &QPushButton::clicked, statsDlg, &QDialog::close);

        statsDlg->exec();
    });
    pageLayout->addLayout(monthLayout);

    // 日历主体
    QGroupBox* calendarGroup = new QGroupBox("📅 月度学习记录");
    calendarGroup->setObjectName("calendarGroup");
    m_monthCalendarLayout = new QGridLayout(calendarGroup);
    m_monthCalendarLayout->setSpacing(4);  // 日历单元格间距极致紧凑
    
    // 星期标题
    QStringList weeks = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel* weekLab = new QLabel(weeks[i]);
        weekLab->setStyleSheet("font-size:12px; font-weight:bold; color:#2D8CF0; text-align:center;");
        weekLab->setAlignment(Qt::AlignCenter);
        m_monthCalendarLayout->addWidget(weekLab, 0, i, Qt::AlignCenter);
    }
    calendarGroup->setLayout(m_monthCalendarLayout);
    pageLayout->addWidget(calendarGroup);

    // 连接信号槽
    connect(prevMonthBtn, &QPushButton::clicked, [=](){ switchMonth(-1); });
    connect(nextMonthBtn, &QPushButton::clicked, [=](){ switchMonth(1); });
    connect(currentMonthBtn, &QPushButton::clicked, this, &MonthView::setToCurrentMonth);
}

// 切换月份
// @param offset 月份偏移量，正数为下一个月，负数为上一个月
void MonthView::switchMonth(int offset)
{
    DateHelper::addCaleMonth(offset);
    m_monthTitleLabel->setText(QString("%1年%2月").arg(DateHelper::caleYear()).arg(DateHelper::caleMonth()));
    generateMonthCalendar();
}

// 生成月历
void MonthView::generateMonthCalendar()
{
    const int year = DateHelper::caleYear(), month = DateHelper::caleMonth();
    
    // 清空现有日历项
    QLayoutItem* item;
    while ((item = m_monthCalendarLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // 清空日期标签映射
    m_dateLabelMap.clear();

    // 重新添加星期标题
    QStringList weeks = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel* weekLab = new QLabel(weeks[i]);
        weekLab->setStyleSheet("font-size:12px;font-weight:bold;color:#2D8CF0;text-align:center;");
        weekLab->setAlignment(Qt::AlignCenter);
        m_monthCalendarLayout->addWidget(weekLab, 0, i, Qt::AlignCenter);
    }

    // 获取当月第一天和起始星期
    QDate firstDay(year, month, 1);
    int startWeek = firstDay.dayOfWeek();
    startWeek = (startWeek == 7) ? 0 : startWeek;

    int daysInMonth = firstDay.daysInMonth();
    int row = 1;
    int col = startWeek;

    // 获取当前日期
    QDate today = QDate::currentDate();
    
    // 生成日期标签
    for (int day = 1; day <= daysInMonth; ++day) {
        QDate currentDate(year, month, day);
        DateStudyData data = appDatas.value(currentDate);

        QLabel* dayLabel = new QLabel(QString("%1\n%2h").arg(day).arg(data.studyHours));
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setFixedSize(48, 48);  // 日历单元格尺寸紧凑压缩
        dayLabel->setCursor(Qt::PointingHandCursor); // 设置鼠标指针为手型
        
        // 检查是否为当天日期
        bool isToday = (currentDate == today);
        
        // 根据学习时长和是否为当天日期设置不同的背景色
        if (isToday) {
            // 当天日期的特殊样式
            dayLabel->setStyleSheet("background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF6B6B,stop:1 #EE5A24);color:white;border-radius:8px;font-size:12px;font-weight:bold;border:2px solid #FFD700;");
        } else if (data.studyHours == 0) {
            dayLabel->setStyleSheet("background-color:#FFFFFF;border:1px solid #F0F0F0;border-radius:8px;font-size:11px;color:#909399;");
        } else if (data.studyHours >= appDatas.targetHour()) {
            dayLabel->setStyleSheet("background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #27AE60,stop:1 #219653);color:white;border-radius:8px;font-size:11px;font-weight:bold;");
        } else {
            dayLabel->setStyleSheet("background-color:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #2D8CF0,stop:1 #1D7AD9);color:white;border-radius:8px;font-size:11px;font-weight:bold;");
        }

        // 为日期标签安装事件过滤器
        dayLabel->installEventFilter(this);
        // 存储日期和标签的映射关系
        m_dateLabelMap[dayLabel] = currentDate;

        m_monthCalendarLayout->addWidget(dayLabel, row, col, Qt::AlignCenter);
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
}

// 设置为当前月份
void MonthView::setToCurrentMonth()
{
    DateHelper::resetDate();
    m_monthTitleLabel->setText(QString("%1年%2月").arg(DateHelper::currentYear()).arg(DateHelper::currentMonth()));
    generateMonthCalendar();
}

// 事件过滤器，用于处理日期标签的点击事件
// @param watched 被监视的对象
// @param event 事件对象
// @return 是否处理了该事件
bool MonthView::eventFilter(QObject *watched, QEvent *event)
{
    // 检查事件类型是否为鼠标按下事件
    if (event->type() == QEvent::MouseButtonPress) {
        // 检查被点击的对象是否是QLabel，并且在我们的日期标签映射中
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && m_dateLabelMap.contains(label)) {
            // 获取对应的日期
            QDate clickedDate = m_dateLabelMap[label];
            
            // 设置当前日期
            DateHelper::setCurrentDate(clickedDate);
            
            // 直接调用widgetContainer获取主窗口对象，通过QMetaObject::invokeMethod调用switchToDayView
            QObject *mainWindow = widgetContainer("main");
            if (mainWindow) {
                QMetaObject::invokeMethod(mainWindow, "switchToDayView");
            }
            
            // 更新日视图数据
            DayView *dayView = qobject_cast<DayView*>(widgetContainer("dayView"));
            if (dayView) {
                dayView->loadDateData(clickedDate);
                dayView->updateDayViewStats();
            }
            
            return true; // 事件已处理
        }
    }
    
    // 否则，继续传递事件
    return QWidget::eventFilter(watched, event);
}
