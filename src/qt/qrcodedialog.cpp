#include "qrcodedialog.h"
#include "ui_qrcodedialog.h"
#include <QPixmap>
#include <QUrl>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDebug>

#include <qrencode.h>

#define EXPORT_IMAGE_SIZE   256

QRCodeDialog::QRCodeDialog(const QString &title, const QString &addr, const QString &label, bool enableReq, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QRCodeDialog),
    address(addr)
{
    ui->setupUi(this);
    setWindowTitle(title);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->chkReq->setVisible(enableReq);
    ui->lnReqAmount->setVisible(enableReq);
    ui->lblAm1->setVisible(enableReq);
    ui->lblAm2->setVisible(enableReq);

    ui->lnLabel->setText(label);

    genCode();
}

QRCodeDialog::~QRCodeDialog()
{
    delete ui;
}

void QRCodeDialog::genCode() {

    QString uri = getURI();
    //qDebug() << "Encoding:" << uri.toUtf8().constData();
    QRcode *code = QRcode_encodeString(uri.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
    myImage.fill(0xffffff);
    unsigned char *p = code->data;
    for(int y = 0; y < code->width; y++) {
        for(int x = 0; x < code->width; x++) {
            myImage.setPixel(x + 4, y + 4, ((*p & 1) ? 0x0 : 0xffffff));
            p++;
        }
    }
    QRcode_free(code);
    ui->lblQRCode->setPixmap(QPixmap::fromImage(myImage).scaled(300, 300));
}

QString QRCodeDialog::getURI() {
    QString ret = QString("bitcoin:%1").arg(address);

    int paramCount = 0;
    if(ui->chkReq->isChecked() && ui->lnReqAmount->text().isEmpty() == false) {
        bool ok= false;
        double amount = ui->lnReqAmount->text().toDouble(&ok);
        if(ok) {
            ret += QString("?amount=%1X8").arg(ui->lnReqAmount->text());
            paramCount++;
        }
    }

    if(ui->lnLabel->text().isEmpty() == false) {
        QString lbl(QUrl::toPercentEncoding(ui->lnLabel->text()));
        ret += QString("%1label=%2").arg(paramCount == 0 ? "?" : "&").arg(lbl);
        paramCount++;
    }

    if(ui->lnMessage->text().isEmpty() == false) {
        QString msg(QUrl::toPercentEncoding(ui->lnMessage->text()));
        ret += QString("%1message=%2").arg(paramCount == 0 ? "?" : "&").arg(msg);
        paramCount++;
    }

    return ret;
}

void QRCodeDialog::on_lnReqAmount_textChanged(const QString &) {
    genCode();
}

void QRCodeDialog::on_lnLabel_textChanged(const QString &) {
    genCode();
}

void QRCodeDialog::on_lnMessage_textChanged(const QString &) {
    genCode();
}

void QRCodeDialog::on_btnSaveAs_clicked()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save Image...", QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation), "Images (*.png)");
    if(!fn.isEmpty()) {
        myImage.scaled(EXPORT_IMAGE_SIZE, EXPORT_IMAGE_SIZE).save(fn);
    }
}

void QRCodeDialog::on_chkReq_toggled(bool)
{
    genCode();
}
