#include "gui/style/theme.h"

#include "hal_core/utilities/log.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QStyleHints>
#include <QWidget>

#ifdef _WIN32
#include <windows.h>

// <dwmapi.h> needs GDI types that are excluded from the build, so only the one function is declared
extern "C" HRESULT __stdcall DwmSetWindowAttribute(HWND hwnd, DWORD attribute, LPCVOID value, DWORD size);
#endif

namespace hal
{
    namespace style
    {
        namespace
        {
            // Tokens shared by both themes
            QMap<QString, QString> commonTokens()
            {
                return {
                    {"radius-control", "6px"},    // controls inside a page: buttons, inputs, list rows
                    {"radius-overlay", "10px"},   // cards, dialogs, popups
                    {"control-height", "32px"},
                    {"font-ui", "\"Segoe UI Variable Text\", \"Segoe UI\", sans-serif"},
                    {"font-mono", "\"Iosevka\", \"Cascadia Mono\", Consolas, monospace"},
                    // Type scale, every piece of text uses one of these roles
                    {"font-size", "14px"},            // body
                    {"font-size-small", "13px"},      // panel titles, table headers, secondary text
                    {"font-size-caption", "12px"},    // captions, paths, hints
                    {"font-size-title", "18px"},
                    {"font-size-display", "28px"},
                };
            }

            QMap<QString, QString> darkTokens()
            {
                QMap<QString, QString> t = commonTokens();
                // Surfaces: a tinted dark gray, never pure black. Lighter means closer to the user.
                t["surface-canvas"]  = "#0C0C0D";    // wells: editors, inputs, graph canvas
                t["surface-base"]    = "#131314";    // window background
                t["surface-raised"]  = "#1B1B1D";    // panels, docks, toolbars
                t["surface-overlay"] = "#262628";    // menus, popups, dialogs, tooltips
                // States are translucent steps, so they work on every surface
                t["state-hover"]    = "rgba(255, 255, 255, 0.06)";
                t["state-pressed"]  = "rgba(255, 255, 255, 0.10)";
                t["state-selected"] = "rgba(240, 162, 74, 0.22)";
                t["border-subtle"]  = "#262628";
                t["border"]         = "#343437";
                t["border-strong"]  = "#4E4E53";
                // Text hierarchy: 14:1, 7.5:1 and a disabled level on the base surface
                t["text"]          = "#EEEEEF";
                t["text-muted"]    = "#B5B5BA";
                t["text-disabled"] = "#7F7F86";
                t["icon"]          = "#B0B0B6";
                t["image-tab-close"] = "url(:/icons/tab-close-dark)";
                t["text-on-accent"] = "#1B1206";
                // One accent, lighter and a bit less saturated than on light surfaces
                t["accent"]         = "#F0A24A";
                t["accent-hover"]   = "#F7B666";
                t["accent-pressed"] = "#DB8E36";
                t["selection"]      = "#4A3A20";
                t["success"]        = "#58C28B";
                t["warning"]        = "#E9D158";
                t["danger"]         = "#F0706A";
                t["fill-control"]         = "rgba(255, 255, 255, 0.05)";    // resting fill of buttons
                t["fill-control-hover"]   = "rgba(255, 255, 255, 0.09)";
                t["fill-control-pressed"] = "rgba(255, 255, 255, 0.13)";
                t["success-subtle"]       = "rgba(88, 194, 139, 0.16)";    // tinted backgrounds instead of saturated fills
                t["warning-subtle"]       = "rgba(233, 209, 88, 0.16)";
                t["danger-subtle"]        = "rgba(240, 112, 106, 0.16)";
                t["scroll-thumb"]       = "rgba(255, 255, 255, 0.20)";
                t["scroll-thumb-hover"] = "rgba(255, 255, 255, 0.34)";
                return t;
            }

