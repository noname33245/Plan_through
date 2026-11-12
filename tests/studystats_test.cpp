#include <QtTest>
#include <QDate>
#include <QJsonObject>

#include "utils/studystats.h"

// 数据层纯函数的单测：连续天数、月度统计、JSON 往返
class StudyStatsTest : public QObject
{
    Q_OBJECT

private slots:
    void continuousDays_countsConsecutiveTail();
    void continuousDays_stopsAtGap();
    void continuousDays_zeroWhenTodayMissing();
    void monthStudyHours_onlyCountsThatMonth();
    void json_roundTripKeepsAllFields();
    void json_skipsInvalidHourKey();
};

void StudyStatsTest::continuousDays_countsConsecutiveTail()
{
    const QDate end(2026, 11, 9);
    QMap<QDate, DateStudyData> data;
    for (int i = 0; i < 4; ++i) {
        data.insert(end.addDays(-i), DateStudyData());
    }
    // 中间有个断档，不应该被算进去
    data.insert(end.addDays(-6), DateStudyData());

    QCOMPARE(StudyStats::continuousDays(data, end), 4);
}

void StudyStatsTest::continuousDays_stopsAtGap()
{
    const QDate end(2026, 11, 9);
    QMap<QDate, DateStudyData> data;
    data.insert(end, DateStudyData());
    data.insert(end.addDays(-1), DateStudyData());
    // 第三天缺失
    data.insert(end.addDays(-3), DateStudyData());

    QCOMPARE(StudyStats::continuousDays(data, end), 2);
}

void StudyStatsTest::continuousDays_zeroWhenTodayMissing()
{
    const QDate end(2026, 11, 9);
    QMap<QDate, DateStudyData> data;
    data.insert(end.addDays(-1), DateStudyData());

    QCOMPARE(StudyStats::continuousDays(data, end), 0);
}

void StudyStatsTest::monthStudyHours_onlyCountsThatMonth()
{
    QMap<QDate, DateStudyData> data;
    data.insert(QDate(2026, 10, 31), DateStudyData{ 5, 0, 0, {} });   // 上月，不该计入
    data.insert(QDate(2026, 11, 1), DateStudyData{ 2, 0, 0, {} });
    data.insert(QDate(2026, 11, 9), DateStudyData{ 3, 0, 0, {} });
    data.insert(QDate(2026, 12, 1), DateStudyData{ 9, 0, 0, {} });    // 下月，不该计入

    QCOMPARE(StudyStats::monthStudyHours(data, 2026, 11), 5);
    QCOMPARE(StudyStats::monthStudyHours(data, 2026, 10), 5);
    QCOMPARE(StudyStats::monthStudyHours(data, 2026, 9), 0);
}

void StudyStatsTest::json_roundTripKeepsAllFields()
{
    DateStudyData original;
    original.studyHours = 3;
    original.completedProjects = 2;
    original.totalProjects = 5;
    original.timeAxisData.insert(9, TimeAxisItem{ "学习", true });
    original.timeAxisData.insert(10, TimeAxisItem{ "游戏", false });

    const DateStudyData restored = StudyStats::fromJson(StudyStats::toJson(original));

    QCOMPARE(restored.studyHours, original.studyHours);
    QCOMPARE(restored.completedProjects, original.completedProjects);
    QCOMPARE(restored.totalProjects, original.totalProjects);
    QCOMPARE(restored.timeAxisData.size(), original.timeAxisData.size());
    QCOMPARE(restored.timeAxisData.value(9).type, QStringLiteral("学习"));
    QVERIFY(restored.timeAxisData.value(9).isCompleted);
    QCOMPARE(restored.timeAxisData.value(10).type, QStringLiteral("游戏"));
    QVERIFY(!restored.timeAxisData.value(10).isCompleted);
}

void StudyStatsTest::json_skipsInvalidHourKey()
{
    QJsonObject timeAxis;
    timeAxis.insert("abc", QJsonObject{ { "type", "学习" }, { "isCompleted", true } });
    timeAxis.insert("11", QJsonObject{ { "type", "吃饭" }, { "isCompleted", true } });

    QJsonObject day;
    day.insert("studyHours", 1);
    day.insert("timeAxisData", timeAxis);

    const DateStudyData data = StudyStats::fromJson(day);
    QCOMPARE(data.timeAxisData.size(), 1);
    QVERIFY(data.timeAxisData.contains(11));
}

QTEST_APPLESS_MAIN(StudyStatsTest)

#include "studystats_test.moc"
