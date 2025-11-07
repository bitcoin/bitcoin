// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletcontroller.h>

#include <qt/askpassphrasedialog.h>
#include <qt/clientmodel.h>
#include <qt/createwalletdialog.h>
#include <qt/mnemonicverificationdialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <coinjoin/client.h>
#include <node/context.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/translation.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <chrono>

#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QMutexLocker>
#include <QProgressDialog>
#include <QThread>
#include <QTimer>
#include <QWindow>

using wallet::WALLET_FLAG_BLANK_WALLET;
using wallet::WALLET_FLAG_DESCRIPTORS;
using wallet::WALLET_FLAG_DISABLE_PRIVATE_KEYS;

WalletController::WalletController(ClientModel& client_model, QObject* parent)
    : QObject(parent)
    , m_activity_thread(new QThread(this))
    , m_activity_worker(new QObject)
    , m_client_model(client_model)
    , m_node(client_model.node())
    , m_options_model(client_model.getOptionsModel())
{
    m_handler_load_wallet = m_node.walletLoader().handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        getOrCreateWallet(std::move(wallet));
    });

    m_activity_worker->moveToThread(m_activity_thread);
    m_activity_thread->start();
    QTimer::singleShot(0, m_activity_worker, []() {
        util::ThreadRename("qt-walletctrl");
    });
}

// Not using the default destructor because not all member types definitions are
// available in the header, just forward declared.
WalletController::~WalletController()
{
    m_activity_thread->quit();
    m_activity_thread->wait();
    delete m_activity_worker;
}

std::map<std::string, bool> WalletController::listWalletDir() const
{
    QMutexLocker locker(&m_mutex);
    std::map<std::string, bool> wallets;
    for (const std::string& name : m_node.walletLoader().listWalletDir()) {
        wallets[name] = false;
    }
    for (WalletModel* wallet_model : m_wallets) {
        auto it = wallets.find(wallet_model->wallet().getWalletName());
        if (it != wallets.end()) it->second = true;
    }
    return wallets;
}

void WalletController::closeWallet(WalletModel* wallet_model, QWidget* parent)
{
    QMessageBox box(parent);
    box.setWindowTitle(tr("Close wallet"));
    box.setText(tr("Are you sure you wish to close the wallet <i>%1</i>?").arg(GUIUtil::HtmlEscape(wallet_model->getDisplayName())));
    box.setInformativeText(tr("Closing the wallet for too long can result in having to resync the entire chain if pruning is enabled."));
    box.setStandardButtons(QMessageBox::Yes|QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Yes);
    if (box.exec() != QMessageBox::Yes) return;

    // First remove wallet from node.
    wallet_model->wallet().remove();
    // Now release the model.
    removeAndDeleteWallet(wallet_model);
}

