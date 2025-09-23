// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <qt/optionsdialog.h>
#include <qt/forms/ui_optionsdialog.h>

#include <qt/appearancewidget.h>
#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <interfaces/coinjoin.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <validation.h> // for DEFAULT_SCRIPTCHECK_THREADS and MAX_SCRIPTCHECK_THREADS
#include <netbase.h>
#include <txdb.h> // for -dbcache defaults
#include <util/strencodings.h>
#include <util/underlying.h>

#include <QButtonGroup>
#include <chrono>

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QShowEvent>
#include <QSystemTrayIcon>
#include <QTimer>

OptionsDialog::OptionsDialog(QWidget *parent, bool enableWallet) :
    QDialog(parent, GUIUtil::dialog_flags),
    ui(new Ui::OptionsDialog),
    model(nullptr),
    mapper(nullptr),
    pageButtons(nullptr),
    m_enable_wallet(enableWallet)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->statusLabel}, GUIUtil::FontWeight::Bold, 16);

    GUIUtil::updateFonts();

    GUIUtil::disableMacFocusRect(this);

#ifdef Q_OS_MACOS
    /* Hide some options on Mac */
    ui->showTrayIcon->hide();
    ui->minimizeToTray->hide();
    ui->minimizeOnClose->hide();
#endif

    /* Main elements init */
    ui->databaseCache->setMinimum(nMinDbCache);
    ui->databaseCache->setMaximum(nMaxDbCache);
    ui->threadsScriptVerif->setMinimum(-GetNumCores());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);
    ui->pruneWarning->setVisible(false);
    ui->pruneWarning->setStyleSheet(QString("QLabel { %1 }").arg(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR)));

    ui->pruneSize->setEnabled(false);
    connect(ui->prune, &QPushButton::toggled, ui->pruneSize, &QWidget::setEnabled);

    /* Wallet */
    ui->coinJoinEnabled->setText(tr("Enable %1 features").arg(QString::fromStdString(gCoinJoinName)));

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif
#ifndef USE_NATPMP
    ui->mapPortNatpmp->setEnabled(false);
#endif
    connect(this, &QDialog::accepted, [this](){
        QSettings settings;
        model->node().mapPort(settings.value("fUseUPnP").toBool(), settings.value("fUseNatpmp").toBool());
    });

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->proxyIpTor->setEnabled(false);
    ui->proxyPortTor->setEnabled(false);
    ui->proxyPortTor->setValidator(new QIntValidator(1, 65535, this));

    connect(ui->connectSocks, &QPushButton::toggled, ui->proxyIp, &QWidget::setEnabled);
    connect(ui->connectSocks, &QPushButton::toggled, ui->proxyPort, &QWidget::setEnabled);
    connect(ui->connectSocks, &QPushButton::toggled, this, &OptionsDialog::updateProxyValidationState);

    connect(ui->connectSocksTor, &QPushButton::toggled, ui->proxyIpTor, &QWidget::setEnabled);
    connect(ui->connectSocksTor, &QPushButton::toggled, ui->proxyPortTor, &QWidget::setEnabled);
    connect(ui->connectSocksTor, &QPushButton::toggled, this, &OptionsDialog::updateProxyValidationState);

    /* Window elements init */
#ifdef Q_OS_MACOS
    /* hide launch at startup option on macOS */
    ui->bitcoinAtStartup->setVisible(false);
    ui->verticalLayout_Main->removeWidget(ui->bitcoinAtStartup);
    ui->verticalLayout_Main->removeItem(ui->horizontalSpacer_0_Main);
#endif

    pageButtons = new QButtonGroup(this);
    pageButtons->addButton(ui->btnMain, pageButtons->buttons().size());
    /* Remove Wallet/CoinJoin tabs and 3rd party-URL textbox in case of -disablewallet */
    if (!m_enable_wallet) {
        ui->stackedWidgetOptions->removeWidget(ui->pageWallet);
        ui->btnWallet->hide();
        ui->stackedWidgetOptions->removeWidget(ui->pageCoinJoin);
        ui->btnCoinJoin->hide();
        ui->thirdPartyTxUrlsLabel->setVisible(false);
        ui->thirdPartyTxUrls->setVisible(false);
    } else {
        ui->btnCoinJoin->setText(QString::fromStdString(gCoinJoinName));
        pageButtons->addButton(ui->btnWallet, pageButtons->buttons().size());
        pageButtons->addButton(ui->btnCoinJoin, pageButtons->buttons().size());
    }
    pageButtons->addButton(ui->btnNetwork, pageButtons->buttons().size());
    pageButtons->addButton(ui->btnDisplay, pageButtons->buttons().size());
    pageButtons->addButton(ui->btnAppearance, pageButtons->buttons().size());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(pageButtons, &QButtonGroup::idClicked, this, &OptionsDialog::showPage);
