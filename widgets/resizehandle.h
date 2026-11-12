#ifndef RESIZEHANDLE_H
#define RESIZEHANDLE_H

#include <QWidget>
#include <QMouseEvent>
#include <QSize>

// 右下角缩放手柄
// ---------------------------------------------------------------------------
// 缩放全程由它自己处理：按下后 Qt 会把鼠标隐式抓取到本控件，
// 移动/松开都直接送到这里，不依赖主窗口的 mousePress/Release 配对。
// 光标也只设在自己身上，不会因为"点了但没收到 release"而卡在主窗口上。
// ---------------------------------------------------------------------------
class ResizeHandle : public QWidget
{
public:
    explicit ResizeHandle(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("resizeHandle");
        setFixedSize(12, 12);
        setCursor(Qt::SizeFDiagCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) {
            event->ignore();
            return;
        }
        if (QWidget *w = parentWidget()) {
            m_dragging = true;
            m_startGlobal = event->globalPosition().toPoint();
            m_startSize = w->size();
            event->accept();
            return;
        }
        event->ignore();
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!m_dragging) {
            event->ignore();
            return;
        }
        QWidget *w = parentWidget();
        if (!w) {
            return;
        }
        const QPoint delta = event->globalPosition().toPoint() - m_startGlobal;
        w->resize(qMax(w->minimumWidth(), m_startSize.width() + delta.x()),
                  qMax(w->minimumHeight(), m_startSize.height() + delta.y()));
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_dragging = false;
        event->accept();
    }

private:
    bool m_dragging = false;
    QPoint m_startGlobal;
    QSize m_startSize;
};

#endif // RESIZEHANDLE_H
