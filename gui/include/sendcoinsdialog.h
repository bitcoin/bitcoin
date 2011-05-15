#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include <QDialog>

namespace Ui {
    class SendCoinsDialog;
}

class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0, const QString &address = "");
    ~SendCoinsDialog();

private:
    Ui::SendCoinsDialog *ui;

private slots:
    void on_buttonBox_rejected();
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void on_sendButton_clicked();
};

#endif // SENDCOINSDIALOG_H