void WalletController::closeAllWallets(QWidget* parent)
{
    QMessageBox::StandardButton button = QMessageBox::question(parent, tr("Close all wallets"),
        tr("Are you sure you wish to close all wallets?"),
        QMessageBox::Yes|QMessageBox::Cancel,
        QMessageBox::Yes);
    if (button != QMessageBox::Yes) return;

    QMutexLocker locker(&m_mutex);
    for (WalletModel* wallet_model : m_wallets) {
        wallet_model->wallet().remove();
        Q_EMIT walletRemoved(wallet_model);
        delete wallet_model;
    }
    m_wallets.clear();
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
    WalletModel* wallet_model = new WalletModel(std::move(wallet), m_client_model,
                                                nullptr /* required for the following moveToThread() call */);

    // Move WalletModel object to the thread that created the WalletController
    // object (GUI main thread), instead of the current thread, which could be
    // an outside wallet thread or RPC thread sending a LoadWallet notification.
    // This ensures queued signals sent to the WalletModel object will be
    // handled on the GUI event loop.
    wallet_model->moveToThread(thread());
    // setParent(parent) must be called in the thread which created the parent object. More details in #18948.
    QMetaObject::invokeMethod(this, [wallet_model, this] {
        wallet_model->setParent(this);
    }, GUIUtil::blockingGUIThreadConnection());

    m_wallets.push_back(wallet_model);

    // WalletModel::startPollBalance needs to be called in a thread managed by
    // Qt because of startTimer. Considering the current thread can be a RPC
    // thread, better delegate the calling to Qt with Qt::AutoConnection.
    const bool called = QMetaObject::invokeMethod(wallet_model, "startPollBalance");
    assert(called);

    connect(wallet_model, &WalletModel::unload, this, [this, wallet_model] {
        // Defer removeAndDeleteWallet when no modal widget is active.
        // TODO: remove this workaround by removing usage of QDialog::exec.
        if (QApplication::activeModalWidget()) {
            connect(qApp, &QApplication::focusWindowChanged, wallet_model, [this, wallet_model]() {
                if (!QApplication::activeModalWidget()) {
                    removeAndDeleteWallet(wallet_model);
                }
            }, Qt::QueuedConnection);
        } else {
            removeAndDeleteWallet(wallet_model);
        }
    }, Qt::QueuedConnection);

    // Re-emit coinsSent signal from wallet model.
    connect(wallet_model, &WalletModel::coinsSent, this, &WalletController::coinsSent);

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

WalletControllerActivity::WalletControllerActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : QObject(wallet_controller)
    , m_wallet_controller(wallet_controller)
    , m_parent_widget(parent_widget)
{
    connect(this, &WalletControllerActivity::finished, this, &QObject::deleteLater);
}

void WalletControllerActivity::showProgressDialog(const QString& title_text, const QString& label_text)
{
    auto progress_dialog = new QProgressDialog(m_parent_widget);
    progress_dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(this, &WalletControllerActivity::finished, progress_dialog, &QWidget::close);

    progress_dialog->setWindowTitle(title_text);
    progress_dialog->setLabelText(label_text);
    progress_dialog->setRange(0, 0);
    progress_dialog->setCancelButton(nullptr);
    progress_dialog->setWindowModality(Qt::ApplicationModal);
    GUIUtil::PolishProgressDialog(progress_dialog);
    // The setValue call forces QProgressDialog to start the internal duration estimation.
    // See details in https://bugreports.qt.io/browse/QTBUG-47042.
    progress_dialog->setValue(0);
}

CreateWalletActivity::CreateWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
    m_passphrase.reserve(MAX_PASSPHRASE_SIZE);
}

CreateWalletActivity::~CreateWalletActivity()
{
    delete m_create_wallet_dialog;
    delete m_passphrase_dialog;
}

void CreateWalletActivity::askPassphrase()
{
    m_passphrase_dialog = new AskPassphraseDialog(AskPassphraseDialog::Encrypt, m_parent_widget, &m_passphrase);
    m_passphrase_dialog->setWindowModality(Qt::ApplicationModal);
    m_passphrase_dialog->show();

    connect(m_passphrase_dialog, &QObject::destroyed, [this] {
        m_passphrase_dialog = nullptr;
    });
    connect(m_passphrase_dialog, &QDialog::accepted, [this] {
        createWallet();
    });
    connect(m_passphrase_dialog, &QDialog::rejected, [this] {
        Q_EMIT finished();
    });
}

void CreateWalletActivity::createWallet()
{
    showProgressDialog(
        //: Title of window indicating the progress of creation of a new wallet.
        tr("Create Wallet"),
        /*: Descriptive text of the create wallet progress window which indicates
            to the user which wallet is currently being created. */
        tr("Creating Wallet <b>%1</b>…").arg(m_create_wallet_dialog->walletName().toHtmlEscaped()));

    std::string name = m_create_wallet_dialog->walletName().toStdString();
    uint64_t flags = 0;
    if (m_create_wallet_dialog->isDisablePrivateKeysChecked()) {
        flags |= WALLET_FLAG_DISABLE_PRIVATE_KEYS;
    }
    if (m_create_wallet_dialog->isMakeBlankWalletChecked()) {
        flags |= WALLET_FLAG_BLANK_WALLET;
    }
    if (m_create_wallet_dialog->isDescriptorWalletChecked()) {
        flags |= WALLET_FLAG_DESCRIPTORS;
    }

    QTimer::singleShot(500ms, worker(), [this, name, flags] {
        auto wallet{node().walletLoader().createWallet(name, m_passphrase, flags, m_warning_message)};

        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }

        QTimer::singleShot(500ms, this, &CreateWalletActivity::finish);
    });
}

void CreateWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Create wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Create wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    }

    if (m_wallet_model) {
        // Check if wallet is HD-enabled (has mnemonic) and requires verification
        // Skip verification for blank wallets or wallets with disabled private keys
        if (!m_wallet_model->wallet().hdEnabled() ||
            m_wallet_model->wallet().privateKeysDisabled() ||
            !m_wallet_model->wallet().canGetAddresses()) {
            // Not an HD wallet - skip verification
            Q_EMIT created(m_wallet_model);
            Q_EMIT finished();
            return;
        }

        // Unlock wallet if encrypted (needed to retrieve mnemonic)
        // Note: Newly created wallet can only be locked (if encrypted) or unencrypted.
        // Mixing-only unlock state is not possible at wallet creation time.
        const bool was_locked = (m_wallet_model->getEncryptionStatus() == WalletModel::Locked);
        if (was_locked) {
            // Unlock to retrieve mnemonic using passphrase from wallet creation
            if (!m_wallet_model->setWalletLocked(false, m_passphrase, false)) {
                QMessageBox::warning(m_parent_widget, tr("Unlock failed"),
                    tr("Failed to unlock wallet for mnemonic verification. Wallet creation completed but verification skipped."));
                Q_EMIT created(m_wallet_model);
                Q_EMIT finished();
                return;
            }
        }

        // Check if wallet has a mnemonic and requires verification
        SecureString mnemonic;
        SecureString mnemonic_passphrase;
        bool has_mnemonic = m_wallet_model->wallet().getMnemonic(mnemonic, mnemonic_passphrase);

        if (!has_mnemonic || mnemonic.empty()) {
            // No mnemonic found - log warning and skip verification
            if (was_locked) {
                m_wallet_model->setWalletLocked(true);
            }
            // Clear sensitive data before showing message
            mnemonic.assign(mnemonic.size(), 0);
            mnemonic_passphrase.assign(mnemonic_passphrase.size(), 0);
            QMessageBox::warning(m_parent_widget, tr("Mnemonic retrieval failed"),
                tr("Could not retrieve mnemonic phrase from wallet. Wallet creation completed but verification skipped."));
            Q_EMIT created(m_wallet_model);
            Q_EMIT finished();
            return;
        }

        // Wallet has mnemonic - show verification dialog
        MnemonicVerificationDialog verify_dialog(mnemonic, m_parent_widget);
        verify_dialog.setWindowModality(Qt::ApplicationModal);

        // Clear mnemonic from local variables after dialog has copied it
        // The dialog will manage its own copy securely
        const size_t mnemonic_size = mnemonic.size();
        const size_t passphrase_size = mnemonic_passphrase.size();
        mnemonic.assign(mnemonic_size, 0);
        mnemonic_passphrase.assign(passphrase_size, 0);

        if (verify_dialog.exec() == QDialog::Accepted) {
            // Verification successful
            if (was_locked) {
                m_wallet_model->setWalletLocked(true);
            }
            Q_EMIT created(m_wallet_model);
        } else {
            // User cancelled verification
            if (was_locked) {
                m_wallet_model->setWalletLocked(true);
            }
            QMessageBox::warning(m_parent_widget, tr("Verification cancelled"),
                tr("You cancelled mnemonic verification. Please make sure you have saved your mnemonic phrase safely."));
            Q_EMIT created(m_wallet_model);
        }
    } else {
        // Wallet creation failed - no wallet model
        // Already showed error message above
    }

    Q_EMIT finished();
}

