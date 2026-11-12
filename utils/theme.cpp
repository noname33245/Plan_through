#include "theme.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QList>
#include <QPair>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QWidget>
#include <QDebug>

namespace {

// ---------------------------------------------------------------------------
// 配色方案：想改颜色只改这里
// ---------------------------------------------------------------------------

Theme::Colors makePureWhite()
{
    Theme::Colors c;

    c.windowBg        = "#FFFFFF";
    c.cardBg          = "#FFFFFF";
    c.cardAltBg       = "#F4F7FB";
    c.scrollTrack     = "#F5F7FA";
    c.scrollHandle    = "#C8CEDA";
    c.scrollHandleHover = "#9AA3B2";

    c.border          = "#E4E9F0";
    c.borderSoft      = "#F1F4F8";

    c.text            = "#2B2F36";
    c.textSecondary   = "#555B66";
    c.textMuted       = "#8A929D";
    c.textDisabled    = "#C0C4CC";

    c.accent          = "#2D8CF0";
    c.accentHover     = "#1D7AD9";
    c.accentPressed   = "#1668C0";
    c.accentSoft      = "#EDF6FF";

    c.success         = "#27AE60";
    c.successHover    = "#219653";
    c.successPressed  = "#1E8845";
    c.successSoft     = "#E8F5E9";

    c.danger          = "#FF6B6B";
    c.dangerHover     = "#FF5252";
    c.dangerPressed   = "#FF3B3B";

    c.warning         = "#F59E0B";
    c.warningHover    = "#D98A08";
    c.violet          = "#9370DB";
    c.violetHover     = "#7F5FCB";
    c.cyan            = "#34B7F1";
    c.cyanHover       = "#249FD8";


    c.todayFrom       = "#FF6B6B";
    c.todayTo         = "#EE5A24";
    c.todayBorder     = "#FFD700";
    c.emptyDayBg      = "#FCFDFF";
    c.emptyDayBorder  = "#EDF1F7";
    c.emptyDayText    = "#909399";

    // 时间轴标签：浅底深字
    c.tagNoneBg       = "#F2F4F7";
    c.tagNoneFg       = "#909399";
    c.tagNoneHoverBg  = "#F2F4F7";
    c.tagStudyBg      = "#ECF5FF";
    c.tagStudyFg      = "#2D8CF0";
    c.tagStudyHoverBg = "#DFEEFF";
    c.tagMealBg       = "#E8F5E9";
    c.tagMealFg       = "#27AE60";
    c.tagMealHoverBg  = "#D8EFDB";
    c.tagSleepBg      = "#F3E5F5";
    c.tagSleepFg      = "#6A1B9A";
    c.tagSleepHoverBg = "#EBD9EE";
    c.tagBathBg       = "#E0F7FA";
    c.tagBathFg       = "#006064";
    c.tagBathHoverBg  = "#CCEFF3";
    c.tagGameBg       = "#FFEBEE";
    c.tagGameFg       = "#C62828";
    c.tagGameHoverBg  = "#FFDBE0";
    c.tagChoreBg      = "#FFF8E1";
    c.tagChoreFg      = "#E65100";
    c.tagChoreHoverBg = "#FFEFC4";

    c.arrowIcon       = ":/icons/arrow-down.svg";

    return c;
}

Theme::Colors makeDarkGray()
{
    Theme::Colors c = makePureWhite();

    // 灰黑：深底浅字，卡片靠更亮一档的底色和细边框拉开层次
    c.windowBg        = "#16181C";
    c.cardBg          = "#1F2228";
    c.cardAltBg       = "#262A31";
    c.scrollTrack     = "#1B1E23";
    c.scrollHandle    = "#3E444D";
    c.scrollHandleHover = "#59616C";

    c.border          = "#333941";
    c.borderSoft      = "#272B31";

    c.text            = "#E6E9ED";
    c.textSecondary   = "#B9C0C9";
    c.textMuted       = "#8A929D";
    c.textDisabled    = "#5C646F";

    c.accent          = "#4C9AFF";
    c.accentHover     = "#74B2FF";
    c.accentPressed   = "#3B82D6";
    c.accentSoft      = "#1C2A3D";

    c.success         = "#4BCD7B";
    c.successHover    = "#6BDC94";
    c.successPressed  = "#37A862";
    c.successSoft     = "#17301F";

    c.danger          = "#FF7A7A";
    c.dangerHover     = "#FF9C9C";
    c.dangerPressed   = "#E26060";

    c.warning         = "#FFB84D";
    c.warningHover    = "#FFCB74";
    c.violet          = "#B48CEA";
    c.violetHover     = "#C7A6F3";
    c.cyan            = "#57C8EE";
    c.cyanHover       = "#7DD8F5";


    c.todayFrom       = "#FF7A5C";
    c.todayTo         = "#E35B3C";
    c.todayBorder     = "#FFD666";
    c.emptyDayBg      = "#23272D";
    c.emptyDayBorder  = "#2E343C";
    c.emptyDayText    = "#7B838F";

    // 时间轴标签：深底亮字
    c.tagNoneBg       = "#23272D";
    c.tagNoneFg       = "#8A929D";
    c.tagNoneHoverBg  = "#2C3138";
    c.tagStudyBg      = "#1A2C42";
    c.tagStudyFg      = "#6FB0FF";
    c.tagStudyHoverBg = "#21374F";
    c.tagMealBg       = "#1A2F23";
    c.tagMealFg       = "#5FD98A";
    c.tagMealHoverBg  = "#213B2C";
    c.tagSleepBg      = "#2A2038";
    c.tagSleepFg      = "#C79BEE";
    c.tagSleepHoverBg = "#352844";
    c.tagBathBg       = "#152E36";
    c.tagBathFg       = "#5FD6E8";
    c.tagBathHoverBg  = "#1B3A44";
    c.tagGameBg       = "#362025";
    c.tagGameFg       = "#FF8F8F";
    c.tagGameHoverBg  = "#43282E";
    c.tagChoreBg      = "#362A17";
    c.tagChoreFg      = "#FFC367";
    c.tagChoreHoverBg = "#443520";

    c.arrowIcon       = ":/icons/arrow-down-light.svg";

    return c;
}

const Theme::Colors &whiteColors()
{
    static const Theme::Colors c = makePureWhite();
    return c;
}

const Theme::Colors &darkColors()
{
    static const Theme::Colors c = makeDarkGray();
    return c;
}

// 用户选择（0/1/2）与最终生效（0/1）分开存：
// 选择"跟随系统"时，实际配色由系统明暗决定，且会在系统切换时自动重算
int g_selectedType  = Theme::PureWhite;
int g_effectiveType = Theme::PureWhite;
bool g_applying = false;

// 读取系统真实的明暗偏好。
// 不用 QStyleHints::colorScheme()，因为我们在手动模式下会用 setColorScheme() 覆盖它，
// 覆盖后就拿不到系统原值了；直接读 Windows 个性化设置最可靠
bool systemPrefersDark()
{
#ifdef Q_OS_WIN
    QSettings personalize(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    if (personalize.contains("AppsUseLightTheme")) {
        return personalize.value("AppsUseLightTheme").toInt() == 0;
    }
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
}

int resolveType(int selected)
{
    if (selected == Theme::DarkGray) {
        return Theme::DarkGray;
    }
    if (selected == Theme::FollowSystem) {
        return systemPrefersDark() ? Theme::DarkGray : Theme::PureWhite;
    }
    return Theme::PureWhite;
}

// 系统在运行中切换深色/浅色模式时，Qt 会用平台调色板覆盖我们的设置，
// 这里监听调色板变化并整份重刷（跟随系统模式下借此换到另一套配色）
class PaletteGuard : public QObject
{
public:
    explicit PaletteGuard(QObject *parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::ApplicationPaletteChange ||
            event->type() == QEvent::ThemeChange) {
            // g_applying 在 applyToApplication 内部防重入，这里不必再判断
            Theme::applyToApplication();
        }
        return QObject::eventFilter(obj, event);
    }
};

QString loadTemplate()
{
    static QString cached;
    if (!cached.isEmpty()) {
        return cached;
    }
    QFile file(":/theme.qss");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[Theme] 无法打开样式模板 :/theme.qss";
        return QString();
    }
    cached = QString::fromUtf8(file.readAll());
    file.close();
    return cached;
}

