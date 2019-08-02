#include <qt/guiutil.h>
#include <qt/guiconstants.h>
#include <qt/bitcointalkcoinunits.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>
#include <qt/messagemodel.h>
#include <qt/addresstablemodel.h>
//#include "invoicepage.h"
//#include "invoiceviewpage.h"
//#include "receiptpage.h"
#include <pubkey.h>
#include <key_io.h>
#include <ui_interface.h>
#include <base58.h>
#include <init.h>
#include <util/system.h>
//#include "json_spirit.h"

#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QFont>
#include <QColor>

Q_DECLARE_METATYPE(std::vector<unsigned char>);

QList<QString> ambiguous; /**< Specifies Ambiguous addresses */

const QString MessageModel::Sent = "Sent";
const QString MessageModel::Received = "Received";

struct MessageTableEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const {return a.received_datetime < b.received_datetime;};
    bool operator()(const MessageTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const MessageTableEntry &b) const {return a < b.received_datetime;}
};
/*
struct InvoiceTableEntryLessThan
{
    bool operator()(const InvoiceTableEntry &a, const InvoiceTableEntry &b) const {return a.received_datetime < b.received_datetime;};
    bool operator()(const InvoiceTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const InvoiceTableEntry &b) const {return a < b.received_datetime;}
};

struct ReceiptTableEntryLessThan
{
    bool operator()(const ReceiptTableEntry &a, const ReceiptTableEntry &b) const {return a.received_datetime < b.received_datetime;};
    bool operator()(const ReceiptTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const ReceiptTableEntry &b) const {return a < b.received_datetime;}
};
*/
// Private implementation
class MessageTablePriv
{
public:
    QList<MessageTableEntry> cachedMessageTable;
    //QList<InvoiceTableEntry> cachedInvoiceTable;
    //QList<InvoiceItemTableEntry> cachedInvoiceItemTable;
    //QList<ReceiptTableEntry> cachedReceiptTable;
    MessageModel *parent;

    MessageTablePriv(MessageModel *parent):
        parent(parent) {}

    void refreshMessageTable()
    {
        cachedMessageTable.clear();
        //cachedInvoiceTable.clear();
        //cachedInvoiceItemTable.clear();
        //cachedReceiptTable.clear();

        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- messages are stored encrypted, can't load them without the private keys
            return;
        };

        Dbt datKey;
        Dbt datValue;

        datKey.set_flags(DB_DBT_USERMEM);
        datValue.set_flags(DB_DBT_USERMEM);

        std::vector<unsigned char> vchKeyData;
        std::vector<unsigned char> vchValueData;

        vchKeyData.resize(100);
        vchValueData.resize(100);

        datKey.set_ulen(vchKeyData.size());
        datKey.set_data(&vchKeyData[0]);

        datValue.set_ulen(vchValueData.size());
        datValue.set_data(&vchValueData[0]);

        std::vector<unsigned char> vchKey;

        {
            LOCK(cs_smsgDB);

            SecMsgDB dbSmsg;

            if (!dbSmsg.Open("cr+"))
                //throw runtime_error("Could not open DB.");
                return;
            unsigned char chKey[18];
            std::vector<unsigned char> vchKey;
            vchKey.resize(18);

            SecMsgStored smsgStored;
            MessageData msg;
            QString label;
            QDateTime sent_datetime;
            QDateTime received_datetime;

            std::string sPrefix("im");
            leveldb::Iterator* it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, sPrefix, chKey, smsgStored))
            {

			std::string errorMsg;
			if (SecureMsgDecrypt(smsgStored, msg, errorMsg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString(msg.sFromAddress.c_str()));

                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);

                    memcpy(&vchKey[0], chKey, 18);

                    handleMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Received,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString(msg.sFromAddress.c_str()),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      msg.sMessage.c_str()),
                                    true);
                }
            };

            delete it;

            sPrefix = "sm";
            it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
                SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
                memcpy(smsg.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);
                if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, smsg, pPayload, msg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(smsgStored.sAddrTo));

                    sent_datetime.setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);

                    memcpy(&vchKey[0], chKey, 18);

                    handleMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Sent,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString(msg.sFromAddress.c_str()),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      msg.sMessage.c_str()),
                                    true);
                };
            };

            delete it;
        }
    }

    void newMessage(const SecMsgStored& inboxHdr)
    {
        // we have to copy it, because it doesn't like constants going into Decrypt
        SecMsgStored smsgStored = inboxHdr;
        //SecMsgStored &smsgInbox = const_cast<SecMsgStored&>(inboxHdr); // un-const the reference

        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        std::string errorMsg;
        if (SecureMsgDecrypt(smsgStored, msg, errorMsg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString(msg.sFromAddress.c_str()));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::vector<unsigned char> vchKey;
            std::string sPrefix("im");
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &msg.timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            handleMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Received,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              msg.sFromAddress.c_str(),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              msg.sMessage.c_str()),
                            false);
        }
    }

    void newOutboxMessage(const SecMsgStored& outboxHdr)
    {
        SecMsgStored &smsgStored = const_cast<SecMsgStored&>(outboxHdr); // un-const the reference

        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        const unsigned char* pPayload = &smsgStored.vchMessage[SMSG_HDR_LEN];
        SecureMessageHeader smsg(&smsgStored.vchMessage[0]);
        memcpy(smsg.hash, Hash(&pPayload[0], &pPayload[smsg.nPayload]).begin(), 32);

        if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, smsg, pPayload, msg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(smsgStored.sAddrTo.c_str());

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("sm");
            std::vector<unsigned char> vchKey(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &msg.timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            handleMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Sent,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              msg.sFromAddress.c_str(),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              msg.sMessage.c_str()),
                            false);
        }
    }

    void setEncryptionStatus()
    {
		
        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- Wallet is locked, clear secure message display.
            cachedMessageTable.clear();
            //cachedInvoiceTable.clear();
            //cachedInvoiceItemTable.clear();
            //cachedReceiptTable.clear();

            //parent->reset(); // reload table view
            //parent->getInvoiceTableModel()->getInvoiceItemTableModel()->reset();

        };
    };


    bool markAsRead(const int& idx)
    {
        MessageTableEntry *rec = index(idx);

        if(!rec || rec->read)
            // Can only mark one row at a time, and cannot mark rows not in model.
            return false;

        {
            LOCK(cs_smsgDB);
            SecMsgDB dbSmsg;

            if (!dbSmsg.Open("cr+"))
                //throw runtime_error("Could not open DB.");
                return false;

            SecMsgStored smsgStored;
            dbSmsg.ReadSmesg(&rec->vchKey[0], smsgStored);

            smsgStored.status &= ~SMSG_MASK_UNREAD;
            dbSmsg.WriteSmesg(&rec->vchKey[0], smsgStored);
            rec->read = !(smsgStored.status & SMSG_MASK_UNREAD);
        }

        return true;
    };

    void walletUnlocked()
    {
        // -- wallet is unlocked, can get at the private keys now
        refreshMessageTable();
        //parent->reset(); // reload table view
       // parent->getInvoiceTableModel()->getInvoiceItemTableModel()->reset();

    }
