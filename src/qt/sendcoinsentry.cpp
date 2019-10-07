// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/sendcoinsentry.h>
#include <qt/forms/ui_sendcoinsentry.h>

#include <qt/addressbookpage.h>
#include <qt/addresstablemodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <validation.h>

#include <QApplication>
#include <QClipboard>

#include <array>

static const std::array<int, 11> bindActiveHeights = { {6, 12, 24, 144, 288, 288*2, 288*3, 288*4, 288*5, 288*6, 288*7} };
int getPlotterDataValidHeightForIndex(int index) {
    if (index+1 > static_cast<int>(bindActiveHeights.size())) {
        return bindActiveHeights.back();
    }
    if (index < 0) {
        return bindActiveHeights[0];
    }
    return bindActiveHeights[index];
}
int getIndexForPlotterDataValidHeight(int height) {
    for (unsigned int i = 0; i < bindActiveHeights.size(); i++) {
        if (bindActiveHeights[i] >= height) {
            return i;
        }
    }
    return bindActiveHeights.size() - 1;
}

SendCoinsEntry::SendCoinsEntry(PayOperateMethod _payOperateMethod, const PlatformStyle *_platformStyle, QWidget *parent) :
    QStackedWidget(parent),
    payOperateMethod(_payOperateMethod),
    ui(new Ui::SendCoinsEntry),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    ui->plotterPassphraseLabel->setVisible(false);
    ui->plotterPassphrase->setVisible(false);
    ui->plotterDataValidHeightLabel->setVisible(false);
    ui->plotterDataValidHeightSelector->setVisible(false);

    ui->addressBookButton->setIcon(platformStyle->SingleColorIcon(":/icons/address-book"));
    ui->pasteButton->setIcon(platformStyle->SingleColorIcon(":/icons/editpaste"));
    ui->deleteButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_is->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_s->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->payAmount->setContentsMargins(0, 0, 6, 0);

    setCurrentWidget(ui->SendCoins);

    if (platformStyle->getUseExtraSpacing())
        ui->payToLayout->setSpacing(4);
#if QT_VERSION >= 0x040700
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
#endif

    // normal bitcoin address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying bitcoin address(es)
    ui->payTo_is->setFont(GUIUtil::fixedPitchFont());

    // Connect signals
    connect(ui->payAmount, SIGNAL(valueChanged()), this, SIGNAL(payAmountChanged()));
    connect(ui->checkboxSubtractFeeFromAmount, SIGNAL(toggled(bool)), this, SIGNAL(subtractFeeFromAmountChanged()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->useAvailableBalanceButton, SIGNAL(clicked()), this, SLOT(useAvailableBalanceClicked()));

    // Pay method
    if (payOperateMethod == PayOperateMethod::LoanTo) {
        ui->payToLabel->setText(tr("Loan &To:"));
    } else if (payOperateMethod == PayOperateMethod::BindPlotter) {
        ui->payToLabel->setText(tr("Bind &To:"));
        ui->labellLabel->setVisible(false);
        ui->addAsLabel->setVisible(false);
        ui->amountLabel->setVisible(false);
        ui->payAmount->setVisible(false);
        ui->checkboxSubtractFeeFromAmount->setVisible(false);
        ui->useAvailableBalanceButton->setVisible(false);
        ui->plotterPassphraseLabel->setVisible(true);
        ui->plotterPassphrase->setVisible(true);
    #if QT_VERSION >= 0x040700
        ui->plotterPassphrase->setPlaceholderText(tr("Enter your plotter passphrase or bind hex data"));
    #endif
        ui->plotterDataValidHeightLabel->setVisible(true);
        ui->plotterDataValidHeightSelector->setVisible(true);
        for (const int n : bindActiveHeights) {
            assert(n > 0 && n <= PROTOCOL_BINDPLOTTER_MAXALIVE);
            ui->plotterDataValidHeightSelector->addItem(tr("%1 (%2 blocks)")
                .arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().BHDIP008TargetSpacing))
                .arg(n));
        }
        ui->plotterDataValidHeightSelector->setCurrentIndex(getIndexForPlotterDataValidHeight(PROTOCOL_BINDPLOTTER_DEFAULTMAXALIVE));
    }
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage::Tabs tab = (payOperateMethod == PayOperateMethod::BindPlotter ? AddressBookPage::ReceivingTab : AddressBookPage::SendingTab);
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, tab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
    updateLabel(address);
}