QString fillTemplate(const Theme::Colors &c)
{
    QString sheet = loadTemplate();
    if (sheet.isEmpty()) {
        return sheet;
    }

    // 用 @key@ 定界，避免 @accent@ 与 @accentHover@ 之类的前缀误替换
    const QList<QPair<QString, QString>> vars = {
        { "@windowBg@",          c.windowBg },
        { "@cardBg@",            c.cardBg },
        { "@cardAltBg@",         c.cardAltBg },
        { "@scrollTrack@",       c.scrollTrack },
        { "@scrollHandle@",      c.scrollHandle },
        { "@scrollHandleHover@", c.scrollHandleHover },
        { "@border@",            c.border },
        { "@borderSoft@",        c.borderSoft },
        { "@text@",              c.text },
        { "@textSecondary@",     c.textSecondary },
        { "@textMuted@",         c.textMuted },
        { "@textDisabled@",      c.textDisabled },
        { "@accent@",            c.accent },
        { "@accentHover@",       c.accentHover },
        { "@accentPressed@",     c.accentPressed },
        { "@accentSoft@",        c.accentSoft },
        { "@success@",           c.success },
        { "@successHover@",      c.successHover },
        { "@successPressed@",    c.successPressed },
        { "@successSoft@",       c.successSoft },
        { "@danger@",            c.danger },
        { "@dangerHover@",       c.dangerHover },
        { "@dangerPressed@",     c.dangerPressed },
        { "@warning@",           c.warning },
        { "@warningHover@",      c.warningHover },
        { "@violet@",            c.violet },
        { "@violetHover@",       c.violetHover },
        { "@cyan@",              c.cyan },
        { "@cyanHover@",         c.cyanHover },
        { "@todayFrom@",         c.todayFrom },
        { "@todayTo@",           c.todayTo },
        { "@todayBorder@",       c.todayBorder },
        { "@emptyDayBg@",        c.emptyDayBg },
        { "@emptyDayBorder@",    c.emptyDayBorder },
        { "@emptyDayText@",      c.emptyDayText },
        { "@tagNoneBg@",         c.tagNoneBg },
        { "@tagNoneFg@",         c.tagNoneFg },
        { "@tagNoneHoverBg@",    c.tagNoneHoverBg },
        { "@tagStudyBg@",        c.tagStudyBg },
        { "@tagStudyFg@",        c.tagStudyFg },
        { "@tagStudyHoverBg@",   c.tagStudyHoverBg },
        { "@tagMealBg@",         c.tagMealBg },
        { "@tagMealFg@",         c.tagMealFg },
        { "@tagMealHoverBg@",    c.tagMealHoverBg },
        { "@tagSleepBg@",        c.tagSleepBg },
        { "@tagSleepFg@",        c.tagSleepFg },
        { "@tagSleepHoverBg@",   c.tagSleepHoverBg },
        { "@tagBathBg@",         c.tagBathBg },
        { "@tagBathFg@",         c.tagBathFg },
        { "@tagBathHoverBg@",    c.tagBathHoverBg },
        { "@tagGameBg@",         c.tagGameBg },
        { "@tagGameFg@",         c.tagGameFg },
        { "@tagGameHoverBg@",    c.tagGameHoverBg },
        { "@tagChoreBg@",        c.tagChoreBg },
        { "@tagChoreFg@",        c.tagChoreFg },
        { "@tagChoreHoverBg@",   c.tagChoreHoverBg },
        { "@arrowIcon@",         c.arrowIcon },
    };

    for (const auto &pair : vars) {
        sheet.replace(pair.first, pair.second);
    }
    return sheet;
}

} // namespace