#else
    connect(pageButtons, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &OptionsDialog::showPage);
#endif

    showPage(0);

    /* Display elements init */

    /* Number of displayed decimal digits selector */
    QString digits;
    for(int index = 2; index <=8; index++){
        digits.setNum(index);
        ui->digits->addItem(digits, digits);
    }

    /* Language selector */
    QDir translations(":translations");

    ui->bitcoinAtStartup->setToolTip(ui->bitcoinAtStartup->toolTip().arg(PACKAGE_NAME));
    ui->bitcoinAtStartup->setText(ui->bitcoinAtStartup->text().arg(PACKAGE_NAME));

    ui->lang->setToolTip(ui->lang->toolTip().arg(PACKAGE_NAME));
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    for (const QString &langStr : translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
        else
        {
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
    }
    ui->unit->setModel(new BitcoinUnits(this));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    GUIUtil::ItemDelegate* delegate = new GUIUtil::ItemDelegate(mapper);
    connect(delegate, &GUIUtil::ItemDelegate::keyEscapePressed, this, &OptionsDialog::reject);
    mapper->setItemDelegate(delegate);

    /* setup/change UI elements when proxy IPs are invalid/valid */
    ui->proxyIp->setCheckValidator(new ProxyAddressValidator(parent));
    ui->proxyIpTor->setCheckValidator(new ProxyAddressValidator(parent));
    connect(ui->proxyIp, &QValidatedLineEdit::validationDidChange, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyIpTor, &QValidatedLineEdit::validationDidChange, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyPort, &QLineEdit::textChanged, this, &OptionsDialog::updateProxyValidationState);
    connect(ui->proxyPortTor, &QLineEdit::textChanged, this, &OptionsDialog::updateProxyValidationState);

    QVBoxLayout* appearanceLayout = new QVBoxLayout();
    appearanceLayout->setContentsMargins(0, 0, 0, 0);
    appearance = new AppearanceWidget(ui->widgetAppearance);
    appearanceLayout->addWidget(appearance);
    ui->widgetAppearance->setLayout(appearanceLayout);

    connect(appearance, &AppearanceWidget::appearanceChanged, [this](){
        updateWidth();
        Q_EMIT appearanceChanged();
    });

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        ui->showTrayIcon->setChecked(false);
        ui->showTrayIcon->setEnabled(false);
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }

    GUIUtil::handleCloseWindowShortcut(this);
}

OptionsDialog::~OptionsDialog()
{
    delete pageButtons;
    delete ui;
}

void OptionsDialog::setClientModel(ClientModel* client_model)
{
    m_client_model = client_model;
}