            QMap<QString, QString> lightTokens()
            {
                QMap<QString, QString> t = commonTokens();
                t["surface-canvas"]  = "#FFFFFF";
                t["surface-base"]    = "#F4F4F5";
                t["surface-raised"]  = "#FAFAFA";
                t["surface-overlay"] = "#FFFFFF";
                t["state-hover"]    = "rgba(0, 0, 0, 0.05)";
                t["state-pressed"]  = "rgba(0, 0, 0, 0.09)";
                t["state-selected"] = "rgba(201, 122, 14, 0.18)";
                t["border-subtle"]  = "#E4E4E6";
                t["border"]         = "#D4D4D8";
                t["border-strong"]  = "#AFAFB6";
                t["text"]          = "#1C1C1E";
                t["text-muted"]    = "#5A5A61";
                t["text-disabled"] = "#8E8E96";
                t["icon"]          = "#55555C";
                t["image-tab-close"] = "url(:/icons/tab-close-light)";
                t["text-on-accent"] = "#FFFFFF";
                t["accent"]         = "#C97A0E";
                t["accent-hover"]   = "#B86E0A";
                t["accent-pressed"] = "#A56206";
                t["selection"]      = "#FBE7C2";
                t["success"]        = "#1E9E63";
                t["warning"]        = "#9A7A0C";
                t["danger"]         = "#D64545";
                t["fill-control"]         = "rgba(0, 0, 0, 0.03)";
                t["fill-control-hover"]   = "rgba(0, 0, 0, 0.06)";
                t["fill-control-pressed"] = "rgba(0, 0, 0, 0.09)";
                t["success-subtle"]       = "rgba(30, 158, 99, 0.14)";
                t["warning-subtle"]       = "rgba(183, 121, 31, 0.14)";
                t["danger-subtle"]        = "rgba(214, 69, 69, 0.12)";
                t["scroll-thumb"]       = "rgba(0, 0, 0, 0.22)";
                t["scroll-thumb-hover"] = "rgba(0, 0, 0, 0.38)";
                return t;
            }

            const Theme* sCurrent = nullptr;

            /// First installed family of a list, the application font cannot take a fallback list
            QString firstAvailableFamily(const QStringList& candidates)
            {
                const QStringList installed = QFontDatabase::families();
                for (const QString& family : candidates)
                {
                    if (installed.contains(family))
                        return family;
                }
                return QString();
            }

#ifdef _WIN32
        // Windows draws title bars in the accent color of the system. Give every window the colors of the theme instead.
        void decorateWindow(QWidget* window, const Theme& theme)
        {
            if (!window || !window->isWindow())
                return;
            const HWND hwnd = reinterpret_cast<HWND>(window->winId());
            auto colorRef   = [](const QColor& c) { return static_cast<COLORREF>(c.red() | (c.green() << 8) | (c.blue() << 16)); };

            const BOOL dark = theme.mode() == Theme::Mode::Dark ? TRUE : FALSE;
            const COLORREF caption = colorRef(theme.color("surface-base"));
            const COLORREF text    = colorRef(theme.color("text"));
            const COLORREF border  = colorRef(theme.color("border"));
            // attributes 20, 34, 35 and 36 are only known to Windows 11, older systems ignore them
            DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));       // DWMWA_USE_IMMERSIVE_DARK_MODE
            DwmSetWindowAttribute(hwnd, 34, &border, sizeof(border));   // DWMWA_BORDER_COLOR
            DwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption)); // DWMWA_CAPTION_COLOR
            DwmSetWindowAttribute(hwnd, 36, &text, sizeof(text));       // DWMWA_TEXT_COLOR
        }

        /// Decorates every window when it is shown.
        class WindowDecorator : public QObject
        {
        public:
            using QObject::QObject;

        protected:
            bool eventFilter(QObject* watched, QEvent* event) override
            {
                if (event->type() == QEvent::Show && watched->isWidgetType())
                    decorateWindow(static_cast<QWidget*>(watched), Theme::current());
                return false;
            }
        };
