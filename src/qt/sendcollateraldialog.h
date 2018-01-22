#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include "sendcoinsdialog.h"

class SendCollateralDialog : public SendCoinsDialog
{
    Q_OBJECT

public:
    enum Node {
        SYSTEMNODE,
        MASTERNODE
    };
    explicit SendCollateralDialog(Node node, QWidget *parent = 0);
    void send(QList<SendCoinsRecipient> &recipients);
    bool fAutoCreate;
private:
    bool instantXChecked();
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
    Node node;
};

#endif // SENDCOINSDIALOG_H
