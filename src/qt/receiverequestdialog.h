// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef QRCODEDIALOG_H
#define QRCODEDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QImage>
#include <QLabel>

class OptionsModel;

namespace Ui {
    class Credits_ReceiveRequestDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class Credits_QRImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit Credits_QRImageWidget(QWidget *parent = 0);
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

class Credits_ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit Credits_ReceiveRequestDialog(QWidget *parent = 0);
    ~Credits_ReceiveRequestDialog();

    void setModel(OptionsModel *model);
    void setInfo(const Bitcredit_SendCoinsRecipient &info);

private slots:
    void on_btnCopyURI_clicked();
    void on_btnCopyAddress_clicked();

    void update();

private:
    Ui::Credits_ReceiveRequestDialog *ui;
    OptionsModel *model;
    Bitcredit_SendCoinsRecipient info;
};

#endif // QRCODEDIALOG_H
