// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PAYMENTREQUESTPLUS_H
#define BITCOIN_QT_PAYMENTREQUESTPLUS_H

#include "paymentrequest.pb.h"

#include "base58.h"

#include <openssl/x509.h>

#include <QByteArray>
#include <QList>
#include <QString>

// Used in getMerchant() to return state
enum GetMerchantStatus {
    PR_NOTINITIALIZED = 0,
    PR_PKIERROR = (1U << 0),
    PR_CERTERROR = (1U << 1),
    PR_UNAUTHENTICATED = (1U << 2),
    PR_AUTHENTICATED = (1U << 3),

    PR_ERROR = (PR_NOTINITIALIZED | PR_PKIERROR | PR_CERTERROR)
};

//
// Wraps dumb protocol buffer paymentRequest
// with extra methods
//

class PaymentRequestPlus
{
public:
    PaymentRequestPlus() { }

    bool parse(const QByteArray& data);
    bool SerializeToString(std::string* output) const;

    bool IsInitialized() const;
    // Returns true if merchant's identity is authenticated, and
    // returns human-readable merchant identity in merchant
    GetMerchantStatus getMerchant(X509_STORE* certStore, QString& merchant) const;

    // Returns list of outputs, amount
    QList<std::pair<CScript,CAmount> > getPayTo() const;

    const payments::PaymentDetails& getDetails() const { return details; }

private:
    payments::PaymentRequest paymentRequest;
    payments::PaymentDetails details;
};

#endif // BITCOIN_QT_PAYMENTREQUESTPLUS_H
