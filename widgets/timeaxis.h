#ifndef TIMEAXIS_H
#define TIMEAXIS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QDate>
#include <QMap>
#include <QPoint>

class DayView;

// 一天的时间轴：每个整点一个按钮
// 交互：
//   左键点一下  → 选事项
//   左键拖动    → 划选一段小时，批量设置
//   右键        → 未安排则快速标"学习"，已有内容则清除
class TimeAxis : public QWidget
{
    Q_OBJECT
public:
    // 直接持有 DayView 指针：setWidget() 会把本控件的 Qt parent 改成 scrollArea，
    // 靠 parentWidget() 反查 DayView 是拿不到的
    explicit TimeAxis(DayView *parent = nullptr);
    QPushButton* operator[](int hour);

private:
    QDate currentDate() const;

    void showTypeDialog(int from, int to);
    void applyItem(int hour, const QString &type);
    void clearItem(int hour);
    void persist();
    void updateRangeHighlight();
    void clearRangeHighlight();
    void confirmTimeAxisItem(int hour, const QString &type);
    void clearCurrentHourItem(int hour);
    void notifyChanged();

    bool eventFilter(QObject *watched, QEvent *event) override;

    QMap<int, QPushButton*> m_timeAxisBtnMap;
    DayView *m_dayView = nullptr;

    // 拖拽连选状态
    int m_pressHour = -1;
    int m_rangeFrom = -1;
    int m_rangeTo = -1;
    QPoint m_pressPos;
    bool m_dragSelecting = false;
};

#endif // TIMEAXIS_H