void SendCoinsEntry::setModel(WalletModel *_model)
{
    this->model = _model;

    if (_model && _model->getOptionsModel())
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    clear();
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->plotterPassphrase->clear();
    ui->payAmount->clear();
    ui->checkboxSubtractFeeFromAmount->setCheckState(Qt::Unchecked);
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    // clear UI elements for unauthenticated payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for authenticated payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("BitcoinHD")
    updateDisplayUnit();

    // Update for bind plotter
    if (payOperateMethod == PayOperateMethod::BindPlotter) {
        ui->payAmount->setValue(PROTOCOL_BINDPLOTTER_LOCKAMOUNT);
        ui->checkboxSubtractFeeFromAmount->setCheckState(Qt::Unchecked);
    }
}

void SendCoinsEntry::checkSubtractFeeFromAmount()
{
    ui->checkboxSubtractFeeFromAmount->setChecked(true);
}

void SendCoinsEntry::deleteClicked()
{
    Q_EMIT removeEntry(this);
}

void SendCoinsEntry::useAvailableBalanceClicked()
{
    Q_EMIT useAvailableBalance(this);
}

bool SendCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!model->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Sending a zero amount is invalid
    if (ui->payAmount->value(0) <= 0)
    {
        ui->payAmount->setValid(false);
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    // Special tx amount
    if (payOperateMethod == PayOperateMethod::LoanTo)
    {
        if (ui->payAmount->value() < PROTOCOL_RENTAL_AMOUNT_MIN ||
                (ui->checkboxSubtractFeeFromAmount->checkState() == Qt::Checked && ui->payAmount->value() <= PROTOCOL_RENTAL_AMOUNT_MIN)) {
            ui->payAmount->setValid(false);
            retval = false;
        }
    }
    else if (payOperateMethod == PayOperateMethod::BindPlotter)
    {
        QString passphrase = ui->plotterPassphrase->text().trimmed();
        if (!IsValidPassphrase(passphrase.toStdString())) {
            ui->plotterPassphrase->setValid(false);
            retval = false;
        }
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    if (payOperateMethod == PayOperateMethod::BindPlotter) {
        recipient.plotterPassphrase = ui->plotterPassphrase->text().trimmed();
        recipient.plotterDataValidHeight = getPlotterDataValidHeightForIndex(ui->plotterDataValidHeightSelector->currentIndex());
    }
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->messageTextLabel->text();
    recipient.fSubtractFeeFromAmount = (ui->checkboxSubtractFeeFromAmount->checkState() == Qt::Checked);

    return recipient;
}

QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    QWidget::setTabOrder(ui->addAsLabel, ui->plotterPassphrase);
    QWidget *w = ui->payAmount->setupTabChain(ui->plotterPassphrase);
    QWidget::setTabOrder(w, ui->checkboxSubtractFeeFromAmount);
    QWidget::setTabOrder(ui->checkboxSubtractFeeFromAmount, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    return ui->deleteButton;
}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    recipient = value;

    if (recipient.paymentRequest.IsInitialized()) // payment request
    {
        if (recipient.authenticatedMerchant.isEmpty()) // unauthenticated
        {
            ui->payTo_is->setText(recipient.address);
            ui->memoTextLabel_is->setText(recipient.message);
            ui->payAmount_is->setValue(recipient.amount);
            ui->payAmount_is->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_UnauthenticatedPaymentRequest);
        }
        else // authenticated
        {
            ui->payTo_s->setText(recipient.authenticatedMerchant);
            ui->memoTextLabel_s->setText(recipient.message);
            ui->payAmount_s->setValue(recipient.amount);
            ui->payAmount_s->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_AuthenticatedPaymentRequest);
        }
    }
    else // normal payment
    {
        // message
        ui->messageTextLabel->setText(recipient.message);
        ui->messageTextLabel->setVisible(!recipient.message.isEmpty());

        ui->addAsLabel->clear();
        ui->plotterPassphrase->clear();
        ui->payTo->setText(recipient.address); // this may set a label from addressbook
        if (!recipient.label.isEmpty()) // if a label had been set from the addressbook, don't overwrite with an empty label
            ui->addAsLabel->setText(recipient.label);
        if (!recipient.plotterPassphrase.isEmpty())
            ui->plotterPassphrase->setText(recipient.plotterPassphrase);
        ui->payAmount->setValue(recipient.amount);
    }
}

void SendCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

void SendCoinsEntry::setAmount(const CAmount &amount)
{
    ui->payAmount->setValue(amount);
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}
