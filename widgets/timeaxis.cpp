#include "timeaxis.h"
#include "dayview.h"
#include "monthview.h"
#include "../datastruct.h"
#include "../appdatas.h"
#include "../utils/dialoganimator.h"
#include "../utils/theme.h"
#include <QApplication>
#include <QCursor>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <utility>

TimeAxis::TimeAxis(DayView *parent)
    : QWidget{parent}, m_dayView(parent)
{
    this->setObjectName("timeAxis");

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(6);
    layout->setContentsMargins(3, 6, 3, 6);

    const QList<int> hours = appDatas.dayHours();
    for (int hour : hours) {
        auto *hourLayout = new QHBoxLayout;
        hourLayout->setSpacing(5);

        auto *timeLabel = new QLabel(QString("%1:00").arg(hour));
        timeLabel->setObjectName("timeLabel");
        timeLabel->setMinimumWidth(45);
        timeLabel->setFixedWidth(50);
        timeLabel->setAlignment(Qt::AlignCenter);

        auto *axisBtn = new QPushButton("未安排");
        axisBtn->setObjectName(QString::number(hour));
        axisBtn->setEnabled(true);
        axisBtn->setMinimumHeight(30);
        axisBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_timeAxisBtnMap.insert(hour, axisBtn);

        hourLayout->addWidget(timeLabel);
        hourLayout->addWidget(axisBtn);
        hourLayout->setStretchFactor(axisBtn, 1);

        layout->addLayout(hourLayout);

        // 鼠标操作由 eventFilter 接管；这里只保留键盘触发的回退
        connect(axisBtn, &QPushButton::clicked, this, [this, hour]() {
            showTypeDialog(hour, hour);
        });
        axisBtn->installEventFilter(this);
    }
}

// 正在查看的日期来自 DayView，不再有全局日期状态
QDate TimeAxis::currentDate() const
{
    return m_dayView ? m_dayView->date() : QDate::currentDate();
}

// 数据变化后立刻刷新日视图统计和月视图格子
void TimeAxis::notifyChanged()
{
    if (m_dayView) {
        m_dayView->updateDayViewStats();
    }
    if (QWidget *root = window()) {
        if (auto *monthView = root->findChild<MonthView*>("monthView")) {
            monthView->generateMonthCalendar();
        }
    }
}

// 批量应用期间不落盘，全部改完再统一保存
void TimeAxis::persist()
{
    appDatas.saveDataToFile();
    notifyChanged();
}

void TimeAxis::applyItem(int hour, const QString &type)
{
    const bool isCompleted = true;
    DateStudyData &data = appDatas[currentDate()];

    if (data.timeAxisData.contains(hour)) {
        const TimeAxisItem oldItem = data.timeAxisData[hour];
        if (oldItem.type == "学习" && oldItem.isCompleted) data.studyHours -= 1;
        if (oldItem.isCompleted) data.completedProjects -= 1;
    }

    data.timeAxisData[hour] = { type, isCompleted };

    if (auto *btn = m_timeAxisBtnMap.value(hour, nullptr)) {
        btn->setText(type);
        Theme::repolish(btn);
    }

    data.totalProjects = data.timeAxisData.count();
    if (type == "学习" && isCompleted) data.studyHours += 1;
    if (isCompleted) data.completedProjects += 1;
}

void TimeAxis::clearItem(int hour)
{
    DateStudyData &data = appDatas[currentDate()];

    if (data.timeAxisData.contains(hour)) {
        const TimeAxisItem oldItem = data.timeAxisData[hour];
        if (oldItem.type == "学习" && oldItem.isCompleted) data.studyHours -= 1;
        if (oldItem.isCompleted) data.completedProjects -= 1;
        data.timeAxisData.remove(hour);
        data.totalProjects = data.timeAxisData.count();
    }

    if (auto *btn = m_timeAxisBtnMap.value(hour, nullptr)) {
        btn->setText("未安排");
        Theme::repolish(btn);
    }
}

// 单格设置（右键快捷用）
void TimeAxis::confirmTimeAxisItem(int hour, const QString &type)
{
    applyItem(hour, type);
    persist();
}

void TimeAxis::clearCurrentHourItem(int hour)
{
    clearItem(hour);
    persist();
}

