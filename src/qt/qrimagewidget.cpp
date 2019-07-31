// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qrimagewidget.h>

#include <qt/guiutil.h>

#include <QApplication>
#include <QClipboard>
#include <QDrag>
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

    QImage qrAddrImage = QImage(QR_IMAGE_SIZE, QR_IMAGE_SIZE + (text.isEmpty() ? 0 : 20), QImage::Format_RGB32);
    qrAddrImage.fill(0xffffff);
    QPainter painter(&qrAddrImage);
    painter.drawImage(0, 0, qrImage.scaled(QR_IMAGE_SIZE, QR_IMAGE_SIZE));

    if (!text.isEmpty()) {
        QFont font = GUIUtil::fixedPitchFont();
        QRect paddedRect = qrAddrImage.rect();

        // calculate ideal font size
        qreal font_size = GUIUtil::calculateIdealFontSize(paddedRect.width() - 20, text, font);
        font.setPointSizeF(font_size);

        painter.setFont(font);
        paddedRect.setHeight(QR_IMAGE_SIZE+12);
        painter.drawText(paddedRect, Qt::AlignBottom|Qt::AlignCenter, text);
    }

    painter.end();
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