/*
    void newInvoice(InvoiceTableEntry invoice)
    {
        addInvoiceEntry(invoice, false);
    }

    void newReceipt(ReceiptTableEntry receipt)
    {
        addReceiptEntry(receipt, false);
    }

    void newInvoiceItem()
    {
        InvoiceItemTableEntry invoice(true);
        addInvoiceItemEntry(invoice, false);
    }

    QString getInvoiceJSON(const int row)
    {
        QList<InvoiceTableEntry>::iterator inv;
        json_spirit::Object invoice;

        invoice.push_back(json_spirit::Pair("type", "invoice" ));
        invoice.push_back(json_spirit::Pair("company_info_left",  cachedInvoiceTable[row].company_info_left.toStdString()));
        invoice.push_back(json_spirit::Pair("company_info_right", cachedInvoiceTable[row].company_info_right.toStdString()));
        invoice.push_back(json_spirit::Pair("billing_info_left",  cachedInvoiceTable[row].billing_info_left.toStdString()));
        invoice.push_back(json_spirit::Pair("billing_info_right", cachedInvoiceTable[row].billing_info_right.toStdString()));
        invoice.push_back(json_spirit::Pair("footer",             cachedInvoiceTable[row].footer.toStdString()));
        invoice.push_back(json_spirit::Pair("invoice_number",     cachedInvoiceTable[row].invoice_number.toStdString()));
        invoice.push_back(json_spirit::Pair("due_date",           cachedInvoiceTable[row].due_date.toString().toStdString()));

        json_spirit::Array items;

        for(int i = 0;i<cachedInvoiceItemTable.size();i++)
            if(cachedInvoiceItemTable[i].vchKey == cachedInvoiceTable[row].vchKey)
            {
                json_spirit::Object item;

                item.push_back(json_spirit::Pair("code",         cachedInvoiceItemTable[i].code.toStdString()));
                item.push_back(json_spirit::Pair("description",  cachedInvoiceItemTable[i].description.toStdString()));
                item.push_back(json_spirit::Pair("price",        int64_t(cachedInvoiceItemTable[i].price)));
                //item.push_back(json_spirit::Pair("tax",  i->tax));
                item.push_back(json_spirit::Pair("quantity",     cachedInvoiceItemTable[i].quantity));

                items.push_back(item);

                parent->getInvoiceTableModel()->getInvoiceItemTableModel()->beginRemoveRows(QModelIndex(), 0, 0);
                cachedInvoiceItemTable.removeAt(i);
                parent->getInvoiceTableModel()->getInvoiceItemTableModel()->endRemoveRows();
            }

        invoice.push_back(json_spirit::Pair("items", items));

        if(row==0)
        {
            parent->getInvoiceTableModel()->beginRemoveRows(QModelIndex(), 0, 0);
            cachedInvoiceTable.removeAt(row);
            parent->getInvoiceTableModel()->endRemoveRows();
        }

        return QString::fromStdString(json_spirit::write(invoice));
    }

    QString getReceiptJSON(const int row)
    {
        QList<ReceiptTableEntry>::iterator inv;
        json_spirit::Object receipt;

        receipt.push_back(json_spirit::Pair("type", "receipt" ));
        receipt.push_back(json_spirit::Pair("invoice_number",     cachedReceiptTable[row].invoice_number.toStdString()));
        receipt.push_back(json_spirit::Pair("amount",             int64_t(cachedReceiptTable[row].amount)));
        //receipt.push_back(json_spirit::Pair("outstanding",        "0")); //TODO: Calculate outstanding. cachedReceiptTable[row].amount.toStdString()));

        if(row==0)
        {
            parent->getReceiptTableModel()->beginRemoveRows(QModelIndex(), 0, 0);
            cachedReceiptTable.removeAt(row);
            parent->getReceiptTableModel()->endRemoveRows();
        }
        //qDebug() << QString::fromStdString(json_spirit::write(receipt));

        return QString::fromStdString(json_spirit::write(receipt));
    }
*/
    MessageTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMessageTable.size())
            return &cachedMessageTable[idx];
        else
            return 0;
    }
