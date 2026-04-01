#ifndef TIMEAXIS_H
#define TIMEAXIS_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialog>

class TimeAxis : public QWidget
{
    Q_OBJECT
public:
    explicit TimeAxis(QWidget *parent = nullptr);
    QPushButton* operator[](int hour);

signals:
    void sendText(const QString& str);

private:
    QMap<int, QPushButton*> m_timeAxisBtnMap;

private:
    void onTimeAxisBtnClicked(int hour);
    void confirmTimeAxisItem(int hour,const QString& type);
    void clearCurrentHourItem(int hour);

signals:
};

#endif // TIMEAXIS_H