void OptionsDialog::setModel(OptionsModel *_model)
{
    this->model = _model;

    if(_model)
    {
        /* check if client restart is needed and show persistent message */
        if (_model->isRestartRequired())
            showRestartWarning(true);

        // Prune values are in GB to be consistent with intro.cpp
        static constexpr uint64_t nMinDiskSpace = (MIN_DISK_SPACE_FOR_BLOCK_FILES / GB_BYTES) + (MIN_DISK_SPACE_FOR_BLOCK_FILES % GB_BYTES) ? 1 : 0;
        ui->pruneSize->setRange(nMinDiskSpace, std::numeric_limits<int>::max());

        QString strLabel = _model->getOverriddenByCommandLine();
        if (strLabel.isEmpty()) {
            ui->frame->setHidden(true);
        } else {
            ui->overriddenByCommandLineLabel->setText(strLabel);
        }


#ifdef ENABLE_WALLET
        // If -enablecoinjoin was passed in on the command line, set the checkbox
        // to the value given via commandline and disable it (make it unclickable).
        if (strLabel.contains("-enablecoinjoin")) {
            ui->coinJoinEnabled->setChecked(_model->node().coinJoinOptions().isEnabled());
            ui->coinJoinEnabled->setEnabled(false);
        }
#endif

        mapper->setModel(_model);
        setMapper();
        mapper->toFirst();

        appearance->setModel(_model);

        updateDefaultProxyNets();
    }

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->prune, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->prune, &QCheckBox::clicked, this, &OptionsDialog::togglePruneWarning);
    connect(ui->pruneSize, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    connect(ui->databaseCache, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    connect(ui->threadsScriptVerif, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::showRestartWarning);
    /* Wallet */
    connect(ui->showMasternodesTab, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->showGovernanceTab, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->spendZeroConfChange, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    /* Network */
    connect(ui->allowIncoming, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->enableServer, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->connectSocks, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    connect(ui->connectSocksTor, &QCheckBox::clicked, this, &OptionsDialog::showRestartWarning);
    /* Display */
    connect(ui->digits, qOverload<>(&QValueComboBox::valueChanged), [this]{ showRestartWarning(); });
    connect(ui->lang, qOverload<>(&QValueComboBox::valueChanged), [this]{ showRestartWarning(); });
    connect(ui->thirdPartyTxUrls, &QLineEdit::textChanged, [this]{ showRestartWarning(); });

    connect(ui->coinJoinEnabled, &QCheckBox::clicked, [this](bool fChecked) {
#ifdef ENABLE_WALLET
        model->node().coinJoinOptions().setEnabled(fChecked);
#endif
        updateCoinJoinVisibility();
        if (this->model != nullptr) {
            this->model->emitCoinJoinEnabledChanged();
        }
        updateWidth();
    });

    updateCoinJoinVisibility();

    // Store the current CoinJoin enabled state to recover it if it gets changed but the dialog gets not accepted but declined.
#ifdef ENABLE_WALLET
    fCoinJoinEnabledPrev = model->node().coinJoinOptions().isEnabled();
    connect(this, &OptionsDialog::rejected, [this]() {
        if (fCoinJoinEnabledPrev != model->node().coinJoinOptions().isEnabled()) {
            ui->coinJoinEnabled->click();
        }
    });

    connect(ui->coinJoinDenomsGoal, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::updateCoinJoinDenomGoal);
    connect(ui->coinJoinDenomsHardCap, qOverload<int>(&QSpinBox::valueChanged), this, &OptionsDialog::updateCoinJoinDenomHardCap);
#endif
}

void OptionsDialog::setCurrentTab(OptionsDialog::Tab tab)
{
    showPage(ToUnderlying(tab));
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->bitcoinAtStartup, OptionsModel::StartAtStartup);
#ifndef Q_OS_MACOS
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        mapper->addMapping(ui->showTrayIcon, OptionsModel::ShowTrayIcon);
        mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    }
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);
    mapper->addMapping(ui->prune, OptionsModel::Prune);
    mapper->addMapping(ui->pruneSize, OptionsModel::PruneSize);
    mapper->addMapping(ui->coinJoinEnabled, OptionsModel::CoinJoinEnabled);

    /* Wallet */
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);
    mapper->addMapping(ui->subFeeFromAmount, OptionsModel::SubFeeFromAmount);
    mapper->addMapping(ui->keepChangeAddress, OptionsModel::KeepChangeAddress);
    mapper->addMapping(ui->showMasternodesTab, OptionsModel::ShowMasternodesTab);
    mapper->addMapping(ui->showGovernanceTab, OptionsModel::ShowGovernanceTab);
    mapper->addMapping(ui->showAdvancedCJUI, OptionsModel::ShowAdvancedCJUI);
    mapper->addMapping(ui->showCoinJoinPopups, OptionsModel::ShowCoinJoinPopups);
    mapper->addMapping(ui->lowKeysWarning, OptionsModel::LowKeysWarning);
    mapper->addMapping(ui->coinJoinMultiSession, OptionsModel::CoinJoinMultiSession);
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);
    mapper->addMapping(ui->coinJoinSessions, OptionsModel::CoinJoinSessions);
    mapper->addMapping(ui->coinJoinRounds, OptionsModel::CoinJoinRounds);
    mapper->addMapping(ui->coinJoinAmount, OptionsModel::CoinJoinAmount);
    mapper->addMapping(ui->coinJoinDenomsGoal, OptionsModel::CoinJoinDenomsGoal);
    mapper->addMapping(ui->coinJoinDenomsHardCap, OptionsModel::CoinJoinDenomsHardCap);

    /* Network */
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(ui->mapPortNatpmp, OptionsModel::MapPortNatpmp);
    mapper->addMapping(ui->allowIncoming, OptionsModel::Listen);
    mapper->addMapping(ui->enableServer, OptionsModel::Server);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);

    mapper->addMapping(ui->connectSocksTor, OptionsModel::ProxyUseTor);
    mapper->addMapping(ui->proxyIpTor, OptionsModel::ProxyIPTor);
    mapper->addMapping(ui->proxyPortTor, OptionsModel::ProxyPortTor);

    /* Display */
    mapper->addMapping(ui->digits, OptionsModel::Digits);
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);

    /* Appearance
       See AppearanceWidget::setModel
    */
}

