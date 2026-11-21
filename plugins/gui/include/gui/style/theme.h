#pragma once

#include <QColor>
#include <QMap>
#include <QPalette>
#include <QString>

class QApplication;

namespace hal
{
    namespace style
    {
        /**
         * The design tokens of the GUI, the single source of every color, font and metric.
         *
         * Tokens are named by role (`surface-raised`, `text-muted`, `accent`), never by appearance, so a theme only changes
         * the values. The stylesheet template `:/style/modern` refers to tokens as `@token-name@`, the palette of the
         * application is derived from the same values.
         *
         * Surfaces get lighter the closer they are to the user (canvas < base < raised < overlay), borders and state overlays
         * are translucent steps on top, and a single accent color marks selection, focus and primary actions.
         *
         * @ingroup gui
         */
        class Theme
        {
        public:
            enum class Mode
            {
                Dark,
                Light
            };

            /// @returns the theme for a mode.
            static const Theme& forMode(Mode mode);

            /// @returns the theme that is currently applied.
            static const Theme& current();

            /**
             * Applies a theme to the application: Fusion base style, UI font, palette, system color scheme (dark title bars)
             * and the rendered stylesheet. Cheap enough to be called when the user switches themes, but not on every change.
             *
             * @param app - The application.
             * @param mode - The theme to apply.
             */
            static void apply(QApplication& app, Mode mode);

            /**
             * @param token - Token name, e.g., `surface-base`.
             * @returns The value as it appears in the stylesheet, e.g. `#14181D` or `rgba(255, 255, 255, 0.06)`.
             */
            QString value(const QString& token) const;

            /**
             * @param token - Token name of an opaque color.
             * @returns The color.
             */
            QColor color(const QString& token) const;

            /// @returns the mode of this theme.
            Mode mode() const;

            /// @returns the palette that matches the tokens.
            QPalette palette() const;

            /**
             * Replaces every `@token@` of a stylesheet template by its value.
             *
             * @param stylesheetTemplate - The template.
             * @returns The stylesheet.
             */
            QString render(const QString& stylesheetTemplate) const;

        private:
            explicit Theme(Mode mode, const QMap<QString, QString>& tokens);

            Mode mMode;
            QMap<QString, QString> mTokens;
        };
    }    // namespace style
}    // namespace hal
