// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletcontroller.h>

#include <interfaces/handler.h>
#include <interfaces/node.h>

#include <algorithm>

#include <QApplication>
#include <QMessageBox>
#include <QMutexLocker>
#include <QThread>
#include <QWindow>

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

    m_activity_thread.start();
}

// Not using the default destructor because not all member types definitions are
// available in the header, just forward declared.
WalletController::~WalletController()
{
    m_activity_thread.quit();
    m_activity_thread.wait();
}

std::vector<WalletModel*> WalletController::getOpenWallets() const
{
    QMutexLocker locker(&m_mutex);
    return m_wallets;
}

std::map<std::string, bool> WalletController::listWalletDir() const
{
    QMutexLocker locker(&m_mutex);
    std::map<std::string, bool> wallets;
    for (const std::string& name : m_node.listWalletDir()) {
        wallets[name] = false;
    }
    for (WalletModel* wallet_model : m_wallets) {
        auto it = wallets.find(wallet_model->wallet().getWalletName());
        if (it != wallets.end()) it->second = true;
    }
    return wallets;
}

OpenWalletActivity* WalletController::openWallet(const std::string& name, QWidget* parent)
{
    OpenWalletActivity* activity = new OpenWalletActivity(this, name);
    activity->moveToThread(&m_activity_thread);
    return activity;
}

void WalletController::closeWallet(WalletModel* wallet_model, QWidget* parent)
{
    QMessageBox box(parent);
    box.setWindowTitle(tr("Close wallet"));
    box.setText(tr("Are you sure you wish to close wallet <i>%1</i>?").arg(wallet_model->getDisplayName()));
    box.setInformativeText(tr("Closing the wallet for too long can result in having to resync the entire chain if pruning is enabled."));
    box.setStandardButtons(QMessageBox::Yes|QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Yes);
    if (box.exec() != QMessageBox::Yes) return;

    // First remove wallet from node.
    wallet_model->wallet().remove();
    // Now release the model.
    removeAndDeleteWallet(wallet_model);
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
    // Handler callback runs in a different thread so fix wallet model thread affinity.
    wallet_model->moveToThread(thread());
    wallet_model->setParent(this);
    m_wallets.push_back(wallet_model);

    connect(wallet_model, &WalletModel::unload, [this, wallet_model] {
        // Defer removeAndDeleteWallet when no modal widget is active.
        // TODO: remove this workaround by removing usage of QDiallog::exec.
        if (QApplication::activeModalWidget()) {
            connect(qApp, &QApplication::focusWindowChanged, wallet_model, [this, wallet_model]() {
                if (!QApplication::activeModalWidget()) {
                    removeAndDeleteWallet(wallet_model);
                }
            }, Qt::QueuedConnection);
        } else {
            removeAndDeleteWallet(wallet_model);
        }
    });

    // Re-emit coinsSent signal from wallet model.
    connect(wallet_model, &WalletModel::coinsSent, this, &WalletController::coinsSent);

    // Notify walletAdded signal on the GUI thread.
    Q_EMIT walletAdded(wallet_model);

    return wallet_model;
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


OpenWalletActivity::OpenWalletActivity(WalletController* wallet_controller, const std::string& name)
    : m_wallet_controller(wallet_controller)
    , m_name(name)
{}

void OpenWalletActivity::open()
{
    std::string error, warning;
    std::unique_ptr<interfaces::Wallet> wallet = m_wallet_controller->m_node.loadWallet(m_name, error, warning);
    if (!warning.empty()) {
        Q_EMIT message(QMessageBox::Warning, QString::fromStdString(warning));
    }
    if (wallet) {
        Q_EMIT opened(m_wallet_controller->getOrCreateWallet(std::move(wallet)));
    } else {
        Q_EMIT message(QMessageBox::Critical, QString::fromStdString(error));
    }
    Q_EMIT finished();
}
