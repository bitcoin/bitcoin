#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include "sendcoinsdialog.h"

class SendCollateralDialog : public SendCoinsDialog
{
    Q_OBJECT

public:
    explicit SendCollateralDialog(QWidget *parent = 0)
    {
        fAutoCreate = false;
    }

    void send(QList<SendCoinsRecipient> &recipients)
    {
        QStringList formatted = constructConfirmationMessage(recipients);
        fAutoCreate = true;
        checkAndSend(recipients, formatted);
        fAutoCreate = false;
    }
    bool fAutoCreate;
private:
    bool instantXChecked()
    {
        return false;
    }
};

#endif // SENDCOINSDIALOG_H
