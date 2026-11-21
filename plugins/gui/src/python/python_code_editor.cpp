#include "gui/python/python_code_editor.h"

#include "gui/gui_globals.h"
#include "gui/python/python_context.h"
#include "hal_core/netlist/project_manager.h"
#include "hal_core/utilities/log.h"

#include <QDir>

namespace hal
{
    PythonCodeEditor::PythonCodeEditor(QWidget* parent) : MonacoEditor(parent), mBaseFileModified(false), mUuid(QUuid::createUuid())
    {
        setLanguage("python");
        connect(this, &MonacoEditor::completionRequested, this, &PythonCodeEditor::handleCompletionRequested);
    }

    void PythonCodeEditor::handleCompletionRequested(int requestId, const QString& textBeforeCursor)
    {
        QList<Completion> completions;
        if (gPythonContext)
        {
            for (const auto& [name, suffix] : gPythonContext->complete(textBeforeCursor, false))
                completions.append({QString::fromStdString(name), QString::fromStdString(suffix)});
        }
        setCompletions(requestId, completions);
    }

    QString PythonCodeEditor::getRelFilename() const
    {
        return mRelFilename;
    }

    QString PythonCodeEditor::getAbsFilename() const
    {
        if (mRelFilename.isEmpty() || mRelFilename.startsWith('/'))
            return mRelFilename;
        QDir projectPydir(QString::fromStdU16String((ProjectManager::instance()->get_project_directory().get_filename("py")).u16string()));
        return projectPydir.absoluteFilePath(mRelFilename);
    }

    void PythonCodeEditor::setFilename(const QString &name)
    {
        mRelFilename = name;
        QString projectLoc = QString::fromStdU16String((ProjectManager::instance()->get_project_directory().get_filename("py")).u16string()) + '/';
        if (name.startsWith(projectLoc))
            mRelFilename.remove(0,projectLoc.size());
    }

    void PythonCodeEditor::setBaseFileModified(bool base_file_modified)
    {
        mBaseFileModified = base_file_modified;
    }

    bool PythonCodeEditor::isBaseFileModified()
    {
        return mBaseFileModified;

    }

    QUuid PythonCodeEditor::getUuid() const
    {
        return mUuid;
    }
}
