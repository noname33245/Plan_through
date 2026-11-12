#ifndef DIALOGANIMATOR_H
#define DIALOGANIMATOR_H

#include <QDialog>
#include <QWidget>
#include <QScreen>
#include <QCursor>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QPoint>
#include <QRect>

// 对话框出场动画工具：淡入 + 从下方轻微上滑
class DialogAnimator
{
public:
    // 对指定对话框应用淡入 + 上滑动画，调用后再 exec/show 即可
    static void applyFadeSlideIn(QDialog *dialog, int durationMs = 220)
    {
        if (!dialog) return;

        // 先把对话框居中到父窗口（或屏幕），确保不出现左上角错位
        QPoint centerPos = centerOnParent(dialog);
        dialog->move(centerPos);

        dialog->setWindowOpacity(0.0);

        // 上滑：从居中位置下方 18px 处滑入到居中位置
        QPoint startPos = centerPos + QPoint(0, 18);
        dialog->move(startPos);

        QPropertyAnimation *fadeAnim = new QPropertyAnimation(dialog, "windowOpacity");
        fadeAnim->setDuration(durationMs);
        fadeAnim->setStartValue(0.0);
        fadeAnim->setEndValue(1.0);
        fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

        QPropertyAnimation *slideAnim = new QPropertyAnimation(dialog, "pos");
        slideAnim->setDuration(durationMs);
        slideAnim->setStartValue(startPos);
        slideAnim->setEndValue(centerPos);
        slideAnim->setEasingCurve(QEasingCurve::OutCubic);

        QParallelAnimationGroup *group = new QParallelAnimationGroup(dialog);
        group->addAnimation(fadeAnim);
        group->addAnimation(slideAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // 对按钮应用点击缩放动画（按下时缩小，松开时恢复）
    static void applyPressScale(QWidget *widget, int durationMs = 120)
    {
        if (!widget) return;
        widget->setProperty("pressAnimDuration", durationMs);
    }

private:
    // 计算对话框在顶层主窗口（或当前屏幕）居中时的左上角坐标
    static QPoint centerOnParent(QDialog *dialog)
    {
        QWidget *parent = dialog->parentWidget();
        QRect targetRect;

        if (parent) {
            // 以顶层主窗口的全局几何为参照居中（而非直接父控件，
            // 避免父控件是滚动区内部小组件导致坐标错误）
            QWidget *topWindow = parent->window();
            if (topWindow) {
                targetRect = topWindow->frameGeometry();
            } else {
                targetRect = parent->frameGeometry();
            }
        } else {
            // 无父窗口时，以鼠标所在屏幕居中
            QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen) screen = QGuiApplication::primaryScreen();
            targetRect = screen->availableGeometry();
        }

        // 确保对话框尺寸已计算（部分对话框在 exec 前未布局）
        dialog->adjustSize();
        QSize dlgSize = dialog->size();
        if (dlgSize.width() <= 0 || dlgSize.height() <= 0) {
            dlgSize = dialog->sizeHint();
        }

        int x = targetRect.x() + (targetRect.width() - dlgSize.width()) / 2;
        int y = targetRect.y() + (targetRect.height() - dlgSize.height()) / 2;
        return QPoint(x, y);
    }
};

#endif // DIALOGANIMATOR_H
