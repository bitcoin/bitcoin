#include "offeraccepttablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet/wallet.h"
#include "base58.h"

#include <QFont>
#include <QSettings>
#include "rpc/server.h"
using namespace std;


const QString OfferAcceptTableModel::Offer = "O";


extern CRPCTable tableRPC;
struct OfferAcceptTableEntry
{
    enum Type {
        Offer
    };

    Type type;
    QString guid;
    QString offer;
	QString title;
	QString height;
	QString qty;
	QString currency;
	QString price;
	QString total;
	QString status;
	QString alias;
	QString buyer;

    OfferAcceptTableEntry() {}
    OfferAcceptTableEntry(Type type, const QString &guid, const QString &offer, const QString &title, const QString &height,const QString &price, const QString &currency,const QString &qty,const QString &total, const QString &alias, const QString &status, const QString &buyer):
        type(type), guid(guid), offer(offer), title(title), height(height),price(price), currency(currency),qty(qty), total(total), status(status), alias(alias), buyer(buyer) {}
};

struct OfferAcceptTableEntryLessThan
{
    bool operator()(const OfferAcceptTableEntry &a, const OfferAcceptTableEntry &b) const
    {
        return a.offer < b.offer;
    }
    bool operator()(const OfferAcceptTableEntry &a, const QString &b) const
    {
        return a.offer < b;
    }
    bool operator()(const QString &a, const OfferAcceptTableEntry &b) const
    {
        return a < b.offer;
    }
};


// Private implementation
class OfferAcceptTablePriv
{
public:
    CWallet *wallet;
    QList<OfferAcceptTableEntry> cachedOfferTable;
    OfferAcceptTableModel *parent;

