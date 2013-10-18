#include "receiverequestdialog.h"
#include "ui_receiverequestdialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"

#include <QPixmap>
#include <QClipboard>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#include "bitcoin-config.h" /* for USE_QRCODE */

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

QRImageWidget::QRImageWidget(QWidget *parent):
    QLabel(parent)
{
    setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *saveImageAction = new QAction(tr("&Save Image..."), this);
    connect(saveImageAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    addAction(saveImageAction);
    QAction *copyImageAction = new QAction(tr("&Copy Image"), this);
    connect(copyImageAction, SIGNAL(triggered()), this, SLOT(copyImage()));
    addAction(copyImageAction);
}

QImage QRImageWidget::exportImage()
{
    return pixmap()->toImage().scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE);
}

void QRImageWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
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
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Images (*.png)"));
    if (!fn.isEmpty())
    {
        exportImage().save(fn);
    }
}

void QRImageWidget::copyImage()
{
    QApplication::clipboard()->setImage(exportImage());
}

ReceiveRequestDialog::ReceiveRequestDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveRequestDialog),
    model(0)
{
    ui->setupUi(this);

#ifndef USE_QRCODE
    ui->btnSaveAs->setVisible(false);
    ui->btnCopyImage->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    connect(ui->btnSaveAs, SIGNAL(clicked()), ui->lblQRCode, SLOT(saveImage()));
    connect(ui->btnCopyImage, SIGNAL(clicked()), ui->lblQRCode, SLOT(copyImage()));
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(update()));

    // update the display unit if necessary
    update();
}

void ReceiveRequestDialog::setInfo(const SendCoinsRecipient &info)
{
    this->info = info;
    update();
}

void ReceiveRequestDialog::update()
{
    if(!model)
        return;
    QString target = info.label;
    if(target.isEmpty())
        target = info.address;
    setWindowTitle(tr("Request payment to %1").arg(target));

    QString uri = GUIUtil::formatBitcoinURI(info);
    ui->btnSaveAs->setEnabled(false);
    QString html;
    html += "<html><font face='verdana, arial, helvetica, sans-serif'>";
    html += "<b>"+tr("Payment information")+"</b><br>";
    html += "<b>"+tr("URI")+"</b>: ";
    html += "<a href=\""+uri+"\">" + GUIUtil::HtmlEscape(uri) + "</a><br>";
    html += "<b>"+tr("Address")+"</b>: " + GUIUtil::HtmlEscape(info.address) + "<br>";
    if(info.amount)
        html += "<b>"+tr("Amount")+"</b>: " + BitcoinUnits::formatWithUnit(model->getDisplayUnit(), info.amount) + "<br>";
    if(!info.label.isEmpty())
        html += "<b>"+tr("Label")+"</b>: " + GUIUtil::HtmlEscape(info.label) + "<br>";
    if(!info.message.isEmpty())
        html += "<b>"+tr("Message")+"</b>: " + GUIUtil::HtmlEscape(info.message) + "<br>";
    ui->outUri->setText(html);

#ifdef USE_QRCODE
    ui->lblQRCode->setText("");
    if(!uri.isEmpty())
    {
        // limit URI length
        if (uri.length() > MAX_URI_LENGTH)
        {
            ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        } else {
            QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
            if (!code)
            {
                ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
                return;
            }
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

            ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
            ui->btnSaveAs->setEnabled(true);
        }
    }
#endif
}

void ReceiveRequestDialog::on_btnCopyURI_clicked()
{
    QString uri = GUIUtil::formatBitcoinURI(info);
    QApplication::clipboard()->setText(uri, QClipboard::Clipboard);
    QApplication::clipboard()->setText(uri, QClipboard::Selection);
}

void ReceiveRequestDialog::on_btnCopyAddress_clicked()
{
    QApplication::clipboard()->setText(info.address, QClipboard::Clipboard);
    QApplication::clipboard()->setText(info.address, QClipboard::Selection);
}