/*
    int64_t getTotal(std::vector<unsigned char> & vchKey) {
        int64_t total = 0;

        QList<InvoiceItemTableEntry>::iterator i;

        for (i = cachedInvoiceItemTable.begin(); i != cachedInvoiceItemTable.end(); ++i)
            if(i->vchKey == vchKey)
                total += (i->price * i->quantity);

        return total;
    }
*/
private:
    // Get the json value
/*
    const json_spirit::mValue & find_value(json_spirit::mObject & obj, const char * key)
    {
        std::string newKey = key;

        json_spirit::mObject::const_iterator i = obj.find(newKey);

        if(i != obj.end() && i->first == newKey)
            return i->second;
        else
            return json_spirit::mValue::null;
    }

    const std::string get_value(json_spirit::mObject & obj, const char * key)
    {
        json_spirit::mValue val = find_value(obj, key);

        if(val.is_null())
            return "";
        else
            return val.get_str();
    }
*/
    // Determine if it is a special message, i.e.: Invoice, Receipt, etc...
    void handleMessageEntry(const MessageTableEntry & message, const bool append)
    {
        addMessageEntry(message, append);

/*
        //json_spirit::mValue mVal;
        //json_spirit::read(message.message.toStdString(), mVal);

        if(mVal.is_null())
        {
            addMessageEntry(message, append);
            return;
        }

        //json_spirit::mObject mObj(mVal.get_obj());
        //json_spirit::mValue mvType = find_value(mObj, "type");

        if(!mvType.is_null())
        {
            std::string type = mvType.get_str();

            if (type == "invoice")
            {
                json_spirit::mArray items(find_value(mObj, "items").get_array());

                for(uint i = 0;i < items.size();i++)
                {
                    json_spirit::mObject item_obj = items[i].get_obj();

                    addInvoiceItemEntry(InvoiceItemTableEntry(message.vchKey,
                                                              message.type,
                                                              QString::fromStdString(get_value(item_obj,        "code")),
                                                              QString::fromStdString(get_value(item_obj, "description")),
                                                              find_value(item_obj, "quantity").get_int(),
                                                              find_value(item_obj,    "price").get_int64()),
                                                              //find_value(item_obj,      "tax").get_bool()),
                                        append);

                }

                addInvoiceEntry(InvoiceTableEntry(message,
                                                  QString::fromStdString(get_value(mObj, "company_info_left" )),
                                                  QString::fromStdString(get_value(mObj, "company_info_right")),
                                                  QString::fromStdString(get_value(mObj, "billing_info_left" )),
                                                  QString::fromStdString(get_value(mObj, "billing_info_right")),
                                                  QString::fromStdString(get_value(mObj, "footer"            )),
                                                  QString::fromStdString(get_value(mObj, "invoice_number"    )),
                                                  QDate::fromString(QString::fromStdString(get_value(mObj, "due_date")))),
                                append);

                // DEBUG std::string str = json_spirit::write(mVal);
                // DEBUG qDebug() << "invoice" << str.c_str();
            }
            else if (type == "receipt")
            {

                addReceiptEntry(ReceiptTableEntry(message,
                                                  QString::fromStdString(get_value(mObj, "invoice_number")),
                                                  find_value(mObj, "amount").get_int64()),
                                append);
                // DEBUG std::string str = json_spirit::write(mVal);
                // DEBUG qDebug() << "receipt" << str.c_str();
            }
            else

                addMessageEntry(message, append);
        }
        else
        {
            addMessageEntry(message, append);
            // DEBUG std::string str = json_spirit::write(mVal);
            // DEBUG qDebug() << "str" << str.c_str();
        }*/
    }

    void addMessageEntry(const MessageTableEntry & message, const bool & append)
    {
        if(append)
        {
            cachedMessageTable.append(message);
        } else
        {
            int index = qLowerBound(cachedMessageTable.begin(), cachedMessageTable.end(), message.received_datetime, MessageTableEntryLessThan()) - cachedMessageTable.begin();
            parent->beginInsertRows(QModelIndex(), index, index);
            cachedMessageTable.insert(
                        index,
                        message);
            parent->endInsertRows();
        }
    }
/*
    void addInvoiceEntry(const InvoiceTableEntry & invoice, const bool append)
    {
        if(append) cachedInvoiceTable.append(invoice);
        else
        {
            int index = qLowerBound(cachedInvoiceTable.begin(), cachedInvoiceTable.end(), invoice.received_datetime, InvoiceTableEntryLessThan()) - cachedInvoiceTable.begin();
            parent->getInvoiceTableModel()->beginInsertRows(QModelIndex(), index, index);
            cachedInvoiceTable.insert(
                        index,
                        invoice);
            parent->getInvoiceTableModel()->endInsertRows();
        }
    }

    void addInvoiceItemEntry(const InvoiceItemTableEntry & item, const bool append)
    {
        if(append) cachedInvoiceItemTable.append(item);
        else
        {
            int index = cachedInvoiceItemTable.size();
            parent->getInvoiceTableModel()->getInvoiceItemTableModel()->beginInsertRows(QModelIndex(), index, index);
            cachedInvoiceItemTable.insert(index, item);
            parent->getInvoiceTableModel()->getInvoiceItemTableModel()->endInsertRows();
        }
    }

    void addReceiptEntry(const ReceiptTableEntry & receipt, const bool append)
    {
        if(append) cachedReceiptTable.append(receipt);
        else
        {
            int index = qLowerBound(cachedReceiptTable.begin(), cachedReceiptTable.end(), receipt.received_datetime, ReceiptTableEntryLessThan()) - cachedReceiptTable.begin();
            parent->getReceiptTableModel()->beginInsertRows(QModelIndex(), index, index);
            cachedReceiptTable.insert(index, receipt);
            parent->getReceiptTableModel()->endInsertRows();
        }
    }
*/
};

