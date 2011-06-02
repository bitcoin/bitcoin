#ifndef BITCOINADDRESSVALIDATOR_H
#define BITCOINADDRESSVALIDATOR_H

#include <QRegExpValidator>

class BitcoinAddressValidator : public QRegExpValidator
{
    Q_OBJECT
public:
    explicit BitcoinAddressValidator(QObject *parent = 0);

    State validate(QString &input, int &pos) const;

    static const int MaxAddressLength = 34;
signals:

public slots:

};

#endif // BITCOINADDRESSVALIDATOR_H
