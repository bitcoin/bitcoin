#ifndef TRANSACTIONDESC_H
#define TRANSACTIONDESC_H

#include <string>

class CWallet;
class CWalletTx;

class TransactionDesc
{
public:
    /* Provide human-readable extended HTML description of a transaction */
    static std::string toHTML(CWallet *wallet, CWalletTx &wtx);
};

#endif // TRANSACTIONDESC_H
