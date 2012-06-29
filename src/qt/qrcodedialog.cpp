#include "qrcodedialog.h"
#include "ui_qrcodedialog.h"
#include "guiutil.h"

#include <QPixmap>
#include <QUrl>
#include <QDebug>

#include <qrencode.h>

#define EXPORT_IMAGE_SIZE 256

QRCodeDialog::QRCodeDialog(const QString &addr, const QString &label, bool enableReq, QWidget *parent) :
    QDialog(parent), ui(new Ui::QRCodeDialog), address(addr)
{
    ui->setupUi(this);
    setWindowTitle(QString("%1").arg(address));

    ui->chkReqPayment->setVisible(enableReq);
    ui->lnReqAmount->setVisible(enableReq);
    ui->lblAmount->setVisible(enableReq);
    ui->lblBTC->setVisible(enableReq);

    ui->lnLabel->setText(label);

    genCode();
}

QRCodeDialog::~QRCodeDialog()
{
    delete ui;
}

void QRCodeDialog::genCode()
{
    QString uri = getURI();

    if (uri != "")
    {
        ui->lblQRCode->setText("");

        QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        if (!code)
        {
            ui->lblQRCode->setText(tr("Error encoding URI into QR Code."));
            return;
        }
        myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
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
    }
    else
        ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
}

QString QRCodeDialog::getURI()
{
    QString ret = QString("litecoin:%1").arg(address);

    int paramCount = 0;
    if (ui->chkReqPayment->isChecked() && !ui->lnReqAmount->text().isEmpty())
    {
        bool ok = false;
        ui->lnReqAmount->text().toDouble(&ok);
        if (ok)
        {
            ret += QString("?amount=%1").arg(ui->lnReqAmount->text());
            paramCount++;
        }
    }

    if (!ui->lnLabel->text().isEmpty())
    {
        QString lbl(QUrl::toPercentEncoding(ui->lnLabel->text()));
        ret += QString("%1label=%2").arg(paramCount == 0 ? "?" : "&").arg(lbl);
        paramCount++;
    }

    if (!ui->lnMessage->text().isEmpty())
    {
        QString msg(QUrl::toPercentEncoding(ui->lnMessage->text()));
        ret += QString("%1message=%2").arg(paramCount == 0 ? "?" : "&").arg(msg);
        paramCount++;
    }

    // limit URI length to 255 chars, to prevent a DoS against the QR-Code dialog
    if (ret.length() < 256)
        return ret;
    else
        return QString("");
}

void QRCodeDialog::on_lnReqAmount_textChanged(const QString &arg1)
{
    genCode();
}

void QRCodeDialog::on_lnLabel_textChanged(const QString &arg1)
{
    genCode();
}

void QRCodeDialog::on_lnMessage_textChanged(const QString &arg1)
{
    genCode();
}

void QRCodeDialog::on_btnSaveAs_clicked()
{
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Images (*.png)"));
    if (!fn.isEmpty())
        myImage.scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE).save(fn);
}

void QRCodeDialog::on_chkReqPayment_toggled(bool)
{
    genCode();
}
