#include "gui/welcome_screen/welcome_screen.h"

#include "gui/frames/labeled_frame.h"
#include "gui/graphics_effects/shadow_effect.h"
#include "gui/welcome_screen/get_in_touch_widget.h"
#include "gui/welcome_screen/open_file_widget.h"
#include "gui/welcome_screen/recent_files_widget.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

namespace hal
{
    WelcomeScreen::WelcomeScreen(QWidget* parent)
        : QFrame(parent), mLayout(new QHBoxLayout()), mRecentFilesFrame(new LabeledFrame()), mOpenFileFrame(new LabeledFrame()), mGetInTouchFrame(new LabeledFrame()),
          mRecentFilesWidget(new RecentFilesWidget()), mOpenFileWidget(new OpenFileWidget()), mGetInTouchWidget(new GetInTouchWidget())
    {
        mLayout->setContentsMargins(32, 32, 32, 32);
        mLayout->setSpacing(0);
        mLayout->setAlignment(Qt::AlignCenter);

        mRecentFilesFrame->setObjectName("recent-files-frame");
        mOpenFileFrame->setObjectName("open-file-frame");
        mGetInTouchFrame->setObjectName("get-in-touch-frame");

        mRecentFilesFrame->addContent(mRecentFilesWidget);
        mOpenFileFrame->addContent(mOpenFileWidget);
        mGetInTouchFrame->addContent(mGetInTouchWidget);

        // One centered column: a hero header, then the recent projects next to the open and help cards
        auto* column = new QWidget();
        column->setObjectName("welcome-column");
        column->setMaximumWidth(1080);
        column->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto* columnLayout = new QVBoxLayout(column);
        columnLayout->setContentsMargins(0, 0, 0, 0);
        columnLayout->setSpacing(32);

        auto* hero       = new QWidget();
        auto* heroLayout = new QHBoxLayout(hero);
        heroLayout->setContentsMargins(0, 0, 0, 0);
        heroLayout->setSpacing(16);
        auto* logo = new QLabel();
        logo->setPixmap(QIcon(":/icons/hal").pixmap(QSize(48, 48)));
        logo->setFixedSize(48, 48);
        auto* titles      = new QVBoxLayout();
        titles->setSpacing(2);
        auto* title = new QLabel("HAL");
        title->setObjectName("welcome-title");
        auto* subtitle = new QLabel("Hardware Analyzer for netlist reverse engineering");
        subtitle->setObjectName("welcome-subtitle");
        titles->addWidget(title);
        titles->addWidget(subtitle);
        heroLayout->addWidget(logo);
        heroLayout->addLayout(titles);
        heroLayout->addStretch();
        columnLayout->addWidget(hero);

        auto* body = new QHBoxLayout();
        body->setSpacing(24);
        auto* side = new QVBoxLayout();
        side->setSpacing(24);
        side->addWidget(mOpenFileFrame);
        side->addWidget(mGetInTouchFrame);
        side->addStretch();
        body->addWidget(mRecentFilesFrame, 5);
        body->addLayout(side, 4);
        columnLayout->addLayout(body);

        setLayout(mLayout);
        mLayout->addWidget(column);

        repolish();    // CALL FROM PARENT
    }

    void WelcomeScreen::repolish()
    {
        QStyle* s = style();

        s->unpolish(this);
        s->polish(this);

        mRecentFilesWidget->repolish();
        mOpenFileWidget->repolish();
        mGetInTouchWidget->repolish();
    }
}
