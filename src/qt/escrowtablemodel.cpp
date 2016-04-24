
#include "escrowtablemodel.h"

#include "guiutil.h"
#include "walletmodel.h"

#include "wallet/wallet.h"
#include "base58.h"

#include <QFont>
#include <QDateTime>
#include "rpcserver.h"
using namespace std;



extern const CRPCTable tableRPC;
struct EscrowTableEntry
{
    enum Type {
        Escrow
    };

    Type type;
    QString escrow;
	QString time;
	QString seller;
	QString arbiter;
	QString offer;
	QString offertitle;
	QString offeraccept;
	QString total;
	QString status;
	QString buyer;
    EscrowTableEntry() {}
    EscrowTableEntry(Type type, const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer, const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status, const QString &buyer):
        type(type), escrow(escrow), time(time), seller(seller), arbiter(arbiter), offer(offer), offertitle(offertitle), offeraccept(offeraccept), total(total), status(status), buyer(buyer){}
};

struct EscrowTableEntryLessThan
{
    bool operator()(const EscrowTableEntry &a, const EscrowTableEntry &b) const
    {
        return a.escrow < b.escrow;
    }
    bool operator()(const EscrowTableEntry &a, const QString &b) const
    {
        return a.escrow < b;
    }
    bool operator()(const QString &a, const EscrowTableEntry &b) const
    {
        return a < b.escrow;
    }
};


// Private implementation
class EscrowTablePriv
{
public:
    CWallet *wallet;
    QList<EscrowTableEntry> cachedEscrowTable;
    EscrowTableModel *parent;

