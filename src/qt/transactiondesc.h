#ifndef TRANSACTIONDESC_H
#define TRANSACTIONDESC_H

#include <string>

class CWalletTx;

class TransactionDesc
{
public:
    /* Provide human-readable extended HTML description of a transaction */
    static std::string toHTML(CWalletTx &wtx);
};

#endif // TRANSACTIONDESC_H
