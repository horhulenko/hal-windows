#include "gui/welcome_screen/open_file_widget.h"
#include "gui/toast/toast.h"

#include "gui/export/import_project_dialog.h"
#include "gui/file_manager/file_manager.h"
#include "gui/gui_utils/graphics.h"
#include "gui/user_action/action_open_netlist_file.h"
#include "hal_core/utilities/log.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace hal
{
    OpenFileWidget::OpenFileWidget(QWidget* parent) : QFrame(parent), mLayout(new QVBoxLayout()), mTextLabel(new QLabel()), mIconLabel(new QLabel()), mDragActive(false)
    {
        setAcceptDrops(true);

        // A drop zone in the middle of a card: icon, headline, hint and the three ways to start
        mIconLabel->setObjectName("icon-label");
        mIconLabel->setAlignment(Qt::AlignCenter);

        mTextLabel->setObjectName("text-label");
        mTextLabel->setText("Drop a project folder, netlist or ZIP archive here");
        mTextLabel->setWordWrap(true);
        mTextLabel->setAlignment(Qt::AlignCenter);

        auto* hint = new QLabel("or choose how to start");
        hint->setObjectName("hint-label");
        hint->setAlignment(Qt::AlignCenter);

        auto* open = new QPushButton("Open project…");
        open->setProperty("primary", true);
        open->setCursor(Qt::PointingHandCursor);
        connect(open, &QPushButton::clicked, this, &OpenFileWidget::openProjectRequested);

        auto* import = new QPushButton("Import netlist…");
        import->setCursor(Qt::PointingHandCursor);
        connect(import, &QPushButton::clicked, this, &OpenFileWidget::importNetlistRequested);

        auto* create = new QPushButton("New project");
        create->setCursor(Qt::PointingHandCursor);
        connect(create, &QPushButton::clicked, this, &OpenFileWidget::newProjectRequested);

        auto* buttons = new QHBoxLayout();
        buttons->setSpacing(8);
        buttons->addStretch();
        buttons->addWidget(open);
        buttons->addWidget(import);
        buttons->addWidget(create);
        buttons->addStretch();

        setLayout(mLayout);
        mLayout->setContentsMargins(24, 24, 24, 24);
        mLayout->setSpacing(12);
        mLayout->addStretch();
        mLayout->addWidget(mIconLabel);
        mLayout->addWidget(mTextLabel);
        mLayout->addWidget(hint);
        mLayout->addSpacing(8);
        mLayout->addLayout(buttons);
        mLayout->addStretch();
    }

    void OpenFileWidget::dragEnterEvent(QDragEnterEvent* event)
    {
        const QMimeData* mime_data = event->mimeData();
        if (mime_data->hasUrls())
        {
            event->acceptProposedAction();
            mDragActive = true;
            repolish();
        }
    }

    void OpenFileWidget::dragLeaveEvent(QDragLeaveEvent* event)
    {
        Q_UNUSED(event)

        mDragActive = false;
        repolish();
    }

    void OpenFileWidget::dropEvent(QDropEvent* event)
    {
        // pass url list to file manager and check for matching file extensions,
        //if match found -> show progress screen else do nothing
        const QMimeData* mime_data = event->mimeData();
        if (mime_data->hasUrls())
        {
            //        QStringList path_list;
            //        QList<QUrl> url_list = mime_data->urls();

            //        if (url_list.size() > 1)
            //            return;

            QString file                      = mime_data->urls().at(0).toLocalFile();
            if (QFileInfo(file).suffix().toLower()=="zip")
            {
                qDebug() << "zip file" << file;
                ImportProjectDialog ipd(this);
                ipd.setZippedFile(file);
                if (ipd.exec() == QDialog::Accepted)
                {
                    if (ipd.importProject())
                    {
                        ActionOpenNetlistFile* act = new ActionOpenNetlistFile(ActionOpenNetlistFile::OpenProject,
                                                                               ipd.extractedProjectAbsolutePath());
                        act->exec();
                    }
                    else
                        toast::warning("Import Project Failed",
                                             "Failed to extract a HAL project from selected archive file.\n"
                                             "You might want to uncompress the archive manually and try to fetchGateLibrary the project.");
                }
            }

            FileManager::DirectoryStatus stat = FileManager::directoryStatus(file);

            switch (stat)
            {
                case FileManager::ProjectDirectory:
                    FileManager::get_instance()->openProject(file);
                    break;
                case FileManager::OtherDirectory:
                    log_warning("gui", "directory {} is not a hal project.", file.toStdString());
                    toast::warning("Not a HAL project", file + "\n" + FileManager::directoryStatusText(stat));
                    break;
                case FileManager::IsFile:
                    FileManager::get_instance()->importFile(file);
                    break;
                default:
                    log_warning("gui", "cannot open nor import {}.", file.toStdString());
                    break;
            }

            //        for (int i = 0; i < url_list.size() && i < 32; ++i)
            //        {
            //            path_list.append(url_list.at(i).toLocalFile());
            //        }

            event->acceptProposedAction();
            mDragActive = false;
            repolish();
        }
    }

    bool OpenFileWidget::dragActive()
    {
        return mDragActive;
    }

    QString OpenFileWidget::iconStyle()
    {
        return mIconStyle;
    }

    QString OpenFileWidget::iconPath()
    {
        return mIconPath;
    }

    void OpenFileWidget::setIconStyle(const QString& style)
    {
        mIconStyle = style;
    }

    void OpenFileWidget::setIconPath(const QString& path)
    {
        mIconPath = path;
    }

    void OpenFileWidget::repolish()
    {
        QStyle* s = style();

        s->unpolish(this);
        s->polish(this);

        if (!mIconPath.isEmpty())
            mIconLabel->setPixmap(gui_utility::getStyledSvgIcon(mIconStyle, mIconPath).pixmap(QSize(40, 40)));
    }
}    // namespace hal
