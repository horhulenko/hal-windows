#include "gui/toast/toast_manager.h"

#include "gui/toast/toast.h"
#include "gui/toast/toast_widget.h"

#include <QApplication>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QRect>
#include <QSettings>
#include <QThread>
#include <QVBoxLayout>

namespace hal
{
    namespace
    {
        const int MAX_VISIBLE   = 4;
        const int TOAST_WIDTH   = 380;
        const int GAP           = 8;
        const int INSET_SIDE    = 56;     // clears the dock rails (40 px) and leaves a margin
        const int INSET_TOP     = 104;    // clears menu bar, toolbar and panel headers
        const int INSET_BOTTOM  = 48;     // clears the bottom dock bar
        const int ANIMATION_MS  = 140;

        void fade(QWidget* widget, double from, double to, int ms, std::function<void()> done = nullptr)
        {
            auto* effect = new QGraphicsOpacityEffect(widget);
            effect->setOpacity(from);
            widget->setGraphicsEffect(effect);
            auto* animation = new QPropertyAnimation(effect, "opacity", widget);
            animation->setDuration(ms);
            animation->setStartValue(from);
            animation->setEndValue(to);
            QObject::connect(animation, &QPropertyAnimation::finished, widget, [widget, to, done]() {
                if (to >= 1.0)
                    widget->setGraphicsEffect(nullptr);    // the effect makes rendering slower, drop it when it is not needed
                if (done)
                    done();
            });
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }    // namespace

    ToastManager& ToastManager::instance()
    {
        static ToastManager manager;
        return manager;
    }

    ToastManager::ToastManager() : QObject(nullptr)
    {
    }

    int ToastManager::automaticDuration(Level level, int characters, bool hasActions)
    {
        // about the time it takes to read the text, plus a base so that short texts are noticed
        switch (level)
        {
            case Level::Error:
                return 0;    // errors stay until the user closes them
            case Level::Warning:
                return hasActions ? 0 : qMin(20000, 7000 + characters * 60);
            case Level::Success:
                return qMax(hasActions ? 8000 : 0, qMin(8000, 3000 + characters * 45));
            case Level::Info:
            default:
                return qMax(hasActions ? 8000 : 0, qMin(12000, 3500 + characters * 55));
        }
    }

    void ToastManager::attach(QWidget* host)
    {
        if (mHost)
            mHost->removeEventFilter(this);
        mHost = host;
        if (!mHost)
            return;
        mHost->installEventFilter(this);

        const QString saved = QSettings().value("toasts/corner", "bottom-right").toString();
        if (saved == "top-left")
            mCorner = Corner::TopLeft;
        else if (saved == "top-right")
            mCorner = Corner::TopRight;
        else if (saved == "bottom-left")
            mCorner = Corner::BottomLeft;
        else
            mCorner = Corner::BottomRight;
    }

    bool ToastManager::isAttached() const
    {
        return mHost != nullptr;
    }

    void ToastManager::show(Level level, const QString& title, const QString& message, const Options& options)
    {
        if (QThread::currentThread() != thread())
        {
            QMetaObject::invokeMethod(this, [this, level, title, message, options]() { show(level, title, message, options); }, Qt::QueuedConnection);
            return;
        }
        if (!mHost)
            return;

        // an identical toast that is already on screen gets a counter instead of a second card
        for (ToastWidget* toast : mVisible)
        {
            if (toast->level() == level && toast->title() == title && toast->message() == message)
            {
                toast->repeat();
                return;
            }
        }
        for (const Pending& pending : mQueue)
        {
            if (pending.level == level && pending.title == title && pending.message == message)
                return;
        }

        Pending pending{level, title, message, options};
        if (mVisible.size() >= MAX_VISIBLE)
        {
            mQueue.append(pending);
            updateSummary();
            relayout(true);
            return;
        }
        showNow(pending);
    }

    void ToastManager::showNow(const Pending& pending)
    {
        const bool hasActions = !pending.options.actions.isEmpty();
        const int duration    = pending.options.durationMs >= 0 ? pending.options.durationMs : automaticDuration(pending.level, pending.title.size() + pending.message.size(), hasActions);

        auto* toast = new ToastWidget(pending.level, pending.title, pending.message, pending.options.actions, duration, mHost);
        connect(toast, &ToastWidget::closeRequested, this, &ToastManager::handleCloseRequested);
        connect(toast, &ToastWidget::dragStarted, this, &ToastManager::handleDragStarted);
        connect(toast, &ToastWidget::dragMoved, this, &ToastManager::handleDragMoved);
        connect(toast, &ToastWidget::dragFinished, this, &ToastManager::handleDragFinished);

        mVisible.append(toast);
        toast->setFixedWidth(qMin(TOAST_WIDTH, qMax(240, mHost->width() - 2 * 16)));
        toast->show();
        toast->raise();
        relayout(false);
        fade(toast, 0.0, 1.0, ANIMATION_MS);
        updateSummary();
    }

    void ToastManager::dismissAll()
    {
        mQueue.clear();
        const auto visible = mVisible;
        for (ToastWidget* toast : visible)
            handleCloseRequested(toast);
        updateSummary();
    }

    void ToastManager::handleCloseRequested(ToastWidget* toast)
    {
        if (!mVisible.contains(toast))
            return;
        mVisible.removeOne(toast);
        toast->setEnabled(false);
        fade(toast, 1.0, 0.0, ANIMATION_MS / 2, [toast]() { toast->deleteLater(); });
        fillFromQueue();
        relayout(true);
        updateSummary();
    }

