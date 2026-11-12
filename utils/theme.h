#ifndef THEME_H
#define THEME_H

#include <QString>

class QWidget;

// 全局主题中心
// ---------------------------------------------------------------------------
// 设计目标：
//   1. 颜色的唯一来源是本文件的 Colors 表，QSS 模板里不再出现硬编码色值，
//      避免"某个控件漏改颜色"被 Windows 系统深色主题带偏。
//   2. 启动时按当前主题强制写入调色板（QPalette）并接管系统深浅色状态，
//      让原生绘制的部分（下拉箭头、滚动条、日历翻页按钮等）与主题一致，
//      同时不受 Windows 系统主题 / 强调色影响。
//   3. 样式表挂在 QApplication 上而不是 MainWindow 上，
//      parent 为 nullptr 的 QMessageBox / 弹出菜单同样能被覆盖。
// ---------------------------------------------------------------------------
namespace Theme {

enum Type {
    PureWhite    = 0,   // 纯净白（浅色）
    DarkGray     = 1,   // 灰黑（深色）
    FollowSystem = 2    // 跟随系统明暗
};

struct Colors {
    // 背景
    QString windowBg;         // 主窗口 / 页面底色
    QString cardBg;           // 卡片、对话框、输入框底色
    QString cardAltBg;        // 次级底色（轨道、禁用底）
    QString scrollTrack;      // 滚动条轨道
    QString scrollHandle;     // 滚动条滑块
    QString scrollHandleHover;

    // 边框
    QString border;           // 常规边框
    QString borderSoft;       // 弱化边框 / 分隔线

    // 文本
    QString text;             // 主文本
    QString textSecondary;    // 次要文本
    QString textMuted;        // 弱化文本
    QString textDisabled;     // 禁用文本

    // 主色
    QString accent;
    QString accentHover;
    QString accentPressed;
    QString accentSoft;       // 主色浅底

    // 语义色
    QString success;
    QString successHover;
    QString successPressed;
    QString successSoft;
    QString danger;
    QString dangerHover;
    QString dangerPressed;
    QString warning;
    QString warningHover;
    QString violet;
    QString violetHover;
    QString cyan;
    QString cyanHover;

    // 月视图日期格
    QString todayFrom;
    QString todayTo;
    QString todayBorder;
    QString emptyDayBg;
    QString emptyDayBorder;
    QString emptyDayText;

    // 时间轴分类标签：浅色主题是浅底深字，深色主题是深底亮字
    QString tagNoneBg;
    QString tagNoneFg;
    QString tagNoneHoverBg;
    QString tagStudyBg;
    QString tagStudyFg;
    QString tagStudyHoverBg;
    QString tagMealBg;
    QString tagMealFg;
    QString tagMealHoverBg;
    QString tagSleepBg;
    QString tagSleepFg;
    QString tagSleepHoverBg;
    QString tagBathBg;
    QString tagBathFg;
    QString tagBathHoverBg;
    QString tagGameBg;
    QString tagGameFg;
    QString tagGameHoverBg;
    QString tagChoreBg;
    QString tagChoreFg;
    QString tagChoreHoverBg;

    // 下拉箭头图标（深浅色各一版，避免箭头被底色吃掉）
    QString arrowIcon;
};

// 当前主题的配色
const Colors &colors();

// 切换主题（只改内存中的方案，不刷新界面）
// type 可以是 0 / 1 / 2（2 = 跟随系统，此时实际配色由系统明暗决定）
void setTheme(int type);

// 实际生效的主题编号（只会是 PureWhite 或 DarkGray）
int effectiveType();

// 强制浅色调色板 + 关闭系统深浅色跟随
// 必须在 QApplication 构造完成后、创建任何窗口之前调用
void applyApplicationPalette();

// 按当前主题生成完整 QSS
QString styleSheet();

// 一次性应用：调色板 + 应用级样式表
void applyToApplication();

// 让控件的样式在动态属性/文本变化后重新计算
void repolish(QWidget *widget);

} // namespace Theme

#endif // THEME_H
