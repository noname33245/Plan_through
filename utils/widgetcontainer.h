#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include <QWidget>
#include <QMap>
#include <QString>

class WidgetContainer
{
public:
    QWidget* operator()(QString name);
    void operator()(QString name,QWidget* ptr);

private:
    QMap<QString,QWidget*> m_container;
};

extern WidgetContainer widgetContainer;

#endif // WIDGETCONTAINER_H
