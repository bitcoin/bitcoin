#include "certtablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet/wallet.h"
#include "base58.h"
#include "rpcserver.h"
#include <QFont>
using namespace std;


const QString CertTableModel::Cert = "C";


extern const CRPCTable tableRPC;
struct CertTableEntry
{
    enum Type {
        Cert
    };

    Type type;
    QString title;
    QString cert;
	QString data;
	QString expires_on;
	QString expires_in;
	QString expired;
	QString privatecert;
	QString alias;

    CertTableEntry() {}
    CertTableEntry(Type type, const QString &title, const QString &cert, const QString &data, const QString &expires_on,const QString &expires_in, const QString &expired,const QString &privatecert, const QString &alias):
        type(type), title(title), cert(cert), data(data), expires_on(expires_on), expires_in(expires_in), expired(expired),privatecert(privatecert), alias(alias) {}
};

struct CertTableEntryLessThan
{
    bool operator()(const CertTableEntry &a, const CertTableEntry &b) const
    {
        return a.cert < b.cert;
    }
    bool operator()(const CertTableEntry &a, const QString &b) const
    {
        return a.cert < b;
    }
    bool operator()(const QString &a, const CertTableEntry &b) const
    {
        return a < b.cert;
    }
};


// Private implementation
class CertTablePriv
{
public:
    CWallet *wallet;
    QList<CertTableEntry> cachedCertTable;
    CertTableModel *parent;

    CertTablePriv(CWallet *wallet, CertTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshCertTable(CertModelType type)
    {
        cachedCertTable.clear();
        {
			string strMethod = string("certlist");
	        UniValue params(UniValue::VARR); 
			UniValue result;
			string name_str;
			string data_str;
			string value_str;
			string expires_in_str;
			string expires_on_str;
			string expired_str;
			string private_str;
			string alias_str;
			int expired = 0;
			int expires_in = 0;
			int expires_on = 0;
			

			try {
				result = tableRPC.execute(strMethod, params);
				if (result.type() == UniValue::VARR)
				{
					name_str = "";
					value_str = "";
					expires_in_str = "";
					private_str = "";
					expires_on_str = "";
					alias_str = "";
					data_str = "";
					expired = 0;
					expires_in = 0;
					expires_on = 0;

			
					const UniValue &arr = result.get_array();
				    for (unsigned int idx = 0; idx < arr.size(); idx++) {
					    const UniValue& input = arr[idx];
						if (input.type() != UniValue::VOBJ)
							continue;
						const UniValue& o = input.get_obj();
						name_str = "";
						value_str = "";
						data_str = "";
						private_str = "";
						expires_in_str = "";
						expires_on_str = "";
						alias_str = "";
						expired = 0;
						expires_in = 0;
						expires_on = 0;

				
						const UniValue& name_value = find_value(o, "cert");
						if (name_value.type() == UniValue::VSTR)
							name_str = name_value.get_str();
						const UniValue& value_value = find_value(o, "title");
						if (value_value.type() == UniValue::VSTR)
							value_str = value_value.get_str();
						const UniValue& data_value = find_value(o, "data");
						if (data_value.type() == UniValue::VSTR)
							data_str = data_value.get_str();
						const UniValue& private_value = find_value(o, "private");
						if (private_value.type() == UniValue::VSTR)
							private_str = private_value.get_str();
						const UniValue& alias_value = find_value(o, "alias");
						if (alias_value.type() == UniValue::VSTR)
							alias_str = alias_value.get_str();
						const UniValue& expires_on_value = find_value(o, "expires_on");
						if (expires_on_value.type() == UniValue::VNUM)
							expires_on = expires_on_value.get_int();
						const UniValue& expires_in_value = find_value(o, "expires_in");
						if (expires_in_value.type() == UniValue::VNUM)
							expires_in = expires_in_value.get_int();
						const UniValue& expired_value = find_value(o, "expired");
						if (expired_value.type() == UniValue::VNUM)
							expired = expired_value.get_int();
						const UniValue& pending_value = find_value(o, "pending");
						int pending = 0;
						if (pending_value.type() == UniValue::VNUM)
							pending = pending_value.get_int();

						if(pending == 1)
						{
							expired_str = "Pending";
						}
						else if(expired == 1)
						{
							expired_str = "Expired";
						}
						else
						{
							expired_str = "Valid";
						}
						expires_in_str = strprintf("%d Blocks", expires_in);
						expires_on_str = strprintf("Block %d", expires_on);

						updateEntry(QString::fromStdString(name_str), QString::fromStdString(value_str), QString::fromStdString(data_str), QString::fromStdString(expires_on_str), QString::fromStdString(expires_in_str), QString::fromStdString(expired_str),QString::fromStdString(private_str),QString::fromStdString(alias_str),type, CT_NEW); 
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
        
        // qLowerBound() and qUpperBound() require our cachedCertTable list to be sorted in asc order
        qSort(cachedCertTable.begin(), cachedCertTable.end(), CertTableEntryLessThan());
    }

    void updateEntry(const QString &cert, const QString &title, const QString &data, const QString &expires_on,const QString &expires_in, const QString &expired, const QString &privatecert, const QString &alias, CertModelType type, int status)
    {
		if(!parent || parent->modelType != type)
		{
			return;
		}
        // Find cert / value in model
        QList<CertTableEntry>::iterator lower = qLowerBound(
            cachedCertTable.begin(), cachedCertTable.end(), cert, CertTableEntryLessThan());
        QList<CertTableEntry>::iterator upper = qUpperBound(
            cachedCertTable.begin(), cachedCertTable.end(), cert, CertTableEntryLessThan());
        int lowerIndex = (lower - cachedCertTable.begin());
        int upperIndex = (upper - cachedCertTable.begin());
        bool inModel = (lower != upper);
        CertTableEntry::Type newEntryType = CertTableEntry::Cert;

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedCertTable.insert(lowerIndex, CertTableEntry(newEntryType, title, cert, data, expires_on, expires_in, expired,privatecert, alias));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                break;
            }
            lower->type = newEntryType;
            lower->title = title;
			lower->data = data;
			lower->expires_on = expires_on;
			lower->expires_in = expires_in;
			lower->expired = expired;
			lower->alias = alias;
			lower->privatecert = privatecert;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedCertTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedCertTable.size();
    }

    CertTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedCertTable.size())
        {
            return &cachedCertTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

CertTableModel::CertTableModel(CWallet *wallet, WalletModel *parent,  CertModelType type) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0), modelType(type)
{
    columns << tr("Cert") << tr("Title") << tr("Data") << tr("Private") << tr("Expires On") << tr("Expires In") << tr("Certificate Status") << tr("Owner");
    priv = new CertTablePriv(wallet, this);
    refreshCertTable();
}

CertTableModel::~CertTableModel()
{
    delete priv;
}
void CertTableModel::refreshCertTable() 
{
	if(modelType != MyCert)
		return;
	clear();
	priv->refreshCertTable(modelType);
}
int CertTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int CertTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant CertTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    CertTableEntry *rec = static_cast<CertTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Title:
            return rec->title;
        case Data:
            return rec->data;
        case Private:
            return rec->privatecert;
        case Name:
            return rec->cert;
        case ExpiresOn:
            return rec->expires_on;
        case ExpiresIn:
            return rec->expires_in;
        case Expired:
            return rec->expired;
        case Alias:
            return rec->alias;
        }
    }
    else if (role == NameRole)
    {
        return rec->cert;
    }
    else if (role == PrivateRole)
    {
        return rec->privatecert;
    }
    else if (role == AliasRole)
    {
        return rec->alias;
    }
    else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case CertTableEntry::Cert:
            return Cert;
        default: break;
        }
    }
    return QVariant();
}

