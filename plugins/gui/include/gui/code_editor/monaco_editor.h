// MIT License
//
// Copyright (c) 2019 Ruhr University Bochum, Chair for Embedded Security. All Rights reserved.
// Copyright (c) 2019 Marc Fyrbiak, Sebastian Wallat, Max Hoffmann ("ORIGINAL AUTHORS"). All rights reserved.
// Copyright (c) 2021 Max Planck Institute for Security and Privacy. All Rights reserved.
// Copyright (c) 2021 Jörn Langheinrich, Julian Speith, Nils Albartus, René Walendy, Simon Klix ("ORIGINAL AUTHORS"). All Rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "gui/searchbar/searchoptions.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QWidget>

#include <memory>

class QJsonArray;
class QKeyEvent;

namespace hal
{
    /**
     * Text editor widget that embeds the Monaco editor, the editor component of VS Code, through Microsoft WebView2
     * (the Edge engine that ships with Windows).
     *
     * Monaco runs asynchronously inside a web page. The widget therefore mirrors the text and the modified state in C++,
     * which keeps toPlainText() and isModified() synchronous like the API of a QPlainTextEdit.
     *
     * @ingroup gui
     */
    class MonacoEditor : public QWidget
    {
        Q_OBJECT

    public:
        /// Display name and the text to insert for one code completion candidate.
        using Completion = QPair<QString, QString>;

        explicit MonacoEditor(QWidget* parent = nullptr);

        ~MonacoEditor() override;

        /// @returns the current text of the editor.
        QString toPlainText() const;

        /**
         * Replaces the text of the editor and resets the modified state. The undo history is kept apart from the new text.
         *
         * @param text - The new text.
         */
        void setPlainText(const QString& text);

        /// @returns `true` if the text differs from the state at the last call of setModified(false) or setPlainText().
        bool isModified() const;

        /**
         * Sets the modified state, e.g., `false` after saving.
         *
         * @param modified - The new state.
         */
        void setModified(bool modified);

        /**
         * Sets the Monaco language id used for syntax highlighting, e.g., `python` or `systemverilog`.
         *
         * @param language - The language id.
         */
        void setLanguage(const QString& language);

        void setFontSize(int size);
        void setMinimapEnabled(bool enabled);
        void setLineWrapEnabled(bool enabled);
        void setHighlightCurrentLineEnabled(bool enabled);
        void setLineNumberEnabled(bool enabled);

        /**
         * Answers a completion request that was announced by the completionRequested signal.
         *
         * @param requestId - Id from the signal.
         * @param completions - The candidates.
         */
        void setCompletions(int requestId, const QList<Completion>& completions);

        /// @returns `true` if matches of the last search are highlighted.
        bool hasSearchHighlights() const;

    public Q_SLOTS:
        /**
         * Highlights all matches of a text.
         *
         * @param text - The text to search for, an empty text clears the highlighting.
         * @param options - Search options.
         */
        void search(const QString& text, SearchOptions options = 8);

        void toggleMinimap();
        void toggleLineNumbers();

        void undo();
        void redo();

        /// Jumps to a line, the first line has number 1.
        void goToLine(int line);

        void focusEditor();

    Q_SIGNALS:
        /// Emitted when the text changed, either by the user or through setPlainText().
        void textChanged();

        /// Emitted when the modified state changed.
        void modificationChanged(bool modified);

        /// Emitted when the user presses a key while the editor has the focus.
        void keyPressed(QKeyEvent* event);

        /// Emitted when the user pressed Ctrl+S inside the editor.
        void saveRequested();

        /// Emitted when the user pressed Ctrl+Enter inside the editor.
        void runRequested();

        /// Emitted when the user zoomed the text with Ctrl+Plus, Ctrl+Minus, Ctrl+0 or Ctrl+mouse wheel.
        void fontSizeChanged(int size);

        /// Emitted when the editor asks for code completion, answer with setCompletions().
        void completionRequested(int requestId, const QString& textBeforeCursor);

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;

    private:
        class Private;

        void handleMessage(const QString& type, const QJsonArray& args);
        void handleReady();
        void handleContentChanged(const QString& text, bool modified);
        void handleModifiedChanged(bool modified);
        void runScript(const QString& script);
        void applyOptions();
        void updateBounds();
        static QString toJson(const QString& text);

        std::unique_ptr<Private> mPrivate;

        bool mReady            = false;
        bool mModified         = false;
        bool mHasHighlights    = false;
        QString mText;
        QString mLanguage      = "python";
        int mFontSize          = 14;
        bool mMinimap          = true;
        bool mLineWrap         = false;
        bool mHighlightLine    = true;
        bool mLineNumbers      = true;
    };
}    // namespace hal
