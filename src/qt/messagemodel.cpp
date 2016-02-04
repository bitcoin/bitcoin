#include "guiutil.h"
#include "guiconstants.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "messagemodel.h"
#include "addresstablemodel.h"

#include "ui_interface.h"
#include "base58.h"
#include "json_spirit.h"
#include "init.h"

#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QFont>
#include <QColor>

Q_DECLARE_METATYPE(std::vector<unsigned char>);

QList<QString> ambiguous; /**< Specifies Ambiguous addresses */

const QString MessageModel::Sent = "S";
const QString MessageModel::Received = "R";

struct MessageTableEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const {return a.received_datetime < b.received_datetime;};
    bool operator()(const MessageTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const MessageTableEntry &b) const {return a < b.received_datetime;}
};

// Private implementation
class MessageTablePriv
{
public:
    QList<MessageTableEntry> cachedMessageTable;
    MessageModel *parent;

    MessageTablePriv(MessageModel *parent):
        parent(parent) {}

    void refreshMessageTable()
    {
        cachedMessageTable.clear();

        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- messages are stored encrypted, can't load them without the private keys
            return;
        };

        {
            LOCK2(pwalletMain->cs_wallet, cs_smsgDB);

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
                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
                if (SecureMsgDecrypt(false, smsgStored.sAddrTo, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(msg.sFromAddress));

                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);

                    memcpy(&vchKey[0], chKey, 18);

                    addMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Received,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString::fromStdString(msg.sFromAddress),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      (char*)&msg.vchMessage[0]),
                                    true);
                }
            };

            delete it;

            sPrefix = "sm";
            it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, sPrefix, chKey, smsgStored))
            {
                uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
                if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
                {
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(smsgStored.sAddrTo));

                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);

                    memcpy(&vchKey[0], chKey, 18);

                    addMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Sent,
                                                      label,
                                                      QString::fromStdString(smsgStored.sAddrTo),
                                                      QString::fromStdString(msg.sFromAddress),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      (char*)&msg.vchMessage[0]),
                                    true);
                }
            };

            delete it;
        }
    }

    void newMessage(const SecMsgStored& inboxHdr)
    {
        // we have to copy it, because it doesn't like constants going into Decrypt
        SecMsgStored smsgStored = inboxHdr;
        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
        if (SecureMsgDecrypt(false, smsgStored.sAddrTo, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(msg.sFromAddress));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("im");
            SecureMessage* psmsg = (SecureMessage*) &smsgStored.vchMessage[0];

            std::vector<unsigned char> vchKey;
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &psmsg->timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            addMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Received,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              QString::fromStdString(msg.sFromAddress),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              (char*)&msg.vchMessage[0]),
                            false);
        }
    }

    void newOutboxMessage(const SecMsgStored& outboxHdr)
    {

        SecMsgStored smsgStored = outboxHdr;
        MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        uint32_t nPayload = smsgStored.vchMessage.size() - SMSG_HDR_LEN;
        if (SecureMsgDecrypt(false, smsgStored.sAddrOutbox, &smsgStored.vchMessage[0], &smsgStored.vchMessage[SMSG_HDR_LEN], nPayload, msg) == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(smsgStored.sAddrTo));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("sm");
            SecureMessage* psmsg = (SecureMessage*) &smsgStored.vchMessage[0];
            std::vector<unsigned char> vchKey;
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &psmsg->timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[SMSG_HDR_LEN], 8);    // sample

            addMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Sent,
                                              label,
                                              QString::fromStdString(smsgStored.sAddrTo),
                                              QString::fromStdString(msg.sFromAddress),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              (char*)&msg.vchMessage[0]),
                            false);
        }
    }

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
            dbSmsg.ReadSmesg(&rec->chKey[0], smsgStored);

            smsgStored.status &= ~SMSG_MASK_UNREAD;
            dbSmsg.WriteSmesg(&rec->chKey[0], smsgStored);
            rec->read = !(smsgStored.status & SMSG_MASK_UNREAD);
        }

        return true;
    };

    MessageTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMessageTable.size())
            return &cachedMessageTable[idx];
        else
            return 0;
    }

