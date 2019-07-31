#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include "uint256.h"

#include <vector>
#include <support/allocators/secure.h>  //for SecureString 
#include <smessage/smessage.h>
#include <map>
#include <QAbstractTableModel>
#include <QStringList>
#include <QDateTime>


class MessageTablePriv;
//class InvoiceTableModel;
//class InvoiceItemTableModel;
//class ReceiptTableModel;
class WalletModel;
class OptionsModel;

class SendMessagesRecipient
{
public:
    QString address;
    QString label;
    QString pubkey;
    QString message;
};

struct MessageTableEntry
{
    enum Type {
        Sent,
        Received
    };

    std::vector<unsigned char> vchKey;
    Type type;
    QString label;
    QString to_address;
    QString from_address;
    QDateTime sent_datetime;
    QDateTime received_datetime;
    bool read;
    QString message;

    MessageTableEntry() {}
    MessageTableEntry(const std::vector<unsigned char> vchKey, Type type, const QString &label, const QString &to_address, const QString &from_address,
                      const QDateTime &sent_datetime, const QDateTime &received_datetime, const bool &read, const QString &message):
        vchKey(vchKey), type(type), label(label), to_address(to_address), from_address(from_address), sent_datetime(sent_datetime), received_datetime(received_datetime),
        read(read), message(message) {}
};

// Interface to Secure Messaging from Qt view code. 
class MessageModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit MessageModel(WalletModel *walletModel, QObject *parent = 0);
    ~MessageModel();

    enum StatusCode // Returned by sendMessages
    {
        OK,
        InvalidAddress,
        InvalidMessage,
        DuplicateAddress,
        MessageCreationFailed, // Error returned when DB is still locked
        MessageCommitFailed,
        Aborted,
        FailedErrorShown
    };

    enum ColumnIndex {
        Type = 0,   // Sent/Received 
        SentDateTime = 1, // Time Sent 
        ReceivedDateTime = 2, // Time Received 
        Label = 3,   // User specified label 
        ToAddress = 4, // To Bitcointalkcoin address 
        FromAddress = 5, // From Bitcointalkcoin address 
        Message = 6, // Plaintext 
        Read = 7, // Plaintext 
        TypeInt = 8, // Plaintext 
        Key = 9, // chKey 
        HTML = 10, // HTML Formatted Data 
    };

     //Roles to get specific information from a message row.
     //   These are independent of column.
    
    enum RoleIndex {
        // Type of message 
        TypeRole = Qt::UserRole,
        // message key 
        KeyRole,
         //Date and time this message was sent 
        SentDateRole,
         //Date and time this message was received 
        ReceivedDateRole,
        // From Address of message 
        FromAddressRole,
       //  To Address of message 
        ToAddressRole,
        // Filter address related to message 
        FilterAddressRole,
         //Label of address related to message 
        LabelRole,
         //Full Message 
        MessageRole,
        // Short Message 
        ShortMessageRole,
        // Message Read 
        ReadRole,
        // HTML Formatted 
        HTMLRole,
       //  Ambiguous bool 
        Ambiguous
    };

    static const QString Sent; // Specifies sent message 
    static const QString Received; // Specifies sent message 

     //name Methods overridden from QAbstractTableModel
        
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex & index) const;
    


     //Mark message as read
    
    bool markMessageAsRead(const QString &key) const;
    // Look up row index of a message in the model.
    //   Return -1 if not found.
     
    int lookupMessage(const QString &message) const;

    WalletModel *getWalletModel();
    OptionsModel *getOptionsModel();
    //InvoiceTableModel *getInvoiceTableModel();
    //ReceiptTableModel *getReceiptTableModel();

    bool getAddressOrPubkey( QString &Address,  QString &Pubkey) const;

    // Send messages to a list of recipients
    StatusCode sendMessages(const QList<SendMessagesRecipient> &recipients);
    StatusCode sendMessages(const QList<SendMessagesRecipient> &recipients, const QString &addressFrom);

private:
    CWallet *wallet;
    WalletModel *walletModel;
    OptionsModel *optionsModel;
    MessageTablePriv *priv;
    //InvoiceTableModel *invoiceTableModel;
    //ReceiptTableModel *receiptTableModel;
    QStringList columns;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public Q_SLOTS:

   //  Check for new messages 
    void newMessage(const SecMsgStored& smsg);
    void newOutboxMessage(const SecMsgStored& smsg);

    void walletUnlocked();

    void setEncryptionStatus(int status);

    friend class MessageTablePriv;

Q_SIGNALS:
    // Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);
};

