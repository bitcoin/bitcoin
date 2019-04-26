// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2014-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qrdialog.h>
#include <qt/forms/ui_qrdialog.h>

#include <qt/syscoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>

#include <QClipboard>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h> /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRGeneralImageWidget::QRGeneralImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(0)
{
    contextMenu = new QMenu(this);
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, &QAction::triggered, this, &QRGeneralImageWidget::saveImage);
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, &QAction::triggered, this, &QRGeneralImageWidget::copyImage);
    contextMenu->addAction(copyImageAction);
}

QImage QRGeneralImageWidget::exportImage()
{
    if(!pixmap())
        return QImage();
    return pixmap()->toImage();
}

void QRGeneralImageWidget::mousePressEvent(QMouseEvent *event)
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

void QRGeneralImageWidget::saveImage()
{
    if(!pixmap())
        return;
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), NULL);
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRGeneralImageWidget::copyImage()
{
    if(!pixmap())
        return;
    QApplication::clipboard()->setImage(exportImage());
}

void QRGeneralImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if(!pixmap())
        return;
    contextMenu->exec(event->globalPos());
}

QRDialog::QRDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRDialog),
    model(0)
{
    ui->setupUi(this);

#ifndef USE_QRCODE
    ui->button_saveImage->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->button_saveImage, &QPushButton::clicked, ui->lblQRCode, &QRGeneralImageWidget::saveImage);
}

QRDialog::~QRDialog()
{
    delete ui;
}

void QRDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, &OptionsModel::displayUnitChanged, this, &QRDialog::update);

    // update the display unit if necessary
    update();
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
    if(!model)
        return;

    setWindowTitle(strWindowtitle);
    ui->button_saveImage->setEnabled(false);
    ui->outUri->setText(strTextInfo);

#ifdef USE_QRCODE
    // Create QR-code
    ui->lblQRCode->setText("");
    if(!strQRCode.isEmpty())
    {
        QRcode *code = QRcode_encodeString(strQRCode.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            ui->lblQRCode->setText(tr("Error creating QR Code."));
            return;
        }
        ui->lblQRCode->setToolTip(strQRCode);
        QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
        myImage.fill(0xffffff);
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
                p++;
            }
        }
        QRcode_free(code);

        ui->labelQRCodeTitle->setText(strQRCodeTitle);
        ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
        ui->button_saveImage->setEnabled(true);
    }
#endif
}
