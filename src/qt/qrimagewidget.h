// Copyright (c) 2011-2020 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TORTOISECOIN_QT_QRIMAGEWIDGET_H
#define TORTOISECOIN_QT_QRIMAGEWIDGET_H

#include <QImage>
#include <QLabel>

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* Size of exported QR Code image */
static constexpr int QR_IMAGE_SIZE = 300;
static constexpr int QR_IMAGE_TEXT_MARGIN = 10;
static constexpr int QR_IMAGE_MARGIN = 2 * QR_IMAGE_TEXT_MARGIN;

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class QRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit QRImageWidget(QWidget *parent = nullptr);
    bool setQR(const QString& data, const QString& text = "");
    QImage exportImage();

public Q_SLOTS:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

private:
    QMenu* contextMenu{nullptr};
};

#endif // TORTOISECOIN_QT_QRIMAGEWIDGET_H
