#ifndef VERIFYMESSAGEDIALOG_H
#define VERIFYMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
    class VerifyMessageDialog;
}
class AddressTableModel;

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class VerifyMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VerifyMessageDialog(AddressTableModel *addressModel, QWidget *parent = 0);
    ~VerifyMessageDialog();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    bool checkAddress();

    Ui::VerifyMessageDialog *ui;
    AddressTableModel *model;

private slots:
    void on_verifyMessage_clicked();
    void on_copyToClipboard_clicked();
    void on_clearButton_clicked();
};

#endif // VERIFYMESSAGEDIALOG_H
