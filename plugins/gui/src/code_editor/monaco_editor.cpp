#include "gui/code_editor/monaco_editor.h"

#include "hal_core/utilities/log.h"
#include "hal_core/utilities/utils.h"

#include <QFocusEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QResizeEvent>

#include <functional>
#include <vector>

// <windows.h> has to come first, WebView2.h relies on the COM definitions of <objbase.h>
#include <windows.h>
#include <objbase.h>
#include <wrl.h>
#include <WebView2.h>

#undef IN
#undef OUT

namespace hal
{
    using Microsoft::WRL::Callback;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        const wchar_t* const MONACO_HOST = L"halmonaco.local";
        const wchar_t* const MONACO_PAGE = L"https://halmonaco.local/host.html";

        /**
         * The WebView2 environment is shared by all editors. It starts the browser process the first time an editor is created,
         * which happens asynchronously, so editors queue up until the environment is available.
         */
        class SharedEnvironment
        {
        public:
            static void with(std::function<void(ICoreWebView2Environment*)> callback)
            {
                SharedEnvironment& self = instance();
                if (self.mEnvironment)
                {
                    callback(self.mEnvironment.Get());
                    return;
                }

                self.mWaiters.push_back(std::move(callback));
                if (self.mCreating)
                    return;
                self.mCreating = true;

                const std::wstring userData = (utils::get_user_share_directory() / "webview2").wstring();
                HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
                    nullptr, userData.c_str(), nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                        instance().finish(result, environment);
                        return S_OK;
                    }).Get());
                if (FAILED(hr))
                    self.finish(hr, nullptr);
            }

        private:
            static SharedEnvironment& instance()
            {
                static SharedEnvironment environment;
                return environment;
            }

            void finish(HRESULT result, ICoreWebView2Environment* environment)
            {
                mCreating = false;
                if (SUCCEEDED(result) && environment)
                    mEnvironment = environment;
                else
                    log_error("gui", "cannot start the editor: Microsoft WebView2 runtime is not available (error 0x{:08x})", static_cast<unsigned>(result));

                auto waiters = std::move(mWaiters);
                mWaiters.clear();
                if (mEnvironment)
                {
                    for (auto& waiter : waiters)
                        waiter(mEnvironment.Get());
                }
            }

            ComPtr<ICoreWebView2Environment> mEnvironment;
            std::vector<std::function<void(ICoreWebView2Environment*)>> mWaiters;
            bool mCreating = false;
        };
    }    // namespace

    class MonacoEditor::Private
    {
    public:
        ~Private()
        {
            close();
        }

        /// Called once the WebView2 controller of an editor exists.
        void attach(MonacoEditor* owner, ICoreWebView2Controller* ctrl)
        {
            controller = ctrl;
            controller->get_CoreWebView2(&webview);
            if (!webview)
                return;

            ComPtr<ICoreWebView2Settings> settings;
            webview->get_Settings(&settings);
            if (settings)
            {
                settings->put_AreDefaultContextMenusEnabled(FALSE);
                settings->put_AreDevToolsEnabled(FALSE);
                settings->put_IsStatusBarEnabled(FALSE);
                settings->put_IsZoomControlEnabled(FALSE);

                ComPtr<ICoreWebView2Settings3> settings3;
                if (SUCCEEDED(settings.As(&settings3)))
                    settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE);
            }

            ComPtr<ICoreWebView2Controller2> controller2;
            if (SUCCEEDED(controller.As(&controller2)))
                controller2->put_DefaultBackgroundColor(COREWEBVIEW2_COLOR{255, 30, 30, 30});

            // Serve the Monaco files from a virtual host: web workers, which Monaco needs, do not start from file:// pages
            ComPtr<ICoreWebView2_3> webview3;
            if (SUCCEEDED(webview.As(&webview3)))
            {
                const std::wstring root = (utils::get_share_directory() / "monaco").wstring();
                webview3->SetVirtualHostNameToFolderMapping(MONACO_HOST, root.c_str(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
            }

            QPointer<MonacoEditor> guard(owner);
            webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>([guard](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                                LPWSTR json = nullptr;
                                                if (SUCCEEDED(args->get_WebMessageAsJson(&json)) && json)
                                                {
                                                    const QByteArray utf8 = QString::fromWCharArray(json).toUtf8();
                                                    CoTaskMemFree(json);
                                                    const QJsonObject message = QJsonDocument::fromJson(utf8).object();
                                                    if (guard)
                                                        guard->handleMessage(message["type"].toString(), message["args"].toArray());
                                                }
                                                return S_OK;
                                            }).Get(),
                                            &messageToken);

            webview->Navigate(MONACO_PAGE);
            owner->updateBounds();
            controller->put_IsVisible(owner->isVisible() ? TRUE : FALSE);
        }

        void close()
        {
            if (webview && messageToken.value)
                webview->remove_WebMessageReceived(messageToken);
            if (controller)
                controller->Close();
            webview.Reset();
            controller.Reset();
        }

        ComPtr<ICoreWebView2Controller> controller;
        ComPtr<ICoreWebView2> webview;
        EventRegistrationToken messageToken{};
        bool closed = false;
    };

    MonacoEditor::MonacoEditor(QWidget* parent) : QWidget(parent), mPrivate(new Private)
    {
        // the web view needs a native window to live in
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setFocusPolicy(Qt::StrongFocus);
        setMinimumSize(100, 50);

        const HWND hwnd = reinterpret_cast<HWND>(winId());
        QPointer<MonacoEditor> guard(this);
        SharedEnvironment::with([guard, hwnd](ICoreWebView2Environment* environment) {
            if (!guard)
                return;
            environment->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([guard](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                                                          if (!guard)
                                                              return S_OK;
                                                          if (FAILED(result) || !controller)
                                                          {
                                                              log_error("gui", "cannot create the editor view (error 0x{:08x})", static_cast<unsigned>(result));
                                                              return S_OK;
                                                          }
                                                          guard->mPrivate->attach(guard.data(), controller);
                                                          return S_OK;
                                                      }).Get());
        });
    }

    MonacoEditor::~MonacoEditor() = default;

    QString MonacoEditor::toJson(const QString& text)
    {
        // a JSON array holding the string is valid JavaScript, access its only element
        return QString::fromUtf8(QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact)) + "[0]";
    }

    void MonacoEditor::runScript(const QString& script)
    {
        if (mReady && mPrivate->webview)
            mPrivate->webview->ExecuteScript(script.toStdWString().c_str(), nullptr);
    }

    void MonacoEditor::updateBounds()
    {
        if (!mPrivate->controller)
            return;
        const qreal ratio = devicePixelRatioF();
        RECT bounds{0, 0, static_cast<LONG>(width() * ratio), static_cast<LONG>(height() * ratio)};
        mPrivate->controller->put_Bounds(bounds);
    }

    void MonacoEditor::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        updateBounds();
    }

    void MonacoEditor::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        if (mPrivate->controller)
        {
            mPrivate->controller->put_IsVisible(TRUE);
            updateBounds();
        }
    }

    void MonacoEditor::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        if (mPrivate->controller)
            mPrivate->controller->put_IsVisible(FALSE);
    }

    void MonacoEditor::focusInEvent(QFocusEvent* event)
    {
        QWidget::focusInEvent(event);
        if (mPrivate->controller)
            mPrivate->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }

    void MonacoEditor::handleMessage(const QString& type, const QJsonArray& args)
    {
        if (type == "ready")
            handleReady();
        else if (type == "contentChanged")
            handleContentChanged(args.at(0).toString(), args.at(1).toBool());
        else if (type == "modifiedChanged")
            handleModifiedChanged(args.at(0).toBool());
        else if (type == "saveRequested")
            Q_EMIT saveRequested();
        else if (type == "runRequested")
            Q_EMIT runRequested();
        else if (type == "keyPressed")
            Q_EMIT keyPressed(nullptr);
        else if (type == "completionRequested")
            Q_EMIT completionRequested(args.at(0).toInt(), args.at(1).toString());
    }

    void MonacoEditor::handleReady()
    {
        mReady = true;
        runScript(QString("halEditor.setLanguage(%1);").arg(toJson(mLanguage)));
        applyOptions();
        runScript(QString("halEditor.setText(%1);").arg(toJson(mText)));
        if (mModified)
            runScript("halEditor.markModified();");
    }

    void MonacoEditor::applyOptions()
    {
        QJsonObject options;
        options["fontSize"]            = mFontSize;
        options["wordWrap"]            = mLineWrap ? "on" : "off";
        options["lineNumbers"]         = mLineNumbers ? "on" : "off";
        options["renderLineHighlight"] = mHighlightLine ? "line" : "none";
        options["minimap"]             = QJsonObject{{"enabled", mMinimap}};
        runScript(QString("halEditor.setOptions(%1);").arg(QString::fromUtf8(QJsonDocument(options).toJson(QJsonDocument::Compact))));
    }

    void MonacoEditor::handleContentChanged(const QString& text, bool modified)
    {
        mText = text;
        handleModifiedChanged(modified);
        Q_EMIT textChanged();
    }

    void MonacoEditor::handleModifiedChanged(bool modified)
    {
        if (mModified == modified)
            return;
        mModified = modified;
        Q_EMIT modificationChanged(modified);
    }

    QString MonacoEditor::toPlainText() const
    {
        return mText;
    }

    void MonacoEditor::setPlainText(const QString& text)
    {
        mText     = text;
        mModified = false;
        if (mReady)
            runScript(QString("halEditor.setText(%1);").arg(toJson(text)));
        else
            Q_EMIT textChanged();
    }

    bool MonacoEditor::isModified() const
    {
        return mModified;
    }

    void MonacoEditor::setModified(bool modified)
    {
        if (mModified != modified)
        {
            mModified = modified;
            Q_EMIT modificationChanged(modified);
        }
        runScript(modified ? "halEditor.markModified();" : "halEditor.markSaved();");
    }

    void MonacoEditor::setLanguage(const QString& language)
    {
        mLanguage = language;
        runScript(QString("halEditor.setLanguage(%1);").arg(toJson(language)));
    }

    void MonacoEditor::setFontSize(int size)
    {
        mFontSize = size;
        applyOptions();
    }

    void MonacoEditor::setMinimapEnabled(bool enabled)
    {
        mMinimap = enabled;
        applyOptions();
    }

    void MonacoEditor::setLineWrapEnabled(bool enabled)
    {
        mLineWrap = enabled;
        applyOptions();
    }

    void MonacoEditor::setHighlightCurrentLineEnabled(bool enabled)
    {
        mHighlightLine = enabled;
        applyOptions();
    }

    void MonacoEditor::setLineNumberEnabled(bool enabled)
    {
        mLineNumbers = enabled;
        applyOptions();
    }

    void MonacoEditor::toggleMinimap()
    {
        setMinimapEnabled(!mMinimap);
    }

    void MonacoEditor::toggleLineNumbers()
    {
        setLineNumberEnabled(!mLineNumbers);
    }

    void MonacoEditor::setCompletions(int requestId, const QList<Completion>& completions)
    {
        QJsonArray items;
        for (const auto& c : completions)
            items.append(QJsonObject{{"label", c.first}, {"insertText", c.second}});
        runScript(QString("halEditor.completionsReady(%1, %2);").arg(requestId).arg(QString::fromUtf8(QJsonDocument(items).toJson(QJsonDocument::Compact))));
    }

    bool MonacoEditor::hasSearchHighlights() const
    {
        return mHasHighlights;
    }

    void MonacoEditor::search(const QString& text, SearchOptions options)
    {
        mHasHighlights = !text.isEmpty();
        int flags      = (options.isRegularExpression() ? 1 : 0) | (options.isCaseSensitive() ? 2 : 0) | (options.isExactMatch() ? 4 : 0);
        runScript(QString("halEditor.highlight(%1, %2);").arg(toJson(text)).arg(flags));
    }

    void MonacoEditor::undo()
    {
        runScript("halEditor.undo();");
    }

    void MonacoEditor::redo()
    {
        runScript("halEditor.redo();");
    }

    void MonacoEditor::goToLine(int line)
    {
        runScript(QString("halEditor.goToLine(%1);").arg(line));
    }

    void MonacoEditor::focusEditor()
    {
        setFocus();
        if (mPrivate->controller)
            mPrivate->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
        runScript("halEditor.focus();");
    }
}    // namespace hal
