#include "gui/main_window/about_dialog.h"

#include "gui/toast/toast.h"
#include "hal_core/utilities/utils.h"
#include "hal_version.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSysInfo>
#include <QUrl>
#include <QVBoxLayout>

namespace hal
{
    namespace
    {
        const char* const PROJECT_URL = "https://github.com/horhulenko/hal-windows";
    }

    QString AboutDialog::detailsText() const
    {
        return QString("HAL %1\nBuild: %2\nQt: %3 (built with %4)\nSystem: %5")
            .arg(QString::fromStdString(hal_version::version))
            .arg(QString::fromStdString(hal_version::build_timestamp))
            .arg(qVersion())
            .arg(QT_VERSION_STR)
            .arg(QSysInfo::prettyProductName());
    }

    AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent), mLicenses(new QPlainTextEdit()), mLicensesButton(new QPushButton("Show open source licenses"))
    {
        setWindowTitle("About HAL");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setAttribute(Qt::WA_DeleteOnClose);
        setMinimumWidth(480);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(24, 24, 24, 20);
        root->setSpacing(20);

        // hero: logo, name and version
        auto* hero = new QHBoxLayout();
        hero->setSpacing(16);
        auto* logo = new QLabel();
        logo->setPixmap(QIcon(":/icons/hal").pixmap(QSize(56, 56)));
        logo->setFixedSize(56, 56);
        auto* titles = new QVBoxLayout();
        titles->setSpacing(2);
        auto* title = new QLabel("HAL");
        title->setObjectName("welcome-title");
        auto* subtitle = new QLabel(QString("Hardware Analyzer  ·  version %1").arg(QString::fromStdString(hal_version::version)));
        subtitle->setObjectName("welcome-subtitle");
        subtitle->setTextInteractionFlags(Qt::TextSelectableByMouse);
        titles->addWidget(title);
        titles->addWidget(subtitle);
        hero->addWidget(logo);
        hero->addLayout(titles, 1);
        root->addLayout(hero);

        // facts
        auto* grid = new QGridLayout();
        grid->setHorizontalSpacing(24);
        grid->setVerticalSpacing(8);
        const QList<QPair<QString, QString>> facts = {
            {"Version", QString("%1.%2.%3").arg(hal_version::major).arg(hal_version::minor).arg(hal_version::patch)},
            {"Built", QString::fromStdString(hal_version::build_timestamp)},
            {"Qt", QString("%1 (built with %2)").arg(qVersion(), QT_VERSION_STR)},
            {"System", QSysInfo::prettyProductName()},
        };
        int row = 0;
        for (const auto& fact : facts)
        {
            auto* key = new QLabel(fact.first);
            key->setObjectName("about-key");
            auto* value = new QLabel(fact.second);
            value->setObjectName("about-value");
            value->setTextInteractionFlags(Qt::TextSelectableByMouse);
            grid->addWidget(key, row, 0, Qt::AlignTop);
            grid->addWidget(value, row, 1);
            ++row;
        }
        grid->setColumnStretch(1, 1);
        root->addLayout(grid);

        // licenses stay folded until asked for
        mLicenses->setReadOnly(true);
        mLicenses->setPlainText(QString::fromStdString(utils::get_open_source_licenses()));
        mLicenses->setObjectName("about-licenses");
        mLicenses->setMinimumHeight(220);
        mLicenses->hide();
        root->addWidget(mLicenses);

        // actions
        auto* actions = new QHBoxLayout();
        actions->setSpacing(8);
        auto* project = new QPushButton("Project page");
        connect(project, &QPushButton::clicked, this, []() { QDesktopServices::openUrl(QUrl(PROJECT_URL)); });
        auto* copy = new QPushButton("Copy details");
        connect(copy, &QPushButton::clicked, this, &AboutDialog::copyDetails);
        connect(mLicensesButton, &QPushButton::clicked, this, &AboutDialog::toggleLicenses);
        auto* close = new QPushButton("Close");
        close->setProperty("primary", true);
        close->setDefault(true);
        connect(close, &QPushButton::clicked, this, &QDialog::close);
        actions->addWidget(project);
        actions->addWidget(copy);
        actions->addWidget(mLicensesButton);
        actions->addStretch();
        actions->addWidget(close);
        root->addLayout(actions);
    }

    void AboutDialog::toggleLicenses()
    {
        const bool show = mLicenses->isHidden();
        mLicenses->setVisible(show);
        mLicensesButton->setText(show ? "Hide open source licenses" : "Show open source licenses");
        adjustSize();
    }

    void AboutDialog::copyDetails()
    {
        QApplication::clipboard()->setText(detailsText());
        toast::success("Copied", "The version details are on the clipboard.");
    }
}    // namespace hal
