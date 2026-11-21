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

#include "gui/code_editor/monaco_editor.h"

#include <QUuid>

namespace hal
{
    /**
     * Editor for Python scripts, based on the Monaco editor (the editor component of VS Code).
     * Code completion is answered by the embedded Python interpreter through jedi.
     *
     * @ingroup python
     */
    class PythonCodeEditor : public MonacoEditor
    {
        Q_OBJECT
    public:
        /**
         * Constructor.
         *
         * @param parent - The parent widget.
         */
        PythonCodeEditor(QWidget* parent = nullptr);

        /**
         * Get the file name relative to the project's python directory.
         *
         * @returns The relative file name, empty if the script was never saved.
         */
        QString getRelFilename() const;

        /**
         * Get the absolute path of the script's file.
         *
         * @returns The absolute file name.
         */
        QString getAbsFilename() const;

        /**
         * Set the name of the file this editor belongs to. A path inside the project's python directory is stored relative.
         *
         * @param name - The file name.
         */
        void setFilename(const QString& name);

        /**
         * Remember whether the file on disk changed since it was loaded into the editor.
         *
         * @param base_file_modified - `true` if the file on disk was modified.
         */
        void setBaseFileModified(bool base_file_modified);

        /**
         * @returns `true` if the file on disk was modified since it was loaded.
         */
        bool isBaseFileModified();

        /**
         * @returns The unique id of this editor.
         */
        QUuid getUuid() const;

    private Q_SLOTS:
        void handleCompletionRequested(int requestId, const QString& textBeforeCursor);

    private:
        QString mRelFilename;

        bool mBaseFileModified;

        QUuid mUuid;
    };
}    // namespace hal
