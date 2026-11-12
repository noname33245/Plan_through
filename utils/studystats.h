#ifndef STUDYSTATS_H
#define STUDYSTATS_H

#include <QDate>
#include <QMap>
#include <QJsonObject>

#include "../datastruct.h"

// 学习数据相关的纯函数：不读文件、不依赖全局状态，方便单测和复用
namespace StudyStats {

// 从 endDate 往前数连续有记录的天数
int continuousDays(const QMap<QDate, DateStudyData> &data, const QDate &endDate);

// 指定月份的总学习时长
int monthStudyHours(const QMap<QDate, DateStudyData> &data, int year, int month);

// 单日数据与 JSON 互转（存档 / 日志 / 备份共用同一套字段）
QJsonObject toJson(const DateStudyData &data);
DateStudyData fromJson(const QJsonObject &obj);

} // namespace StudyStats

#endif // STUDYSTATS_H
