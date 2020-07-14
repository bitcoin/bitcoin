// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <qt/optionsdialog.h>
#include <qt/forms/ui_optionsdialog.h>

#include <qt/bitcoinunits.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>

#include <validation.h> // for DEFAULT_SCRIPTCHECK_THREADS and MAX_SCRIPTCHECK_THREADS
#include <netbase.h>
#include <txdb.h> // for -dbcache defaults

#ifdef ENABLE_WALLET
#include <privatesend/privatesend-client.h>
#endif // ENABLE_WALLET

#include <QDataWidgetMapper>
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>

OptionsDialog::OptionsDialog(QWidget *parent, bool enableWallet) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    model(0),
    mapper(0)
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->statusLabel}, GUIUtil::FontWeight::Bold, 16);

    GUIUtil::updateFonts();

    GUIUtil::disableMacFocusRect(this);

    /* Main elements init */
    ui->databaseCache->setMinimum(nMinDbCache);
    ui->databaseCache->setMaximum(nMaxDbCache);
    ui->threadsScriptVerif->setMinimum(-GetNumCores());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->proxyIpTor->setEnabled(false);
    ui->proxyPortTor->setEnabled(false);
    ui->proxyPortTor->setValidator(new QIntValidator(1, 65535, this));

    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyIp, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyPort, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), this, SLOT(updateProxyValidationState()));

    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyIpTor, SLOT(setEnabled(bool)));
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), ui->proxyPortTor, SLOT(setEnabled(bool)));
    connect(ui->connectSocksTor, SIGNAL(toggled(bool)), this, SLOT(updateProxyValidationState()));

    pageButtons.addButton(ui->btnMain, pageButtons.buttons().size());
    /* remove Wallet tab in case of -disablewallet */
    if (!enableWallet) {
        ui->stackedWidgetOptions->removeWidget(ui->pageWallet);
        ui->btnWallet->hide();
    } else {
        pageButtons.addButton(ui->btnWallet, pageButtons.buttons().size());
    }
    pageButtons.addButton(ui->btnNetwork, pageButtons.buttons().size());
#ifdef Q_OS_MAC
    /* remove Window tab on Mac */
    ui->stackedWidgetOptions->removeWidget(ui->pageWindow);
    ui->btnWindow->hide();
#else
    pageButtons.addButton(ui->btnWindow, pageButtons.buttons().size());
#endif
    pageButtons.addButton(ui->btnDisplay, pageButtons.buttons().size());

    connect(&pageButtons, SIGNAL(buttonClicked(int)), this, SLOT(showPage(int)));

    showPage(0);

    /* Display elements init */

    /* Number of displayed decimal digits selector */
    QString digits;
    for(int index = 2; index <=8; index++){
        digits.setNum(index);
        ui->digits->addItem(digits, digits);
    }

    /* Theme selector */
    for (const QString& entry : GUIUtil::listThemes()) {
        ui->theme->addItem(entry, QVariant(entry));
    }

    /* Language selector */
    QDir translations(":translations");

    ui->bitcoinAtStartup->setToolTip(ui->bitcoinAtStartup->toolTip().arg(tr(PACKAGE_NAME)));
    ui->bitcoinAtStartup->setText(ui->bitcoinAtStartup->text().arg(tr(PACKAGE_NAME)));

    ui->lang->setToolTip(ui->lang->toolTip().arg(tr(PACKAGE_NAME)));
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
    ui->thirdPartyTxUrls->setPlaceholderText("https://example.com/tx/%s");

    ui->unit->setModel(new BitcoinUnits(this));

    /* Widget-to-option mapper */
    mapper = new QDataWidgetMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    /* setup/change UI elements when proxy IPs are invalid/valid */
    ui->proxyIp->setCheckValidator(new ProxyAddressValidator(parent));
    ui->proxyIpTor->setCheckValidator(new ProxyAddressValidator(parent));
    connect(ui->proxyIp, SIGNAL(validationDidChange(QValidatedLineEdit *)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyIpTor, SIGNAL(validationDidChange(QValidatedLineEdit *)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyPort, SIGNAL(textChanged(const QString&)), this, SLOT(updateProxyValidationState()));
    connect(ui->proxyPortTor, SIGNAL(textChanged(const QString&)), this, SLOT(updateProxyValidationState()));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::setModel(OptionsModel *_model)
{
    this->model = _model;

    if(_model)
    {
        /* check if client restart is needed and show persistent message */
        if (_model->isRestartRequired())
            showRestartWarning(true);

        QString strLabel = _model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui->overriddenByCommandLineLabel->setText(strLabel);

        mapper->setModel(_model);
        setMapper();
        mapper->toFirst();

        updateDefaultProxyNets();
    }

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->databaseCache, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    connect(ui->threadsScriptVerif, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    /* Wallet */
    connect(ui->showMasternodesTab, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->spendZeroConfChange, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Network */
    connect(ui->allowIncoming, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    connect(ui->connectSocksTor, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Display */
    connect(ui->digits, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->theme, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning()));
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->bitcoinAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);

    /* Wallet */
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);
    mapper->addMapping(ui->showMasternodesTab, OptionsModel::ShowMasternodesTab);
    mapper->addMapping(ui->showAdvancedPSUI, OptionsModel::ShowAdvancedPSUI);
    mapper->addMapping(ui->showPrivateSendPopups, OptionsModel::ShowPrivateSendPopups);
    mapper->addMapping(ui->lowKeysWarning, OptionsModel::LowKeysWarning);
    mapper->addMapping(ui->privateSendMultiSession, OptionsModel::PrivateSendMultiSession);
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);
    mapper->addMapping(ui->privateSendRounds, OptionsModel::PrivateSendRounds);
    mapper->addMapping(ui->privateSendAmount, OptionsModel::PrivateSendAmount);

    /* Network */
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);
    mapper->addMapping(ui->allowIncoming, OptionsModel::Listen);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);

    mapper->addMapping(ui->connectSocksTor, OptionsModel::ProxyUseTor);
    mapper->addMapping(ui->proxyIpTor, OptionsModel::ProxyIPTor);
    mapper->addMapping(ui->proxyPortTor, OptionsModel::ProxyPortTor);

    /* Window */
#ifndef Q_OS_MAC
    mapper->addMapping(ui->hideTrayIcon, OptionsModel::HideTrayIcon);
    mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->digits, OptionsModel::Digits);
    mapper->addMapping(ui->theme, OptionsModel::Theme);
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);

}