void OptionsDialog::showPage(int index)
{
    std::vector<QWidget*> vecNormal;
    QAbstractButton* btnActive = pageButtons->button(index);
    for (QAbstractButton* button : pageButtons->buttons()) {
        if (button != btnActive) {
            vecNormal.push_back(button);
        }
    }

    GUIUtil::setFont({btnActive}, GUIUtil::FontWeight::Bold, 16);
    GUIUtil::setFont(vecNormal, GUIUtil::FontWeight::Normal, 16);
    GUIUtil::updateFonts();

    ui->stackedWidgetOptions->setCurrentIndex(index);
    btnActive->setChecked(true);
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_resetButton_clicked()
{
    if (model) {
        // confirmation dialog
        /*: Text explaining that the settings changed will not come into effect
            until the client is restarted. */
        QString reset_dialog_text = tr("Client restart required to activate changes.") + "<br><br>";
        /*: Text explaining to the user that the client's current settings
            will be backed up at a specific location. %1 is a stand-in
            argument for the backup location's path. */
        reset_dialog_text.append(tr("Current settings will be backed up at \"%1\".").arg(m_client_model->dataDir()) + "<br><br>");
        /*: Text asking the user to confirm if they would like to proceed
            with a client shutdown. */
        reset_dialog_text.append(tr("Client will be shut down. Do you want to proceed?"));
        //: Window title text of pop-up window shown when the user has chosen to reset options.
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm options reset"),
            reset_dialog_text, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if (btnRetVal == QMessageBox::Cancel)
            return;

        /* reset all options and close GUI */
        model->Reset();
        model->resetSettingsOnShutdown = true;
        close();
        Q_EMIT quitOnReset();
    }
}

void OptionsDialog::on_okButton_clicked()
{
    mapper->submit();
    appearance->accept();
#ifdef ENABLE_WALLET
    if (m_enable_wallet) {
        for (auto& wallet : model->node().walletLoader().getWallets()) {
            model->node().coinJoinLoader()->GetClient(wallet->getWalletName())->resetCachedBlocks();
            wallet->markDirty();
        }
    }
#endif // ENABLE_WALLET
    accept();
    updateDefaultProxyNets();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::on_showTrayIcon_stateChanged(int state)
{
    if (state == Qt::Checked) {
        ui->minimizeToTray->setEnabled(true);
    } else {
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }
}

void OptionsDialog::togglePruneWarning(bool enabled)
{
    ui->pruneWarning->setVisible(!ui->pruneWarning->isVisible());
}

void OptionsDialog::showRestartWarning(bool fPersistent)
{
    ui->statusLabel->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));

    if(fPersistent)
    {
        ui->statusLabel->setText(tr("Client restart required to activate changes."));
    }
    else
    {
        ui->statusLabel->setText(tr("This change would require a client restart."));
        // clear non-persistent status label after 10 seconds
        // Todo: should perhaps be a class attribute, if we extend the use of statusLabel
        QTimer::singleShot(10s, this, &OptionsDialog::clearStatusLabel);
    }
}

void OptionsDialog::clearStatusLabel()
{
    ui->statusLabel->clear();
    if (model && model->isRestartRequired()) {
        showRestartWarning(true);
    }
}

