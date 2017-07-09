// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDCOINSRECIPIENT_H
#define BITCOIN_QT_SENDCOINSRECIPIENT_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <amount.h>
#include <serialize.h>

#include <string>

#include <QString>

//! Wrapper class to serialize QString objects as std::strings.
struct AsStdString
{
    template<typename Q>
    class Wrapper
    {
    private:
        Q& m_qstring;
    public:
        Wrapper(Q& qstring) : m_qstring(qstring) {}

        template<typename Stream>
        void Serialize(Stream& s) const { s << m_qstring.toStdString(); }

        template<typename Stream>
        void Unserialize(Stream& s)
        {
            std::string str;
            s >> str;
            m_qstring = QString::fromStdString(std::move(str));
        }
    };
};

class SendCoinsRecipient
{
public:
    explicit SendCoinsRecipient() : amount(0), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) { }
    explicit SendCoinsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
        address(addr), label(_label), amount(_amount), message(_message), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) {}

    // If from an unauthenticated payment request, this is used for storing
    // the addresses, e.g. address-A<br />address-B<br />address-C.
    // Info: As we don't need to process addresses in here when using
    // payment requests, we can abuse it for displaying an address list.
    // Todo: This is a hack, should be replaced with a cleaner solution!
    QString address;
    QString label;
    CAmount amount;
    // If from a payment request, this is used for storing the memo
    QString message;
    // Keep the payment request around as a serialized string to ensure
    // load/store is lossless.
    std::string sPaymentRequest;
    // Empty if no authentication or invalid signature/cert/etc.
    QString authenticatedMerchant;

    bool fSubtractFeeFromAmount; // memory only

    static const int CURRENT_VERSION = 1;
    int nVersion;

    SERIALIZE_METHODS(SendCoinsRecipient, obj)
    {
        READWRITE(obj.nVersion);
        READWRITE(Wrap<AsStdString>(obj.address));
        READWRITE(Wrap<AsStdString>(obj.label));
        READWRITE(obj.amount);
        READWRITE(Wrap<AsStdString>(obj.message));
        READWRITE(obj.sPaymentRequest);
        READWRITE(Wrap<AsStdString>(obj.authenticatedMerchant));
    }
};

#endif // BITCOIN_QT_SENDCOINSRECIPIENT_H
