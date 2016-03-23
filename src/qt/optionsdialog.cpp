#include "optionsdialog.h"
#include "ui_optionsdialog.h"

#include "bitcoinunits.h"
#include "monitoreddatamapper.h"
#include "netbase.h"
#include "optionsmodel.h"
#include "dialogwindowflags.h"

#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QRegExp>
#include <QRegExpValidator>
#include <QKeyEvent>
#include <QFileDialog>

#if QT_VERSION < 0x050000
#include <QDesktopServices>
#else
#include <QStandardPaths>
#endif

OptionsDialog::OptionsDialog(QWidget *parent) :
    QWidget(parent, DIALOGWINDOWHINTS),
    ui(new Ui::OptionsDialog),
    model(0),
    mapper(0),
    fRestartWarningDisplayed_Proxy(false),
    fRestartWarningDisplayed_Tor(false),
    fRestartWarningDisplayed_Lang(false),
    fRestartWarningDisplayed_URL(false),
    fProxyIpValid(true),
    fTorIpValid(true)
{
    ui->setupUi(this);

    /* Network elements init */
    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    ui->torIp->setEnabled(false);
    ui->torPort->setEnabled(false);
    ui->torPort->setValidator(new QIntValidator(1, 65535, this));
    ui->TorOnly->setEnabled(false);
    ui->torName->setEnabled(false);

    ui->socksVersion->setEnabled(false);
    ui->socksVersion->addItem("5", 5);
    ui->socksVersion->addItem("4", 4);
    ui->socksVersion->setCurrentIndex(0);

    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyIp, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyPort, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->socksVersion, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning_Proxy()));

    connect(ui->connectTor, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning_Tor()));
    connect(ui->connectTor, SIGNAL(toggled(bool)), ui->torIp, SLOT(setEnabled(bool)));
    connect(ui->connectTor, SIGNAL(toggled(bool)), ui->torPort, SLOT(setEnabled(bool)));
    connect(ui->connectTor, SIGNAL(toggled(bool)), ui->TorOnly, SLOT(setEnabled(bool)));
    connect(ui->connectTor, SIGNAL(toggled(bool)), ui->torName, SLOT(setEnabled(bool)));
    connect(ui->TorOnly, SIGNAL(toggled(bool)), ui->connectSocks, SLOT(setDisabled(bool)));

    ui->proxyIp->installEventFilter(this);
    ui->torIp->installEventFilter(this);

    /* Window elements init */
#ifdef Q_OS_MAC
    ui->tabWindow->setVisible(false);
#endif

    /* Display elements init */
    QDir translations(":translations");
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    foreach(const QString &langStr, translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language - country (locale name)", e.g. "German - Germany (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" - ") + QLocale::countryToString(locale.country()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
        else
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language (locale name)", e.g. "German (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
    }

#if QT_VERSION >= 0x040700
    ui->thirdPartyTxUrls->setPlaceholderText("https://example.com/tx/%s");
#endif


    ui->unit->setModel(new BitcoinUnits(this));

    /* Widget-to-option mapper */
    mapper = new MonitoredDataMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    /* enable apply button when data modified */
    connect(mapper, SIGNAL(viewModified()), this, SLOT(enableApplyButton()));
    /* disable apply button when new data loaded */
    connect(mapper, SIGNAL(currentIndexChanged(int)), this, SLOT(disableApplyButton()));
    /* setup/change UI elements when proxy IP is invalid/valid */
    connect(this, SIGNAL(proxyIpValid(QValidatedLineEdit *, bool)), this, SLOT(handleProxyIpValid(QValidatedLineEdit *, bool)));
    /* setup/change UI elements when Tor IP is invalid/valid */
    connect(this, SIGNAL(torIpValid(QValidatedLineEdit *, bool)), this, SLOT(handleTorIpValid(QValidatedLineEdit *, bool)));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if(model)
    {
        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        mapper->setModel(model);
        setMapper();
        mapper->toFirst();
    }

    /* update the display unit, to not use the default ("BTC") */
    updateDisplayUnit();

    /* warn only when language selection changes by user action (placed here so init via mapper doesn't trigger this) */
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning_Lang()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning_URL()));

    /* disable apply button after settings are loaded as there is nothing to save */
    disableApplyButton();
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->transactionFee, OptionsModel::Fee);
    mapper->addMapping(ui->bitcoinAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->detachDatabases, OptionsModel::DetachDatabases);

    /* Network */
    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);
    mapper->addMapping(ui->socksVersion, OptionsModel::ProxySocksVersion);

    mapper->addMapping(ui->connectTor, OptionsModel::TorUse);
    mapper->addMapping(ui->torIp, OptionsModel::TorIP);
    mapper->addMapping(ui->torPort, OptionsModel::TorPort);
    mapper->addMapping(ui->TorOnly, OptionsModel::TorOnly);
    mapper->addMapping(ui->torName, OptionsModel::TorName);
    mapper->addMapping(ui->externalSeederCommand, OptionsModel::ExternalSeeder);


    /* Window */