    OfferAcceptTablePriv(CWallet *wallet, OfferAcceptTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshOfferTable(OfferAcceptModelType type)
    {
        cachedOfferTable.clear();
        {
			string strMethod = string("offeracceptlist");
	        UniValue params(UniValue::VARR); 
			UniValue listAliases(UniValue::VARR);
			appendListAliases(listAliases);
			params.push_back(listAliases);
			UniValue result ;
			string name_str;
			string value_str;
			string title_str;
			string height_str;
			string qty_str;
			string currency_str;
			string price_str;
			string total_str;
			string status_str;
			string ismine_str;
			string seller_str;
			string buyer_str;
			try {
				result = tableRPC.execute(strMethod, params);		
				if (result.type() == UniValue::VARR)
				{
					name_str = "";
					value_str = "";
					const UniValue &arr = result.get_array();
					for (unsigned int idx = 0; idx < arr.size(); idx++) {
						const UniValue& input = arr[idx];
						if (input.type() != UniValue::VOBJ)
							continue;
						const UniValue& o = input.get_obj();
						name_str = "";
						value_str = "";
	
						const UniValue& name_value = find_value(o, "offer");
						if (name_value.type() == UniValue::VSTR)
							name_str = name_value.get_str();
						const UniValue& value_value = find_value(o, "id");
						if (value_value.type() == UniValue::VSTR)
							value_str = value_value.get_str();
						const UniValue& height_value = find_value(o, "height");
						if (height_value.type() == UniValue::VSTR)
							height_str = height_value.get_str();
						const UniValue& title_value = find_value(o, "title");
						if (title_value.type() == UniValue::VSTR)
							title_str = title_value.get_str();
						const UniValue& price_value = find_value(o, "price");
						if (price_value.type() == UniValue::VSTR)
							price_str = price_value.get_str();
						const UniValue& currency_value = find_value(o, "currency");
						if (currency_value.type() == UniValue::VSTR)
							currency_str = currency_value.get_str();
						const UniValue& qty_value = find_value(o, "quantity");
						if (qty_value.type() == UniValue::VSTR)
							qty_str = qty_value.get_str();
						const UniValue& total_value = find_value(o, "total");
						if (total_value.type() == UniValue::VSTR)
							total_str = total_value.get_str();
						const UniValue& status_value = find_value(o, "status");
						if (status_value.type() == UniValue::VSTR)
							status_str = status_value.get_str();
						const UniValue& ismine_value = find_value(o, "ismine");
						if (ismine_value.type() == UniValue::VSTR)
							ismine_str = ismine_value.get_str();
						const UniValue& buyer_value = find_value(o, "buyer");
						if (buyer_value.type() == UniValue::VSTR)
							buyer_str = buyer_value.get_str();
						const UniValue& seller_value = find_value(o, "seller");
						if (seller_value.type() == UniValue::VSTR)
							seller_str = seller_value.get_str();

						if((FindAliasInList(listAliases, buyer_str) && type == Accept) || (FindAliasInList(listAliases, seller_str) && type == MyAccept))
							updateEntry(QString::fromStdString(name_str), QString::fromStdString(value_str), QString::fromStdString(title_str), QString::fromStdString(height_str), QString::fromStdString(price_str), QString::fromStdString(currency_str), QString::fromStdString(qty_str), QString::fromStdString(total_str), QString::fromStdString(seller_str),QString::fromStdString(status_str), QString::fromStdString(buyer_str),type, CT_NEW); 
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
	bool FindAliasInList(const UniValue& listArray, const string& aliasName)
	{
		for(unsigned int i = 0;i<listArray.size();i++)
		{
			if(listArray[i].get_str() == aliasName)
				return true;
		}
		return false;
	}
    void updateEntry(const QString &offer, const QString &guid, const QString &title, const QString &height,const QString &price, const QString &currency,const QString &qty,const QString &total, const QString &alias, const QString &status,  const QString &buyer, OfferAcceptModelType type, int statusi)
    {
		if(!parent || parent->modelType != type)
		{
			return;
		}
        // Find offer / value in model
        QList<OfferAcceptTableEntry>::iterator lower = qLowerBound(
            cachedOfferTable.begin(), cachedOfferTable.end(), guid, OfferAcceptTableEntryLessThan());
        QList<OfferAcceptTableEntry>::iterator upper = qUpperBound(
            cachedOfferTable.begin(), cachedOfferTable.end(), guid, OfferAcceptTableEntryLessThan());
        int lowerIndex = (lower - cachedOfferTable.begin());
        int upperIndex = (upper - cachedOfferTable.begin());
        bool inModel = (lower != upper);
        OfferAcceptTableEntry::Type newEntryType = OfferAcceptTableEntry::Offer;

        switch(statusi)
        {
        case CT_NEW:
            if(inModel)
            {
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedOfferTable.insert(lowerIndex, OfferAcceptTableEntry(newEntryType, guid, offer, title, height, price, currency, qty, total, alias, status, buyer));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                break;
            }
            lower->type = newEntryType;
			lower->title = title;
            lower->guid = guid;
			lower->height = height;
			lower->price = price;
			lower->currency = currency;
			lower->qty = qty;
			lower->total = total;
			lower->status = status;
			lower->buyer = buyer;
			lower->alias = alias;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedOfferTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedOfferTable.size();
    }

    OfferAcceptTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedOfferTable.size())
        {
            return &cachedOfferTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

OfferAcceptTableModel::OfferAcceptTableModel(CWallet *wallet, WalletModel *parent,  OfferAcceptModelType type) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0), modelType(type)
{
	columns << tr("Offer ID") << tr("Accept ID") << tr("Title") << tr("Height") << tr("Price") << tr("Currency") << tr("Qty") << tr("Total") << tr("Seller") << tr("Buyer") << tr("Status");
    priv = new OfferAcceptTablePriv(wallet, this);
    refreshOfferTable();
}

OfferAcceptTableModel::~OfferAcceptTableModel()
{
    delete priv;
}
void OfferAcceptTableModel::refreshOfferTable() 
{
	clear();
	priv->refreshOfferTable(modelType);
}
int OfferAcceptTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int OfferAcceptTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant OfferAcceptTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    OfferAcceptTableEntry *rec = static_cast<OfferAcceptTableEntry*>(index.internalPointer());
    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case GUID:
            return rec->guid;
        case Name:
            return rec->offer;
        case Title:
            return rec->title;
        case Height:
            return rec->height;
        case Price:
            return rec->price;
        case Currency:
            return rec->currency;
        case Qty:
            return rec->qty;
        case Total:
            return rec->total;
        case Status:
            return rec->status;
        case Alias:
            return rec->alias;
        case Buyer:
            return rec->buyer;
        }
    }
    else if (role == TypeRole)
    {
        switch(rec->type)
        {
        case OfferAcceptTableEntry::Offer:
            return Offer;
        default: break;
        }
    }
    else if (role == NameRole)
    {
        return rec->offer;
    }
    else if (role == GUIDRole)
    {
        return rec->guid;
    }
    else if (role == BuyerRole)
    {
        return rec->buyer;
    }
    else if (role == AliasRole)
    {
        return rec->alias;
    }
    return QVariant();
}

bool OfferAcceptTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    OfferAcceptTableEntry *rec = static_cast<OfferAcceptTableEntry*>(index.internalPointer());

    editStatus = OK;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Height:
            // Do nothing, if old value == new value
            if(rec->height == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Price:
            // Do nothing, if old value == new value
            if(rec->price == value.toString())
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
        case Currency:
            // Do nothing, if old value == new value
            if(rec->currency == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Qty:
            // Do nothing, if old value == new value
            if(rec->qty == value.toString())
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
        case Total:
            // Do nothing, if old value == new value
            if(rec->total == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
       case GUID:
            // Do nothing, if old value == new value
            if(rec->guid == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate offers to prevent accidental deletion of offers, if you try
            // to paste an existing offer over another offer (with a different label)
            else if(lookupOffer(rec->guid) != -1)
            {
                editStatus = DUPLICATE_OFFER;
                return false;
            }

            break;
        case Name:
            // Do nothing, if old offer == new offer
            if(rec->offer == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }

            break;
        case Status:
            // Do nothing, if old offer == new offer
            if(rec->status == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }

            break;
        case Buyer:
            // Do nothing, if old offer == new offer
            if(rec->buyer == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }

            break;
        }

        
        return true;
    }
    return false;
}

QVariant OfferAcceptTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags OfferAcceptTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex OfferAcceptTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    OfferAcceptTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void OfferAcceptTableModel::updateEntry(const QString &offer, const QString &value, const QString &title, const QString &height,const QString &price, const QString &currency, const QString &qty, const QString &total, const QString &alias, const QString &status, const QString &buyer, OfferAcceptModelType type, int statusi)
{
    priv->updateEntry(offer, value, title, height, price, currency, qty, total, alias, status, buyer, type, statusi);
}

QString OfferAcceptTableModel::addRow(const QString &type, const QString &offer, const QString &title, const QString &value, const QString &height,const QString &price, const QString &currency, const QString &qty, const QString &total, const QString &alias, const QString &status, const QString &buyer)
{
    editStatus = OK;
    // Check for duplicate offer
    {
        LOCK(wallet->cs_wallet);
        if(lookupOffer(value) != -1)
        {
            editStatus = DUPLICATE_OFFER;
            return QString();
        }
    }

    // Add entry

    return value;
}
void OfferAcceptTableModel::clear()
{
	beginResetModel();
    priv->cachedOfferTable.clear();
	endResetModel();
}


int OfferAcceptTableModel::lookupOffer(const QString &value) const
{
    QModelIndexList lst = match(index(1, GUID, QModelIndex()),
                                Qt::EditRole, value, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void OfferAcceptTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
