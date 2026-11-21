#include "gui/style/theme.h"

#include "hal_core/utilities/log.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QStyleFactory>
#include <QStyleHints>

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
                    {"radius-control", "4px"},    // controls inside a page: buttons, inputs, list rows
                    {"radius-overlay", "8px"},    // menus, dialogs, popups
                    {"control-height", "28px"},
                    {"font-ui", "\"Segoe UI Variable Text\", \"Segoe UI\", sans-serif"},
                    {"font-mono", "\"Iosevka\", \"Cascadia Mono\", Consolas, monospace"},
                    {"font-size", "13px"},
                    {"font-size-small", "12px"},
                };
            }

            QMap<QString, QString> darkTokens()
            {
                QMap<QString, QString> t = commonTokens();
                // Surfaces: a tinted dark gray, never pure black. Lighter means closer to the user.
                t["surface-canvas"]  = "#0E1115";    // wells: editors, inputs, graph canvas
                t["surface-base"]    = "#14181D";    // window background
                t["surface-raised"]  = "#1A1F25";    // panels, docks, toolbars
                t["surface-overlay"] = "#222830";    // menus, popups, dialogs, tooltips
                // States are translucent steps, so they work on every surface
                t["state-hover"]    = "rgba(255, 255, 255, 0.06)";
                t["state-pressed"]  = "rgba(255, 255, 255, 0.10)";
                t["state-selected"] = "rgba(76, 154, 255, 0.24)";
                t["border-subtle"]  = "#242B33";
                t["border"]         = "#2F3741";
                t["border-strong"]  = "#444F5C";
                // Text hierarchy: 14:1, 7.5:1 and a disabled level on the base surface
                t["text"]          = "#E6EAF0";
                t["text-muted"]    = "#A4ADBA";
                t["text-disabled"] = "#6B7480";
                t["text-on-accent"] = "#07101C";
                // One accent, lighter and a bit less saturated than on light surfaces
                t["accent"]         = "#4C9AFF";
                t["accent-hover"]   = "#6BAEFF";
                t["accent-pressed"] = "#3A86E8";
                t["selection"]      = "#1E3A5F";
                t["success"]        = "#4CC38A";
                t["warning"]        = "#E5B454";
                t["danger"]         = "#F2706B";
                t["fill-control"]         = "rgba(255, 255, 255, 0.05)";    // resting fill of buttons
                t["fill-control-hover"]   = "rgba(255, 255, 255, 0.09)";
                t["fill-control-pressed"] = "rgba(255, 255, 255, 0.13)";
                t["success-subtle"]       = "rgba(76, 195, 138, 0.16)";    // tinted backgrounds instead of saturated fills
                t["warning-subtle"]       = "rgba(229, 180, 84, 0.16)";
                t["danger-subtle"]        = "rgba(242, 112, 107, 0.16)";
                t["scroll-thumb"]       = "rgba(255, 255, 255, 0.20)";
                t["scroll-thumb-hover"] = "rgba(255, 255, 255, 0.34)";
                return t;
            }

            QMap<QString, QString> lightTokens()
            {
                QMap<QString, QString> t = commonTokens();
                t["surface-canvas"]  = "#FFFFFF";
                t["surface-base"]    = "#F3F5F8";
                t["surface-raised"]  = "#FAFBFC";
                t["surface-overlay"] = "#FFFFFF";
                t["state-hover"]    = "rgba(0, 0, 0, 0.05)";
                t["state-pressed"]  = "rgba(0, 0, 0, 0.09)";
                t["state-selected"] = "rgba(31, 111, 224, 0.16)";
                t["border-subtle"]  = "#E3E7ED";
                t["border"]         = "#D3D9E1";
                t["border-strong"]  = "#AEB7C3";
                t["text"]          = "#1B2330";
                t["text-muted"]    = "#566172";
                t["text-disabled"] = "#9AA3AF";
                t["text-on-accent"] = "#FFFFFF";
                t["accent"]         = "#1F6FE0";
                t["accent-hover"]   = "#3A83EA";
                t["accent-pressed"] = "#1A5BC0";
                t["selection"]      = "#D6E6FF";
                t["success"]        = "#1E9E63";
                t["warning"]        = "#B7791F";
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
