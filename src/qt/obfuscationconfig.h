#ifndef OBFUSCATIONCONFIG_H
#define OBFUSCATIONCONFIG_H

#include <QDialog>

namespace Ui
{
class ObfuscationConfig;
}
class WalletModel;

/** Multifunctional dialog to ask for passphrases. Used for encryption, unlocking, and changing the passphrase.
 */
class ObfuscationConfig : public QDialog
{
    Q_OBJECT

public:
    ObfuscationConfig(QWidget* parent = 0);
    ~ObfuscationConfig();

    void setModel(WalletModel* model);


private:
    Ui::ObfuscationConfig* ui;
    WalletModel* model;
    void configure(bool enabled, int coins, int rounds);

private Q_SLOTS:

    void clickBasic();
    void clickHigh();
    void clickMax();
};

#endif // OBFUSCATIONCONFIG_H