// from == to 就是普通的单格选择
void TimeAxis::showTypeDialog(int from, int to)
{
    if (from > to) {
        std::swap(from, to);
    }

    QDialog dialog(this);
    dialog.setObjectName("timeAxisBtnDialog");
    dialog.setWindowTitle("选择事项");
    dialog.setModal(true);
    dialog.resize(240, from == to ? 260 : 280);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(8);

    const bool isRange = (from != to);
    auto *titleLabel = new QLabel(isRange
        ? QString("为 %1:00 - %2:00 选择事项").arg(from).arg(to + 1)
        : QString("请选择事项类型"));
    layout->addWidget(titleLabel);

    const QStringList types = { "学习", "吃饭", "睡觉", "洗澡", "游戏", "杂事" };
    for (const QString &type : types) {
        auto *typeBtn = new QPushButton(type);
        layout->addWidget(typeBtn);
        connect(typeBtn, &QPushButton::clicked, &dialog, [this, from, to, type, &dialog]() {
            for (int h = from; h <= to; ++h) {
                applyItem(h, type);
            }
            persist();
            dialog.close();
        });
    }

    auto *btnGroupLayout = new QHBoxLayout;
    btnGroupLayout->setSpacing(8);
    auto *clearBtn = new QPushButton("清除");
    auto *cancelBtn = new QPushButton("取消");
    clearBtn->setObjectName("clearBtn");
    cancelBtn->setObjectName("cancelBtn");

    btnGroupLayout->addStretch();
    btnGroupLayout->addWidget(clearBtn);
    btnGroupLayout->addWidget(cancelBtn);
    btnGroupLayout->addStretch();
    layout->addLayout(btnGroupLayout);

    connect(clearBtn, &QPushButton::clicked, &dialog, [this, from, to, &dialog]() {
        for (int h = from; h <= to; ++h) {
            clearItem(h);
        }
        persist();
        dialog.close();
    });
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::close);

    DialogAnimator::applyFadeSlideIn(&dialog);
    dialog.exec();
}

// 把 [m_rangeFrom, m_rangeTo] 的按钮标成高亮
void TimeAxis::updateRangeHighlight()
{
    for (auto it = m_timeAxisBtnMap.constBegin(); it != m_timeAxisBtnMap.constEnd(); ++it) {
        const bool selected = m_dragSelecting
            && it.key() >= m_rangeFrom && it.key() <= m_rangeTo;
        QPushButton *btn = it.value();
        if (btn->property("rangeSel").toBool() != selected) {
            btn->setProperty("rangeSel", selected);
            Theme::repolish(btn);
        }
    }
}

void TimeAxis::clearRangeHighlight()
{
    for (QPushButton *btn : m_timeAxisBtnMap) {
        if (btn->property("rangeSel").toBool()) {
            btn->setProperty("rangeSel", false);
            Theme::repolish(btn);
        }
    }
}

QPushButton* TimeAxis::operator[](int hour)
{
    return m_timeAxisBtnMap[hour];
}

bool TimeAxis::eventFilter(QObject *watched, QEvent *event)
{
    auto *btn = qobject_cast<QPushButton*>(watched);
    if (!btn) {
        return QWidget::eventFilter(watched, event);
    }
    const int hour = m_timeAxisBtnMap.key(btn, -1);
    if (hour < 0) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            if (btn->text() == "未安排") {
                confirmTimeAxisItem(hour, "学习");
            } else {
                clearCurrentHourItem(hour);
            }
            return true;
        }
        if (mouseEvent->button() == Qt::LeftButton) {
            // 接管按下事件，避免按钮自行发出 clicked（否则松手会重复弹窗）
            m_pressHour = hour;
            m_rangeFrom = hour;
            m_rangeTo = hour;
            m_pressPos = mouseEvent->globalPosition().toPoint();
            m_dragSelecting = false;
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        if (m_pressHour < 0) {
            break;
        }
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (!(mouseEvent->buttons() & Qt::LeftButton)) {
            break;
        }
        if (!m_dragSelecting) {
            const QPoint delta = mouseEvent->globalPosition().toPoint() - m_pressPos;
            if (delta.manhattanLength() < QApplication::startDragDistance()) {
                break;
            }
            m_dragSelecting = true;
        }
        // 指针当前压在哪个小时格上
        const QPoint local = mapFromGlobal(QCursor::pos());
        const int hoverHour = m_timeAxisBtnMap.key(qobject_cast<QPushButton*>(childAt(local)), -1);
        if (hoverHour >= 0) {
            m_rangeFrom = qMin(m_pressHour, hoverHour);
            m_rangeTo = qMax(m_pressHour, hoverHour);
            updateRangeHighlight();
        }
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() != Qt::LeftButton || m_pressHour < 0) {
            break;
        }

        const int pressedHour = m_pressHour;
        const bool dragged = m_dragSelecting;
        const int from = m_rangeFrom;
        const int to = m_rangeTo;

        m_pressHour = -1;
        m_dragSelecting = false;
        clearRangeHighlight();

        if (dragged && from != to) {
            showTypeDialog(from, to);
        } else {
            showTypeDialog(pressedHour, pressedHour);
        }
        return true;
    }
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}
