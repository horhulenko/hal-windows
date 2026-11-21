#include "gui/toast/toast_widget.h"

#include "gui/gui_utils/graphics.h"
#include "gui/style/theme.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace hal
{
    namespace
    {
        const char* levelName(ToastManager::Level level)
        {
            switch (level)
            {
                case ToastManager::Level::Success:
                    return "success";
                case ToastManager::Level::Warning:
                    return "warning";
                case ToastManager::Level::Error:
                    return "error";
                default:
                    return "info";
            }
        }

        QString levelToken(ToastManager::Level level)
        {
            switch (level)
            {
                case ToastManager::Level::Success:
                    return "success";
                case ToastManager::Level::Warning:
                    return "warning";
                case ToastManager::Level::Error:
                    return "danger";
                default:
                    return "accent";
            }
        }

        QString levelIconPath(ToastManager::Level level)
        {
            switch (level)
            {
                case ToastManager::Level::Success:
                    return ":/icons/toast-success";
                case ToastManager::Level::Warning:
                    return ":/icons/toast-warning";
                case ToastManager::Level::Error:
                    return ":/icons/toast-error";
                default:
                    return ":/icons/info";
            }
        }

        const int MESSAGE_LIMIT = 280;    // longer messages are collapsed behind "Show more"
        const int RESUME_MS     = 2000;    // a toast never vanishes right after the pointer left it
        const int DRAG_START    = 6;
    }    // namespace

    ToastWidget::ToastWidget(ToastManager::Level level, const QString& title, const QString& message, const QList<ToastManager::Action>& actions, int durationMs, QWidget* parent)
        : QFrame(parent), mLevel(level), mTitle(title), mMessage(message), mTitleLabel(new QLabel(title)), mCountLabel(new QLabel()), mDurationMs(durationMs), mRemainingMs(durationMs)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setProperty("level", levelName(level));
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setAccessibleName(title);
        setAccessibleDescription(message);

        auto* outer = new QHBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        auto* bar = new QFrame();
        bar->setObjectName("toast-bar");
        bar->setFixedWidth(4);
        outer->addWidget(bar);

        auto* content = new QHBoxLayout();
        content->setContentsMargins(12, 12, 8, 12);
        content->setSpacing(12);
        outer->addLayout(content, 1);

        const QColor tint = style::Theme::current().color(levelToken(level));
        auto* icon        = new QLabel();
        icon->setPixmap(gui_utility::getStyledSvgIcon("all->" + tint.name(), levelIconPath(level)).pixmap(QSize(20, 20)));
        icon->setFixedSize(20, 20);
        content->addWidget(icon, 0, Qt::AlignTop);

        auto* column = new QVBoxLayout();
        column->setSpacing(4);
        content->addLayout(column, 1);

        auto* titleRow = new QHBoxLayout();
        titleRow->setSpacing(8);
        mTitleLabel->setObjectName("toast-title");
        mTitleLabel->setWordWrap(true);
        mCountLabel->setObjectName("toast-count");
        mCountLabel->hide();
        titleRow->addWidget(mTitleLabel, 1);
        titleRow->addWidget(mCountLabel, 0, Qt::AlignTop);
        column->addLayout(titleRow);

        if (!message.isEmpty())
        {
            auto* messageLabel = new QLabel(message.size() > MESSAGE_LIMIT ? message.left(MESSAGE_LIMIT).trimmed() + "…" : message);
            messageLabel->setObjectName("toast-message");
            messageLabel->setWordWrap(true);
            messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            messageLabel->setCursor(Qt::IBeamCursor);
            column->addWidget(messageLabel);

            if (message.size() > MESSAGE_LIMIT)
            {
                auto* more = new QPushButton("Show more");
                more->setObjectName("toast-link");
                more->setFlat(true);
                more->setCursor(Qt::PointingHandCursor);
                connect(more, &QPushButton::clicked, this, [this, messageLabel, more]() {
                    messageLabel->setText(mMessage);
                    more->hide();
                    // the toast has grown, the manager has to place it again
                    Q_EMIT dragFinished();
                });
                column->addWidget(more, 0, Qt::AlignLeft);
            }
        }

        if (!actions.isEmpty())
        {
            auto* row = new QHBoxLayout();
            row->setSpacing(8);
            row->setContentsMargins(0, 6, 0, 0);
            for (const auto& action : actions)
            {
                auto* button = new QPushButton(action.label);
                button->setObjectName("toast-action");
                button->setCursor(Qt::PointingHandCursor);
                auto callback = action.callback;
                connect(button, &QPushButton::clicked, this, [this, callback]() {
                    if (callback)
                        callback();
                    Q_EMIT closeRequested(this);
                });
                row->addWidget(button);
            }
            row->addStretch();
            column->addLayout(row);
        }

        auto* close = new QToolButton();
        close->setObjectName("toast-close");
        close->setIcon(gui_utility::getStyledSvgIcon("", ":/icons/cross"));
        close->setIconSize(QSize(14, 14));
        close->setToolTip("Close");
        close->setCursor(Qt::PointingHandCursor);
        close->setAccessibleName("Close notification");
        connect(close, &QToolButton::clicked, this, [this]() { Q_EMIT closeRequested(this); });
        content->addWidget(close, 0, Qt::AlignTop);

        mTimer.setInterval(50);
        connect(&mTimer, &QTimer::timeout, this, &ToastWidget::tick);
        restartTimer();
    }

    ToastManager::Level ToastWidget::level() const
    {
        return mLevel;
    }

    QString ToastWidget::title() const
    {
        return mTitle;
    }

    QString ToastWidget::message() const
    {
        return mMessage;
    }

    void ToastWidget::repeat()
    {
        ++mCount;
        mCountLabel->setText(QString("×%1").arg(mCount));
        mCountLabel->show();
        restartTimer();
    }

    void ToastWidget::restartTimer()
    {
        mRemainingMs = mDurationMs;
        if (mDurationMs > 0)
        {
            mClock.start();
            mTimer.start();
        }
        else
        {
            mTimer.stop();
        }
        update();
    }

    void ToastWidget::tick()
    {
        const qint64 elapsed = mClock.restart();

        const QWidget* focus = QApplication::focusWidget();
        mFocused             = focus && isAncestorOf(focus);

        if (mHovered || mFocused || mPressed)
            return;

        mRemainingMs -= static_cast<int>(elapsed);
        if (mRemainingMs <= 0)
        {
            mTimer.stop();
            Q_EMIT closeRequested(this);
            return;
        }
        update();
    }

    void ToastWidget::enterEvent(QEnterEvent* event)
    {
        QFrame::enterEvent(event);
        mHovered = true;
    }

    void ToastWidget::leaveEvent(QEvent* event)
    {
        QFrame::leaveEvent(event);
        mHovered = false;
        if (mDurationMs > 0 && mRemainingMs < RESUME_MS)
            mRemainingMs = RESUME_MS;
    }

    void ToastWidget::focusInEvent(QFocusEvent* event)
    {
        QFrame::focusInEvent(event);
        mFocused = true;
    }

    void ToastWidget::mousePressEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::MiddleButton)
        {
            Q_EMIT closeRequested(this);
            return;
        }
        if (event->button() == Qt::LeftButton)
        {
            mPressed       = true;
            mDragged       = false;
            mPressPosition = event->globalPosition().toPoint();
            mLastPosition  = mPressPosition;
        }
        QFrame::mousePressEvent(event);
    }

    void ToastWidget::mouseMoveEvent(QMouseEvent* event)
    {
        if (mPressed)
        {
            const QPoint now = event->globalPosition().toPoint();
            if (!mDragged && (now - mPressPosition).manhattanLength() >= DRAG_START)
            {
                mDragged = true;
                setCursor(Qt::ClosedHandCursor);
                Q_EMIT dragStarted();
            }
            if (mDragged)
            {
                Q_EMIT dragMoved(now - mLastPosition);
                mLastPosition = now;
            }
        }
        QFrame::mouseMoveEvent(event);
    }

    void ToastWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton && mPressed)
        {
            mPressed = false;
            if (mDragged)
            {
                mDragged = false;
                setCursor(Qt::ArrowCursor);
                Q_EMIT dragFinished();
            }
        }
        QFrame::mouseReleaseEvent(event);
    }

    void ToastWidget::keyPressEvent(QKeyEvent* event)
    {
        if (event->key() == Qt::Key_Escape)
        {
            Q_EMIT closeRequested(this);
            return;
        }
        QFrame::keyPressEvent(event);
    }

    void ToastWidget::paintEvent(QPaintEvent* event)
    {
        QFrame::paintEvent(event);

        // thin countdown line at the bottom edge, it shows how long the toast stays
        if (mDurationMs > 0)
        {
            QPainter painter(this);
            QColor color = style::Theme::current().color(levelToken(mLevel));
            color.setAlphaF(mHovered || mFocused ? 0.35 : 0.7);
            const int barLeft  = 4;
            const int width    = static_cast<int>((this->width() - barLeft) * (static_cast<double>(qMax(0, mRemainingMs)) / mDurationMs));
            painter.fillRect(QRect(barLeft, height() - 2, width, 2), color);
        }
    }

    ToastSummary::ToastSummary(QWidget* parent) : QFrame(parent), mLabel(new QLabel())
    {
        setAttribute(Qt::WA_StyledBackground, true);
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 4, 4, 4);
        layout->setSpacing(8);
        mLabel->setObjectName("toast-summary-label");
        layout->addWidget(mLabel, 1);

        auto* dismiss = new QPushButton("Dismiss all");
        dismiss->setObjectName("toast-link");
        dismiss->setFlat(true);
        dismiss->setCursor(Qt::PointingHandCursor);
        connect(dismiss, &QPushButton::clicked, this, &ToastSummary::dismissAllRequested);
        layout->addWidget(dismiss);
    }

    void ToastSummary::setWaiting(int waiting, int visible)
    {
        mLabel->setText(waiting > 0 ? QString("+%1 more waiting").arg(waiting) : QString("%1 notifications").arg(visible));
    }
}    // namespace hal