void OptionsDialog::showPage(int index)
{
    std::vector<QWidget*> vecNormal;
    QAbstractButton* btnActive = pageButtons.button(index);
    for (QAbstractButton* button : pageButtons.buttons()) {
        if (button != btnActive) {
            vecNormal.push_back(button);
        }
    }

    GUIUtil::setFont({btnActive}, GUIUtil::FontWeight::Bold, 16);
    GUIUtil::setFont(vecNormal, GUIUtil::FontWeight::Normal, 16);

    ui->stackedWidgetOptions->setCurrentIndex(index);
    btnActive->setChecked(true);
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_resetButton_clicked()
{
    if(model)
    {
        // confirmation dialog
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm options reset"),
            tr("Client restart required to activate changes.") + "<br><br>" + tr("Client will be shut down. Do you want to proceed?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Cancel)
            return;

        /* reset all options and close GUI */
        model->Reset();
        QApplication::quit();
    }
}

void OptionsDialog::on_okButton_clicked()
{
    mapper->submit();
#ifdef ENABLE_WALLET
    privateSendClient.nCachedNumBlocks = std::numeric_limits<int>::max();
    if(HasWallets())
        GetWallets()[0]->MarkDirty();
#endif // ENABLE_WALLET
    accept();
    updateDefaultProxyNets();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::on_hideTrayIcon_stateChanged(int fState)
{
    if(fState)
    {
        ui->minimizeToTray->setChecked(false);
        ui->minimizeToTray->setEnabled(false);
    }
    else
    {
        ui->minimizeToTray->setEnabled(true);
    }
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
        QTimer::singleShot(10000, this, SLOT(clearStatusLabel()));
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
    proxyType proxy;
    std::string strProxy;
    QString strDefaultProxyGUI;

    GetProxy(NET_IPV4, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv4->setChecked(true) : ui->proxyReachIPv4->setChecked(false);

    GetProxy(NET_IPV6, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachIPv6->setChecked(true) : ui->proxyReachIPv6->setChecked(false);

    GetProxy(NET_ONION, proxy);
    strProxy = proxy.proxy.ToStringIP() + ":" + proxy.proxy.ToStringPort();
    strDefaultProxyGUI = ui->proxyIp->text() + ":" + ui->proxyPort->text();
    (strProxy == strDefaultProxyGUI.toStdString()) ? ui->proxyReachTor->setChecked(true) : ui->proxyReachTor->setChecked(false);
}

ProxyAddressValidator::ProxyAddressValidator(QObject *parent) :
QValidator(parent)
{
}

QValidator::State ProxyAddressValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos);
    // Validate the proxy
    CService serv(LookupNumeric(input.toStdString().c_str(), DEFAULT_GUI_PROXY_PORT));
    proxyType addrProxy = proxyType(serv, true);
    if (addrProxy.IsValid())
        return QValidator::Acceptable;

    return QValidator::Invalid;
}
