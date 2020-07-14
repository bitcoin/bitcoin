// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/qrdialog.h>
#include <qt/forms/ui_qrdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <QClipboard>
#include <QDrag>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h> /* for USE_QRCODE */
#endif

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRGeneralImageWidget::QRGeneralImageWidget(QWidget *parent):
    QLabel(parent), contextMenu(0)
{
    contextMenu = new QMenu(this);
    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    contextMenu->addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(copyImage()));
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
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Image (*.png)"), nullptr);
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

    GUIUtil::setFont({ui->labelQRCodeTitle}, GUIUtil::FontWeight::Bold, 16);

    GUIUtil::updateFonts();

#ifndef USE_QRCODE
    ui->button_saveImage->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->button_saveImage, SIGNAL(clicked()), ui->lblQRCode, SLOT(saveImage()));
}

QRDialog::~QRDialog()
{
    delete ui;
}

void QRDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(update()));

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
    if (strTextInfo.isEmpty()) {
        ui->outUri->setVisible(false);
        adjustSize();
    }
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
        myImage.fill(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));
        unsigned char *p = code->data;
        for (int y = 0; y < code->width; y++)
        {
            for (int x = 0; x < code->width; x++)
            {
                myImage.setPixel(x + 4, y + 4, ((*p & 1) ? GUIUtil::getThemedQColor(GUIUtil::ThemedColor::QR_PIXEL).rgb() : GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET).rgb()));
                p++;
            }
        }
        QRcode_free(code);

        QImage qrImage = QImage(QR_IMAGE_SIZE, QR_IMAGE_SIZE, QImage::Format_RGB32);
        qrImage.fill(GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BORDER_WIDGET));
        QPainter painter(&qrImage);
        QRect paddedRect = qrImage.rect().adjusted(1, 1, -1, -1);
        painter.fillRect(paddedRect, GUIUtil::getThemedQColor(GUIUtil::ThemedColor::BACKGROUND_WIDGET));
        painter.drawImage(1, 1, myImage.scaled(QR_IMAGE_SIZE - 2, QR_IMAGE_SIZE - 2));

        ui->labelQRCodeTitle->setText(strQRCodeTitle);
        ui->lblQRCode->setPixmap(QPixmap::fromImage(qrImage));
        ui->button_saveImage->setEnabled(true);
    }
#endif
}