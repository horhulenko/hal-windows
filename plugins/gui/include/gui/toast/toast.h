#pragma once

#include "gui/toast/toast_manager.h"

#include <QString>

namespace hal
{
    /**
     * Helpers to show notifications. Prefer them over modal message boxes for everything that does not need a decision.
     * Without a main window (e.g., during startup) a message box is shown instead, so nothing gets lost.
     *
     * @ingroup gui
     */
    namespace toast
    {
        void info(const QString& title, const QString& message = QString(), const ToastManager::Options& options = ToastManager::Options());
        void success(const QString& title, const QString& message = QString(), const ToastManager::Options& options = ToastManager::Options());
        void warning(const QString& title, const QString& message = QString(), const ToastManager::Options& options = ToastManager::Options());
        void error(const QString& title, const QString& message = QString(), const ToastManager::Options& options = ToastManager::Options());
    }    // namespace toast
}    // namespace hal
