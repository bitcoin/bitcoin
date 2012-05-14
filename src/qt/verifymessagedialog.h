#ifndef VERIFYMESSAGEDIALOG_H
#define VERIFYMESSAGEDIALOG_H

#include <QDialog>

class AddressTableModel;

QT_BEGIN_NAMESPACE
class QAbstractButton;
QT_END_NAMESPACE

namespace Ui {
    class VerifyMessageDialog;
}

class VerifyMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VerifyMessageDialog(AddressTableModel *addressModel, QWidget *parent = 0);
    ~VerifyMessageDialog();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_copyToClipboard_clicked();

private:
    bool checkAddress();

    Ui::VerifyMessageDialog *ui;
    AddressTableModel *model;
};

#endif // VERIFYMESSAGEDIALOG_H
