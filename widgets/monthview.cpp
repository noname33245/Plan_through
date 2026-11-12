#include "monthview.h"
#include "dayview.h"
#include "heatmapwidget.h"
#include "./appdatas.h"
#include "./utils/dialoganimator.h"
#include "./utils/theme.h"
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>

MonthView::MonthView(QWidget *parent)
    : QWidget{parent}
{
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
    m_monthTitleLabel = new QLabel(QString("%1年%2月").arg(QDate::currentDate().year()).arg(QDate::currentDate().month()));
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
        studyHoursLayout->addWidget(new QLabel("近一月平均每天学习时长："), 2, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getRecentMonthAverageStudyHoursPerDay(), 'f', 1) + " 小时（不含今日，按有记录天数计）"), 2, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("完成项目数："), 3, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.getCompletedProjects()) + " 个"), 3, 1, 1, 1, Qt::AlignLeft);
        studyHoursLayout->addWidget(new QLabel("最大连续学习天数："), 4, 0, 1, 1, Qt::AlignRight);
        studyHoursLayout->addWidget(new QLabel(QString::number(appDatas.maxContinDays()) + " 天"), 4, 1, 1, 1, Qt::AlignLeft);

        // 最近30天学习趋势折线图
        QGroupBox *lineChartGroup = new QGroupBox("最近30天学习趋势");
        QVBoxLayout *lineChartLayout = new QVBoxLayout(lineChartGroup);
        lineChartLayout->setContentsMargins(6, 6, 6, 6);
        
        // groupbox 已经有标题，图表内不再重复；图例只有一个系列也没信息量。
        // 这几处省下的纵向空间全部让给绘图区，避免曲线被压成一条直线
        QChart *lineChart = new QChart();
        lineChart->setAnimationOptions(QChart::SeriesAnimations);
        lineChart->legend()->setVisible(false);

        // 图表不跟随系统配色，统一取当前主题色
        {
            const Theme::Colors &tc = Theme::colors();
            lineChart->setBackgroundBrush(QBrush(QColor(tc.cardBg)));
            lineChart->setBackgroundPen(Qt::NoPen);
            lineChart->setBackgroundRoundness(0);
            lineChart->setTitleBrush(QBrush(QColor(tc.text)));
            lineChart->setPlotAreaBackgroundVisible(false);
            // 默认外边距 20px，会让本来就不大的绘图区再缩一圈
            lineChart->setMargins(QMargins(6, 6, 6, 6));
        }

        // 使用 QSplineSeries 让折线更圆滑（三次样条插值）
        QSplineSeries *lineSeries = new QSplineSeries();
        lineSeries->setName("学习时长");

        QDateTimeAxis *lineAxisX = new QDateTimeAxis();
        lineAxisX->setFormat("MM-dd");
        lineAxisX->setLabelsBrush(QBrush(QColor(Theme::colors().textSecondary)));
        lineAxisX->setGridLineColor(QColor(Theme::colors().borderSoft));
        lineAxisX->setLinePenColor(QColor(Theme::colors().border));

        // 获取最近30天的学习数据
        QMap<QDate, DateStudyData> monthData = appDatas.getRecentStudyData(30);
        QList<QDate> monthDates = monthData.keys();
        std::sort(monthDates.begin(), monthDates.end());

        int maxHours = 0;
        for (const QDate &date : monthDates) {
            QDateTime dateTime;
            dateTime.setDate(date);
            const int hours = monthData[date].studyHours;
            lineSeries->append(dateTime.toMSecsSinceEpoch(), hours);
            maxHours = qMax(maxHours, hours);
        }

        // 30 个日期标签塞不下时 Qt 会把它们缩成 "..."，限制刻度数量即可
        lineAxisX->setTickCount(qBound(2, monthDates.size(), 6));

        QValueAxis *lineAxisY = new QValueAxis();
        // 最大值至少给到 4 小时，否则数据都很小时曲线会被放大得失去参考价值
        lineAxisY->setRange(0, qMax(4, maxHours + 1));
        lineAxisY->setTickCount(5);
        lineAxisY->setLabelsBrush(QBrush(QColor(Theme::colors().textSecondary)));
        lineAxisY->setGridLineColor(QColor(Theme::colors().borderSoft));
        lineAxisY->setLinePenColor(QColor(Theme::colors().border));

        lineChart->addSeries(lineSeries);
        lineChart->addAxis(lineAxisX, Qt::AlignBottom);
        lineChart->addAxis(lineAxisY, Qt::AlignLeft);
        lineSeries->attachAxis(lineAxisX);
        lineSeries->attachAxis(lineAxisY);

        // 全为 0 时画出来就是一条贴底的直线，给句提示更直观
        if (maxHours == 0) {
            lineChart->setTitle("最近 30 天暂无学习记录");
        }

        // 线条加粗 + 圆形数据点，让曲线更清晰
        QPen pen(QColor(Theme::colors().accent));
        pen.setWidth(2);
        lineSeries->setPen(pen);
        lineSeries->setPointLabelsVisible(false);
        lineSeries->setPointsVisible(true);
        lineSeries->setMarkerSize(5.0);

        QChartView *lineChartView = new QChartView(lineChart);
        lineChartView->setRenderHint(QPainter::Antialiasing);
        lineChartView->setMinimumHeight(240);
        lineChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        lineChartLayout->addWidget(lineChartView);

        statsLayout->addWidget(studyHoursGroup);
        statsLayout->addWidget(lineChartGroup);

        // 近一年打卡热力图
        const QMap<QDate, DateStudyData> yearData = appDatas.getRecentStudyData(365);
        const Theme::Colors &tc = Theme::colors();

        auto *heatmapGroup = new QGroupBox("近一年打卡热力图");
        auto *heatmapLayout = new QVBoxLayout(heatmapGroup);
        heatmapLayout->setContentsMargins(10, 10, 10, 10);
        auto *heatmap = new HeatmapWidget;
        {
            QMap<QDate, int> daily;
            for (auto it = yearData.constBegin(); it != yearData.constEnd(); ++it) {
                daily.insert(it.key(), it.value().studyHours);
            }
            heatmap->setData(daily, appDatas.targetHour());
        }
        heatmapLayout->addWidget(heatmap);
        statsLayout->addWidget(heatmapGroup);

        // 各时段学习次数
        QMap<int, int> hourCount;
        for (auto it = yearData.constBegin(); it != yearData.constEnd(); ++it) {
            const QMap<int, TimeAxisItem> &items = it.value().timeAxisData;
            for (auto t = items.constBegin(); t != items.constEnd(); ++t) {
                if (t.value().type == "学习" && t.value().isCompleted) {
                    hourCount[t.key()] += 1;
                }
            }
        }

        auto *barGroup = new QGroupBox("各时段学习次数");
        auto *barLayout = new QVBoxLayout(barGroup);
        barLayout->setContentsMargins(10, 10, 10, 10);

        auto *barSet = new QBarSet("学习次数");
        barSet->setColor(QColor(tc.accent));
        barSet->setBorderColor(QColor(tc.accent));
        for (int h = 0; h < 24; ++h) {
            barSet->append(hourCount.value(h, 0));
        }

        auto *barSeries = new QBarSeries;
        barSeries->append(barSet);

        auto *barChart = new QChart;
        barChart->addSeries(barSeries);
        barChart->setBackgroundBrush(QBrush(QColor(tc.cardBg)));
        barChart->setBackgroundPen(Qt::NoPen);
        barChart->setBackgroundRoundness(0);
        barChart->setTitle("各时段学习次数");
        barChart->setTitleBrush(QBrush(QColor(tc.text)));
        barChart->legend()->setVisible(false);

        auto *barAxisX = new QBarCategoryAxis;
        for (int h = 0; h < 24; ++h) {
            barAxisX->append(QString::number(h));
        }
        barAxisX->setLabelsBrush(QBrush(QColor(tc.textSecondary)));
        barAxisX->setGridLineColor(QColor(tc.borderSoft));

        auto *barAxisY = new QValueAxis;
        barAxisY->setMin(0);
        barAxisY->setLabelsBrush(QBrush(QColor(tc.textSecondary)));
        barAxisY->setGridLineColor(QColor(tc.borderSoft));

        barChart->addAxis(barAxisX, Qt::AlignBottom);
        barChart->addAxis(barAxisY, Qt::AlignLeft);
        barSeries->attachAxis(barAxisX);
        barSeries->attachAxis(barAxisY);

        auto *barView = new QChartView(barChart);
        barView->setRenderHint(QPainter::Antialiasing);
        barView->setMinimumHeight(170);
        barLayout->addWidget(barView);
        statsLayout->addWidget(barGroup);

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

        DialogAnimator::applyFadeSlideIn(statsDlg);
        statsDlg->exec();
    });
    pageLayout->addLayout(monthLayout);

    // 日历主体
    int currentYear = QDate::currentDate().year();
    int currentMonth = QDate::currentDate().month();
    int monthStudyHours = appDatas.getMonthStudyHours(currentYear, currentMonth);
    QGroupBox* calendarGroup = new QGroupBox(QString("📅 月度学习记录                 当月总学习时长: %1小时").arg(monthStudyHours));
    calendarGroup->setObjectName("calendarGroup");
    m_monthCalendarLayout = new QGridLayout(calendarGroup);
    m_monthCalendarLayout->setSpacing(4);  // 日历单元格间距极致紧凑
    
    // 星期标题与日期格统一由 generateMonthCalendar() 生成，这里不再重复创建
    pageLayout->addWidget(calendarGroup);

    // 连接信号槽
    connect(prevMonthBtn, &QPushButton::clicked, [=](){ switchMonth(-1); });
    connect(nextMonthBtn, &QPushButton::clicked, [=](){ switchMonth(1); });
    connect(currentMonthBtn, &QPushButton::clicked, this, &MonthView::setToCurrentMonth);
}

