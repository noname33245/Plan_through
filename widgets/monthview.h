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
#include <QLineSeries>
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
    
    // 事件过滤器，用于处理日期标签的点击事件
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QGridLayout *m_monthCalendarLayout = nullptr;
    QLabel* m_monthTitleLabel = nullptr;
    
    // 存储日期标签和对应日期的映射关系
    QMap<QLabel*, QDate> m_dateLabelMap;

signals:
};

#endif // MONTHVIEW_H
