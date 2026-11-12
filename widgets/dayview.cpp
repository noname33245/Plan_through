#include "dayview.h"
#include "monthview.h"
#include "./datastruct.h"
#include "./appdatas.h"
#include "./utils/dialoganimator.h"
#include "./utils/theme.h"
#include <QEvent>
#include <QStyle>

DayView::DayView(QWidget *parent)
    : QWidget{parent}
{
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

    // 标题行不放进布局，而是浮在分组框上边线上（对齐 QGroupBox 标题的视觉位置），
    // 宽度随分组框变化，见 layoutProgressHeader()
    m_progressHeader = new QWidget(progressGroup);
    m_progressHeader->setObjectName("progressHeader");
    m_progressHeader->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    QHBoxLayout* progressHeaderLayout = new QHBoxLayout(m_progressHeader);
    progressHeaderLayout->setContentsMargins(10, 0, 10, 0);
    progressHeaderLayout->setSpacing(6);

    QLabel* progressTitle = new QLabel("📚 学习进度");
    progressTitle->setObjectName("progressTitle");
    // 固定 16px 高：让标题中线正好落在分组框上边线（margin-top=8）上，
    // QSS 里那条「上半页面色 / 下半卡片色」的渐变分界才能和边框线对齐
    progressTitle->setFixedHeight(16);

    m_selectedDateLabel = new QLabel();
    m_selectedDateLabel->setObjectName("selectedDateLabel");
    m_selectedDateLabel->setFixedHeight(16);
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(m_date.toString("yyyy年MM月dd日")));
    m_selectedDateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    progressHeaderLayout->addWidget(progressTitle);
    progressHeaderLayout->addStretch();
    progressHeaderLayout->addWidget(m_selectedDateLabel);

    progressGroup->installEventFilter(this);

    // 创建今日学习和设置目标的水平布局
    QHBoxLayout* todayStudyLayout = new QHBoxLayout;
    todayStudyLayout->setObjectName("todayStudyLayout");
    m_todayStudyHourLabel = new QLabel();
    m_todayStudyHourLabel->setObjectName("todayStudyHourLabel");
    // 使用HTML格式化文本，将目标部分设为绿色
    m_todayStudyHourLabel->setText(QString("今日学习：0小时 / <font color='%1'>目标%2小时</font>").arg(Theme::colors().success).arg(appDatas.targetHour()));
    m_todayStudyHourLabel->setTextFormat(Qt::RichText);

    // 添加设置目标按钮
    QPushButton* setTargetBtn = new QPushButton("设置目标");
    setTargetBtn->setObjectName("setTargetBtn");

    todayStudyLayout->addWidget(m_todayStudyHourLabel);
    todayStudyLayout->addStretch();
    todayStudyLayout->addWidget(setTargetBtn);

    m_dayProgressBar = new RoundProgressBar;
    m_dayProgressBar->setAlignment(Qt::AlignCenter);
    m_dayProgressBar->setRange(0, appDatas.targetHour() * PROGRESS_SCALE);
    m_dayProgressBar->setValue(0);
    m_dayProgressBar->setFormat("%p%");

    // 进度条动画：增加时弹性回弹，减少时平滑缓出（缓动曲线在 animateProgress 中按方向动态切换）
    m_progressAnimation = new QPropertyAnimation(m_dayProgressBar, "value", this);
    m_progressAnimation->setDuration(900);
    m_progressAnimation->setEasingCurve(QEasingCurve::OutCubic);

    progressLayout->addLayout(todayStudyLayout);
    progressLayout->addWidget(m_dayProgressBar);
    pageLayout->addWidget(progressGroup);

    // 首次按实际宽度摆好标题行
    layoutProgressHeader();



    QGroupBox* statsGroup = new QGroupBox("📊 打卡统计");
    statsGroup->setObjectName("statsGroup");
    QGridLayout* statsLayout = new QGridLayout(statsGroup);
    statsLayout->setSpacing(10);  // 统计项间距紧凑
    m_continuousDaysLabel = new QLabel("当前连续天数：0");
    m_maxContinuousDaysLabel = new QLabel("最长连续天数：0");
    m_completedProjectsLabel = new QLabel("已完成项目：0");
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

    m_timeAxisScroll = new QScrollArea(this);
    m_timeAxisScroll->setWidgetResizable(true);
    m_timeAxisScroll->setObjectName("timeAxisScroll");
    m_timeAxisWidget = new TimeAxis(this);
    m_timeAxisScroll->setWidget(m_timeAxisWidget);
    pageLayout->addWidget(m_timeAxisScroll);

    connect(dateSelectBtn, &QPushButton::clicked, this, &DayView::showDateSelectDialog);
    connect(todayBtn, &QPushButton::clicked, this, &DayView::setToTodayDate);
    connect(setTargetBtn, &QPushButton::clicked, this, &DayView::showSetTargetDialog);
    connect(clearBtn, &QPushButton::clicked, this, &DayView::clearCurrentData);
}

