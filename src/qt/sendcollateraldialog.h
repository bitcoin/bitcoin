#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include "sendcoinsdialog.h"

class SendCollateralDialog : public SendCoinsDialog
{
public:
    void send(QList<SendCoinsRecipient> &recipients)
    {
        QStringList formatted = constructConfirmationMessage(recipients);
        checkAndSend(recipients, formatted);
    }
private:
    bool instantXChecked()
    {
        return false;
    }
};

#endif // SENDCOINSDIALOG_H
