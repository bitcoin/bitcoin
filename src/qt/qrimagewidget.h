// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_QRIMAGEWIDGET_H
#define BITCOIN_QT_QRIMAGEWIDGET_H

#include <qt/optionsmodel.h>

#include <QImage>
#include <QLabel>

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* Size of exported QR Code image */
static constexpr int QR_IMAGE_SIZE = 252;
static constexpr int QR_IMAGE_TEXT_MARGIN = 8;
static constexpr int QR_IMAGE_MARGIN = 24;

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
    bool setQR(const QString& data);
    bool setQR(const QString& data, const QString& text, const OptionsModel::FontChoice& fontchoice);
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

#endif // BITCOIN_QT_QRIMAGEWIDGET_H
