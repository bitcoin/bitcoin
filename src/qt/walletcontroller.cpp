// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletcontroller.h>

#include <qt/askpassphrasedialog.h>
#include <qt/clientmodel.h>
#include <qt/createwalletdialog.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>

#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <util/string.h>
#include <util/threadnames.h>
#include <util/translation.h>
#include <wallet/wallet.h>

#include <algorithm>

#include <QApplication>
#include <QMessageBox>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include <QWindow>

WalletController::WalletController(ClientModel& client_model, const PlatformStyle* platform_style, QObject* parent)
    : QObject(parent)
    , m_activity_thread(new QThread(this))
    , m_activity_worker(new QObject)
    , m_client_model(client_model)
    , m_node(client_model.node())
    , m_platform_style(platform_style)
    , m_options_model(client_model.getOptionsModel())
{
    m_handler_load_wallet = m_node.walletClient().handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        getOrCreateWallet(std::move(wallet));
    });

    for (std::unique_ptr<interfaces::Wallet>& wallet : m_node.walletClient().getWallets()) {
        getOrCreateWallet(std::move(wallet));
    }

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

std::vector<WalletModel*> WalletController::getOpenWallets() const
{
    QMutexLocker locker(&m_mutex);
    return m_wallets;
}

std::map<std::string, bool> WalletController::listWalletDir() const
{
    QMutexLocker locker(&m_mutex);
    std::map<std::string, bool> wallets;
    for (const std::string& name : m_node.walletClient().listWalletDir()) {
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
    WalletModel* wallet_model = new WalletModel(std::move(wallet), m_client_model, m_platform_style,
                                                nullptr /* required for the following moveToThread() call */);

    // Move WalletModel object to the thread that created the WalletController
    // object (GUI main thread), instead of the current thread, which could be
    // an outside wallet thread or RPC thread sending a LoadWallet notification.
    // This ensures queued signals sent to the WalletModel object will be
    // handled on the GUI event loop.
    wallet_model->moveToThread(thread());
    // setParent(parent) must be called in the thread which created the parent object. More details in #18948.
    GUIUtil::ObjectInvoke(this, [wallet_model, this] {
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
}

WalletControllerActivity::~WalletControllerActivity()
{
    delete m_progress_dialog;
}

void WalletControllerActivity::showProgressDialog(const QString& label_text)
{
    assert(!m_progress_dialog);
    m_progress_dialog = new QProgressDialog(m_parent_widget);

    m_progress_dialog->setLabelText(label_text);
    m_progress_dialog->setRange(0, 0);
    m_progress_dialog->setCancelButton(nullptr);
    m_progress_dialog->setWindowModality(Qt::ApplicationModal);
    GUIUtil::PolishProgressDialog(m_progress_dialog);
}

void WalletControllerActivity::destroyProgressDialog()
{
    assert(m_progress_dialog);
    delete m_progress_dialog;
    m_progress_dialog = nullptr;
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
    showProgressDialog(tr("Creating Wallet <b>%1</b>...").arg(m_create_wallet_dialog->walletName().toHtmlEscaped()));

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

    QTimer::singleShot(500, worker(), [this, name, flags] {
        std::unique_ptr<interfaces::Wallet> wallet = node().walletClient().createWallet(name, m_passphrase, flags, m_error_message, m_warning_message);

        if (wallet) m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(wallet));

        QTimer::singleShot(500, this, &CreateWalletActivity::finish);
    });
}

void CreateWalletActivity::finish()
{
    destroyProgressDialog();

    if (!m_error_message.empty()) {
        QMessageBox::critical(m_parent_widget, tr("Create wallet failed"), QString::fromStdString(m_error_message.translated));
    } else if (!m_warning_message.empty()) {
        QMessageBox::warning(m_parent_widget, tr("Create wallet warning"), QString::fromStdString(Join(m_warning_message, Untranslated("\n")).translated));
    }

    if (m_wallet_model) Q_EMIT created(m_wallet_model);

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
    destroyProgressDialog();

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

    showProgressDialog(tr("Opening Wallet <b>%1</b>...").arg(name.toHtmlEscaped()));

    QTimer::singleShot(0, worker(), [this, path] {
        std::unique_ptr<interfaces::Wallet> wallet = node().walletClient().loadWallet(path, m_error_message, m_warning_message);

        if (wallet) m_wallet_model = m_wallet_controller->getOrCreateWallet(std::move(wallet));

        QTimer::singleShot(0, this, &OpenWalletActivity::finish);
    });
}