/*
struct InvoiceItemTableEntry
{

    std::vector<unsigned char> vchKey;
    MessageTableEntry::Type type;
    QString code;
    QString description;
    int     quantity;
    int64_t  price;
    //bool    tax;

    InvoiceItemTableEntry(){};
    InvoiceItemTableEntry(const bool newInvoice):
        vchKey(0), type(MessageTableEntry::Sent), code(""), description(""), quantity(0), price(0)
    {
        vchKey.resize(3);
        memcpy(&vchKey[0], "new", 3);
    };
    InvoiceItemTableEntry(const std::vector<unsigned char> vchKey, MessageTableEntry::Type type, const QString &code, const QString &description, const int &quantity, const qint64 &price): //, const bool &tax):
        vchKey(vchKey), type(type), code(code), description(description), quantity(quantity), price(price) {} //, tax(tax) {}
};


struct InvoiceTableEntry
{
    std::vector<unsigned char> vchKey;
    MessageTableEntry::Type type;
    QString label;
    QString to_address;
    QString from_address;
    QDateTime sent_datetime;
    QDateTime received_datetime;
    QString company_info_left;
    QString company_info_right;
    QString billing_info_left;
    QString billing_info_right;
    QString footer;
    QString invoice_number;
    QDate   due_date;

    InvoiceTableEntry() {}
    InvoiceTableEntry(const bool newInvoice):
        vchKey(0), type(MessageTableEntry::Sent), label(""), to_address(""), from_address(""), sent_datetime(), received_datetime(), company_info_left(""), company_info_right(""),
        billing_info_left(""), billing_info_right(""), footer(""), invoice_number(""), due_date()
    {
        vchKey.resize(3);
        memcpy(&vchKey[0], "new", 3);
    };
    InvoiceTableEntry(const std::vector<unsigned char> vchKey, MessageTableEntry::Type type, const QString &label, const QString &to_address, const QString &from_address,
                      const QDateTime &sent_datetime, const QDateTime &received_datetime, const QString &company_info_left, const QString &company_info_right,
                      const QString &billing_info_left, const QString &billing_info_right, const QString &footer, const QString &invoice_number, const QDate &due_date):
        vchKey(vchKey), type(type), label(label), to_address(to_address), from_address(from_address), sent_datetime(sent_datetime), received_datetime(received_datetime),
        company_info_left(company_info_left), company_info_right(company_info_right), billing_info_left(billing_info_left), billing_info_right(billing_info_right),
        footer(footer), invoice_number(invoice_number), due_date(due_date)
    {}
    InvoiceTableEntry(const MessageTableEntry &messageTableEntry, const QString &company_info_left, const QString &company_info_right,
                      const QString &billing_info_left, const QString &billing_info_right, const QString &footer, const QString &invoice_number, const QDate &due_date):
        vchKey(messageTableEntry.vchKey), type(messageTableEntry.type), label(messageTableEntry.label), to_address(messageTableEntry.to_address), from_address(messageTableEntry.from_address),
        sent_datetime(messageTableEntry.sent_datetime), received_datetime(messageTableEntry.received_datetime),
        company_info_left(company_info_left), company_info_right(company_info_right), billing_info_left(billing_info_left), billing_info_right(billing_info_right),
        footer(footer), invoice_number(invoice_number), due_date(due_date)
    {}
};


// Interface to Secure Messaging Invoices from Qt view code. 
class InvoiceTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit InvoiceTableModel(MessageTablePriv *priv, QObject *parent = 0);
    ~InvoiceTableModel();

    enum ColumnIndex {
        Type = 0,   //< Sent/Received 
        SentDateTime = 1, //< Time Sent 
        ReceivedDateTime = 2, //< Time Received 
        Label = 3,   //< User specified label 
        ToAddress = 4, //< To Bitcointalkcoin address 
        FromAddress = 5, /< From Bitcointalkcoin address 
        InvoiceNumber = 6, < Plaintext 
        DueDate = 7, < Plaintext 
        //SubTotal = 8,           < SubTotal 
        Total = 8,           < Total 
        Paid = 9,             < Amount Paid
        Outstanding = 10, < Amount Outstanding 
        // Hidden fields
        CompanyInfoLeft = 11, < Plaintext 
        CompanyInfoRight = 12, < Plaintext 
        BillingInfoLeft = 13, < Plaintext 
        BillingInfoRight = 14, < Plaintext 
        Footer = 15, < Plaintext 
    };

     //name Methods overridden from QAbstractTableModel
        
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    //bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex & index) const;
    

    MessageModel *getMessageModel();
    InvoiceItemTableModel *getInvoiceItemTableModel();

    void newInvoice(QString CompanyInfoLeft,
                    QString CompanyInfoRight,
                    QString BillingInfoLeft,
                    QString BillingInfoRight,
                    QString Footer,
                    QDate   DueDate,
                    QString InvoiceNumber);

    void newInvoiceItem();
    void newReceipt(QString InvoiceNumber, CAmount ReceiptAmount);
    //void setData(const int row, const int col, const QVariant & value);

    QString getInvoiceJSON(const int row);

    int lookupMessage(const QString &message) const;

private:
    QStringList columns;
    MessageTablePriv *priv;
    InvoiceItemTableModel *invoiceItemTableModel;

public Q_SLOTS:
    friend class MessageTablePriv;

};


 Interface to Secure Messaging Invoice Items from Qt view code. 
class InvoiceItemTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit InvoiceItemTableModel(MessageTablePriv *priv, QObject *parent = 0);
    ~InvoiceItemTableModel();

    enum ColumnIndex {
        Type = 0,   < Sent/Received 
        Code = 1,   < Item Code 
        Description = 2, < Item Description 
        Quantity = 3, < Item quantity 
        Price = 4,   < Item Price 
        //Tax = 4,   < Item Price 
        Amount = 5, < Total for row 
    };

     @name Methods overridden from QAbstractTableModel
        @{
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex & index) const;
    @}


private:
    QStringList columns;
    MessageTablePriv *priv;

     Notify listeners that data changed. 
    void emitDataChanged(const int idx);

public Q_SLOTS:
    friend class MessageTablePriv;
};


struct ReceiptTableEntry
{
    std::vector<unsigned char> vchKey;
    MessageTableEntry::Type type;
    QString label;
    QString to_address;
    QString from_address;
    QDateTime sent_datetime;
    QDateTime received_datetime;
    QString invoice_number;
    qint64  amount;

    ReceiptTableEntry() {}
    ReceiptTableEntry(const bool newReceipt):
        vchKey(0), type(MessageTableEntry::Sent), label(""), to_address(""), from_address(""), sent_datetime(), received_datetime(), invoice_number(""), amount(0)
    {
        vchKey.resize(3);
        memcpy(&vchKey[0], "new", 3);
    };

    ReceiptTableEntry(const std::vector<unsigned char> vchKey, MessageTableEntry::Type type, const QString &label, const QString &to_address, const QString &from_address,
                      const QDateTime &sent_datetime, const QDateTime &received_datetime, const QString &invoice_number, const qint64 &amount):
        vchKey(vchKey), type(type), label(label), to_address(to_address), from_address(from_address), sent_datetime(sent_datetime), received_datetime(received_datetime),
        invoice_number(invoice_number), amount(amount)
    {}
    ReceiptTableEntry(const MessageTableEntry &messageTableEntry, const QString &invoice_number, const qint64 &amount):
        vchKey(messageTableEntry.vchKey), type(messageTableEntry.type), label(messageTableEntry.label), to_address(messageTableEntry.to_address), from_address(messageTableEntry.from_address),
        sent_datetime(messageTableEntry.sent_datetime), received_datetime(messageTableEntry.received_datetime), invoice_number(invoice_number), amount(amount)
    {}
    ReceiptTableEntry(const InvoiceTableEntry &invoiceTableEntry, const qint64 &amount):
        vchKey(invoiceTableEntry.vchKey), type(invoiceTableEntry.type), label(invoiceTableEntry.label), to_address(invoiceTableEntry.to_address), from_address(invoiceTableEntry.from_address),
        sent_datetime(invoiceTableEntry.sent_datetime), received_datetime(invoiceTableEntry.received_datetime), invoice_number(invoiceTableEntry.invoice_number), amount(amount)
    {}
};


 Interface to Secure Messaging Receipts from Qt view code. 
class ReceiptTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ReceiptTableModel(MessageTablePriv *priv, QObject *parent = 0);
    ~ReceiptTableModel();

    enum ColumnIndex {
        Type = 0,   < Sent/Received 
        SentDateTime = 1, < Time Sent 
        ReceivedDateTime = 2, < Time Received 
        Label = 3,   < User specified label 
        ToAddress = 4, < To Bitcointalkcoin address 
        FromAddress = 5, < From Bitcointalkcoin address 
        InvoiceNumber = 6, < Plaintext 
        Amount = 7, < qint64 
        Outstanding = 8, < qint64 
    };

     @name Methods overridden from QAbstractTableModel
        @{
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    //bool setData(const QModelIndex & index, const QVariant & value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent) const;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex & index) const;
    @}

    QString getReceiptJSON(const int row);

private:
    QStringList columns;
    MessageTablePriv *priv;

public Q_SLOTS:
    friend class MessageTablePriv;
};
*/
#endif // MESSAGEMODEL_H
