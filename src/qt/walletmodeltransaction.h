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

    void setTransactionFee(qint64 newFee, bool isVoluntary);
    qint64 getTransactionFee();
    
    bool isTransactionFeeVoluntary();

    qint64 getTotalTransactionAmount();

    void newPossibleKeyChange(CWallet *wallet);
    CReserveKey *getPossibleKeyChange();

private:
    const QList<SendCoinsRecipient> recipients;
    CWalletTx *walletTransaction;
    CReserveKey *keyChange;
    qint64 fee;
    bool isVoluntary;

public slots:

};

#endif // WALLETMODELTRANSACTION_H