MessageModel::MessageModel(WalletModel *walletModel, QObject *parent) :
    QAbstractTableModel(parent), walletModel(walletModel), optionsModel(0), priv(0)
{
    columns << tr("Type") << tr("Sent Date Time") << tr("Received Date Time") << tr("Label") << tr("To Address") << tr("From Address") << tr("Message");

    optionsModel = walletModel->getOptionsModel();

    priv = new MessageTablePriv(this);
    priv->refreshMessageTable();
    //invoiceTableModel = new InvoiceTableModel(priv, parent);
    //receiptTableModel = new ReceiptTableModel(priv, parent);

    subscribeToCoreSignals();
}

MessageModel::~MessageModel()
{
    delete priv;
    //delete invoiceTableModel;
    //delete receiptTableModel;
    unsubscribeFromCoreSignals();
}

bool MessageModel::getAddressOrPubkey(QString &address, QString &pubkey) const
{
	
	CTxDestination dest = DecodeDestination(address.toStdString());
	if (IsValidDestination(dest)){

		const PKHash *pkhash = boost::get<PKHash>(&dest);
		if (!pkhash) {
			return false;
		}

        CKeyID destinationAddress(*pkhash);		
		//CKeyID destinationAddress = GetKeyForDestination(dest);
		if (destinationAddress.IsNull())
			return false;

		CPubKey destinationKey;
		if (walletModel->getPubKey(destinationAddress, destinationKey))
			return false;
		if (!destinationKey.IsValid() || !destinationKey.IsCompressed()) {
			return false;
		};

        if (SecureMsgGetStoredKey(destinationAddress, destinationKey) != 0 && SecureMsgGetLocalKey(destinationAddress, destinationKey) != 0) // test if it's a local key
            return false;

        address = EncodeDestination(dest).c_str();
        pubkey = QString::fromStdString(EncodeBase58(&destinationKey[0], destinationKey.end()));

        return true;

    }
    return false;
}

WalletModel *MessageModel::getWalletModel()
{
    return walletModel;
}

OptionsModel *MessageModel::getOptionsModel()
{
    return optionsModel;
}
/*
InvoiceTableModel *MessageModel::getInvoiceTableModel()
{
    return invoiceTableModel;
}

ReceiptTableModel *MessageModel::getReceiptTableModel()
{
    return receiptTableModel;
}
*/
MessageModel::StatusCode MessageModel::sendMessages(const QList<SendMessagesRecipient> &recipients, const QString &addressFrom)
{

    QSet<QString> setAddress;

    if(recipients.empty())
        return OK;

    // Pre-check input data for validity
    for(const SendMessagesRecipient &rcp: recipients)
    {
        if(!walletModel->validateAddress(rcp.address))
            return InvalidAddress;

        if(rcp.message == "")
            return MessageCreationFailed;

        std::string sendTo  = rcp.address.toStdString();
        std::string pubkey  = rcp.pubkey.toStdString();
        std::string message = rcp.message.toStdString();
        std::string addFrom = addressFrom.toStdString();

        SecureMsgAddAddress(sendTo, pubkey);
        setAddress.insert(rcp.address);

        std::string sError;
        if (SecureMsgSend(addFrom, sendTo, message, sError) != 0)
        {
            QMessageBox::warning(NULL, tr("Send Secure Message"),
                tr("Send failed: %1.").arg(sError.c_str()),
                QMessageBox::Ok, QMessageBox::Ok);

            return FailedErrorShown;
        };

        // Add addresses / update labels that we've sent to to the address book
        std::string strAddress = rcp.address.toStdString();
        CTxDestination dest = DecodeDestination(strAddress);
        std::string strLabel = rcp.label.toStdString();
        {
            /*
            LOCK(wallet->cs_wallet);

            std::map<CTxDestination, std::string>::iterator mi = wallet->mapAddressBook.find(dest);

             Check if we have a new address or an updated label
            if (mi == wallet->mapAddressBook.end() || mi->second != strLabel)
            {
                wallet->SetAddressBookName(dest, strLabel);
            }*/
        }
    }

    if(recipients.size() > setAddress.size())
        return DuplicateAddress;

    return OK;
}

MessageModel::StatusCode MessageModel::sendMessages(const QList<SendMessagesRecipient> &recipients)
{
    return sendMessages(recipients, "anon");
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->cachedMessageTable.size();
}

int MessageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    MessageTableEntry *rec = static_cast<MessageTableEntry*>(index.internalPointer());

    switch(role)
    {

    case Qt::DisplayRole:
        switch(index.column())
        {
            case Label:	           return (rec->label.isEmpty() ? tr("(no label)") : rec->label);
            case ToAddress:	       return rec->to_address;
            case FromAddress:      return rec->from_address;
            case SentDateTime:     return rec->sent_datetime;
            case ReceivedDateTime: return rec->received_datetime;
            case Message:          return rec->message;
            case Read:             return rec->read;
            case TypeInt:          return rec->type;
            case HTML:             return rec->received_datetime.toString() + "<br>"  + (rec->label.isEmpty() ? rec->from_address : rec->label)  + "<br>" + rec->message;
            case Type:
                switch(rec->type)
                {
                    case MessageTableEntry::Sent:     return Sent;
                    case MessageTableEntry::Received: return Received;
                    default: break;
                }
            case Key: return (rec->type == MessageTableEntry::Sent ? Sent : Received) + rec->from_address + QString::number(rec->sent_datetime.toTime_t());
        }
        break;

    case Qt::EditRole:
        if(index.column() == Key)
            return (rec->type == MessageTableEntry::Sent ? Sent : Received) + rec->from_address + QString::number(rec->sent_datetime.toTime_t());
        break;

    case KeyRole:           return QVariant::fromValue(rec->vchKey);
    case TypeRole:          return rec->type;
    case SentDateRole:      return rec->sent_datetime;
    case ReceivedDateRole:  return rec->received_datetime;
    case FromAddressRole:   return rec->from_address;
    case ToAddressRole:     return rec->to_address;
    case FilterAddressRole: return (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address);
    case LabelRole:         return rec->label;
    case MessageRole:       return rec->message;
    case ShortMessageRole:  return rec->message; // TODO: Short message
    case ReadRole:          return rec->read;
    case HTMLRole:          return rec->received_datetime.toString() + "<br>"  + (rec->label.isEmpty() ? rec->from_address : rec->label)  + "<br>" + rec->message;
    case Ambiguous:
        int it;

        for (it = 0; it<ambiguous.length(); it++) {
            if(ambiguous[it] == (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address))
                return false;
        }
        QString address = (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address);
        ambiguous.append(address);

        return "true";
        break;
    }

    return QVariant();
}

QVariant MessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole ? columns[section] : QVariant());
}

Qt::ItemFlags MessageModel::flags(const QModelIndex & index) const
{
    if(index.isValid())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return 0;
}

QModelIndex MessageModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    MessageTableEntry *data = priv->index(row);
    return (data ? createIndex(row, column, priv->index(row)) : QModelIndex());
}

bool MessageModel::removeRows(int row, int count, const QModelIndex & parent)
{
    MessageTableEntry *rec = priv->index(row);
    if(count != 1 || !rec)
        // Can only remove one row at a time, and cannot remove rows not in model.
        return false;

    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSmsg;

        if (!dbSmsg.Open("cr+"))
            //throw runtime_error("Could not open DB.");
            return false;

       dbSmsg.EraseSmesg(&rec->vchKey[0]);
    }

    beginRemoveRows(parent, row, row);
    priv->cachedMessageTable.removeAt(row);
    endRemoveRows();

    return true;
}

void MessageModel::newMessage(const SecMsgStored &smsgInbox)
{
    priv->newMessage(smsgInbox);
}

void MessageModel::newOutboxMessage(const SecMsgStored &smsgOutbox)
{
    priv->newOutboxMessage(smsgOutbox);
}

void MessageModel::setEncryptionStatus(int status)
{
     // We're only interested in NotifySecMsgWalletUnlocked when unlocked, as its called after new messagse are processed
    if(status == WalletModel::Unlocked && QObject::sender()!=NULL)
        return;

    priv->refreshMessageTable();

    //reset(); // reload table view
}

void MessageModel::walletUnlocked()
{
    priv->walletUnlocked();
}

bool MessageModel::markMessageAsRead(const QString &key) const
{
    return priv->markAsRead(lookupMessage(key));
}

int MessageModel::lookupMessage(const QString &key) const
{
    QModelIndexList lst = match(index(0, Key, QModelIndex()), Qt::EditRole, key, 1, Qt::MatchExactly);
    if(lst.isEmpty())
        return -1;
    else
        return lst.at(0).row();
}

static void NotifySecMsgInbox(MessageModel *messageModel, SecMsgStored inboxHdr)
{
    // Too noisy: OutputDebugStringF("NotifySecMsgInboxChanged %s\n", message);
    QMetaObject::invokeMethod(messageModel, "newMessage", Qt::QueuedConnection,
                              Q_ARG(SecMsgStored, inboxHdr));
}

static void NotifySecMsgOutbox(MessageModel *messageModel, SecMsgStored outboxHdr)
{
    QMetaObject::invokeMethod(messageModel, "newOutboxMessage", Qt::QueuedConnection,
                              Q_ARG(SecMsgStored, outboxHdr));
}

static void NotifySecMsgWallet(MessageModel *messageModel)
{
    QMetaObject::invokeMethod(messageModel, "walletUnlocked", Qt::QueuedConnection);
}

void MessageModel::subscribeToCoreSignals()
{
    qRegisterMetaType<SecMsgStored>("SecMsgStored");

    // Connect signals
    NotifySecMsgInboxChanged.connect(boost::bind(NotifySecMsgInbox, this, _1));
    NotifySecMsgOutboxChanged.connect(boost::bind(NotifySecMsgOutbox, this, _1));
    NotifySecMsgWalletUnlocked.connect(boost::bind(NotifySecMsgWallet, this));

    connect(walletModel, SIGNAL(encryptionStatusChanged()), this, SLOT(setEncryptionStatus()));
}

void MessageModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals
    NotifySecMsgInboxChanged.disconnect(boost::bind(NotifySecMsgInbox, this, _1));
    NotifySecMsgOutboxChanged.disconnect(boost::bind(NotifySecMsgOutbox, this, _1));
    NotifySecMsgWalletUnlocked.disconnect(boost::bind(NotifySecMsgWallet, this));

    disconnect(walletModel, SIGNAL(encryptionStatusChanged()), this, SLOT(setEncryptionStatus()));
}