DayView::~DayView(){

}

// 把标题行摆到分组框顶部：文字中心正对分组框上边线
void DayView::layoutProgressHeader()
{
    if (!m_progressHeader) {
        return;
    }
    QWidget *group = m_progressHeader->parentWidget();
    if (!group) {
        return;
    }
    m_progressHeader->setGeometry(0, 0, group->width(), m_progressHeader->sizeHint().height());
}

bool DayView::eventFilter(QObject *watched, QEvent *event)
{
    if (m_progressHeader && watched == m_progressHeader->parentWidget()
        && (event->type() == QEvent::Resize || event->type() == QEvent::Show)) {
        layoutProgressHeader();
    }
    return QWidget::eventFilter(watched, event);
}

void DayView::setDate(const QDate &date)
{
    m_date = date;
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(m_date.toString("yyyy年MM月dd日")));
    loadDateData(m_date);
    updateDayViewStats();
}

// 时间轴起止小时变化后，重建整块时间轴
void DayView::rebuildTimeAxis()
{
    auto *pageLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!pageLayout) {
        return;
    }

    if (m_timeAxisScroll) {
        pageLayout->removeWidget(m_timeAxisScroll);
        delete m_timeAxisScroll;      // 连带里面的 TimeAxis 一起销毁
        m_timeAxisScroll = nullptr;
        m_timeAxisWidget = nullptr;
    }

    m_timeAxisScroll = new QScrollArea(this);
    m_timeAxisScroll->setWidgetResizable(true);
    m_timeAxisScroll->setObjectName("timeAxisScroll");
    m_timeAxisWidget = new TimeAxis(this);
    m_timeAxisScroll->setWidget(m_timeAxisWidget);
    pageLayout->addWidget(m_timeAxisScroll);

    loadDateData(m_date);
}

void DayView::updateDayViewStats()
{
    DateStudyData data = appDatas[m_date];
    int continuousDays = appDatas.calculateContinuousDays();
    appDatas.setMaxContinDays(qMax(appDatas.maxContinDays(), continuousDays));
    m_todayStudyHourLabel->setText(QString("今日学习：%1小时 / <font color='%2'>目标%3小时</font>").arg(data.studyHours).arg(Theme::colors().success).arg(appDatas.targetHour()));
    m_todayStudyHourLabel->setTextFormat(Qt::RichText);
    if(data.studyHours >= appDatas.targetHour())
    {
        animateProgress(appDatas.targetHour());
    }
    else
    {
        animateProgress(data.studyHours);
    }

    m_continuousDaysLabel->setText(QString("当前连续天数：%1").arg(continuousDays));
    m_maxContinuousDaysLabel->setText(QString("最长连续天数：%1").arg(appDatas.maxContinDays()));
    m_completedProjectsLabel->setText(QString("已完成项目：%1").arg(data.completedProjects));
    m_studyCheckLabel->setText(QString("学习打卡：%1/%2").arg(data.studyHours).arg(appDatas.targetHour()));

    emit statsChanged();
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
    calendar->setSelectedDate(m_date);
    // 日历配色由统一主题表提供（QCalendarWidget 全局规则），此处不再写死颜色

    layout->addWidget(calendar);

    QPushButton* confirmBtn = new QPushButton("确定");
    layout->addWidget(confirmBtn, 0, Qt::AlignCenter);
    connect(confirmBtn, &QPushButton::clicked, [=](){
        setDate(calendar->selectedDate());
        if (auto *mv = window() ? window()->findChild<MonthView*>("monthView") : nullptr) {
            mv->showMonthOf(m_date);
        }
        dialog->close();
    });

    DialogAnimator::applyFadeSlideIn(dialog);
    dialog->exec();
}

