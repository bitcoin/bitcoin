#ifndef WALLETMODELTRANSACTION_H
#define WALLETMODELTRANSACTION_H

#include "walletmodel.h"

class SendCoinsRecipient;

/** Data model for a walletmodel transaction. */
class WalletModelTransaction
{
public:
    explicit WalletModelTransaction(const QList<SendCoinsRecipient> &recipients);
    ~WalletModelTransaction();

    QList<SendCoinsRecipient> getRecipients();

    CWalletTx *getTransaction();

    void setTransactionFee(qint64 newFee);
    qint64 getTransactionFee();

    qint64 getTotalTransactionAmount();

    void newPossibleKeyChange(CWallet *wallet);
    CReserveKey *getPossibleKeyChange();

private:
    const QList<SendCoinsRecipient> recipients;
    CWalletTx *walletTransaction;
    CReserveKey *keyChange;
    qint64 fee;

public slots:

};

#endif // WALLETMODELTRANSACTION_H