void OptionsDialog::updateProxyValidationState()
{
    QValidatedLineEdit *pUiProxyIp = ui->proxyIp;
    QValidatedLineEdit *otherProxyWidget = (pUiProxyIp == ui->proxyIpTor) ? ui->proxyIp : ui->proxyIpTor;
    if (pUiProxyIp->isValid() && (!ui->proxyPort->isEnabled() || ui->proxyPort->text().toInt() > 0) && (!ui->proxyPortTor->isEnabled() || ui->proxyPortTor->text().toInt() > 0))
    {
        setOkButtonState(otherProxyWidget->isValid()); //only enable ok button if both proxys are valid
        clearStatusLabel();
    }
    else
    {
        setOkButtonState(false);
        ui->statusLabel->setStyleSheet(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR));
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
}

void OptionsDialog::updateDefaultProxyNets()
{
    std::string proxyIpText{ui->proxyIp->text().toStdString()};
    if (!IsUnixSocketPath(proxyIpText)) {
        const std::optional<CNetAddr> ui_proxy_netaddr{LookupHost(proxyIpText, /*fAllowLookup=*/false)};
        const CService ui_proxy{ui_proxy_netaddr.value_or(CNetAddr{}), ui->proxyPort->text().toUShort()};
        proxyIpText = ui_proxy.ToStringAddrPort();
    }

    Proxy proxy;
    bool has_proxy;

    has_proxy = model->node().getProxy(NET_IPV4, proxy);
    ui->proxyReachIPv4->setChecked(has_proxy && proxy.ToString() == proxyIpText);

    has_proxy = model->node().getProxy(NET_IPV6, proxy);
    ui->proxyReachIPv6->setChecked(has_proxy && proxy.ToString() == proxyIpText);

    has_proxy = model->node().getProxy(NET_ONION, proxy);
    ui->proxyReachTor->setChecked(has_proxy && proxy.ToString() == proxyIpText);
}

void OptionsDialog::updateCoinJoinVisibility()
{
#ifdef ENABLE_WALLET
    bool fEnabled = model->node().coinJoinOptions().isEnabled();
#else
    bool fEnabled = false;
#endif
    ui->btnCoinJoin->setVisible(fEnabled);
    GUIUtil::updateButtonGroupShortcuts(pageButtons);
}

void OptionsDialog::updateCoinJoinDenomGoal()
{
    if (ui->coinJoinDenomsGoal->value() > ui->coinJoinDenomsHardCap->value()) {
        ui->coinJoinDenomsGoal->setValue(ui->coinJoinDenomsHardCap->value());
    }
}

void OptionsDialog::updateCoinJoinDenomHardCap()
{
    if (ui->coinJoinDenomsGoal->value() > ui->coinJoinDenomsHardCap->value()) {
        ui->coinJoinDenomsHardCap->setValue(ui->coinJoinDenomsGoal->value());
    }
}

void OptionsDialog::updateWidth()
{
    int nWidthWidestButton{0};
    int nButtonsVisible{0};
    for (QAbstractButton* button : pageButtons->buttons()) {
        if (!button->isVisible()) {
            continue;
        }
        QFontMetrics fm(button->font());
        nWidthWidestButton = std::max<int>(nWidthWidestButton, GUIUtil::TextWidth(fm, button->text()));
        ++nButtonsVisible;
    }
    // Add 10 per button as padding and use minimum 585 which is what we used in css before
    int nWidth = std::max<int>(585, (nWidthWidestButton + 10) * nButtonsVisible);
    setMinimumWidth(nWidth);

    // Resize to new minimum width but don't shrink window
    resize(std::max(width(), nWidth), height());
}

void OptionsDialog::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        updateWidth();
        GUIUtil::updateButtonGroupShortcuts(pageButtons);
    }
    QDialog::showEvent(event);
}

ProxyAddressValidator::ProxyAddressValidator(QObject *parent) :
QValidator(parent)
{
}

QValidator::State ProxyAddressValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    uint16_t port{0};
    std::string hostname;
    if (!SplitHostPort(input.toStdString(), port, hostname) || port != 0) return QValidator::Invalid;

    CService serv(LookupNumeric(input.toStdString(), DEFAULT_GUI_PROXY_PORT));
    Proxy addrProxy = Proxy(serv, true);
    if (addrProxy.IsValid())
        return QValidator::Acceptable;

    return QValidator::Invalid;
}
