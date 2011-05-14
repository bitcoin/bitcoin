#ifndef BITCOINADDRESSVALIDATOR_H
#define BITCOINADDRESSVALIDATOR_H

#include <QRegExpValidator>

#include <base58.h>

class BitcoinAddressValidator : public QRegExpValidator
{
    Q_OBJECT
public:
    explicit BitcoinAddressValidator(QObject *parent = 0);

    static const int MaxAddressLength = 34;
signals:

public slots:

};

#endif // BITCOINADDRESSVALIDATOR_H
