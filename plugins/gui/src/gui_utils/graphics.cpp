#include "gui/gui_utils/graphics.h"

#include "gui/style/theme.h"
#include "gui/svg_icon_engine/svg_icon_engine.h"

#include <QFile>
#include <QImage>
#include <QRegularExpression>

namespace hal
{
    namespace gui_utility
    {
        void changeSvgColor(QString& svg_data, const QColor& from, const QColor& to)
        {
            QRegularExpression regex(from.name());
            svg_data.replace(regex, to.name().toUtf8());
        }

        void changeAllSvgColors(QString& svg_data, const QColor& to)
        {
            QRegularExpression regex("#[0-9a-f]{6}",QRegularExpression::CaseInsensitiveOption);
            svg_data.replace(regex, to.name().toUtf8());
        }

        bool applyColorStyle(QString& svg_data, const QString& color_style)
        {
            QRegularExpression color_regex("#[0-9a-f]{6}", QRegularExpression::CaseInsensitiveOption);
            QRegularExpression all_to_color_regex("\\s*all\\s*->\\s*#[0-9a-f]{6}\\s*", QRegularExpression::CaseInsensitiveOption);
            QRegularExpression color_to_color_regex("\\s*(#[0-9a-f]{6}\\s*->\\s*#[0-9a-f]{6}\\s*,\\s*)*#[0-9a-f]{6}\\s*->\\s*#[0-9a-f]{6}\\s*", QRegularExpression::CaseInsensitiveOption);
            if (all_to_color_regex.match(color_style).hasMatch())
            {
                QString color = color_regex.match(color_style).captured();
                svg_data.replace(color_regex, color.toUtf8());
            }
            else if (color_to_color_regex.match(color_style).hasMatch())
            {
                QString copy = color_style;
                copy         = copy.simplified();
                copy.replace(" ", "");

                QStringList list = copy.split(",");

                for (QString string : list)
                {
                    QString from_color = string.left(7);
                    QString to_color   = string.right(7);

                    QRegularExpression regex(from_color);
                    svg_data.replace(regex, to_color.toUtf8());
                }
            }
            else
            {
                return false;
            }
            return true;
        }


        QIcon getStyledSvgIcon(const QString& from_to_colors_enabled, const QString& svg_path, QString from_to_colors_disabled)
        {
            QFile file(svg_path);
            if (!file.open(QFile::ReadOnly))
            {
                //TODO: print error
                return QIcon();
            }

            QString svgDataEnabled(file.readAll());
            if (svgDataEnabled.isEmpty())
            {
                //TODO: print error
                return QIcon();
            }

            const style::Theme& theme = style::Theme::current();

            // Monochrome icons come black, without a style they get the neutral icon color of the theme.
            // Artwork with other colors (the HAL logo, brand icons) is left untouched.
            bool monochrome = true;
            auto colors = QRegularExpression("#[0-9a-fA-F]{6}").globalMatch(svgDataEnabled);
            while (colors.hasNext())
            {
                if (colors.next().captured().compare("#000000", Qt::CaseInsensitive) != 0)
                    monochrome = false;
            }
            const QString enabledStyle  = (from_to_colors_enabled.isEmpty() && monochrome) ? "all->" + theme.color("icon").name() : from_to_colors_enabled;
            const QString disabledStyle = (from_to_colors_disabled.isEmpty() && monochrome) ? "all->" + theme.color("text-disabled").name() : from_to_colors_disabled;

            QString svgDataDisabled(svgDataEnabled);
            if (!enabledStyle.isEmpty() && !applyColorStyle(svgDataEnabled, enabledStyle))
            {
                //TODO: print error
                return QIcon();
            }
            if (disabledStyle.isEmpty() || !applyColorStyle(svgDataDisabled, disabledStyle))
                svgDataDisabled = QString();

            // while hovered, neutral icons switch to the text color, colored (semantic) icons keep their color
            QString svgDataActive(svgDataEnabled);
            const QColor tint = QColor(QRegularExpression("#[0-9a-fA-F]{6}").match(svgDataEnabled).captured());
            if (tint.isValid() && tint.saturation() < 64)
                changeAllSvgColors(svgDataActive, theme.color("text"));

            return QIcon(new SvgIconEngine(svgDataEnabled.toUtf8(), svgDataDisabled.toUtf8(), svgDataActive.toUtf8()));
        }
    }    // namespace gui_utility
}
