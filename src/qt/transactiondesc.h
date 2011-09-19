#ifndef TRANSACTIONDESC_H
#define TRANSACTIONDESC_H

#include <QString>
#include <QObject>
#include <string>

class CWallet;
class CWalletTx;

class TransactionDesc: public QObject
{
    Q_OBJECT
public:
    // Provide human-readable extended HTML description of a transaction
    static QString toHTML(CWallet *wallet, CWalletTx &wtx);
private:
    TransactionDesc() {}

    static QString HtmlEscape(const QString& str, bool fMultiLine=false);
    static QString HtmlEscape(const std::string &str, bool fMultiLine=false);
    static QString FormatTxStatus(const CWalletTx& wtx);
};

#endif // TRANSACTIONDESC_H