namespace Theme {

const Colors &colors()
{
    return g_effectiveType == DarkGray ? darkColors() : whiteColors();
}

void setTheme(int type)
{
    if (type == DarkGray || type == FollowSystem) {
        g_selectedType = type;
    } else {
        g_selectedType = PureWhite;
    }
    g_effectiveType = resolveType(g_selectedType);
}

int effectiveType()
{
    return g_effectiveType;
}

void applyApplicationPalette()
{
    if (!qApp) {
        return;
    }

    // 1. 先决定明暗：
    //    - 指定了主题：告诉 Qt 用对应的明暗方案，阻止它跟随 Windows
    //    - 跟随系统：交还给系统（Unknown），再用系统真实偏好算出实际方案
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    if (g_selectedType == FollowSystem) {
        QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Unknown);
    } else {
        QGuiApplication::styleHints()->setColorScheme(
            g_effectiveType == DarkGray ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    }
#endif
    g_effectiveType = resolveType(g_selectedType);

    // 2. 写死一份调色板：原生绘制的箭头、边框、滚动条都从这里取色，
    //    不再受系统主题 / 强调色影响
    const Colors &c = colors();

    const QColor window(c.windowBg);
    const QColor base(c.cardBg);
    const QColor text(c.text);
    const QColor accent(c.accent);
    const QColor disabled(c.textDisabled);

    QPalette p;
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, QColor(c.cardAltBg));
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, base);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor("#FFFFFF"));
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::LinkVisited, QColor(c.violet));
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    p.setColor(QPalette::PlaceholderText, QColor(c.textMuted));

    // 禁用态
    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(c.cardAltBg));
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(c.border));
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);

    QApplication::setPalette(p);
}

QString styleSheet()
{
    return fillTemplate(colors());
}

void applyToApplication()
{
    // 防重入：setPalette / setColorScheme 会触发调色板变化事件，
    // 事件回到 PaletteGuard 后又会调回这里
    if (g_applying) {
        return;
    }
    g_applying = true;

    applyApplicationPalette();

    if (qApp) {
        static bool guardInstalled = false;
        if (!guardInstalled) {
            guardInstalled = true;
            qApp->installEventFilter(new PaletteGuard(qApp));

            // 跟随系统模式下，系统切换明暗时直接整份重刷
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                             qApp, [](Qt::ColorScheme) {
                if (g_selectedType == FollowSystem) {
                    Theme::applyToApplication();
                }
            });
#endif
        }
        // 挂在 QApplication 上，parent 为空的 QMessageBox、右键菜单等也能覆盖
        qApp->setStyleSheet(styleSheet());
    }

    g_applying = false;
}

void repolish(QWidget *widget)
{
    if (!widget) {
        return;
    }
    QStyle *s = widget->style();
    if (!s) {
        return;
    }
    s->unpolish(widget);
    s->polish(widget);
    widget->update();
}

} // namespace Theme