private:
    // Get the json value
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

    // Determine if it is a special message, i.e.: Invoice, Receipt, etc...
    void handleMessageEntry(const MessageTableEntry & message, const bool append)
    {
        addMessageEntry(message, append);
        json_spirit::mValue mVal;
        json_spirit::read(message.message.toStdString(), mVal);

        if(mVal.is_null())
        {
            addMessageEntry(message, append);
            return;
        }

        json_spirit::mObject mObj(mVal.get_obj());
        json_spirit::mValue mvType = find_value(mObj, "type");

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

};

MessageModel::MessageModel(CWallet *wallet, WalletModel *walletModel, QObject *parent) :
    QAbstractTableModel(parent), wallet(wallet), walletModel(walletModel), optionsModel(0), priv(0)
{
    columns << tr("Type") << tr("Sent Date Time") << tr("Received Date Time") << tr("Label") << tr("To Address") << tr("From Address") << tr("Message");

    optionsModel = walletModel->getOptionsModel();

    priv = new MessageTablePriv(this);
    priv->refreshMessageTable();

    subscribeToCoreSignals();
}

MessageModel::~MessageModel()
{
    delete priv;
    unsubscribeFromCoreSignals();
}

WalletModel *MessageModel::getWalletModel()
{
    return walletModel;
}

OptionsModel *MessageModel::getOptionsModel()
{
    return optionsModel;
}

MessageModel::StatusCode MessageModel::sendMessage(const QString &address, const QString &message, const QString &addressFrom)
{
    if(!walletModel->validateAddress(address))
        return InvalidAddress;

    if(message == "")
        return MessageCreationFailed;

    std::string sendTo = address.toStdString();
    std::string msg    = message.toStdString();
    std::string from   = addressFrom.toStdString();

    std::string sError;
    if (SecureMsgSend(from, sendTo, msg, sError) != 0)
    {
        QMessageBox::warning(NULL, tr("Send Secure Message"),
            tr("Send failed: %1.").arg(sError.c_str()),
            QMessageBox::Ok, QMessageBox::Ok);

        return FailedErrorShown;
    };

    return OK;
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

    case KeyRole:           return QVariant::fromValue(rec->chKey);
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

        dbSmsg.EraseSmesg(&rec->chKey[0]);
    }

    beginRemoveRows(parent, row, row);
    priv->cachedMessageTable.removeAt(row);
    endRemoveRows();

    return true;
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    return priv->cachedMessageTable.length();
}

int MessageModel::columnCount(const QModelIndex &parent) const
{
    return columns.length();
}

void MessageModel::resetFilter()
{
    ambiguous.clear();
}

void MessageModel::newMessage(const SecMsgStored &smsg)
{
    priv->newMessage(smsg);
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

    reset(); // reload table view
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
    // Too noisy: LogPrintf("NotifySecMsgInboxChanged %s\n", message);
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
    messageModel->setEncryptionStatus(WalletModel::Unlocked);
}

void MessageModel::subscribeToCoreSignals()
{
    qRegisterMetaType<SecMsgStored>("SecMsgStored");

    // Connect signals
    NotifySecMsgInboxChanged.connect(boost::bind(NotifySecMsgInbox, this, _1));
    NotifySecMsgOutboxChanged.connect(boost::bind(NotifySecMsgOutbox, this, _1));
    NotifySecMsgWalletUnlocked.connect(boost::bind(NotifySecMsgWallet, this));

    connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));
}

void MessageModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals
    NotifySecMsgInboxChanged.disconnect(boost::bind(NotifySecMsgInbox, this, _1));
    NotifySecMsgOutboxChanged.disconnect(boost::bind(NotifySecMsgOutbox, this, _1));
    NotifySecMsgWalletUnlocked.disconnect(boost::bind(NotifySecMsgWallet, this));

    disconnect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));
}

