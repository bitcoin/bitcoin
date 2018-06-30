// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_QRDIALOG_H
#define BITCOIN_QT_QRDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QImage>
#include <QLabel>

class OptionsModel;

namespace Ui {
    class QRDialog;
}

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

/* Label widget for QR code. This image can be dragged, dropped, copied and saved
 * to disk.
 */
class QRGeneralImageWidget : public QLabel
{
    Q_OBJECT

public:
    explicit QRGeneralImageWidget(QWidget *parent = 0);
    QImage exportImage();

public Q_SLOTS:
    void saveImage();
    void copyImage();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);

private:
    QMenu *contextMenu;
};

class QRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QRDialog(QWidget *parent = 0);
    ~QRDialog();

    void setModel(OptionsModel *model);
    void setInfo(QString strWindowtitle, QString strQRCode, QString strTextInfo, QString strQRCodeTitle);

private Q_SLOTS:
    void update();

private:
    Ui::QRDialog *ui;
    OptionsModel *model;
    QString strWindowtitle;
    QString strQRCode;
    QString strTextInfo;
    QString strQRCodeTitle;
};

#endif // BITCOIN_QT_QRDIALOG_H
