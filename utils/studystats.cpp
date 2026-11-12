#include "studystats.h"

namespace StudyStats {

int continuousDays(const QMap<QDate, DateStudyData> &data, const QDate &endDate)
{
    int days = 0;
    QDate current = endDate;
    while (data.contains(current)) {
        ++days;
        current = current.addDays(-1);
    }
    return days;
}

int monthStudyHours(const QMap<QDate, DateStudyData> &data, int year, int month)
{
    const QDate firstDay(year, month, 1);
    const int daysInMonth = firstDay.daysInMonth();

    int total = 0;
    for (int day = 1; day <= daysInMonth; ++day) {
        total += data.value(QDate(year, month, day)).studyHours;
    }
    return total;
}

QJsonObject toJson(const DateStudyData &data)
{
    QJsonObject studyObj;
    studyObj.insert("studyHours", data.studyHours);
    studyObj.insert("completedProjects", data.completedProjects);
    studyObj.insert("totalProjects", data.totalProjects);

    QJsonObject timeAxisObj;
    for (auto it = data.timeAxisData.constBegin(); it != data.timeAxisData.constEnd(); ++it) {
        QJsonObject itemObj;
        itemObj.insert("type", it.value().type);
        itemObj.insert("isCompleted", it.value().isCompleted);
        timeAxisObj.insert(QString::number(it.key()), itemObj);
    }
    studyObj.insert("timeAxisData", timeAxisObj);

    return studyObj;
}

DateStudyData fromJson(const QJsonObject &obj)
{
    DateStudyData data;
    data.studyHours = obj["studyHours"].toInt();
    data.completedProjects = obj["completedProjects"].toInt();
    data.totalProjects = obj["totalProjects"].toInt();

    const QJsonObject timeAxisObj = obj["timeAxisData"].toObject();
    for (auto it = timeAxisObj.constBegin(); it != timeAxisObj.constEnd(); ++it) {
        bool ok = false;
        const int hour = it.key().toInt(&ok);
        if (!ok) {
            continue;   // 跳过无效的小时键
        }
        const QJsonObject itemObj = it.value().toObject();
        TimeAxisItem item;
        item.type = itemObj["type"].toString();
        item.isCompleted = itemObj["isCompleted"].toBool();
        data.timeAxisData.insert(hour, item);
    }

    return data;
}

} // namespace StudyStats
