#ifndef QRCODEDIALOG_H
#define QRCODEDIALOG_H

#include <QDialog>
#include <QImage>

namespace Ui {
    class ReceiveRequestDialog;
}
class OptionsModel;

class ReceiveRequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveRequestDialog(const QString &addr, const QString &label, quint64 amount, const QString &message, QWidget *parent = 0);
    ~ReceiveRequestDialog();

    void setModel(OptionsModel *model);

private slots:
    void on_lnReqAmount_textChanged();
    void on_lnLabel_textChanged();
    void on_lnMessage_textChanged();
    void on_btnSaveAs_clicked();

    void updateDisplayUnit();

private:
    Ui::ReceiveRequestDialog *ui;
    OptionsModel *model;
    QString address;
    QImage myImage;

    void genCode();
    QString getURI();
};

#endif // QRCODEDIALOG_H