void MonthView::switchMonth(int offset)
{
    m_month = m_month.addMonths(offset);
    generateMonthCalendar();
    updateMonthTitle();
}

void MonthView::updateMonthTitle()
{
    m_monthTitleLabel->setText(QString("%1年%2月").arg(displayYear()).arg(displayMonth()));

    if (QGroupBox *calendarGroup = findChild<QGroupBox*>("calendarGroup")) {
        const int hours = appDatas.getMonthStudyHours(displayYear(), displayMonth());
        calendarGroup->setTitle(QString("📅 月度学习记录                 当月总学习时长: %1小时").arg(hours));
    }
}

void MonthView::generateMonthCalendar()
{
    const int year = displayYear(), month = displayMonth();
    
    QLayoutItem* item;
    while ((item = m_monthCalendarLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    m_dateLabelMap.clear();

    QStringList weeks = {"日", "一", "二", "三", "四", "五", "六"};
    for (int i = 0; i < 7; ++i) {
        QLabel* weekLab = new QLabel(weeks[i]);
        weekLab->setObjectName("weekdayLabel");
        weekLab->setAlignment(Qt::AlignCenter);
        m_monthCalendarLayout->addWidget(weekLab, 0, i, Qt::AlignCenter);
    }

    QDate firstDay(year, month, 1);
    int startWeek = firstDay.dayOfWeek();
    startWeek = (startWeek == 7) ? 0 : startWeek;

    int daysInMonth = firstDay.daysInMonth();
    int row = 1;
    int col = startWeek;

    QDate today = QDate::currentDate();
    
    for (int day = 1; day <= daysInMonth; ++day) {
        QDate currentDate(year, month, day);
        DateStudyData data = appDatas.value(currentDate);

        QLabel* dayLabel = new QLabel(QString("%1\n%2h").arg(day).arg(data.studyHours));
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setFixedSize(48, 48);
        dayLabel->setCursor(Qt::PointingHandCursor);
        
        bool isToday = (currentDate == today);

        // 日期格外观交由 QSS 的 [dayState="..."] 规则控制，颜色只从主题表来
        QString dayState;
        if (isToday) {
            dayState = "today";
        } else if (data.studyHours == 0) {
            dayState = "empty";
        } else if (data.studyHours >= appDatas.targetHour()) {
            dayState = "done";
        } else {
            dayState = "partial";
        }
        dayLabel->setProperty("dayState", dayState);

        dayLabel->installEventFilter(this);
        m_dateLabelMap[dayLabel] = currentDate;

        m_monthCalendarLayout->addWidget(dayLabel, row, col, Qt::AlignCenter);
        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }
}

void MonthView::setToCurrentMonth()
{
    const QDate today = QDate::currentDate();
    m_month = QDate(today.year(), today.month(), 1);
    updateMonthTitle();
    generateMonthCalendar();
}

void MonthView::showMonthOf(const QDate &date)
{
    if (!date.isValid()) {
        return;
    }
    m_month = QDate(date.year(), date.month(), 1);
    updateMonthTitle();
    generateMonthCalendar();
}

bool MonthView::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(watched);
        if (label && m_dateLabelMap.contains(label)) {
            const QDate clickedDate = m_dateLabelMap[label];

            QWidget *root = window();
            if (auto *dayView = root ? root->findChild<DayView*>("dayView") : nullptr) {
                dayView->setDate(clickedDate);
            }
            if (root) {
                QMetaObject::invokeMethod(root, "switchToDayView");
            }

            return true;
        }
    }
    
    return QWidget::eventFilter(watched, event);
}