    void ToastManager::fillFromQueue()
    {
        while (mVisible.size() < MAX_VISIBLE && !mQueue.isEmpty())
            showNow(mQueue.takeFirst());
    }

    void ToastManager::updateSummary()
    {
        const bool needed = mVisible.size() + mQueue.size() >= 3;
        if (needed && !mSummary && mHost)
        {
            mSummary = new ToastSummary(mHost);
            connect(mSummary, &ToastSummary::dismissAllRequested, this, &ToastManager::dismissAll);
        }
        if (!mSummary)
            return;
        if (needed)
        {
            mSummary->setWaiting(mQueue.size(), mVisible.size());
            mSummary->show();
            mSummary->raise();
            relayout(false);
        }
        else
        {
            mSummary->hide();
        }
    }

    void ToastManager::relayout(bool animate)
    {
        if (!mHost)
            return;

        const int width      = qMin(TOAST_WIDTH, qMax(240, mHost->width() - 2 * 16));
        const bool bottom    = mCorner == Corner::BottomLeft || mCorner == Corner::BottomRight;
        const bool right     = mCorner == Corner::TopRight || mCorner == Corner::BottomRight;
        const int x          = right ? mHost->width() - INSET_SIDE - width : INSET_SIDE;
        int cursor           = bottom ? mHost->height() - INSET_BOTTOM : INSET_TOP;

        auto place = [&](QWidget* widget) {
            widget->setFixedWidth(width);
            int height = widget->sizeHint().height();
            if (widget->layout() && widget->layout()->hasHeightForWidth())
                height = widget->layout()->totalHeightForWidth(width);
            widget->setFixedHeight(height);

            const int y = bottom ? cursor - height : cursor;
            const QPoint target(x, y);
            if (animate && widget->isVisible() && widget->pos() != target && widget->pos() != QPoint(0, 0))
            {
                auto* move = new QPropertyAnimation(widget, "pos", widget);
                move->setDuration(ANIMATION_MS);
                move->setEasingCurve(QEasingCurve::OutCubic);
                move->setStartValue(widget->pos());
                move->setEndValue(target);
                move->start(QAbstractAnimation::DeleteWhenStopped);
            }
            else
            {
                widget->move(target);
            }
            cursor += bottom ? -(height + GAP) : (height + GAP);
        };

        // the newest toast sits next to the corner, older ones are pushed away from it
        for (int i = mVisible.size() - 1; i >= 0; --i)
            place(mVisible.at(i));
        if (mSummary && mSummary->isVisible())
            place(mSummary);
    }

    bool ToastManager::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == mHost && !mDragging)
        {
            if (event->type() == QEvent::Resize || event->type() == QEvent::Show)
                relayout(false);
            if (event->type() == QEvent::ChildAdded || event->type() == QEvent::WindowActivate)
            {
                // other widgets of the host must not cover the toasts
                for (ToastWidget* toast : mVisible)
                    toast->raise();
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void ToastManager::handleDragStarted()
    {
        mDragging = true;
        for (ToastWidget* toast : mVisible)
            toast->raise();
    }

    void ToastManager::handleDragMoved(const QPoint& delta)
    {
        for (ToastWidget* toast : mVisible)
            toast->move(toast->pos() + delta);
        if (mSummary && mSummary->isVisible())
            mSummary->move(mSummary->pos() + delta);
    }

    void ToastManager::handleDragFinished()
    {
        mDragging = false;
        if (!mHost)
            return;

        // snap to the corner of the window that is closest to the middle of the stack
        QRect bounds;
        for (ToastWidget* toast : mVisible)
            bounds = bounds.united(toast->geometry());
        if (bounds.isValid())
        {
            const QPoint center = bounds.center();
            const bool right    = center.x() > mHost->width() / 2;
            const bool bottom   = center.y() > mHost->height() / 2;
            mCorner             = bottom ? (right ? Corner::BottomRight : Corner::BottomLeft) : (right ? Corner::TopRight : Corner::TopLeft);

            const char* name = mCorner == Corner::TopLeft ? "top-left" : mCorner == Corner::TopRight ? "top-right" : mCorner == Corner::BottomLeft ? "bottom-left" : "bottom-right";
            QSettings().setValue("toasts/corner", name);
        }
        relayout(true);
    }

    namespace toast
    {
        namespace
        {
            void show(ToastManager::Level level, const QString& title, const QString& message, const ToastManager::Options& options)
            {
                if (ToastManager::instance().isAttached())
                {
                    ToastManager::instance().show(level, title, message, options);
                    return;
                }
                // no main window yet: fall back to a message box, a notification must not get lost
                const QString text = message.isEmpty() ? title : message;
                switch (level)
                {
                    case ToastManager::Level::Error:
                        QMessageBox::critical(nullptr, title, text);
                        break;
                    case ToastManager::Level::Warning:
                        QMessageBox::warning(nullptr, title, text);
                        break;
                    default:
                        QMessageBox::information(nullptr, title, text);
                        break;
                }
            }
        }    // namespace

        void info(const QString& title, const QString& message, const ToastManager::Options& options)
        {
            show(ToastManager::Level::Info, title, message, options);
        }

        void success(const QString& title, const QString& message, const ToastManager::Options& options)
        {
            show(ToastManager::Level::Success, title, message, options);
        }

        void warning(const QString& title, const QString& message, const ToastManager::Options& options)
        {
            show(ToastManager::Level::Warning, title, message, options);
        }

        void error(const QString& title, const QString& message, const ToastManager::Options& options)
        {
            show(ToastManager::Level::Error, title, message, options);
        }
    }    // namespace toast
}    // namespace hal
