#ifndef QRCODEDIALOG_H
#define QRCODEDIALOG_H

#include <QDialog>
#include <QImage>

namespace Ui {
    class QRCodeDialog;
}

class QRCodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QRCodeDialog(const QString &addr, const QString &label, bool enableReq, QWidget *parent = 0);
    ~QRCodeDialog();

private slots:
    void on_lnReqAmount_textChanged(const QString &arg1);
    void on_lnLabel_textChanged(const QString &arg1);
    void on_lnMessage_textChanged(const QString &arg1);
    void on_btnSaveAs_clicked();

    void on_chkReqPayment_toggled(bool checked);

private:
    Ui::QRCodeDialog *ui;
    QImage myImage;

    QString getURI();
    QString address;

    void genCode();
};

#endif // QRCODEDIALOG_H
