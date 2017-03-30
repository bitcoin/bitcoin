
#include "messagetablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet/wallet.h"
#include "base58.h"

#include <QFont>
#include <QDateTime>
#include <QSettings>
#include "rpc/server.h"
#include "clientmodel.h"
using namespace std;



extern CRPCTable tableRPC;
struct MessageTableEntry
{

    QString guid;
    QString time;
	int itime;
	QString subject;
	QString message;
	QString from;
	QString to;

    MessageTableEntry() {}
    MessageTableEntry(const QString &guid, const int itime, const QString &time,const QString &from, const QString &to, const QString &subject, const QString &message):
        guid(guid), itime(itime), time(time), from(from), to(to), subject(subject), message(message) {}
};

struct MessageTableEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const
    {
        return a.guid < b.guid;
    }
    bool operator()(const MessageTableEntry &a, const QString &b) const
    {
        return a.guid < b;
    }
    bool operator()(const QString &a, const MessageTableEntry &b) const
    {
        return a < b.guid;
    }
};

struct MessageEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const
    {
        return a.itime < b.itime;
    }
    bool operator()(const MessageTableEntry &a, const int &b) const
    {
        return a.itime > b;
    }
    bool operator()(const int &a, const MessageTableEntry &b) const
    {
        return a < b.itime;
    }
};
// Private implementation
class MessageTablePriv
{
public:
    CWallet *wallet;
    QList<MessageTableEntry> cachedMessageTable;
    MessageTableModel *parent;

