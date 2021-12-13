// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qrimagewidget.h>

#include <qt/guiutil.h>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
#include <QFontDatabase>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRImageWidget::QRImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(nullptr)
{
    contextMenu = new QMenu(this);
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, &QAction::triggered, this, &QRImageWidget::saveImage);
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, &QAction::triggered, this, &QRImageWidget::copyImage);
    contextMenu->addAction(copyImageAction);
}

bool QRImageWidget::setQR(const QString& data, const QString& text)
{
#ifdef USE_QRCODE
    setText("");
    if (data.isEmpty()) return false;

    // limit length
    if (data.length() > MAX_URI_LENGTH) {
        setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return false;
    }

    QRcode *code = QRcode_encodeString(data.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);

    if (!code) {
        setText(tr("Error encoding URI into QR Code."));
        return false;
    }

    QImage qrImage = QImage(code->width, code->width, QImage::Format_RGB32);
    unsigned char *p = code->data;
    for (int y = 0; y < code->width; ++y) {
        for (int x = 0; x < code->width; ++x) {
            qrImage.setPixel(x, y, ((*p & 1) ? 0x0 : 0xffffff));
            ++p;
        }
    }
    QRcode_free(code);

    int qr_image_width = QR_IMAGE_SIZE + (2 * QR_IMAGE_MARGIN);
    int qr_image_height = qr_image_width;
    int qr_image_x_margin = QR_IMAGE_MARGIN;
    int text_lines;
    QFont font;
    if (text.isEmpty()) {
        text_lines = 0;
    } else {
        // Determine font to use
        font = GUIUtil::fixedPitchFont();
        font.setStretch(QFont::SemiCondensed);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        const int max_text_width = qr_image_width - (2 * QR_IMAGE_TEXT_MARGIN);
        const qreal font_size = GUIUtil::calculateIdealFontSize(max_text_width, text, font);
        font.setPointSizeF(font_size);

        // Plan how many lines are needed
        QFontMetrics fm(font);
        const int text_width = GUIUtil::TextWidth(fm, text);
        if (text_width > max_text_width && text_width < max_text_width * 5 / 4) {
            // Allow the image to grow up to 25% wider
            qr_image_width = text_width + (2 * QR_IMAGE_TEXT_MARGIN);
            qr_image_x_margin = (qr_image_width - QR_IMAGE_SIZE) / 2;
            text_lines = 1;
        } else {
            text_lines = (text_width + max_text_width - 1) / max_text_width;
        }
        qr_image_height += (fm.height() * text_lines) + QR_IMAGE_TEXT_MARGIN;
    }
    QImage qrAddrImage(qr_image_width, qr_image_height, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);
    {
        QPainter painter(&qrAddrImage);
        painter.drawImage(qr_image_x_margin, QR_IMAGE_MARGIN, qrImage.scaled(QR_IMAGE_SIZE, QR_IMAGE_SIZE));

        if (!text.isEmpty()) {
            QRect paddedRect = qrAddrImage.rect();
            paddedRect.setHeight(paddedRect.height() - QR_IMAGE_TEXT_MARGIN);

            QString text_wrapped = text;
            const int char_per_line = (text.size() + text_lines - 1) / text_lines;
            for (int line = 1, pos = 0; line < text_lines; ++line) {
                pos += char_per_line;
                text_wrapped.insert(pos, QChar{'\n'});
            }

            painter.setFont(font);
            painter.drawText(paddedRect, Qt::AlignBottom | Qt::AlignCenter, text_wrapped);
        }
    }

    setPixmap(QPixmap::fromImage(qrAddrImage));

    return true;
#else
    setText(tr("QR code support not available."));
    return false;
#endif
}

QImage QRImageWidget::exportImage()
{
    if(!pixmap())
        return QImage();
    return pixmap()->toImage();
}

void QRImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton && pixmap())
    {
        event->accept();
        QMimeData *mimeData = new QMimeData;
        mimeData->setImageData(exportImage());

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->exec();
    } else {
        QLabel::mousePressEvent(event);
    }
}

void QRImageWidget::saveImage()
{
    if(!pixmap())
        return;
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), nullptr);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRImageWidget::copyImage()
{
    if(!pixmap())
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void QRImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if(!pixmap())
        return;
    contextMenu->exec(event->globalPos());
}