bool CertTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    CertTableEntry *rec = static_cast<CertTableEntry*>(index.internalPointer());

    editStatus = OK;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case ExpiresOn:
            // Do nothing, if old value == new value
            if(rec->expires_on == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case ExpiresIn:
            // Do nothing, if old value == new value
            if(rec->expires_in == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Expired:
            // Do nothing, if old value == new value
            if(rec->expired == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
       case Title:
            // Do nothing, if old value == new value
            if(rec->title == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
       case Data:
            // Do nothing, if old value == new value
            if(rec->data == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
       case Private:
            // Do nothing, if old value == new value
            if(rec->privatecert == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
       case Alias:
            // Do nothing, if old value == new value
            if(rec->alias == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
        case Name:
            // Do nothing, if old cert == new cert
            if(rec->cert == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate certs to prevent accidental deletion of certs, if you try
            // to paste an existing cert over another cert (with a different label)
            else if(lookupCert(rec->cert) != -1)
            {
                editStatus = DUPLICATE_CERT;
                return false;
            }
            // Double-check that we're not overwriting a receiving cert
            else if(rec->type == CertTableEntry::Cert)
            {
            }
            break;
        }
        return true;
    }
    return false;
}

QVariant CertTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags CertTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex CertTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CertTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void CertTableModel::updateEntry(const QString &cert, const QString &value, const QString &data, const QString &expires_on,const QString &expires_in, const QString &expired, const QString &privatecert, const QString &alias, CertModelType type, int status)
{
    // Update cert book model from Syscoin core
    priv->updateEntry(cert, value, data, expires_on, expires_in, expired, privatecert, alias, type, status);
}

QString CertTableModel::addRow(const QString &type, const QString &cert, const QString &value, const QString &data, const QString &expires_on,const QString &expires_in, const QString &expired, const QString &privatecert, const QString &alias)
{
    std::string strCert = cert.toStdString();
    editStatus = OK;
    // Check for duplicate cert
    {
        LOCK(wallet->cs_wallet);
        if(lookupCert(cert) != -1)
        {
            editStatus = DUPLICATE_CERT;
            return QString();
        }
    }

    // Add entry

    return QString::fromStdString(strCert);
}
void CertTableModel::clear()
{
	beginResetModel();
    priv->cachedCertTable.clear();
	endResetModel();
}

int CertTableModel::lookupCert(const QString &cert) const
{
    QModelIndexList lst = match(index(0, Name, QModelIndex()),
                                Qt::EditRole, cert, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void CertTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
