#include "receiverequestdialog.h"
#include "ui_receiverequestdialog.h"

#include "bitcoinunits.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"

#include <QPixmap>
#if QT_VERSION < 0x050000
#include <QUrl>
#endif

#include "bitcoin-config.h" /* for USE_QRCODE */

#ifdef USE_QRCODE
#include <qrencode.h>
#endif

ReceiveRequestDialog::ReceiveRequestDialog(const QString &addr, const QString &label, quint64 amount, const QString &message, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiveRequestDialog),
    model(0),
    address(addr)
{
    ui->setupUi(this);

    QString target = label;
    if(target.isEmpty())
        target = addr;
    setWindowTitle(tr("Request payment to %1").arg(target));

    ui->lnAddress->setText(addr);
    if(amount)
        ui->lnReqAmount->setValue(amount);
    ui->lnReqAmount->setReadOnly(true);
    ui->lnLabel->setText(label);
    ui->lnMessage->setText(message);

#ifndef USE_QRCODE
    ui->btnSaveAs->setVisible(false);
    ui->lblQRCode->setVisible(false);
#endif

    genCode();
}

ReceiveRequestDialog::~ReceiveRequestDialog()
{
    delete ui;
}

void ReceiveRequestDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if (model)
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void ReceiveRequestDialog::genCode()
{
    QString uri = getURI();
    ui->btnSaveAs->setEnabled(false);
    ui->outUri->setPlainText(uri);
#ifdef USE_QRCODE
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
        ui->btnSaveAs->setEnabled(true);
    }
#endif
}

QString ReceiveRequestDialog::getURI()
{
    QString ret = QString("bitcoin:%1").arg(address);
    int paramCount = 0;

    if (ui->lnReqAmount->validate())
    {
        // even if we allow a non BTC unit input in lnReqAmount, we generate the URI with BTC as unit (as defined in BIP21)
        ret += QString("?amount=%1").arg(BitcoinUnits::format(BitcoinUnits::BTC, ui->lnReqAmount->value()));
        paramCount++;
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

    // limit URI length
    if (ret.length() > MAX_URI_LENGTH)
    {
        ui->lblQRCode->setText(tr("Resulting URI too long, try to reduce the text for label / message."));
        return QString("");
    }

    return ret;
}

void ReceiveRequestDialog::on_lnReqAmount_textChanged()
{
    genCode();
}

void ReceiveRequestDialog::on_lnLabel_textChanged()
{
    genCode();
}

void ReceiveRequestDialog::on_lnMessage_textChanged()
{
    genCode();
}

void ReceiveRequestDialog::on_btnSaveAs_clicked()
{
#ifdef USE_QRCODE
    QString fn = GUIUtil::getSaveFileName(this, tr("Save QR Code"), QString(), tr("PNG Images (*.png)"));
    if (!fn.isEmpty())
        myImage.scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE).save(fn);
#endif
}

void ReceiveRequestDialog::updateDisplayUnit()
{
    if (model)
    {
        // Update lnReqAmount with the current unit
        ui->lnReqAmount->setDisplayUnit(model->getDisplayUnit());
    }
}