void DayView::setToTodayDate()
{
    setDate(QDate::currentDate());
    if (auto *mv = window() ? window()->findChild<MonthView*>("monthView") : nullptr) {
        mv->setToCurrentMonth();
    }
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
                m_todayStudyHourLabel->setText(QString("今日学习：%1小时 / <font color='%2'>目标%3小时</font>").arg(appDatas[m_date].studyHours).arg(Theme::colors().success).arg(appDatas.targetHour()));
                m_todayStudyHourLabel->setTextFormat(Qt::RichText);
                m_dayProgressBar->setRange(0, appDatas.targetHour() * PROGRESS_SCALE);
                if(appDatas[m_date].studyHours >= appDatas.targetHour())
                    animateProgress(appDatas.targetHour());
                else
                    animateProgress(appDatas[m_date].studyHours);
                m_dayProgressBar->update();
                dialog->close();
                dialog->deleteLater();
            });
    }

    QPushButton* cancelBtn = new QPushButton("取消");
    cancelBtn->setObjectName("cancelBtn");
    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, [=](){
        dialog->close();
        dialog->deleteLater();
    });

    // 处理对话框关闭事件
    connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);

    DialogAnimator::applyFadeSlideIn(dialog);
    dialog->exec();
}

void DayView::clearCurrentData()
{
    appDatas[m_date] = DateStudyData();
    const QList<int> hours = appDatas.dayHours();
    for(int hour : hours)
    {
        QPushButton* btn = m_timeAxisWidget->operator[](hour);

        btn->setText("未安排");
        btn->setObjectName("unPlanned");
    }
    appDatas.saveDataToFile();
    loadDateData(m_date);
    updateDayViewStats();
    if (auto *mv = window() ? window()->findChild<MonthView*>("monthView") : nullptr) {
        mv->generateMonthCalendar();
    }
    QMessageBox::information(this, "提示", "当日数据已清除！");
}

void DayView::loadDateData(const QDate& date)
{
    // 更新当前日期标签
    m_selectedDateLabel->setText(QString("当前日期：%1").arg(date.toString("yyyy年MM月dd日")));
    
    if (!appDatas.contains(date)) {
        appDatas[date] = DateStudyData();
    }
    DateStudyData data = appDatas[date];

    const QList<int> hours = appDatas.dayHours();
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
        // 文本变化后让 QSS 的 [text="..."] 属性选择器重新求值
        Theme::repolish(btn);
    }
}

void DayView::animateProgress(int targetHours)
{
    int scaledTarget = targetHours * PROGRESS_SCALE;
    // 目标不能超过范围
    int maxVal = m_dayProgressBar->maximum();
    if (scaledTarget > maxVal) scaledTarget = maxVal;
    if (scaledTarget < 0) scaledTarget = 0;

    if (m_progressAnimation->state() == QAbstractAnimation::Running) {
        m_progressAnimation->stop();
    }
    int currentValue = m_dayProgressBar->value();
    m_progressAnimation->setStartValue(currentValue);
    m_progressAnimation->setEndValue(scaledTarget);

    // 根据数值变化方向选择缓动曲线：
    // 增加 → 弹性回弹（OutElastic），减少 → 平滑缓出（OutCubic），避免回弹到0时生硬
    if (scaledTarget >= currentValue) {
        QEasingCurve elasticCurve(QEasingCurve::OutElastic);
        elasticCurve.setAmplitude(1.0);
        elasticCurve.setPeriod(0.4);
        m_progressAnimation->setEasingCurve(elasticCurve);
    } else {
        m_progressAnimation->setEasingCurve(QEasingCurve::OutCubic);
    }

    m_progressAnimation->start();
}
