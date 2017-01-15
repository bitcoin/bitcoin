#ifndef DARKSENDCONFIG_H
#define DARKSENDCONFIG_H

#include <QDialog>

namespace Ui {
    class DarksendConfig;
}
class WalletModel;

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class DarksendConfig : public QDialog
{
    Q_OBJECT

public:

    DarksendConfig(QWidget *parent = 0);
    ~DarksendConfig();

    void setModel(WalletModel *model);


private:
    Ui::DarksendConfig *ui;
    WalletModel *model;
    void configure(bool enabled, int coins, int rounds);

private slots:

    void clickBasic();
    void clickHigh();
    void clickMax();
};

#endif // DARKSENDCONFIG_H
