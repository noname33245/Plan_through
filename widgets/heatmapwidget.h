#ifndef HEATMAPWIDGET_H
#define HEATMAPWIDGET_H

#include <QWidget>
#include <QDate>
#include <QMap>
#include <QPainter>
#include <QFont>

#include "../utils/theme.h"

// 一年打卡热力图（类似 GitHub 贡献图）
// 每列一周，每行一天，格子颜色按当日学习时长相对目标的比例分档
class HeatmapWidget : public QWidget
{
public:
    explicit HeatmapWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(124);
    }

    void setData(const QMap<QDate, int> &hoursByDate, int targetHours)
    {
        m_data = hoursByDate;
        m_target = qMax(1, targetHours);
        update();
    }

    QSize sizeHint() const override { return QSize(700, 124); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Theme::Colors &tc = Theme::colors();

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        const int cell = 11;
        const int step = cell + 2;
        const int leftPad = 24;
        const int topPad = 14;

        const QDate today = QDate::currentDate();
        const QDate firstDay = today.addDays(-364);
        const QDate start = firstDay.addDays(-(firstDay.dayOfWeek() - 1));
        const int weeks = (start.daysTo(today) / 7) + 1;

        const QColor empty(tc.cardAltBg);
        const QColor base(tc.accent);
        const QColor full(tc.success);

        // 左侧星期标签
        QFont small = p.font();
        small.setPointSize(7);
        p.setFont(small);
        p.setPen(QColor(tc.textMuted));
        const QStringList weekNames = { "一", "", "三", "", "五", "", "日" };
        for (int r = 0; r < 7; ++r) {
            if (weekNames[r].isEmpty()) {
                continue;
            }
            p.drawText(QRect(0, topPad + r * step, leftPad - 5, cell),
                       Qt::AlignRight | Qt::AlignVCenter, weekNames[r]);
        }

        for (int w = 0; w < weeks; ++w) {
            for (int r = 0; r < 7; ++r) {
                const QDate date = start.addDays(w * 7 + r);
                if (date > today) {
                    continue;
                }

                const int hours = m_data.value(date, 0);
                QColor color = empty;
                if (hours > 0) {
                    const double ratio = double(hours) / double(m_target);
                    if (ratio >= 1.0) {
                        color = full;
                    } else {
                        color = base;
                        color.setAlpha(ratio >= 0.66 ? 200 : (ratio >= 0.33 ? 130 : 70));
                    }
                }

                p.fillRect(QRect(leftPad + w * step, topPad + r * step, cell, cell), color);
            }
        }

        // 图例
        const int legendY = topPad + 7 * step + 8;
        p.setPen(QColor(tc.textMuted));
        p.drawText(QRect(leftPad, legendY, 40, 12), Qt::AlignLeft | Qt::AlignVCenter, "少");
        const QColor legend[] = { empty, QColor(base.red(), base.green(), base.blue(), 70),
                                  QColor(base.red(), base.green(), base.blue(), 130),
                                  base, full };
        for (int i = 0; i < 5; ++i) {
            p.fillRect(QRect(leftPad + 26 + i * step, legendY, cell, cell), legend[i]);
        }
        p.drawText(QRect(leftPad + 26 + 5 * step + 4, legendY, 40, 12),
                   Qt::AlignLeft | Qt::AlignVCenter, "多");
    }

private:
    QMap<QDate, int> m_data;
    int m_target = 4;
};

#endif // HEATMAPWIDGET_H
