// Copyright (c) 2011-2020 The Bitcoin Core developers
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
    contextMenu->addAction(tr("Save Image..."), this, &QRImageWidget::saveImage);
    contextMenu->addAction(tr("Copy Image"), this, &QRImageWidget::copyImage);
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

    QImage qrImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
    qrImage.fill(0xffffff);
    unsigned char *p = code->data;
    for (int y = 0; y < code->width; ++y) {
        for (int x = 0; x < code->width; ++x) {
            qrImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
            ++p;
        }
    }
    QRcode_free(code);

    const int qr_image_size = QR_IMAGE_SIZE + (text.isEmpty() ? 0 : 2 * QR_IMAGE_MARGIN);
    QImage qrAddrImage(qr_image_size, qr_image_size, QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);
    {
        QPainter painter(&qrAddrImage);
        painter.drawImage(QR_IMAGE_MARGIN, 0, qrImage.scaled(QR_IMAGE_SIZE, QR_IMAGE_SIZE));

        if (!text.isEmpty()) {
            QRect paddedRect = qrAddrImage.rect();
            paddedRect.setHeight(QR_IMAGE_SIZE + QR_IMAGE_TEXT_MARGIN);

            QFont font = GUIUtil::fixedPitchFont();
            font.setStretch(QFont::SemiCondensed);
            font.setLetterSpacing(QFont::AbsoluteSpacing, 1);
            const qreal font_size = GUIUtil::calculateIdealFontSize(paddedRect.width() - 2 * QR_IMAGE_TEXT_MARGIN, text, font);
            font.setPointSizeF(font_size);

            painter.setFont(font);
            painter.drawText(paddedRect, Qt::AlignBottom | Qt::AlignCenter, text);
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
    return GUIUtil::GetImage(this);
}

void QRImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && GUIUtil::HasPixmap(this)) {
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
    if (!GUIUtil::HasPixmap(this))
        return;
    QString fn = GUIUtil::getSaveFileName(
        this, tr("Save QR Code"), QString(),
        tr("PNG Image", "Name of PNG file format") + QLatin1String(" (*.png)"), nullptr);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRImageWidget::copyImage()
{
    if (!GUIUtil::HasPixmap(this))
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void QRImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (!GUIUtil::HasPixmap(this))
        return;
    contextMenu->exec(event->globalPos());
}
