#ifndef MONTHVIEW_H
#define MONTHVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QMouseEvent>
#include <QMap>
#include <QDate>
#include <QChart>
#include <QChartView>
#include <QSplineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>

// 前向声明
class MainWindow;

class MonthView : public QWidget
{
    Q_OBJECT
public:
    explicit MonthView(QWidget *parent = nullptr);
    void switchMonth(int offset);
    void generateMonthCalendar();
    void setToCurrentMonth();
    // 跳到指定日期所在的月份并刷新
    void showMonthOf(const QDate &date);

    // 当前显示的月份
    int displayYear() const { return m_month.year(); }
    int displayMonth() const { return m_month.month(); }

    // 事件过滤器，用于处理日期标签的点击事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void updateMonthTitle();

    QGridLayout *m_monthCalendarLayout = nullptr;
    QLabel* m_monthTitleLabel = nullptr;

    // 显示中的月份（固定取该月 1 号）
    QDate m_month = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);

    // 存储日期标签和对应日期的映射关系
    QMap<QLabel*, QDate> m_dateLabelMap;
};

#endif // MONTHVIEW_H
