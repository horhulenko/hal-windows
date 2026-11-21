#include "hal_core/utilities/log.h"

#include "gui/docking_system/dock_button.h"
#include "gui/content_widget/content_widget.h"
#include "gui/gui_globals.h"
#include "gui/gui_utils/graphics.h"

#include <QPainter>

namespace hal
{
    DockButton::DockButton(ContentWidget* widget, button_orientation orientation, QObject* eventFilter, QWidget* parent) : QToolButton(parent), mWidget(widget), mOrientation(orientation)
    {
        setText(mWidget->name());
        setToolTip(mWidget->name());
        setCheckable(true);
        setIcon(mWidget->icon());
        installEventFilter(eventFilter);

        // vertical rails show only the icon (square button), the bottom bar shows icon and label
        mIconSize       = orientation == button_orientation::horizontal ? 16 : 20;
        mWidthPadding   = 20;
        mHeightPadding  = 0;
        mRelativeHeight = 32;
        adjustSize();

        connect(this, &DockButton::clicked, this, &DockButton::handleClicked);
    }

    void DockButton::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const bool active = isChecked();
        const QRect bounds(0, 0, mWidth, mHeight);

        // hover and open state fill the whole button, the open state also gets an accent edge on the side that faces the window border
        if (underMouse())
            painter.fillRect(bounds, mHoverColor);
        if (active)
        {
            painter.fillRect(bounds, mHoverColor);
            const int edge = 2;
            QRect marker;
            switch (mOrientation)
            {
                case button_orientation::horizontal:
                    marker = QRect(0, mHeight - edge, mWidth, edge);
                    break;
                case button_orientation::vertical_up:
                    marker = QRect(0, 0, edge, mHeight);
                    break;
                case button_orientation::vertical_down:
                    marker = QRect(mWidth - edge, 0, edge, mHeight);
                    break;
            }
            painter.fillRect(marker, mCheckedColor);
        }

        const QIcon::Mode mode = (active || underMouse()) ? QIcon::Active : QIcon::Normal;

        if (mOrientation == button_orientation::horizontal)
        {
            const int top = (mHeight - mIconSize) / 2;
            icon().paint(&painter, QRect(mWidthPadding / 2, top, mIconSize, mIconSize), Qt::AlignCenter, mode);
            painter.setPen(active || underMouse() ? palette().color(QPalette::WindowText) : mTextColor);
            painter.drawText(QRect(mWidthPadding / 2 + mIconSize + 8, 0, mWidth - mIconSize - 8 - mWidthPadding, mHeight), Qt::AlignVCenter | Qt::AlignLeft, text());
        }
        else if (!icon().isNull())
        {
            icon().paint(&painter, QRect((mWidth - mIconSize) / 2, (mHeight - mIconSize) / 2, mIconSize, mIconSize), Qt::AlignCenter, mode);
        }
        else
        {
            // no icon: the first letters of the name
            painter.setPen(active || underMouse() ? palette().color(QPalette::WindowText) : mTextColor);
            painter.drawText(bounds, Qt::AlignCenter, text().left(2));
        }
    }

    QColor DockButton::textColor()
    {
        return mTextColor;
    }

    QColor DockButton::checkedColor()
    {
        return mCheckedColor;
    }

    QColor DockButton::hoverColor()
    {
        return mHoverColor;
    }

    void DockButton::setTextColor(const QColor& color)
    {
        mTextColor = color;
    }

    void DockButton::setCheckedColor(const QColor& color)
    {
        mCheckedColor = color;
    }

    void DockButton::setHoverColor(const QColor& color)
    {
        mHoverColor = color;
    }

    void DockButton::adjustSize()
    {
        if (mOrientation == button_orientation::horizontal)
        {
            QFontMetrics fm(font());
            mRelativeWidth = mWidthPadding + mIconSize + 8 + fm.horizontalAdvance(text());
            mWidth         = mRelativeWidth;
            mHeight        = mRelativeHeight;
        }
        else
        {
            // square, as wide as the rail
            mRelativeWidth = mRelativeHeight;
            mWidth         = mRelativeHeight;
            mHeight        = mRelativeHeight;
        }

        setFixedHeight(mHeight);
        setFixedWidth(mWidth);
    }

    int DockButton::relativeWidth()
    {
        return mRelativeWidth;
    }

    void DockButton::handleClicked(bool mChecked)
    {
        if (mChecked)
            mWidget->open();
        else
            mWidget->close();
    }

    ContentWidget* DockButton::widget()
    {
        return mWidget;
    }

    void DockButton::hide()
    {
        QWidget::hide();
        mHidden = true;
    }

    void DockButton::show()
    {
        QWidget::show();
        mHidden = false;
    }

    bool DockButton::hidden()
    {
        return mHidden;
    }

    bool DockButton::available()
    {
        return mAvailable;
    }

    void DockButton::setAvailable(bool available)
    {
        mAvailable = available;
    }

    void DockButton::setRelativeHeight(int height)
    {
        mRelativeHeight = height;
        adjustSize();
    }
}
