#include "gui/svg_icon_engine/svg_icon_engine.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QtSvg/QSvgRenderer>

namespace hal
{
    namespace
    {
        std::shared_ptr<QSvgRenderer> makeRenderer(const QByteArray& data)
        {
            if (data.isEmpty())
                return nullptr;
            auto renderer = std::make_shared<QSvgRenderer>(data);
            // never stretch an icon into a rectangle that is not square
            renderer->setAspectRatioMode(Qt::KeepAspectRatio);
            return renderer;
        }
    }    // namespace

    SvgIconEngine::SvgIconEngine(const QByteArray& dataEnabled, const QByteArray& dataDisabled, const QByteArray& dataActive)
        : mEnabledData(dataEnabled), mDisabledData(dataDisabled), mActiveData(dataActive), mEnabled(makeRenderer(dataEnabled)), mDisabled(makeRenderer(dataDisabled)), mActive(makeRenderer(dataActive))
    {
    }

    void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State)
    {
        QSvgRenderer* renderer = mEnabled.get();
        if (mode == QIcon::Disabled && mDisabled)
            renderer = mDisabled.get();
        else if ((mode == QIcon::Active || mode == QIcon::Selected) && mActive)
            renderer = mActive.get();
        if (!renderer || !renderer->isValid())
            return;

        // the largest centered square, icons are drawn on a square grid
        const int side = qMin(rect.width(), rect.height());
        const QRectF square(rect.x() + (rect.width() - side) / 2.0, rect.y() + (rect.height() - side) / 2.0, side, side);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        renderer->render(painter, square);
        painter->restore();
    }

    QIconEngine* SvgIconEngine::clone() const
    {
        return new SvgIconEngine(mEnabledData, mDisabledData, mActiveData);
    }

    QPixmap SvgIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
    {
        QImage image(size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        paint(&painter, QRect(QPoint(0, 0), size), mode, state);
        painter.end();
        return QPixmap::fromImage(image);
    }
}    // namespace hal
