#ifndef MEMREDUCT_H
#define MEMREDUCT_H

#include <QWidget>

// 内存清理函数声明
void PerformMemoryClean(QWidget *parentWidget);
bool CheckAndCleanMemory(int thresholdPercent);
int GetCurrentMemoryUsage();

#endif // MEMREDUCT_H
