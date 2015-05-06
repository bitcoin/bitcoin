// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QRCODEDIALOG_H
#define BITCOIN_QRCODEDIALOG_H

#include "bitcoin_walletmodel.h"

#include <QDialog>
#include <QImage>
#include <QLabel>

class OptionsModel;

namespace Ui {
    class Bitcoin_ReceiveRequestDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class Bitcoin_QRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit Bitcoin_QRImageWidget(QWidget *parent = 0);
    QImage exportImage();

public slots:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    QMenu *contextMenu;
};

class Bitcoin_ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Bitcoin_ReceiveRequestDialog(QWidget *parent = 0);
    ~Bitcoin_ReceiveRequestDialog();

    void setModel(OptionsModel *model);
    void setInfo(const Bitcoin_SendCoinsRecipient &info);

private slots:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();

    void update();

private:
    Ui::Bitcoin_ReceiveRequestDialog *ui;
    OptionsModel *model;
    Bitcoin_SendCoinsRecipient info;
};

#endif // BITCOIN_QRCODEDIALOG_H