    MessageTablePriv(CWallet *wallet, MessageTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshMessageTable(MessageModelType type)
    {
        cachedMessageTable.clear();
        {
			string strMethod;
			UniValue params(UniValue::VARR); 
			if(type == OutMessage)
				strMethod = string("messagesentlist");
			else if(type == InMessage)
				strMethod = string("messagereceivelist");
			UniValue result ;
			string guid_str;
			string time_str;
			string subject_str;
			string message_str;
			string from_str;
			string to_str;
			int unixTime;
			QDateTime dateTime;	
			try {
				result = tableRPC.execute(strMethod, params);
				if (result.type() == UniValue::VARR)
				{
					guid_str = "";
					time_str = "";
					subject_str = "";
					message_str = "";
					from_str = "";
					to_str = "";

					const UniValue &arr = result.get_array();
				    for (unsigned int idx = 0; idx < arr.size(); idx++) {
					    const UniValue& input = arr[idx];
						if (input.type() != UniValue::VOBJ)
							continue;
						const UniValue& o = input.get_obj();
						guid_str = "";
						time_str = "";
						subject_str = "";
						message_str = "";
						from_str = "";
						to_str = "";
						
						const UniValue& guid_value = find_value(o, "GUID");
						if (guid_value.type() == UniValue::VSTR)
							guid_str = guid_value.get_str();
						const UniValue& from_value = find_value(o, "from");
						if (from_value.type() == UniValue::VSTR)
							from_str = from_value.get_str();
						const UniValue& to_value = find_value(o, "to");
						if (to_value.type() == UniValue::VSTR)
							to_str = to_value.get_str();
						const UniValue& time_value = find_value(o, "time");
						if (time_value.type() == UniValue::VSTR)
							time_str = time_value.get_str();

						unixTime = atoi(time_str.c_str());
						dateTime.setTime_t(unixTime);
						time_str = dateTime.toString().toStdString();


						const UniValue& subject_value = find_value(o, "subject");
						if (subject_value.type() == UniValue::VSTR)
							subject_str = subject_value.get_str();
						const UniValue& message_value = find_value(o, "message");
						if (message_value.type() == UniValue::VSTR)
							message_str = message_value.get_str();
				
						updateEntry(QString::fromStdString(guid_str), unixTime, QString::fromStdString(time_str), QString::fromStdString(from_str), QString::fromStdString(to_str), QString::fromStdString(subject_str), QString::fromStdString(message_str), type, CT_NEW); 
					}
				}
 			}
			catch (UniValue& objError)
			{
				return;
			}
			catch(std::exception& e)
			{
				return;
			}           
         }
    }

    void updateEntry(const QString &guid, const int itime, const QString &time, const QString &from, const QString &to, const QString &subject, const QString &message, MessageModelType type,int status)
    {
		if(!parent || parent->modelType != type)
		{
			return;
		}
        // Find message / value in model
        QList<MessageTableEntry>::iterator lowerGuid = qLowerBound(
            cachedMessageTable.begin(), cachedMessageTable.end(), guid, MessageTableEntryLessThan());
        QList<MessageTableEntry>::iterator upperGuid = qUpperBound(
            cachedMessageTable.begin(), cachedMessageTable.end(), guid, MessageTableEntryLessThan());

		QList<MessageTableEntry>::iterator lower = qLowerBound(
            cachedMessageTable.begin(), cachedMessageTable.end(), itime, MessageEntryLessThan());
        QList<MessageTableEntry>::iterator upper = qUpperBound(
            cachedMessageTable.begin(), cachedMessageTable.end(), itime, MessageEntryLessThan());
        int lowerIndex = (lower - cachedMessageTable.begin());
        int upperIndex = (upper - cachedMessageTable.begin());

        bool inModel = (lowerGuid != upperGuid);
		int index;
        switch(status)
        {
        case CT_NEW:
			index = parent->lookupMessage(guid);
            if(inModel || index != -1)
            {
                break;
            
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedMessageTable.insert(lowerIndex, MessageTableEntry(guid, itime, time, from, to, subject, message));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                break;
            }
            lower->guid = guid;
			lower->itime = itime;
			lower->time = time;
			lower->from = from;
			lower->to = to;
			lower->subject = subject;
			lower->message = message;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedMessageTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedMessageTable.size();
    }

    MessageTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMessageTable.size())
        {
            return &cachedMessageTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

MessageTableModel::MessageTableModel(CWallet *wallet, WalletModel *parent, MessageModelType type) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0), modelType(type)
{

	columns << tr("GUID") << tr("Time") << tr("From") << tr("To") << tr("Subject") << tr("Message");		 
    priv = new MessageTablePriv(wallet, this);
	refreshMessageTable();
}

MessageTableModel::~MessageTableModel()
{
    delete priv;
}
void MessageTableModel::refreshMessageTable() 
{

	clear();
	priv->refreshMessageTable(modelType);
}
int MessageTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int MessageTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant MessageTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    MessageTableEntry *rec = static_cast<MessageTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case GUID:
            return rec->guid;
        case Time:
            return rec->time;
        case From:
            return rec->from;
        case To:
            return rec->to;
        case Subject:
            return rec->subject;
        case Message:
            return rec->message;
        }
    }
    else if (role == ToRole)
    {
        return rec->to;
    }
    return QVariant();
}

bool MessageTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    MessageTableEntry *rec = static_cast<MessageTableEntry*>(index.internalPointer());

    editStatus = OK;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        
        case Time:
            // Do nothing, if old value == new value
            if(rec->time == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Subject:
            // Do nothing, if old value == new value
            if(rec->subject == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case From:
            // Do nothing, if old value == new value
            if(rec->from == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case To:
            // Do nothing, if old value == new value
            if(rec->to == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Message:
            // Do nothing, if old value == new value
            if(rec->message == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case GUID:
            // Do nothing, if old message == new message
            if(rec->guid == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate messagees to prevent accidental deletion of messagees, if you try
            // to paste an existing message over another message (with a different label)
            else if(lookupMessage(rec->guid) != -1)
            {
                editStatus = DUPLICATE_MESSAGE;
                return false;
            }
            
            break;
        }
        return true;
    }
    return false;
}

QVariant MessageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
    }
    return QVariant();
}

Qt::ItemFlags MessageTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex MessageTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    MessageTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void MessageTableModel::clear()
{
	beginResetModel();
    priv->cachedMessageTable.clear();
	endResetModel();
}


int MessageTableModel::lookupMessage(const QString &guid) const
{
    QModelIndexList lst = match(index(0, GUID, QModelIndex()),
                                Qt::EditRole, guid, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void MessageTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
