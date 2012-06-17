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
    explicit VerifyMessageDialog(QWidget *parent);
    ~VerifyMessageDialog();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::VerifyMessageDialog *ui;

private slots:
    void on_verifyMessage_clicked();
    void on_clearButton_clicked();
};

#endif // VERIFYMESSAGEDIALOG_H
