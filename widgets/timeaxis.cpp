#include "timeaxis.h"
#include "dayview.h"
#include "./datastruct.h"
#include "./appdatas.h"
#include "./utils/datehelper.h"
#include "./utils/widgetcontainer.h"
#include "monthview.h"

TimeAxis::TimeAxis(QWidget *parent)
    : QWidget{parent}
{
    this->setObjectName("timeAxis");
    widgetContainer("timeAxis",this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(6);   // 时间轴小时项间距紧凑
    layout->setContentsMargins(3, 6, 3, 6);

    QList<int> hours = {8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
    for (int hour : hours) {
        QHBoxLayout* hourLayout = new QHBoxLayout;
        hourLayout->setSpacing(5);

        QLabel* timeLabel = new QLabel(QString("%1:00").arg(hour));
        timeLabel->setObjectName("timeLabel");
        timeLabel->setMinimumWidth(45);
        timeLabel->setFixedWidth(50);
        timeLabel->setAlignment(Qt::AlignCenter);

        QPushButton* axisBtn = new QPushButton("未安排");
        axisBtn->setObjectName(QString::number(hour));
        axisBtn->setEnabled(true);
        axisBtn->setMinimumHeight(30);  // 时间轴按钮高度压缩
        axisBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_timeAxisBtnMap.insert(hour, axisBtn);

        hourLayout->addWidget(timeLabel);
        hourLayout->addWidget(axisBtn);
        hourLayout->setStretchFactor(axisBtn, 1);

        layout->addLayout(hourLayout);

        connect(axisBtn, &QPushButton::clicked, [=](){ onTimeAxisBtnClicked(hour); });
    }
}

void TimeAxis::onTimeAxisBtnClicked(int hour)
{
    QDialog* dialog = new QDialog(this);
    dialog->setObjectName("timeAxisBtnDialog");
    dialog->setWindowTitle("选择事项");
    dialog->setModal(true);
    dialog->resize(240, 260);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(15,15,15,15);
    layout->setSpacing(8);

    QLabel* titleLabel = new QLabel("请选择事项类型");
    layout->addWidget(titleLabel);

    QStringList types = {"学习", "吃饭", "睡觉", "洗澡", "游戏", "杂事"};
    for (const QString& type : types) {
        QPushButton* typeBtn = new QPushButton(type);
        layout->addWidget(typeBtn);

        connect(typeBtn, &QPushButton::clicked, [=](){
            confirmTimeAxisItem(hour, type);
            dialog->close();
        });
    }

    QHBoxLayout* btnGroupLayout = new QHBoxLayout;
    btnGroupLayout->setSpacing(8);
    QPushButton* clearBtn = new QPushButton("清除");
    QPushButton* cancelBtn = new QPushButton("取消");
    clearBtn->setObjectName("clearBtn");
    cancelBtn->setObjectName("cancelBtn");

    btnGroupLayout->addStretch();
    btnGroupLayout->addWidget(clearBtn);
    btnGroupLayout->addWidget(cancelBtn);
    btnGroupLayout->addStretch();
    layout->addLayout(btnGroupLayout);

    connect(clearBtn, &QPushButton::clicked, this, [=](){
        clearCurrentHourItem(hour);
        dialog->close();
    });
    connect(cancelBtn, &QPushButton::clicked, dialog, &QDialog::close);

    dialog->exec();
}

void TimeAxis::confirmTimeAxisItem(int hour, const QString& type)
{
    bool isCompleted = true;
    DateStudyData& data = appDatas[DateHelper::currentDate()];

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
        btn->setStyle(QApplication::style());
    }

    data.totalProjects = data.timeAxisData.count();
    if (type == "学习" && isCompleted) data.studyHours += 1;
    if (isCompleted) data.completedProjects += 1;

    appDatas.saveDataToFile();
    qobject_cast<DayView*>(widgetContainer("dayView"))->updateDayViewStats();
    qobject_cast<MonthView*>(widgetContainer("monthView"))->generateMonthCalendar();
}

void TimeAxis::clearCurrentHourItem(int hour)
{
    DateStudyData& data = appDatas[DateHelper::currentDate()];
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
    btn->setStyle(QApplication::style());

    appDatas.saveDataToFile();
    qobject_cast<DayView*>(widgetContainer("dayView"))->updateDayViewStats();
    qobject_cast<MonthView*>(widgetContainer("monthView"))->generateMonthCalendar();
}

QPushButton* TimeAxis::operator[](int hour){
    return m_timeAxisBtnMap[hour];
}
