#ifndef PAYMENTREQUESTPLUS_H
#define PAYMENTREQUESTPLUS_H

#include <QByteArray>
#include <QList>
#include <QString>

#include "base58.h"
#include "paymentrequest.pb.h"

//
// Wraps dumb protocol buffer paymentRequest
// with extra methods
//

class PaymentRequestPlus
{
public:
    PaymentRequestPlus() { }

    bool parse(const QByteArray& data);
    bool SerializeToString(string* output) const;

    bool IsInitialized() const;
    QString getPKIType() const;
    // Returns true if merchant's identity is authenticated, and
    // returns human-readable merchant identity in merchant
    bool getMerchant(X509_STORE* certStore, QString& merchant) const;

    // Returns list of outputs, amount
    QList<std::pair<CScript,qint64> > getPayTo() const;

    const payments::PaymentDetails& getDetails() const { return details; }

private:
    payments::PaymentRequest paymentRequest;
    payments::PaymentDetails details;
};

#endif // PAYMENTREQUESTPLUS_H

