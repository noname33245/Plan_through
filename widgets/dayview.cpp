#include "dayview.h"
#include "monthview.h"
#include "./datastruct.h"
#include "./appdatas.h"
#include "./utils/datehelper.h"
#include "./utils/widgetcontainer.h"
#include <QStyle>

DayView::DayView(QWidget *parent)
    : QWidget{parent}
{
    widgetContainer("dayView",this);
    this->setObjectName("dayView");
    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    pageLayout->setObjectName("pageLayout");
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(10);  // 日视图间距紧凑

    // 第一行：日期相关按钮
    QHBoxLayout* dateBtnLayout = new QHBoxLayout;
    QPushButton* dateSelectBtn = new QPushButton("日期选择");
    QPushButton* todayBtn = new QPushButton("今日");
    QPushButton* clearBtn = new QPushButton("清除当日");
    clearBtn->setObjectName("clearBtn");

    dateBtnLayout->addWidget(dateSelectBtn);
    dateBtnLayout->addWidget(todayBtn);
    dateBtnLayout->addStretch();
    dateBtnLayout->addWidget(clearBtn);
    pageLayout->addLayout(dateBtnLayout);



    QGroupBox* progressGroup = new QGroupBox;
    progressGroup->setObjectName("progressGroup");

    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);

    QHBoxLayout* progressHeaderLayout = new QHBoxLayout;
    QLabel* progressTitle = new QLabel("📚 学习进度");
    progressTitle->setObjectName("progressTitle");
    m_selectedDateLabel = new QLabel();
    m_selectedDateLabel->setObjectName("selectedDateLabel");
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(DateHelper::currentDate().toString("yyyy年MM月dd日")));
    progressHeaderLayout->addWidget(progressTitle);
    progressHeaderLayout->addStretch();
    progressHeaderLayout->addWidget(m_selectedDateLabel);

    // 创建今日学习和设置目标的水平布局
    QHBoxLayout* todayStudyLayout = new QHBoxLayout;
    todayStudyLayout->setObjectName("todayStudyLayout");
    m_todayStudyHourLabel = new QLabel();
    m_todayStudyHourLabel->setObjectName("todayStudyHourLabel");
    // 使用HTML格式化文本，将目标部分设为绿色
    m_todayStudyHourLabel->setText(QString("今日学习：0小时 / <font color='#27AE60'>目标%1小时</font>").arg(appDatas.targetHour()));
    m_todayStudyHourLabel->setTextFormat(Qt::RichText);

    // 添加设置目标按钮
    QPushButton* setTargetBtn = new QPushButton("设置目标");
    setTargetBtn->setObjectName("setTargetBtn");

    todayStudyLayout->addWidget(m_todayStudyHourLabel);
    todayStudyLayout->addStretch();
    todayStudyLayout->addWidget(setTargetBtn);

    m_dayProgressBar = new QProgressBar;
    m_dayProgressBar->setAlignment(Qt::AlignCenter);
    m_dayProgressBar->setRange(0, appDatas.targetHour());
    m_dayProgressBar->setValue(0);

    progressLayout->addLayout(progressHeaderLayout);
    progressLayout->addLayout(todayStudyLayout);
    progressLayout->addWidget(m_dayProgressBar);
    pageLayout->addWidget(progressGroup);

    // 连接设置目标按钮信号
    connect(setTargetBtn, &QPushButton::clicked, this, &DayView::showSetTargetDialog);

    QGroupBox* statsGroup = new QGroupBox("📊 打卡统计");
    statsGroup->setObjectName("statsGroup");
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    statsLayout->setSpacing(10);  // 统计项间距紧凑
    m_continuousDaysLabel = new QLabel("当前连续天数：0");
    m_maxContinuousDaysLabel = new QLabel("最长连续天数：0");
    m_completedProjectsLabel = new QLabel("已完成项目：0/0");
    m_studyCheckLabel = new QLabel(QString("学习打卡：0/%1").arg(0));

    m_continuousDaysLabel->setObjectName("continuousDaysLabel");
    m_maxContinuousDaysLabel->setObjectName("maxContinuousDaysLabel");
    m_completedProjectsLabel->setObjectName("completedProjectsLabel");
    m_studyCheckLabel->setObjectName("studyCheckLabel");

    statsLayout->addWidget(m_continuousDaysLabel, 0, 0);
    statsLayout->addWidget(m_maxContinuousDaysLabel, 0, 1);
    statsLayout->addWidget(m_completedProjectsLabel, 1, 0);
    statsLayout->addWidget(m_studyCheckLabel, 1, 1);
    pageLayout->addWidget(statsGroup);

    QScrollArea* timeAxisScroll = new QScrollArea(this);
    timeAxisScroll->setWidgetResizable(true);
    timeAxisScroll->setObjectName("timeAxisScroll");
    m_timeAxisWidget = new TimeAxis(this);
    timeAxisScroll->setWidget(m_timeAxisWidget);
    pageLayout->addWidget(timeAxisScroll);

    connect(dateSelectBtn, &QPushButton::clicked, this, &DayView::showDateSelectDialog);
    connect(todayBtn, &QPushButton::clicked, this, &DayView::setToTodayDate);
    connect(setTargetBtn, &QPushButton::clicked, this, &DayView::showSetTargetDialog);
    connect(clearBtn, &QPushButton::clicked, this, &DayView::clearCurrentData);
}

DayView::~DayView(){

}