/*

// InvoiceTableModel
InvoiceTableModel::InvoiceTableModel(MessageTablePriv *priv, QObject *parent) :
    QAbstractTableModel(parent), priv(priv), invoiceItemTableModel(0)
{
    columns << tr("Type") << tr("Sent Date Time") << tr("Recieved Date Time") << tr("Label") << tr("To Address") << tr("From Address") << tr("Invoice No.") << tr("Due Date") << "Invoice Amount" << "Amount Paid" << "Amount Outstanding";

    invoiceItemTableModel = new InvoiceItemTableModel(priv, parent);
}

InvoiceTableModel::~InvoiceTableModel()
{
    delete invoiceItemTableModel;
}

MessageModel *InvoiceTableModel::getMessageModel()
{
    return priv->parent;
}

InvoiceItemTableModel *InvoiceTableModel::getInvoiceItemTableModel()
{
    return invoiceItemTableModel;
}

void InvoiceTableModel::newInvoice(QString CompanyInfoLeft,
                                   QString CompanyInfoRight,
                                   QString BillingInfoLeft,
                                   QString BillingInfoRight,
                                   QString Footer,
                                   QDate   DueDate,
                                   QString InvoiceNumber)
{
    InvoiceTableEntry invoice(true);

    invoice.company_info_left  = CompanyInfoLeft;
    invoice.company_info_right = CompanyInfoRight;
    invoice.billing_info_left  = BillingInfoLeft;
    invoice.billing_info_right = BillingInfoRight;
    invoice.footer             = Footer;
    invoice.due_date           = DueDate;
    invoice.invoice_number     = InvoiceNumber;

    priv->newInvoice(invoice);
}

void InvoiceTableModel::newInvoiceItem()
{
    priv->newInvoiceItem();
}

void InvoiceTableModel::newReceipt(QString InvoiceNumber, CAmount  ReceiptAmount)
{
    ReceiptTableEntry receipt(true);
    receipt.invoice_number = InvoiceNumber;
    receipt.amount         = ReceiptAmount;

    priv->newReceipt(receipt);
}

QString InvoiceTableModel::getInvoiceJSON(const int row)
{
    return priv->getInvoiceJSON(row);
}

int InvoiceTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->cachedInvoiceTable.size();
}

int InvoiceTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant InvoiceTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    InvoiceTableEntry *rec;
    int64_t total;

    if(role != Qt::TextAlignmentRole)
         rec = static_cast<InvoiceTableEntry*>(index.internalPointer());

    switch(role)
    {
        case Qt::DisplayRole:
            switch(index.column())
            {
                case Label:             return (rec->label.isEmpty() ? tr("(no label)") : rec->label);
                case ToAddress:         return rec->to_address;
                case FromAddress:       return rec->from_address;
                case SentDateTime:      return rec->sent_datetime;
                case ReceivedDateTime:  return rec->received_datetime;
                case CompanyInfoLeft:   return rec->company_info_left;
                case CompanyInfoRight:  return rec->company_info_right;
                case BillingInfoLeft:   return rec->billing_info_left;
                case BillingInfoRight:  return rec->billing_info_right;
                case Footer:            return rec->footer;
                case InvoiceNumber:     return rec->invoice_number;
                case DueDate:           return rec->due_date;
                case Paid:              return 0; // TODO: Calculate Paid
                case Outstanding:       return 0; // TODO: Calculate Outstanding
                case Type:
                    switch(rec->type)
                    {
                    case MessageTableEntry::Sent:     return MessageModel::Sent;
                    case MessageTableEntry::Received: return MessageModel::Received;
                    }

                case Total:
                    total = priv->getTotal(rec->vchKey);
                    return BitoinUnits::formatWithUnit(priv->parent->getOptionsModel()->getDisplayUnit(), total);
                    break;
                default: break;
            }
            break;

        case Qt::TextAlignmentRole:
            switch(index.column())
            {
                case InvoiceNumber:
                case Total:
                case Paid:
                case Outstanding:
                    return Qt::AlignRight;
                default: break;
            }
            break;

        case Qt::UserRole:
            switch(index.column())
            {
            case Type:
                switch(rec->type)
                {
                    case MessageTableEntry::Sent:     return MessageModel::Sent;
                    case MessageTableEntry::Received: return MessageModel::Received;
                }
            default:
                return QString((char*)&rec->vchKey[0]) + QString::number(rec->type);
            }
    }

    return QVariant();
}


bool InvoiceTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid())
        return false;

    qDebug() << value << index;
    InvoiceTableEntry *rec = static_cast<InvoiceTableEntry*>(index.internalPointer());

    //editStatus = OK;

    if(role == Qt::EditRole)
    {
        qDebug() << value << index;

        switch(index.column())
        {
            case InvoiceNumber:    rec->invoice_number     = value.toString(); break;
            case DueDate:          rec->due_date           = value.toDate(); break;
            case CompanyInfoLeft:  rec->company_info_left  = value.toString(); break;
            case CompanyInfoRight: rec->company_info_right = value.toString(); break;
            case BillingInfoLeft:  rec->billing_info_left  = value.toString(); break;
            case BillingInfoRight: rec->billing_info_right = value.toString(); break;
            case Footer:           rec->footer             = value.toString(); break;
        }

        return true;
    }
    return false;
}

void InvoiceTableModel::setData(const int row, const int col, const QVariant & value)
{
    InvoiceTableEntry rec = priv->cachedInvoiceTable.at(row);

    qDebug() << value << col;

    switch(col)
    {
        case InvoiceNumber:    rec.invoice_number     = value.toString(); break;
        case DueDate:          rec.due_date           = value.toDate();   break;
        case CompanyInfoLeft:  rec.company_info_left  = value.toString(); break;
        case CompanyInfoRight: rec.company_info_right = value.toString(); break;
        case BillingInfoLeft:  rec.billing_info_left  = value.toString(); break;
        case BillingInfoRight: rec.billing_info_right = value.toString(); break;
        case Footer:           rec.footer             = value.toString(); break;
    }
}


QVariant InvoiceTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole ? columns[section] : QVariant());
}

Qt::ItemFlags InvoiceTableModel::flags(const QModelIndex & index) const
{
    if(index.isValid())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return 0;
}

QModelIndex InvoiceTableModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return (row >= 0 && row < priv->cachedInvoiceTable.size() ? createIndex(row, column, &priv->cachedInvoiceTable[row]) : QModelIndex());
}

bool InvoiceTableModel::removeRows(int row, int count, const QModelIndex & parent)
{

    if(count != 1 || !(row >= 0 && row < priv->cachedInvoiceTable.size()))
        // Can only remove one row at a time, and cannot remove rows not in model.
        // Also refuse to remove receiving addresses.
        return false;

    InvoiceTableEntry rec = priv->cachedInvoiceTable.at(row);

    for (int i = 0; i < priv->cachedInvoiceItemTable.size(); i++)
        if(priv->cachedInvoiceItemTable[i].vchKey == rec.vchKey && priv->cachedInvoiceItemTable[i].type == rec.type)
            invoiceItemTableModel->removeRow(i);

    if(rec.type == MessageTableEntry::Received)
    {

        LOCK(cs_smsgDB);
        SecMsgDB dbSmsg;

        dbSmsg.EraseSmesg(&rec.vchKey[0]);

    } else
    if(rec.type == MessageTableEntry::Sent)
    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSmsg;

        dbSmsg.EraseSmesg(&rec.vchKey[0]);
    }

    beginRemoveRows(parent, row, row);
    priv->cachedInvoiceTable.removeAt(row);
    endRemoveRows();

    return true;
}


// InvoiceItemTableModel
InvoiceItemTableModel::InvoiceItemTableModel(MessageTablePriv *priv, QObject *parent) :
    QAbstractTableModel(parent), priv(priv)
{
    columns << tr("Type") << tr("Code") << tr("Description") << tr("Quantity") << tr("Price")  << tr("Amount");

}

InvoiceItemTableModel::~InvoiceItemTableModel()
{

}

int InvoiceItemTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->cachedInvoiceItemTable.size();
}

int InvoiceItemTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant InvoiceItemTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    InvoiceItemTableEntry *rec;

    if(role != Qt::TextAlignmentRole)
         rec = static_cast<InvoiceItemTableEntry*>(index.internalPointer());

    switch(role)
    {
        case Qt::DisplayRole:
            switch(index.column())
            {
                case Code:
                     return rec->code;
                case Description:
                    return rec->description;
                case Quantity:
                    return rec->quantity;
                //case Tax:
                //    return rec->tax;
                case Price:
                    return BitcointalkcoinUnits::formatWithUnit(priv->parent->getOptionsModel()->getDisplayUnit(), rec->price);
                case Amount:
                    return BitcointalkcoinUnits::formatWithUnit(priv->parent->getOptionsModel()->getDisplayUnit(), (rec->quantity * rec->price));
                case Type:
                    switch(rec->type)
                    {
                        case MessageTableEntry::Sent:     return MessageModel::Sent;
                        case MessageTableEntry::Received: return MessageModel::Received;
                    }
            }
            break;

        case Qt::EditRole:
            switch(index.column())
            {
                case Code:
                     return rec->code;
                case Description:
                    return rec->description;
                case Quantity:
                    return rec->quantity;
                //case Tax:
                //    return rec->tax;
                case Price:
                    return BitcointalkcoinUnits::format(priv->parent->getOptionsModel()->getDisplayUnit(), rec->price);

                case Amount:
                    return (qint64)rec->quantity * rec->price;
            }
            break;

        case Qt::TextAlignmentRole:
            switch(index.column())
            {
                case Quantity:
                //case Tax:
                case Price:
                case Amount:
                    return Qt::AlignRight;
                default: break;
            }
            break;

        case Qt::UserRole:
            return QString((char*)&rec->vchKey[0]) + QString::number(rec->type);
    }

    return QVariant();
}

bool InvoiceItemTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid())
        return false;

    InvoiceItemTableEntry *rec = static_cast<InvoiceItemTableEntry*>(index.internalPointer());

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
            case Code:        rec->code        = value.toString(); break;
            case Description: rec->description = value.toString(); break;
            case Quantity:
                rec->quantity    = value.toInt();
                emitDataChanged(index.row());
                break;
            case Price:
                BitcointalkcoinUnits::parse(priv->parent->getOptionsModel()->getDisplayUnit(), value.toString(), &rec->price);
                emitDataChanged(index.row());
                break;
        }

        return true;
    }
    return false;
}

QVariant InvoiceItemTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole ? columns[section] : QVariant());
}

Qt::ItemFlags InvoiceItemTableModel::flags(const QModelIndex & index) const
{
    if(index.isValid())
    {
        Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if(index.column() != Amount)
            retval |= Qt::ItemIsEditable;

        return retval;
    }

    return 0;
}

QModelIndex InvoiceItemTableModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return (row >= 0 && row < priv->cachedInvoiceItemTable.size() ? createIndex(row, column, &priv->cachedInvoiceItemTable[row]) : QModelIndex());
}

bool InvoiceItemTableModel::removeRows(int row, int count, const QModelIndex & parent)
{

    if(count != 1 || !(row >= 0 && row < priv->cachedInvoiceItemTable.size()))
        // Can only remove one row at a time, and cannot remove rows not in model.
        // Also refuse to remove receiving addresses.
        return false;

    beginRemoveRows(parent, row, row);
    priv->cachedInvoiceItemTable.removeAt(row);
    endRemoveRows();

    return true;
}

void InvoiceItemTableModel::emitDataChanged(const int idx)
{
    emit dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}



// ReceiptTableModel
ReceiptTableModel::ReceiptTableModel(MessageTablePriv *priv, QObject *parent) :
    QAbstractTableModel(parent), priv(priv)
{
    columns << tr("Type") << tr("Sent Date Time") << tr("Recieved Date Time") << tr("Label") << tr("To Address") << tr("From Address") << tr("Invoice No.") << "Receipt Amount" << "Amount Outstanding";
}

ReceiptTableModel::~ReceiptTableModel()
{

}

QString ReceiptTableModel::getReceiptJSON(const int row)
{
    return priv->getReceiptJSON(row);
}

int ReceiptTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->cachedReceiptTable.size();
}

int ReceiptTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant ReceiptTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    ReceiptTableEntry *rec;

    if(role != Qt::TextAlignmentRole)
         rec = static_cast<ReceiptTableEntry*>(index.internalPointer());

    switch(role)
    {
        case Qt::DisplayRole:
            switch(index.column())
            {
                case Label:             return (rec->label.isEmpty() ? tr("(no label)") : rec->label);
                case ToAddress:         return rec->to_address;
                case FromAddress:       return rec->from_address;
                case SentDateTime:      return rec->sent_datetime;
                case ReceivedDateTime:  return rec->received_datetime;
                case InvoiceNumber:     return rec->invoice_number;
                case Amount:            return BitcointalkcoinUnits::formatWithUnit(priv->parent->getOptionsModel()->getDisplayUnit(), rec->amount);
                case Type:
                    switch(rec->type)
                    {
                    case MessageTableEntry::Sent:     return MessageModel::Sent;
                    case MessageTableEntry::Received: return MessageModel::Received;
                    }
                default: break;
            }
            break;

        case Qt::TextAlignmentRole:
            switch(index.column())
            {
                case InvoiceNumber:
                case Amount:
                    return Qt::AlignRight;
                default: break;
            }
            break;

        case Qt::UserRole:
            switch(index.column())
            {
            case Type:
                switch(rec->type)
                {
                    case MessageTableEntry::Sent:     return MessageModel::Sent;
                    case MessageTableEntry::Received: return MessageModel::Received;
                }
            default:
                return QString((char*)&rec->vchKey[0]) + QString::number(rec->type);
            }
    }

    return QVariant();
}


bool ReceiptTableModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if(!index.isValid())
        return false;

    qDebug() << value << index;
    InvoiceTableEntry *rec = static_cast<InvoiceTableEntry*>(index.internalPointer());

    //editStatus = OK;

    if(role == Qt::EditRole)
    {
        qDebug() << value << index;

        switch(index.column())
        {
            case InvoiceNumber:    rec->invoice_number     = value.toString(); break;
            case DueDate:          rec->due_date           = value.toDate(); break;
            case CompanyInfoLeft:  rec->company_info_left  = value.toString(); break;
            case CompanyInfoRight: rec->company_info_right = value.toString(); break;
            case BillingInfoLeft:  rec->billing_info_left  = value.toString(); break;
            case BillingInfoRight: rec->billing_info_right = value.toString(); break;
            case Footer:           rec->footer             = value.toString(); break;
        }

        return true;
    }
    return false;
}

void ReceiptTableModel::setData(const int row, const int col, const QVariant & value)
{
    InvoiceTableEntry rec = priv->cachedInvoiceTable.at(row);

    qDebug() << value << col;

    switch(col)
    {
        case InvoiceNumber:    rec.invoice_number     = value.toString(); break;
        case DueDate:          rec.due_date           = value.toDate();   break;
        case CompanyInfoLeft:  rec.company_info_left  = value.toString(); break;
        case CompanyInfoRight: rec.company_info_right = value.toString(); break;
        case BillingInfoLeft:  rec.billing_info_left  = value.toString(); break;
        case BillingInfoRight: rec.billing_info_right = value.toString(); break;
        case Footer:           rec.footer             = value.toString(); break;
    }
}


QVariant ReceiptTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole ? columns[section] : QVariant());
}

Qt::ItemFlags ReceiptTableModel::flags(const QModelIndex & index) const
{
    if(index.isValid())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return 0;
}

QModelIndex ReceiptTableModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return (row >= 0 && row < priv->cachedReceiptTable.size() ? createIndex(row, column, &priv->cachedReceiptTable[row]) : QModelIndex());
}

bool ReceiptTableModel::removeRows(int row, int count, const QModelIndex & parent)
{

    if(count != 1 || !(row >= 0 && row < priv->cachedReceiptTable.size()))
        // Can only remove one row at a time, and cannot remove rows not in model.
        return false;

    ReceiptTableEntry rec = priv->cachedReceiptTable.at(row);

    if(rec.type == MessageTableEntry::Received)
    {

        LOCK(cs_smsgDB);
        SecMsgDB dbSmsg;

        dbSmsg.EraseSmesg(&rec.vchKey[0]);

    } else
    if(rec.type == MessageTableEntry::Sent)
    {
        LOCK(cs_smsgDB);
        SecMsgDB dbSmsg;

        dbSmsg.EraseSmesg(&rec.vchKey[0]);
    }

    beginRemoveRows(parent, row, row);
    priv->cachedReceiptTable.removeAt(row);
    endRemoveRows();

    return true;
}
*/
