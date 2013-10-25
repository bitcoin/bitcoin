#ifndef RECEIVECOINSDIALOG_H
#define RECEIVECOINSDIALOG_H

#include <QDialog>
#include <QVariant>

namespace Ui {
    class ReceiveCoinsDialog;
}
class WalletModel;
class OptionsModel;

/** Dialog for requesting payment of bitcoins */
class ReceiveCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiveCoinsDialog(QWidget *parent = 0);
    ~ReceiveCoinsDialog();

    void setModel(WalletModel *model);

public slots:
    void clear();
    void reject();
    void accept();

private:
    Ui::ReceiveCoinsDialog *ui;
    WalletModel *model;

private slots:
    void on_receiveButton_clicked();
    void updateDisplayUnit();
};

#endif // RECEIVECOINSDIALOG_H