#ifndef Q_OS_MAC
    mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->displayAddresses, OptionsModel::DisplayAddresses);
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);
}

void OptionsDialog::enableApplyButton()
{
    ui->applyButton->setEnabled(true);
}

void OptionsDialog::disableApplyButton()
{
    ui->applyButton->setEnabled(false);
}

void OptionsDialog::enableSaveButtons()
{
    /* prevent enabling of the save buttons when data modified, if there is an invalid proxy address present */
    if(fProxyIpValid && fTorIpValid)
        setSaveButtonState(true);
}

void OptionsDialog::disableSaveButtons()
{
    setSaveButtonState(false);
}

void OptionsDialog::setSaveButtonState(bool fState)
{
    ui->applyButton->setEnabled(fState);
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_okButton_clicked()
{
    mapper->submit();
//    accept();
    close();
}

void OptionsDialog::on_cancelButton_clicked()
{
//    reject();
    close();
}

void OptionsDialog::on_applyButton_clicked()
{
    mapper->submit();
    disableApplyButton();
}

void OptionsDialog::showRestartWarning_Proxy()
{
    if(!fRestartWarningDisplayed_Proxy)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting NovaCoin."), QMessageBox::Ok);
        fRestartWarningDisplayed_Proxy = true;
    }
}

void OptionsDialog::showRestartWarning_Tor()
{
    if(!fRestartWarningDisplayed_Proxy)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting NovaCoin."), QMessageBox::Ok);
        fRestartWarningDisplayed_Tor = true;
    }
}

void OptionsDialog::showRestartWarning_Lang()
{
    if(!fRestartWarningDisplayed_Lang)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting NovaCoin."), QMessageBox::Ok);
        fRestartWarningDisplayed_Lang = true;
    }
}

void OptionsDialog::showRestartWarning_URL()
{
    if(!fRestartWarningDisplayed_URL)
    {
        QMessageBox::warning(this, tr("Warning"), tr("This setting will take effect after restarting NovaCoin."), QMessageBox::Ok);
        fRestartWarningDisplayed_URL = true;
    }
}


void OptionsDialog::updateDisplayUnit()
{
    if(model)
    {
        /* Update transactionFee with the current unit */
        ui->transactionFee->setDisplayUnit(model->getDisplayUnit());
    }
}

void OptionsDialog::handleProxyIpValid(QValidatedLineEdit *object, bool fState)
{
    // this is used in a check before re-enabling the save buttons
    fProxyIpValid = fState;

    if(fProxyIpValid)
    {
        enableSaveButtons();
        ui->statusLabel->clear();
    }
    else
    {
        disableSaveButtons();
        object->setValid(fProxyIpValid);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
}

void OptionsDialog::handleTorIpValid(QValidatedLineEdit *object, bool fState)
{
    // this is used in a check before re-enabling the save buttons
    fTorIpValid = fState;

    if(fTorIpValid)
    {
        enableSaveButtons();
        ui->statusLabel->clear();
    }
    else
    {
        disableSaveButtons();
        object->setValid(fTorIpValid);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The supplied tor address is invalid."));
    }
}

bool OptionsDialog::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::FocusOut)
    {
        if(object == ui->proxyIp)
        {
            CService addr;
            /* Check proxyIp for a valid IPv4/IPv6 address and emit the proxyIpValid signal */
            emit proxyIpValid(ui->proxyIp, LookupNumeric(ui->proxyIp->text().toStdString().c_str(), addr));
        }

        if(object == ui->torIp)
        {
            CService addr;
            /* Check proxyIp for a valid IPv4/IPv6 address and emit the torIpValid signal */
            emit torIpValid(ui->torIp, LookupNumeric(ui->torIp->text().toStdString().c_str(), addr));
        }
    }
    return QWidget::eventFilter(object, event);
}

void OptionsDialog::keyPressEvent(QKeyEvent *event)
{
#ifdef ANDROID
    if(windowType() != Qt::Widget && event->key() == Qt::Key_Back)
    {
        close();
    }
#else
    if(windowType() != Qt::Widget && event->key() == Qt::Key_Escape)
    {
        close();
    }
#endif
}
void OptionsDialog::on_chooseSeeder_clicked()
{
#if QT_VERSION < 0x050000
    QString openDir = QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation);
#else
    QString openDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
#endif

    QString filename = QFileDialog::getOpenFileName(this, tr("Choose peer collector application"), openDir, tr("Applications (*.*)"));
    if(!filename.isEmpty()) {
        ui->externalSeederCommand->setText(filename);
    }
}
