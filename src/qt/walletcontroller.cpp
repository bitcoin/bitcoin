// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletcontroller.h>

#include <interfaces/handler.h>
#include <interfaces/node.h>

#include <algorithm>

#include <QMutexLocker>
#include <QThread>

WalletController::WalletController(interfaces::Node& node, const PlatformStyle* platform_style, OptionsModel* options_model, QObject* parent)
    : QObject(parent)
    , m_node(node)
    , m_platform_style(platform_style)
    , m_options_model(options_model)
{
    m_handler_load_wallet = m_node.handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        getOrCreateWallet(std::move(wallet));
    });

    for (std::unique_ptr<interfaces::Wallet>& wallet : m_node.getWallets()) {
        getOrCreateWallet(std::move(wallet));
    }
}

// Not using the default destructor because not all member types definitions are
// available in the header, just forward declared.
WalletController::~WalletController() {}

std::vector<WalletModel*> WalletController::getWallets() const
{
    QMutexLocker locker(&m_mutex);
    return m_wallets;
}

WalletModel* WalletController::getOrCreateWallet(std::unique_ptr<interfaces::Wallet> wallet)
{
    QMutexLocker locker(&m_mutex);

    // Return model instance if exists.
    if (!m_wallets.empty()) {
        std::string name = wallet->getWalletName();
        for (WalletModel* wallet_model : m_wallets) {
            if (wallet_model->wallet().getWalletName() == name) {
                return wallet_model;
            }
        }
    }

    // Instantiate model and register it.
    WalletModel* wallet_model = new WalletModel(std::move(wallet), m_node, m_platform_style, m_options_model, nullptr);
    m_wallets.push_back(wallet_model);

    connect(wallet_model, &WalletModel::unload, [this, wallet_model] {
        removeAndDeleteWallet(wallet_model);
    });

    // Re-emit coinsSent signal from wallet model.
    connect(wallet_model, &WalletModel::coinsSent, this, &WalletController::coinsSent);

    // Notify walletAdded signal on the GUI thread.
    if (QThread::currentThread() == thread()) {
        addWallet(wallet_model);
    } else {
        // Handler callback runs in a different thread so fix wallet model thread affinity.
        wallet_model->moveToThread(thread());
        QMetaObject::invokeMethod(this, "addWallet", Qt::QueuedConnection, Q_ARG(WalletModel*, wallet_model));
    }

    return wallet_model;
}

void WalletController::addWallet(WalletModel* wallet_model)
{
    // Take ownership of the wallet model and register it.
    wallet_model->setParent(this);
    Q_EMIT walletAdded(wallet_model);
}

void WalletController::removeAndDeleteWallet(WalletModel* wallet_model)
{
    // Unregister wallet model.
    {
        QMutexLocker locker(&m_mutex);
        m_wallets.erase(std::remove(m_wallets.begin(), m_wallets.end(), wallet_model));
    }
    Q_EMIT walletRemoved(wallet_model);
    // Currently this can trigger the unload since the model can hold the last
    // CWallet shared pointer.
    delete wallet_model;
}