#endif
        }    // namespace

        Theme::Theme(Mode mode, const QMap<QString, QString>& tokens) : mMode(mode), mTokens(tokens)
        {
        }

        const Theme& Theme::forMode(Mode mode)
        {
            static const Theme dark(Mode::Dark, darkTokens());
            static const Theme light(Mode::Light, lightTokens());
            return mode == Mode::Light ? light : dark;
        }

        const Theme& Theme::current()
        {
            return sCurrent ? *sCurrent : forMode(Mode::Dark);
        }

        Theme::Mode Theme::mode() const
        {
            return mMode;
        }

        QString Theme::value(const QString& token) const
        {
            auto it = mTokens.constFind(token);
            if (it == mTokens.constEnd())
            {
                log_warning("gui", "unknown style token '{}'", token.toStdString());
                return QString();
            }
            return it.value();
        }

        QColor Theme::color(const QString& token) const
        {
            return QColor(value(token));
        }

        QString Theme::render(const QString& stylesheetTemplate) const
        {
            static const QRegularExpression placeholder("@([a-z][a-z0-9-]*)@");
            QString result;
            result.reserve(stylesheetTemplate.size());

            qsizetype last = 0;
            auto matches   = placeholder.globalMatch(stylesheetTemplate);
            while (matches.hasNext())
            {
                const auto match = matches.next();
                result += QStringView(stylesheetTemplate).mid(last, match.capturedStart() - last);
                result += value(match.captured(1));
                last = match.capturedEnd();
            }
            result += QStringView(stylesheetTemplate).mid(last);
            return result;
        }

        QPalette Theme::palette() const
        {
            const bool dark = mMode == Mode::Dark;
            QPalette p;
            auto set = [&p](QPalette::ColorRole role, const QColor& color) { p.setColor(role, color); };

            set(QPalette::Window, color("surface-base"));
            set(QPalette::WindowText, color("text"));
            set(QPalette::Base, color("surface-canvas"));
            set(QPalette::AlternateBase, color("surface-raised"));
            set(QPalette::Text, color("text"));
            set(QPalette::Button, color("surface-raised"));
            set(QPalette::ButtonText, color("text"));
            set(QPalette::BrightText, color("danger"));
            set(QPalette::ToolTipBase, color("surface-overlay"));
            set(QPalette::ToolTipText, color("text"));
            set(QPalette::PlaceholderText, color("text-disabled"));
            set(QPalette::Highlight, color("selection"));
            set(QPalette::HighlightedText, color("text"));
            set(QPalette::Link, color("accent"));
            set(QPalette::LinkVisited, color("accent-pressed"));
            // bevel colors used by Fusion
            set(QPalette::Light, color("border-strong"));
            set(QPalette::Midlight, color("border"));
            set(QPalette::Mid, color("border-subtle"));
            set(QPalette::Dark, dark ? color("surface-canvas") : color("border-strong"));
            set(QPalette::Shadow, QColor(0, 0, 0));

            for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
                p.setColor(QPalette::Disabled, role, color("text-disabled"));
            return p;
        }

        void Theme::apply(QApplication& app, Mode mode)
        {
            const Theme& theme = forMode(mode);
            sCurrent           = &theme;

            // Fusion paints from the palette, which keeps every widget consistent. The stylesheet only adds shape and spacing.
            app.setStyle(QStyleFactory::create("Fusion"));
            app.setPalette(theme.palette());

            // dark window frames and menus follow the theme, not the system setting
            QGuiApplication::styleHints()->setColorScheme(mode == Mode::Dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

            const QString family = firstAvailableFamily({"Segoe UI Variable Text", "Segoe UI Variable", "Segoe UI", "Inter", "Roboto"});
            if (!family.isEmpty())
            {
                QFont font(family);
                font.setPixelSize(theme.value("font-size").remove("px").toInt());
                app.setFont(font);
            }

#ifdef _WIN32
            static WindowDecorator* decorator = nullptr;
            if (!decorator)
            {
                decorator = new WindowDecorator(&app);
                app.installEventFilter(decorator);
            }
            for (QWidget* window : QApplication::topLevelWidgets())
                decorateWindow(window, theme);
#endif
            app.setWindowIcon(QIcon(":/icons/hal"));

            QFile templateFile(":/style/modern");
            if (!templateFile.open(QFile::ReadOnly))
            {
                log_error("gui", "cannot read the stylesheet template");
                return;
            }
            app.setStyleSheet(theme.render(QString::fromUtf8(templateFile.readAll())));
        }
    }    // namespace style
}    // namespace hal
