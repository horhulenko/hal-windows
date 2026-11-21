#pragma once

#include "gui/toast/toast_manager.h"

#include <QElapsedTimer>
#include <QFrame>
#include <QTimer>

class QLabel;
class QToolButton;

namespace hal
{
    /**
     * One notification card. Created and positioned by the ToastManager.
     *
     * @ingroup gui
     */
    class ToastWidget : public QFrame
    {
        Q_OBJECT

    public:
        ToastWidget(ToastManager::Level level, const QString& title, const QString& message, const QList<ToastManager::Action>& actions, int durationMs, QWidget* parent);

        ToastManager::Level level() const;
        QString title() const;
        QString message() const;

        /// Counts a repeated, identical notification and restarts the timer.
        void repeat();

        /// Starts or restarts the countdown, 0 means the toast stays.
        void restartTimer();

    Q_SIGNALS:
        void closeRequested(ToastWidget* toast);
        void dragStarted();
        void dragMoved(const QPoint& delta);
        void dragFinished();

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private Q_SLOTS:
        void tick();

    private:
        ToastManager::Level mLevel;
        QString mTitle;
        QString mMessage;
        QLabel* mTitleLabel;
        QLabel* mCountLabel;
        QTimer mTimer;
        QElapsedTimer mClock;
        int mDurationMs;
        int mRemainingMs;
        int mCount    = 1;
        bool mHovered = false;
        bool mFocused = false;
        bool mPressed = false;
        bool mDragged = false;
        QPoint mPressPosition;
        QPoint mLastPosition;
    };

    /**
     * The "+N more, dismiss all" pill below the visible toasts.
     *
     * @ingroup gui
     */
    class ToastSummary : public QFrame
    {
        Q_OBJECT

    public:
        explicit ToastSummary(QWidget* parent);

        void setWaiting(int waiting, int visible);

    Q_SIGNALS:
        void dismissAllRequested();

    private:
        QLabel* mLabel;
    };
}    // namespace hal
