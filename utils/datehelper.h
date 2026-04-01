#ifndef DATEHELPER_H
#define DATEHELPER_H

#include <QDate>

class DateHelper{
public:
    QDate static currentDate();
    int static currentYear();
    int static currentMonth();
    QDate static caleDate();
    int static caleYear();
    int static caleMonth();

public:
    void static setCurrentDate(const QDate& date);
    void static setCaleDate(const QDate& date);
    void static resetDate();
    void static addMonth(const int diff);
    void static addCaleMonth(const int diff);
    int static calcMonthDiff(const QDate& date);
    int static calcCaleMonthDiff(const QDate& date);

private:
    QDate static m_currentDate;
    QDate static m_caleDate;
};

#endif // DATEHELPER_H
