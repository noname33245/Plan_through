#ifndef ROUNDPROGRESSBAR_H
#define ROUNDPROGRESSBAR_H

#include <QProgressBar>
#include <QPainter>
#include <QPainterPath>
#include "../utils/theme.h"

// 自定义圆角进度条：使用 QPainter 手动绘制，确保任意宽度（含接近0%）下
// 左端始终保持圆角，避免 Qt 原生 ::chunk 在窄宽度时圆角失效变方角。
class RoundProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit RoundProgressBar(QWidget *parent = nullptr)
        : QProgressBar(parent)
    {
        setTextVisible(true);
        setAlignment(Qt::AlignCenter);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 颜色统一取当前主题，切换主题后自动跟随
        const Theme::Colors &tc = Theme::colors();

        const int w = width();
        const int h = height();
        const int radius = h / 2;

        // 1. 绘制背景轨道（完整圆角矩形）
        QPainterPath bgPath;
        bgPath.addRoundedRect(0, 0, w, h, radius, radius);
        painter.fillPath(bgPath, QColor(tc.accentSoft));

        // 2. 计算进度填充宽度
        int range = maximum() - minimum();
        if (range <= 0) return;
        int val = value() - minimum();
        int fillWidth = static_cast<int>((val * 1.0 / range) * w);

        if (fillWidth > 0) {
            fillWidth = qMin(fillWidth, w);

            // 手动构造填充路径，保证左端始终是圆弧
            QPainterPath fillPath;
            if (fillWidth >= 2 * radius) {
                // 宽度足够：标准圆角矩形（两端圆角）
                fillPath.addRoundedRect(0, 0, fillWidth, h, radius, radius);
            } else {
                // 宽度不足：左端圆弧 + 右侧直线，保证左端圆角不变方
                int r = qMin(radius, fillWidth / 2);
                fillPath.moveTo(fillWidth, 0);
                fillPath.lineTo(r, 0);
                fillPath.arcTo(0, 0, 2 * r, 2 * r, 90, 90);           // 左上圆角
                fillPath.lineTo(0, h - r);
                fillPath.arcTo(0, h - 2 * r, 2 * r, 2 * r, 180, 90);  // 左下圆角
                fillPath.lineTo(fillWidth, h);
                fillPath.closeSubpath();
            }

            // 蓝色渐变填充
            QLinearGradient grad(0, 0, qMax(fillWidth, 1), 0);
            grad.setColorAt(0.0, QColor(tc.accent));
            grad.setColorAt(1.0, QColor(tc.accentPressed));
            painter.fillPath(fillPath, grad);
        }

        // 3. 绘制文本（百分比）
        if (isTextVisible()) {
            painter.setPen(QColor(tc.text));
            QFont font = painter.font();
            font.setPointSize(10);
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(rect(), Qt::AlignCenter, text());
        }
    }
};

#endif // ROUNDPROGRESSBAR_H