void CreateWalletActivity::create()
{
    m_create_wallet_dialog = new CreateWalletDialog(m_parent_widget);
    m_create_wallet_dialog->setWindowModality(Qt::ApplicationModal);
    m_create_wallet_dialog->show();

    connect(m_create_wallet_dialog, &QObject::destroyed, [this] {
        m_create_wallet_dialog = nullptr;
    });
    connect(m_create_wallet_dialog, &QDialog::rejected, [this] {
        Q_EMIT finished();
    });
    connect(m_create_wallet_dialog, &QDialog::accepted, [this] {
        if (m_create_wallet_dialog->isEncryptWalletChecked()) {
            askPassphrase();
        } else {
            createWallet();
        }
    });
}

OpenWalletActivity::OpenWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}

void OpenWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Open wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Open wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    }

    if (m_wallet_model) Q_EMIT opened(m_wallet_model);

    Q_EMIT finished();
}

void OpenWalletActivity::open(const std::string& path)
{
    QString name = path.empty() ? QString("["+tr("default wallet")+"]") : QString::fromStdString(path);

    showProgressDialog(
        //: Title of window indicating the progress of opening of a wallet.
        tr("Open Wallet"),
        /*: Descriptive text of the open wallet progress window which indicates
            to the user which wallet is currently being opened. */
        tr("Opening Wallet <b>%1</b>…").arg(name.toHtmlEscaped()));

    QTimer::singleShot(0, worker(), [this, path] {
        auto wallet{node().walletLoader().loadWallet(path, m_warning_message)};

        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }

        QTimer::singleShot(0, this, &OpenWalletActivity::finish);
    });
}

LoadWalletsActivity::LoadWalletsActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}

void LoadWalletsActivity::load()
{
    showProgressDialog(tr("Loading wallets…"), tr("Loading wallets…"));

    QTimer::singleShot(0, worker(), [this] {
        for (auto& wallet : node().walletLoader().getWallets()) {
            m_wallet_controller->getOrCreateWallet(std::move(wallet));
        }

        QTimer::singleShot(0, this, [this] { Q_EMIT finished(); });
    });
}

RestoreWalletActivity::RestoreWalletActivity(WalletController* wallet_controller, QWidget* parent_widget)
    : WalletControllerActivity(wallet_controller, parent_widget)
{
}

void RestoreWalletActivity::restore(const fs::path& backup_file, const std::string& wallet_name)
{
    QString name = QString::fromStdString(wallet_name);

    showProgressDialog(
        //: Title of progress window which is displayed when wallets are being restored.
        tr("Restore Wallet"),
        /*: Descriptive text of the restore wallets progress window which indicates to
            the user that wallets are currently being restored.*/
        tr("Restoring Wallet <b>%1</b>…").arg(name.toHtmlEscaped()));

    QTimer::singleShot(0, worker(), [this, backup_file, wallet_name] {
        auto wallet{node().walletLoader().restoreWallet(backup_file, wallet_name, m_warning_message)};

        if (wallet) {
            m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(*wallet));
        } else {
            m_error_message = util::ErrorString(wallet);
        }

        QTimer::singleShot(0, this, &RestoreWalletActivity::finish);
    });
}

void RestoreWalletActivity::finish()
{
    if (!m_error_message.empty()) {
        //: Title of message box which is displayed when the wallet could not be restored.
        QMessageBox::critical(m_parent_widget, tr("Restore wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        //: Title of message box which is displayed when the wallet is restored with some warning.
        QMessageBox::warning(m_parent_widget, tr("Restore wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    } else {
        //: Title of message box which is displayed when the wallet is successfully restored.
        QMessageBox::information(m_parent_widget, tr("Restore wallet message"), QString::fromStdString(Untranslated("Wallet restored successfully \n").translated));
    }

    if (m_wallet_model) Q_EMIT restored(m_wallet_model);

    Q_EMIT finished();
}
