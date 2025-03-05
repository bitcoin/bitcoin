// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PAIRINGPAGE_H
#define BITCOIN_QT_PAIRINGPAGE_H

#include <QWidget>

class ClientModel;
class QRImageWidget;

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

class PairingPage : public QWidget
{
    Q_OBJECT

public:
    explicit PairingPage(QWidget *parent = nullptr);

    void setClientModel(ClientModel *);

public Q_SLOTS:
    void refresh();

private:
    ClientModel *m_client_model{nullptr};
    QLineEdit *m_onion_address{nullptr};
    QRImageWidget *m_qrcode{nullptr};
};

#endif // BITCOIN_QT_PAIRINGPAGE_H
