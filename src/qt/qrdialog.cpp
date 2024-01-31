// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qrdialog.h>
#include <qt/forms/ui_qrdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/qrimagewidget.h>

#include <QPainter>
#include <QPixmap>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> /* for USE_QRCODE */
#endif

QRDialog::QRDialog(QWidget *parent) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::QRDialog)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->labelQRCodeTitle}, GUIUtil::FontWeight::Bold, 16);

    GUIUtil::updateFonts();

#ifndef USE_QRCODE
    ui->button_saveImage->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->button_saveImage, &QPushButton::clicked, ui->lblQRCode, &QRImageWidget::saveImage);
}

QRDialog::~QRDialog()
{
    delete ui;
}

void QRDialog::setInfo(QString strWindowtitle, QString strQRCode, QString strTextInfo, QString strQRCodeTitle)
{
    this->strWindowtitle = strWindowtitle;
    this->strQRCode = strQRCode;
    this->strTextInfo = strTextInfo;
    this->strQRCodeTitle = strQRCodeTitle;
    update();
}

void QRDialog::update()
{
    setWindowTitle(strWindowtitle);
    ui->button_saveImage->setEnabled(false);
    if (strTextInfo.isEmpty()) {
        ui->outUri->setVisible(false);
        adjustSize();
    } else {
        ui->outUri->setText(strTextInfo);
    }

    if(!strQRCode.isEmpty() && ui->lblQRCode->setQR(strQRCode, "")) {
        ui->button_saveImage->setEnabled(true);
    }
    ui->labelQRCodeTitle->setText(strQRCodeTitle);
}