void DayView::updateDayViewStats()
{
    DateStudyData data = appDatas[DateHelper::currentDate()];
    int continuousDays = appDatas.calculateContinuousDays();
    appDatas.setMaxContinDays(qMax(appDatas.maxContinDays(), continuousDays));
    m_todayStudyHourLabel->setText(QString("今日学习：%1小时 / <font color='#27AE60'>目标%2小时</font>").arg(data.studyHours).arg(appDatas.targetHour()));
    m_todayStudyHourLabel->setTextFormat(Qt::RichText);
    if(data.studyHours >= appDatas.targetHour())
    {
        m_dayProgressBar->setValue(appDatas.targetHour());
    }
    else
    {
        m_dayProgressBar->setValue(data.studyHours);
    }

    m_continuousDaysLabel->setText(QString("当前连续天数：%1").arg(continuousDays));
    m_maxContinuousDaysLabel->setText(QString("最长连续天数：%1").arg(appDatas.maxContinDays()));
    m_completedProjectsLabel->setText(QString("已完成项目：%1/%2").arg(data.completedProjects).arg(data.totalProjects));
    m_studyCheckLabel->setText(QString("学习打卡：%1/%2").arg(data.studyHours).arg(appDatas.targetHour()));
}

void DayView::showDateSelectDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setObjectName("dateSelectDialog");
    dialog->setWindowTitle("选择日期");
    dialog->setModal(true);
    dialog->resize(260, 200);  // 弹窗尺寸紧凑

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(15,15,15,15);
    layout->setSpacing(8);
    QCalendarWidget* calendar = new QCalendarWidget;
    calendar->setSelectedDate(DateHelper::currentDate());

    layout->addWidget(calendar);

    QPushButton* confirmBtn = new QPushButton("确定");
    layout->addWidget(confirmBtn, 0, Qt::AlignCenter);
    connect(confirmBtn, &QPushButton::clicked, [=](){
        const QDate date = calendar->selectedDate();
        DateHelper::setCurrentDate(date);
        m_selectedDateLabel->setText(QString("当前日期：%1").arg(date.toString("yyyy年MM月dd日")));
        loadDateData(DateHelper::currentDate());
        updateDayViewStats();
        qobject_cast<MonthView*>(widgetContainer("monthView"))->switchMonth(DateHelper::calcCaleMonthDiff(date));
        dialog->close();
    });

    dialog->exec();
}

void DayView::setToTodayDate()
{
    DateHelper::resetDate();
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(DateHelper::currentDate().toString("yyyy年MM月dd日")));
    loadDateData(DateHelper::currentDate());
    updateDayViewStats();
    qobject_cast<MonthView*>(widgetContainer("monthView"))->switchMonth(0);
}

void DayView::showSetTargetDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setObjectName("setTargetDialog");
    dialog->setWindowTitle("设置每日学习目标");
    dialog->setModal(true);
    dialog->resize(240, 280);  // 弹窗尺寸紧凑

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(15,15,15,15);
    layout->setSpacing(8);

    QLabel* titleLabel = new QLabel("请选择每日学习小时数");
    layout->addWidget(titleLabel);

    QList<int> targetHours = {1,2,3,4,5,6,7,8};
    for (int hour : targetHours) {
        QPushButton* hourBtn = new QPushButton(QString("%1 小时").arg(hour));
        hourBtn->setObjectName("hourBtn");
        layout->addWidget(hourBtn);

        connect(hourBtn, &QPushButton::clicked, [=](){
                appDatas.setTargetHour(hour);
                m_todayStudyHourLabel->setText(QString("今日学习：%1小时 / <font color='#27AE60'>目标%2小时</font>").arg(appDatas[DateHelper::currentDate()].studyHours).arg(appDatas.targetHour()));
                m_todayStudyHourLabel->setTextFormat(Qt::RichText);
                m_dayProgressBar->setRange(0,appDatas.targetHour());
                if(appDatas[DateHelper::currentDate()].studyHours >= appDatas.targetHour())
                    m_dayProgressBar->setValue(appDatas.targetHour());
                else
                    m_dayProgressBar->setValue(appDatas[DateHelper::currentDate()].studyHours);
                m_dayProgressBar->update();
                dialog->close();
            });
    }

    QPushButton* cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::close);

    dialog->exec();
}

void DayView::clearCurrentData()
{
    appDatas[DateHelper::currentDate()] = DateStudyData();
    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for(int hour : hours)
    {
        QPushButton* btn = m_timeAxisWidget->operator[](hour);

        btn->setText("未安排");
        btn->setObjectName("unPlanned");
    }
    appDatas.saveDataToFile();
    loadDateData(DateHelper::currentDate());
    updateDayViewStats();
    qobject_cast<MonthView*>(widgetContainer("monthView"))->switchMonth(0);
    QMessageBox::information(this, "提示", "当日数据已清除！");
}

void DayView::loadDateData(const QDate& date)
{
    if (!appDatas.contains(date)) {
        appDatas[date] = DateStudyData();
    }
    DateStudyData data = appDatas[date];

    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for(int hour : hours)
    {
        QPushButton* btn = m_timeAxisWidget->operator[](hour);
        if(data.timeAxisData.contains(hour))
        {
            TimeAxisItem item = data.timeAxisData[hour];
            btn->setText(item.type);
        }
        else
        {
            btn->setText("未安排");
        }
        btn->setStyle(QApplication::style());
    }
}

void DayView::setProgress(int hour){
    m_dayProgressBar->setValue(hour);
}

void DayView::setProgressStyle(QString style){
    m_dayProgressBar->setStyleSheet(style);
}
