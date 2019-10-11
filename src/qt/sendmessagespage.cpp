#include <qt/sendmessagespage.h>
#include <qt/forms/ui_sendmessagespage.h>
#include <qt/walletmodel.h>
#include <qt/messagemodel.h>
#include <qt/addressbookpage.h>
#include <qt/optionsmodel.h>
#include <qt/guiutil.h>
#include <qt/sendmessagesentry.h>
#include <qt/walletview.h>
#include <qt/platformstyle.h>

#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>
#include <QDataWidgetMapper>

SendMessagesPage::SendMessagesPage(const PlatformStyle *_platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SendMessagesPage),
    walletmodel(nullptr),
    model(nullptr),
    platformStyle(_platformStyle)
{

    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

#if QT_VERSION >= 0x040700
     /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    if(mode == SendMessagesPage::Encrypted)
        ui->addressFrom->setPlaceholderText(tr("Enter one of your  Addresses"));
 #endif
    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(reject()));

    connect(ui->inboxButton, SIGNAL(clicked()), this, SLOT(on_inboxButton_clicked()));


    fNewRecipientAllowed = true;

    if(mode == SendMessagesPage::Anonymous)
        ui->frameAddressFrom->hide();

    if(type == SendMessagesPage::Page)
        ui->closeButton->hide();
}

void SendMessagesPage::setModel(MessageModel *model)
{
    this->model = model;

    if(model)
    {
        // Get IRC Messages
        connect(model->getOptionsModel(), SIGNAL(enableMessageSendConfChanged(bool)), this, SLOT(enableSendMessageConf(bool)));

        enableSendMessageConf(model->getOptionsModel()->getEnableMessageSendConf());
    }

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());

        if(entry)
            entry->setModel(model);
    }
}

void SendMessagesPage::setWalletModel(WalletModel *walletmodel)
{
    this->walletmodel = walletmodel;
}

void SendMessagesPage::loadRow(int row)
{
    if(model->data(model->index(row, model->Type, QModelIndex()), Qt::DisplayRole).toString() == MessageModel::Received)
        ui->addressFrom->setText(model->data(model->index(row, model->ToAddress, QModelIndex()), Qt::DisplayRole).toString());
    else
        ui->addressFrom->setText(model->data(model->index(row, model->FromAddress, QModelIndex()), Qt::DisplayRole).toString());

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());

        if(entry)
            entry->loadRow(row);
    }
}

void SendMessagesPage::loadInvoice(QString message, QString from_address, QString to_address)
{
    ui->addressFrom->setText(from_address);

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());

        if(entry)
            entry->loadInvoice(message, to_address);
    }

    ui->addButton->setVisible(false);
}

bool SendMessagesPage::checkMode(Mode mode)
{
    return (mode == this->mode);
}

bool SendMessagesPage::validate()
{
    if(mode == SendMessagesPage::Encrypted && ui->addressFrom->text() == "")
    {
        ui->addressFrom->setValid(false);

        return false;
    }

    return true;
}

SendMessagesPage::~SendMessagesPage()
{
    delete ui;
}

void SendMessagesPage::enableSendMessageConf(bool enabled = true)
{
    fEnableSendMessageConf = enabled;
}

void SendMessagesPage::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->addressFrom->setText(QApplication::clipboard()->text());
}

void SendMessagesPage::on_addressBookButton_clicked()
{
    if(!walletmodel)
        return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
    dlg.setModel(walletmodel->getAddressTableModel());
    if(dlg.exec())
    {
        ui->addressFrom->setText(dlg.getReturnValue());
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(0)->widget());
        entry->setFocus();
               // findChild( const QString "sentTo")->setFocus();
    }
}

void SendMessagesPage::on_sendButton_clicked()
{
    QList<SendMessagesRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    valid = validate();

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
                recipients.append(entry->getValue());
            else
                valid = false;
        }
    }

    if(!valid || recipients.isEmpty())
        return;

    // Format confirmation message
    QStringList formatted;
    for(const SendMessagesRecipient &rcp: recipients)
    {
        formatted.append(tr("<b>%1</b> to %2 (%3)").arg(rcp.message, GUIUtil::HtmlEscape(rcp.label), rcp.address));
    }

    fNewRecipientAllowed = false;

    if(fEnableSendMessageConf)
    {
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send messages"),
                              tr("Are you sure you want to send %1?").arg(formatted.join(tr(" and "))),
              QMessageBox::Yes|QMessageBox::Cancel,
              QMessageBox::Cancel);

        if(retval != QMessageBox::Yes)
        {
            fNewRecipientAllowed = true;
            return;
        }
    }

    MessageModel::StatusCode sendstatus;

    if(mode == SendMessagesPage::Anonymous)
        sendstatus = model->sendMessages(recipients);
    else
        sendstatus = model->sendMessages(recipients, ui->addressFrom->text());

    switch(sendstatus)
    {
    case MessageModel::InvalidAddress:
        QMessageBox::warning(this, tr("Send Message"),
            tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::InvalidMessage:
        QMessageBox::warning(this, tr("Send Message"),
            tr("The message can't be empty."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::DuplicateAddress:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::MessageCreationFailed:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Error: Message creation failed."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::MessageCommitFailed:
        QMessageBox::warning(this, tr("Send Message"),
            tr("Error: The message was rejected."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case MessageModel::Aborted:             // User aborted, nothing to do
        break;
    case MessageModel::FailedErrorShown:    // Send failed, error message was displayed
        break;
    case MessageModel::OK:
        accept();
        break;
    }

    fNewRecipientAllowed = true;
}

void SendMessagesPage::reject()
{
    clear();
}

void SendMessagesPage::accept()
{
    clear();
}

void SendMessagesPage::done(int retval)
{
    clear();
}

void SendMessagesPage::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
        delete ui->entries->takeAt(0)->widget();

    addEntry();

    updateRemoveEnabled();

    ui->sendButton->setDefault(true);
}

SendMessagesEntry *SendMessagesPage::addEntry()
{
    SendMessagesEntry *entry = new SendMessagesEntry(platformStyle,this);

    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendMessagesEntry*)), this, SLOT(removeEntry(SendMessagesEntry*)));
    updateRemoveEnabled();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();

    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QCoreApplication::instance()->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();

    if(bar)
        bar->setSliderPosition(bar->maximum());

    return entry;
}

void SendMessagesPage::updateRemoveEnabled()
{
    // Remove buttons are enabled as soon as there is more than one send-entry
    bool enabled = (ui->entries->count() > 1);

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            entry->setRemoveEnabled(enabled);
    }

    setupTabChain(0);
}

void SendMessagesPage::removeEntry(SendMessagesEntry* entry)
{
    delete entry;

    updateRemoveEnabled();
}

QWidget *SendMessagesPage::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendMessagesEntry *entry = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }

    QWidget::setTabOrder(prev, ui->addButton);
    QWidget::setTabOrder(ui->addButton, ui->sendButton);

    return ui->sendButton;
}

void SendMessagesPage::pasteEntry(const SendMessagesRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendMessagesEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendMessagesEntry *first = qobject_cast<SendMessagesEntry*>(ui->entries->itemAt(0)->widget());

        if(first->isClear())
            entry = first;
    }

    if(!entry)
        entry = addEntry();

    entry->setValue(rv);
}

void SendMessagesPage::on_inboxButton_clicked()
{
    WalletView* pMainWindow = qobject_cast<WalletView*>(parent());
    if (pMainWindow)
        pMainWindow->gotoMessagesPage();
}