    EscrowTablePriv(CWallet *wallet, EscrowTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshEscrowTable(EscrowModelType type)
    {

        cachedEscrowTable.clear();
        {
			string strMethod = string("escrowlist");
	        UniValue params(UniValue::VARR); 
			UniValue result ;
			string name_str;
			string time_str;
			string seller_str;
			string arbiter_str;
			string status_str;
			string offeraccept_str;
			string offer_str;
			string offertitle_str;
			string total_str;
			string buyer_str;
			int unixTime;
			QDateTime dateTime;	
			try {
				result = tableRPC.execute(strMethod, params);
				if (result.type() == UniValue::VARR)
				{
					name_str = "";
					time_str = "";
					seller_str = "";
					arbiter_str = "";
					status_str = "";
					offeraccept_str = "";
					offer_str = "";
					offertitle_str = "";
					total_str = "";	
					buyer_str = "";
					const UniValue &arr = result.get_array();
				    for (unsigned int idx = 0; idx < arr.size(); idx++) {
					    const UniValue& input = arr[idx];
						if (input.type() != UniValue::VOBJ)
							continue;
						const UniValue& o = input.get_obj();
						name_str = "";
						name_str = "";
						seller_str = "";
						arbiter_str = "";
						status_str = "";
						offeraccept_str = "";
						offer_str = "";
						total_str = "";
			
				
						const UniValue& name_value = find_value(o, "escrow");
						if (name_value.type() == UniValue::VSTR)
							name_str = name_value.get_str();
						const UniValue& time_value = find_value(o, "time");
						if (time_value.type() == UniValue::VSTR)
							time_str = time_value.get_str();
						const UniValue& seller_value = find_value(o, "seller");
						if (seller_value.type() == UniValue::VSTR)
							seller_str = seller_value.get_str();
						const UniValue& arbiter_value = find_value(o, "arbiter");
						if (arbiter_value.type() == UniValue::VSTR)
							arbiter_str = arbiter_value.get_str();
						const UniValue& buyer_value = find_value(o, "buyer");
						if (buyer_value.type() == UniValue::VSTR)
							buyer_str = buyer_value.get_str();
						const UniValue& offer_value = find_value(o, "offer");
						if (offer_value.type() == UniValue::VSTR)
							offer_str = offer_value.get_str();
						const UniValue& offertitle_value = find_value(o, "offertitle");
						if (offertitle_value.type() == UniValue::VSTR)
							offertitle_str = offertitle_value.get_str();
						const UniValue& offeraccept_value = find_value(o, "offeracceptlink");
						if (offeraccept_value.type() == UniValue::VSTR)
							offeraccept_str = offeraccept_value.get_str();
						const UniValue& total_value = find_value(o, "total");
						if (total_value.type() == UniValue::VSTR)
							total_str = total_value.get_str();
						const UniValue& status_value = find_value(o, "status");
						if (status_value.type() == UniValue::VSTR)
							status_str = status_value.get_str();
						
						unixTime = atoi(time_str.c_str());
						dateTime.setTime_t(unixTime);
						time_str = dateTime.toString().toStdString();	

						updateEntry(QString::fromStdString(name_str), QString::fromStdString(time_str), QString::fromStdString(seller_str), QString::fromStdString(arbiter_str), QString::fromStdString(offer_str), QString::fromStdString(offertitle_str), QString::fromStdString(offeraccept_str), QString::fromStdString(total_str), QString::fromStdString(status_str), QString::fromStdString(buyer_str), type, CT_NEW); 
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
        // qLowerBound() and qUpperBound() require our cachedEscrowTable list to be sorted in asc order
        qSort(cachedEscrowTable.begin(), cachedEscrowTable.end(), EscrowTableEntryLessThan());
    }

    void updateEntry(const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer, const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status, const QString &buyer, EscrowModelType type, int statusi)
    {
		if(!parent || parent->modelType != type)
		{
			return;
		}
        // Find escrow / value in model
        QList<EscrowTableEntry>::iterator lower = qLowerBound(
            cachedEscrowTable.begin(), cachedEscrowTable.end(), escrow, EscrowTableEntryLessThan());
        QList<EscrowTableEntry>::iterator upper = qUpperBound(
            cachedEscrowTable.begin(), cachedEscrowTable.end(), escrow, EscrowTableEntryLessThan());
        int lowerIndex = (lower - cachedEscrowTable.begin());
        int upperIndex = (upper - cachedEscrowTable.begin());
        bool inModel = (lower != upper);
		int index;
        EscrowTableEntry::Type newEntryType = /*isData ? EscrowTableEntry::DataEscrow :*/ EscrowTableEntry::Escrow;

        switch(statusi)
        {
        case CT_NEW:
			index = parent->lookupEscrow(escrow);
            if(inModel || index != -1)
            {
                break; 
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedEscrowTable.insert(lowerIndex, EscrowTableEntry(newEntryType, escrow, time, seller, arbiter, offer, offertitle, offeraccept, total, status, buyer));
            parent->endInsertRows();
            break;
        case CT_UPDATED:
            if(!inModel)
            {             
                break;
            }
            lower->type = newEntryType;
			lower->time = time;
            lower->seller = seller;
			lower->arbiter = arbiter;
			lower->offer = offer;
			lower->offertitle = offertitle;
			lower->offeraccept = offeraccept;
			lower->total = total;
			lower->status = status;
			lower->buyer = buyer;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
            cachedEscrowTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedEscrowTable.size();
    }

    EscrowTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedEscrowTable.size())
        {
            return &cachedEscrowTable[idx];
        }
        else
        {
            return 0;
        }
    }
};

EscrowTableModel::EscrowTableModel(CWallet *wallet, WalletModel *parent,  EscrowModelType type) :
    QAbstractTableModel(parent),walletModel(parent),wallet(wallet),priv(0), modelType(type)
{

	columns << tr("Escrow") << tr("Time") <<  tr("Seller") << tr("Arbiter") << tr("Buyer") << tr("Offer") << tr("Title") << tr("Confirmation") << tr("Total") << tr("Status");		 
    priv = new EscrowTablePriv(wallet, this);
	refreshEscrowTable();
}

EscrowTableModel::~EscrowTableModel()
{
    delete priv;
}
void EscrowTableModel::refreshEscrowTable() 
{
	if(modelType != MyEscrow)
		return;
	clear();
	priv->refreshEscrowTable(modelType);
}
int EscrowTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int EscrowTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant EscrowTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    EscrowTableEntry *rec = static_cast<EscrowTableEntry*>(index.internalPointer());

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case Escrow:
            return rec->escrow;
        case Time:
            return rec->time;
        case Arbiter:
            return rec->arbiter;
        case Seller:
            return rec->seller;
        case Offer:
            return rec->offer;
        case OfferTitle:
            return rec->offertitle;
        case OfferAccept:
            return rec->offeraccept;
        case Total:
            return rec->total;
        case Status:
            return rec->status;
        case Buyer:
            return rec->buyer;
        }
    }
    else if (role == EscrowRole)
    {
        return rec->escrow;
    }
    else if (role == BuyerRole)
    {
        return rec->buyer;
    }
    else if (role == SellerRole)
    {
        return rec->seller;
    }
    else if (role == ArbiterRole)
    {
        return rec->arbiter;
    }
    else if (role == StatusRole)
    {
        return rec->status;
    }
    return QVariant();
}

bool EscrowTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    EscrowTableEntry *rec = static_cast<EscrowTableEntry*>(index.internalPointer());

    editStatus = OK;

    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
         case Status:
            // Do nothing, if old value == new value
            if(rec->status == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;   
         case Time:
            // Do nothing, if old value == new value
            if(rec->time == value.toString())
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
        case OfferAccept:
            // Do nothing, if old value == new value
            if(rec->offeraccept == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Offer:
            // Do nothing, if old value == new value
            if(rec->offer == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case OfferTitle:
            // Do nothing, if old value == new value
            if(rec->offertitle == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Seller:
            // Do nothing, if old value == new value
            if(rec->seller == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Buyer:
            // Do nothing, if old value == new value
            if(rec->buyer == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
           
            break;
        case Arbiter:
            // Do nothing, if old value == new value
            if(rec->arbiter == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            break;
        case Escrow:
            // Do nothing, if old escrow == new escrow
            if(rec->escrow == value.toString())
            {
                editStatus = NO_CHANGES;
                return false;
            }
            // Check for duplicate Escrows to prevent accidental deletion of Escrows, if you try
            // to paste an existing escrow over another escrow (with a different label)
            else if(lookupEscrow(rec->escrow) != -1)
            {
                editStatus = DUPLICATE_ESCROW;
                return false;
            }

            break;
        }
        return true;
    }
    return false;
}

QVariant EscrowTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

Qt::ItemFlags EscrowTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;
    Qt::ItemFlags retval = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return retval;
}

QModelIndex EscrowTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    EscrowTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void EscrowTableModel::updateEntry(const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer, const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status, const QString &buyer, EscrowModelType type, int statusi)
{
    // Update escrow book model from Syscoin core
    priv->updateEntry(escrow, time, seller, arbiter, offer, offertitle, offeraccept, total, status, buyer, type, statusi);
}

QString EscrowTableModel::addRow(const QString &escrow, const QString &time, const QString &seller, const QString &arbiter, const QString &offer, const QString &offertitle, const QString &offeraccept, const QString &total, const QString &status,  const QString &buyer)
{
    std::string strEscrow = escrow.toStdString();
    editStatus = OK;
    // Check for duplicate Escrows
    {
        LOCK(wallet->cs_wallet);
        if(lookupEscrow(escrow) != -1)
        {
            editStatus = DUPLICATE_ESCROW;
            return QString();
        }
    }

    // Add entry

    return QString::fromStdString(strEscrow);
}
void EscrowTableModel::clear()
{
	beginResetModel();
    priv->cachedEscrowTable.clear();
	endResetModel();
}


int EscrowTableModel::lookupEscrow(const QString &escrow) const
{
    QModelIndexList lst = match(index(0, Escrow, QModelIndex()),
                                Qt::EditRole, escrow, 1, Qt::MatchExactly);
    if(lst.isEmpty())
    {
        return -1;
    }
    else
    {
        return lst.at(0).row();
    }
}

void EscrowTableModel::emitDataChanged(int idx)
{
    Q_EMIT dataChanged(index(idx, 0, QModelIndex()), index(idx, columns.length()-1, QModelIndex()));
}
