#ifndef IMPORTPRIVATEKEYDIALOG_H
#define IMPORTPRIVATEKEYDIALOG_H

#include <QDialog>

namespace Ui {
    class ImportPrivateKeyDialog;
}
class WalletModel;

/** "Import Private Key" dialog box */
class ImportPrivateKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportPrivateKeyDialog(QWidget *parent = 0);
    ~ImportPrivateKeyDialog();

    void setModel(WalletModel *model);
private:
    Ui::ImportPrivateKeyDialog *ui;

private slots:
    void on_buttonBox_accepted();
};

#endif // IMPORTPRIVATEKEYDIALOG_H

