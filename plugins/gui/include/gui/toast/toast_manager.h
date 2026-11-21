#pragma once

#include <QList>
#include <QObject>
#include <QPoint>
#include <QString>

#include <functional>

class QWidget;

namespace hal
{
    class ToastWidget;
    class ToastSummary;

    /**
     * Non-modal notifications that stack in a corner of the main window.
     *
     * - A toast closes by itself after a time that depends on its severity and the length of its text (about the time it
     *   takes to read it). Errors and toasts with actions stay until the user closes them.
     * - The timer pauses while the pointer is over a toast or a button of it has the focus, and resumes with at least two
     *   seconds left, so a toast never vanishes under the user's eyes.
     * - A toast has a close button, a click with the middle mouse button closes it as well.
     * - Dragging a toast moves the whole stack, it snaps to the nearest corner of the window and remembers it.
     * - At most four toasts are visible, further ones wait and are summarized in a "+N more" pill with "Dismiss all".
     * - An identical toast that is already visible does not stack up, it gets a counter and a fresh timer.
     *
     * Use the helpers of `gui/toast/toast.h` instead of this class directly.
     *
     * @ingroup gui
     */
    class ToastManager : public QObject
    {
        Q_OBJECT

    public:
        enum class Level
        {
            Info,
            Success,
            Warning,
            Error
        };

        enum class Corner
        {
            TopLeft,
            TopRight,
            BottomLeft,
            BottomRight
        };

        struct Action
        {
            QString label;
            std::function<void()> callback;
        };

        struct Options
        {
            QList<Action> actions;    ///< buttons below the message, the toast closes after one was pressed
            int durationMs = -1;      ///< -1: automatic, 0: stays until closed, otherwise milliseconds
        };

        static ToastManager& instance();

        /**
         * Sets the window the toasts are shown in. Before that, toasts cannot be displayed.
         *
         * @param host - The main window.
         */
        void attach(QWidget* host);

        /// @returns `true` if toasts can be displayed.
        bool isAttached() const;

        /**
         * Shows a toast. Can be called from any thread.
         *
         * @param level - Severity.
         * @param title - Short headline.
         * @param message - Details, may be empty.
         * @param options - Actions and duration.
         */
        void show(Level level, const QString& title, const QString& message, const Options& options = Options());

        /// Closes all toasts and drops the waiting ones.
        void dismissAll();

        /**
         * The time a toast stays when its duration is automatic.
         *
         * @param level - Severity.
         * @param characters - Length of title and message.
         * @param hasActions - `true` if the toast offers actions.
         * @returns Milliseconds, 0 if it never closes by itself.
         */
        static int automaticDuration(Level level, int characters, bool hasActions);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private Q_SLOTS:
        void handleCloseRequested(ToastWidget* toast);
        void handleDragStarted();
        void handleDragMoved(const QPoint& delta);
        void handleDragFinished();

    private:
        struct Pending
        {
            Level level;
            QString title;
            QString message;
            Options options;
        };

        ToastManager();

        void showNow(const Pending& pending);
        void fillFromQueue();
        void relayout(bool animate);
        void updateSummary();

        QWidget* mHost = nullptr;
        QList<ToastWidget*> mVisible;
        QList<Pending> mQueue;
        ToastSummary* mSummary = nullptr;
        Corner mCorner         = Corner::BottomRight;
        bool mDragging         = false;
    };
}    // namespace hal
