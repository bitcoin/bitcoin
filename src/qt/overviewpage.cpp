// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <uint256.h>
#include <qt/walletmodel.h>

#include <omnicore/activation.h>
#include <omnicore/dbtxlist.h>
#include <omnicore/notifications.h>
#include <omnicore/omnicore.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/tx.h>
#include <omnicore/parsing.h>
#include <omnicore/pending.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/walletutils.h>

#include <chainparams.h>
#include <validation.h>
#include <sync.h>

#include <QAbstractItemDelegate>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidgetItem>
#include <QPainter>
#include <QRect>
#include <QString>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

using namespace mastercore;

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

struct OverviewCacheEntry
{
    OverviewCacheEntry()
      : address("unknown"), amount("0.0000000"), valid(false), sendToSelf(false), outbound(false)
    {}

    OverviewCacheEntry(const QString& addressIn, const QString& amountIn, bool validIn, bool sendToSelfIn, bool outboundIn)
      : address(addressIn), amount(amountIn), valid(validIn), sendToSelf(sendToSelfIn), outbound(outboundIn)
    {}

    QString address;
    QString amount;
    bool valid;
    bool sendToSelf;
    bool outbound;
};

std::map<uint256, OverviewCacheEntry> recentCache;

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(BitcoinUnits::BTC),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);

        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);

        // Rather ugly way to provide recent transaction display support - each time we paint a transaction we will check if
        // it's Omni and override the values if so.  This will not scale at all, but since we're only ever doing 6 txns via the occasional
        // repaint performance should be a non-issue and it'll provide the functionality short term while a better approach is devised.
        uint256 hash;
        hash.SetHex(index.data(TransactionTableModel::TxHashRole).toString().toStdString());
        bool omniOverride = false, omniSendToSelf = false, valid = false, omniOutbound = true;
        QString omniAmountStr;

        // check pending
        {
            LOCK(cs_pending);

            PendingMap::iterator it = my_pending.find(hash);
            if (it != my_pending.end()) {
                omniOverride = true;
                valid = true; // assume all outbound pending are valid prior to confirmation
                CMPPending *p_pending = &(it->second);
                address = QString::fromStdString(p_pending->src);
                if (isPropertyDivisible(p_pending->prop)) {
                    omniAmountStr = QString::fromStdString(FormatDivisibleShortMP(p_pending->amount) + getTokenLabel(p_pending->prop));
                } else {
                    omniAmountStr = QString::fromStdString(FormatIndivisibleMP(p_pending->amount) + getTokenLabel(p_pending->prop));
                }
                // override amount for cancels
                if (p_pending->type == MSC_TYPE_METADEX_CANCEL_PRICE || p_pending->type == MSC_TYPE_METADEX_CANCEL_PAIR ||
                    p_pending->type == MSC_TYPE_METADEX_CANCEL_ECOSYSTEM || p_pending->type == MSC_TYPE_SEND_ALL) {
                    omniAmountStr = QString::fromStdString("N/A");
                }
            }
        }

        // check cache (avoid reparsing the same transactions repeatedly over and over on repaint)
        std::map<uint256, OverviewCacheEntry>::iterator cacheIt = recentCache.find(hash);
        if (cacheIt != recentCache.end()) {
            OverviewCacheEntry txEntry = cacheIt->second;
            address = txEntry.address;
            valid = txEntry.valid;
            omniSendToSelf = txEntry.sendToSelf;
            omniOutbound = txEntry.outbound;
            omniAmountStr = txEntry.amount;
            omniOverride = true;
            amount = 0;
        } else { // cache miss, check database
            if (pDbTransactionList->exists(hash)) {
                omniOverride = true;
                amount = 0;
                CTransactionRef wtx;
                uint256 blockHash;
                if (GetTransaction(hash, wtx, Params().GetConsensus(), blockHash)) {
                    if (!blockHash.IsNull() || nullptr == GetBlockIndex(blockHash)) {
                        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
                        if (nullptr != pBlockIndex) {
                            int blockHeight = pBlockIndex->nHeight;
                            CMPTransaction mp_obj;
                            int parseRC = ParseTransaction(*wtx, blockHeight, 0, mp_obj);
                            if (0 < parseRC) { //positive RC means DEx payment
                                std::string tmpBuyer, tmpSeller;
                                uint64_t total = 0, tmpVout = 0, tmpNValue = 0, tmpPropertyId = 0;
                                {
                                    LOCK(cs_tally);
                                    pDbTransactionList->getPurchaseDetails(hash,1,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                                }
                                bool bIsBuy = IsMyAddress(tmpBuyer);
                                LOCK(cs_tally);
                                int numberOfPurchases=pDbTransactionList->getNumberOfSubRecords(hash);
                                if (0<numberOfPurchases) { // calculate total bought/sold
                                    for(int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++) {
                                        pDbTransactionList->getPurchaseDetails(hash,purchaseNumber,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                                        total += tmpNValue;
                                    }
                                    if (!bIsBuy) {
                                          address = QString::fromStdString(tmpSeller);
                                    } else {
                                          address = QString::fromStdString(tmpBuyer);
                                          omniOutbound = false;
                                    }
                                    omniAmountStr = QString::fromStdString(FormatDivisibleMP(total));
                                }
                            } else if (0 == parseRC) {
                                if (mp_obj.interpret_Transaction()) {
                                    valid = pDbTransactionList->getValidMPTX(hash);
                                    uint32_t omniPropertyId = mp_obj.getProperty();
                                    int64_t omniAmount = mp_obj.getAmount();
                                    if (isPropertyDivisible(omniPropertyId)) {
                                        omniAmountStr = QString::fromStdString(FormatDivisibleShortMP(omniAmount) + getTokenLabel(omniPropertyId));
                                    } else {
                                        omniAmountStr = QString::fromStdString(FormatIndivisibleMP(omniAmount) + getTokenLabel(omniPropertyId));
                                    }
                                    if (!mp_obj.getReceiver().empty()) {
                                        if (IsMyAddress(mp_obj.getReceiver())) {
                                            omniOutbound = false;
                                            if (IsMyAddress(mp_obj.getSender())) omniSendToSelf = true;
                                        }
                                        address = QString::fromStdString(mp_obj.getReceiver());
                                    } else {
                                        address = QString::fromStdString(mp_obj.getSender());
                                    }
                                }
                            }

                            // override amount for cancels
                            if (mp_obj.getType() == MSC_TYPE_METADEX_CANCEL_PRICE || mp_obj.getType() == MSC_TYPE_METADEX_CANCEL_PAIR ||
                                mp_obj.getType() == MSC_TYPE_METADEX_CANCEL_ECOSYSTEM || mp_obj.getType() == MSC_TYPE_SEND_ALL) {
                                omniAmountStr = QString::fromStdString("N/A");
                            }

                            // insert into cache
                            OverviewCacheEntry newEntry;
                            newEntry.valid = valid;
                            newEntry.sendToSelf = omniSendToSelf;
                            newEntry.outbound = omniOutbound;
                            newEntry.address = address;
                            newEntry.amount = omniAmountStr;
                            recentCache.insert(std::make_pair(hash, newEntry));
                        }
                    }
                }
            }
        }

        if (omniOverride) {
            if (!valid) {
                icon = QIcon(":/icons/omni_invalid");
            } else {
                icon = QIcon(":/icons/omni_out");
                if (!omniOutbound) icon = QIcon(":/icons/omni_in");
                if (omniSendToSelf) icon = QIcon(":/icons/omni_inout");
            }
        }

        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText;
        if (!omniOverride) {
            amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        } else {
            amountText = omniAmountStr;
        }
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(nullptr),
    walletModel(nullptr),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    m_balances.balance = -1;

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, &QListView::clicked, this, &OverviewPage::handleTransactionClicked);

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // make sure BTC is always first in the list by adding it first
    UpdatePropertyBalance(0,0,0);

    updateOmni();

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
    connect(ui->labelTransactionsStatus, &QPushButton::clicked, this, &OverviewPage::handleOutOfSyncWarningClicks);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    // is this an Omni transaction that has been clicked?  Use pending & cache to find out quickly
    uint256 hash;
    hash.SetHex(index.data(TransactionTableModel::TxHashRole).toString().toStdString());
    bool omniTx = false;
    {
        LOCK(cs_pending);

        PendingMap::iterator it = my_pending.find(hash);
        if (it != my_pending.end()) omniTx = true;
    }
    std::map<uint256, OverviewCacheEntry>::iterator cacheIt = recentCache.find(hash);
    if (cacheIt != recentCache.end()) omniTx = true;

    // override if it's an Omni transaction
    if (omniTx) {
        // TODO emit omniTransactionClicked(hash);
    } else {
        // TODO if (filter) emit transactionClicked(filter->mapToSource(index));
    }
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::UpdatePropertyBalance(unsigned int propertyId, uint64_t available, uint64_t reserved)
{
    // look for this property, does it already exist in overview and if so are the balances correct?
    int existingItem = -1;
    for(int i=0; i < ui->overviewLW->count(); i++) {
        uint64_t itemPropertyId = ui->overviewLW->item(i)->data(Qt::UserRole + 1).value<uint64_t>();
        if (itemPropertyId == propertyId) {
            uint64_t itemAvailableBalance = ui->overviewLW->item(i)->data(Qt::UserRole + 2).value<uint64_t>();
            uint64_t itemReservedBalance = ui->overviewLW->item(i)->data(Qt::UserRole + 3).value<uint64_t>();
            if ((available == itemAvailableBalance) && (reserved == itemReservedBalance)) {
                return; // norhing more to do, balance exists and is up to date
            } else {
                existingItem = i;
                break;
            }
        }
    }

    // this property doesn't exist in overview, create an entry for it
    QWidget *listItem = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    bool divisible = false;
    std::string tokenStr;
    // property label
    std::string spName;
    if (propertyId == 0) {// Override for Overpageview init during GUI tests
        spName = "Bitcoin";
    } else {
        spName = getPropertyName(propertyId).c_str();
    }
    if(spName.size()>22) spName=spName.substr(0,22)+"...";
    spName += strprintf(" (#%d)", propertyId);
    QLabel *propLabel = new QLabel(QString::fromStdString(spName));
    propLabel->setStyleSheet("QLabel { font-weight:bold; }");
    vlayout->addWidget(propLabel);

    if (propertyId == 0) { // override for bitcoin
        divisible = true;
        tokenStr = " BTC";
    } else {
        divisible = isPropertyDivisible(propertyId);
        tokenStr = getTokenLabel(propertyId);
    }

    // Left Panel
    QVBoxLayout *vlayoutleft = new QVBoxLayout();
    QLabel *balReservedLabel = new QLabel;
    if(propertyId != 0) { balReservedLabel->setText("Reserved:"); } else { balReservedLabel->setText("Pending:"); propLabel->setText("Bitcoin"); } // override for bitcoin
    QLabel *balAvailableLabel = new QLabel("Available:");
    QLabel *balTotalLabel = new QLabel("Total:");
    vlayoutleft->addWidget(balReservedLabel);
    vlayoutleft->addWidget(balAvailableLabel);
    vlayoutleft->addWidget(balTotalLabel);
    // Right panel
    QVBoxLayout *vlayoutright = new QVBoxLayout();
    QLabel *balReservedLabelAmount = new QLabel();
    QLabel *balAvailableLabelAmount = new QLabel();
    QLabel *balTotalLabelAmount = new QLabel();
    if(divisible) {
        balReservedLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(reserved) + tokenStr));
        balAvailableLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(available) + tokenStr));
        balTotalLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(available+reserved) + tokenStr));
    } else {
        balReservedLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(reserved) + tokenStr));
        balAvailableLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(available) + tokenStr));
        balTotalLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(available+reserved) + tokenStr));
    }
    balReservedLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balAvailableLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balTotalLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balReservedLabel->setStyleSheet("QLabel { font-size:12px; }");
    balAvailableLabel->setStyleSheet("QLabel { font-size:12px; }");
    balReservedLabelAmount->setStyleSheet("QLabel { font-size:12px;padding-right:2px; }");
    balAvailableLabelAmount->setStyleSheet("QLabel { font-size:12px;padding-right:2px; }");
    balTotalLabelAmount->setStyleSheet("QLabel { padding-right:2px; font-weight:bold; }");
    vlayoutright->addWidget(balReservedLabelAmount);
    vlayoutright->addWidget(balAvailableLabelAmount);
    vlayoutright->addWidget(balTotalLabelAmount);
    // put together
    vlayoutleft->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
    vlayoutright->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
    vlayoutleft->setContentsMargins(0,0,0,0);
    vlayoutright->setContentsMargins(0,0,0,0);
    vlayoutleft->setMargin(0);
    vlayoutright->setMargin(0);
    vlayoutleft->setSpacing(3);
    vlayoutright->setSpacing(3);
    hlayout->addLayout(vlayoutleft);
    hlayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed));
    hlayout->addLayout(vlayoutright);
    hlayout->setContentsMargins(0,0,0,0);
    vlayout->addLayout(hlayout);
    vlayout->addSpacerItem(new QSpacerItem(1,10,QSizePolicy::Fixed,QSizePolicy::Fixed));
    vlayout->setMargin(0);
    vlayout->setSpacing(3);
    listItem->setLayout(vlayout);
    listItem->setContentsMargins(0,0,0,0);
    listItem->layout()->setContentsMargins(0,0,0,0);
    // set data
    if(existingItem == -1) { // new
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole + 1, QVariant::fromValue<qulonglong>(propertyId));
        item->setData(Qt::UserRole + 2, QVariant::fromValue<qulonglong>(available));
        item->setData(Qt::UserRole + 3, QVariant::fromValue<qulonglong>(reserved));
        item->setSizeHint(QSize(0,listItem->sizeHint().height())); // resize
        // add the entry
        ui->overviewLW->addItem(item);
        ui->overviewLW->setItemWidget(item, listItem);
    } else {
        ui->overviewLW->item(existingItem)->setData(Qt::UserRole + 2, QVariant::fromValue<qulonglong>(available));
        ui->overviewLW->item(existingItem)->setData(Qt::UserRole + 3, QVariant::fromValue<qulonglong>(reserved));
        ui->overviewLW->setItemWidget(ui->overviewLW->item(existingItem), listItem);
    }
}

void OverviewPage::reinitOmni()
{
    recentCache.clear();
    ui->overviewLW->clear();
    if (walletModel != nullptr) {
        UpdatePropertyBalance(0, walletModel->wallet().getBalance(), walletModel->wallet().getBalances().unconfirmed_balance);
    }
    UpdatePropertyBalance(1, 0, 0);
    updateOmni();
}

/** Loop through properties and update the overview - only properties with token balances will be displayed **/
void OverviewPage::updateOmni()
{
    LOCK(cs_tally);

    unsigned int propertyId;
    unsigned int maxPropIdMainEco = GetNextPropertyId(true);
    unsigned int maxPropIdTestEco = GetNextPropertyId(false);

    // main eco
    for (propertyId = 1; propertyId < maxPropIdMainEco; propertyId++) {
        if ((global_balance_money[propertyId] > 0) || (global_balance_reserved[propertyId] > 0)) {
            UpdatePropertyBalance(propertyId,global_balance_money[propertyId],global_balance_reserved[propertyId]);
        }
    }
    // test eco
    for (propertyId = 2147483647; propertyId < maxPropIdTestEco; propertyId++) {
        if ((global_balance_money[propertyId] > 0) || (global_balance_reserved[propertyId] > 0)) {
            UpdatePropertyBalance(propertyId,global_balance_money[propertyId],global_balance_reserved[propertyId]);
        }
    }
}

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    UpdatePropertyBalance(0, balances.balance, balances.unconfirmed_balance);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    // Omni Core does not currently fully support watch only
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, &ClientModel::alertsChanged, this, &OverviewPage::updateAlerts);
        updateAlerts(model->getStatusBarWarnings());

        // Refresh Omni info if there have been Omni layer transactions with balances affecting wallet
        connect(model, &ClientModel::refreshOmniBalance, this, &OverviewPage::updateOmni);

        // Reinit Omni info if there has been a chain reorg
        connect(model, &ClientModel::reinitOmniState, this, &OverviewPage::reinitOmni);

        // Refresh alerts when there has been a change to the Omni State
        connect(model, &ClientModel::refreshOmniState, this, &OverviewPage::updateOmniAlerts);
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &OverviewPage::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &OverviewPage::updateDisplayUnit);

        updateWatchOnlyLabels(wallet.haveWatchOnly() && !model->privateKeysDisabled());
        connect(model, &WalletModel::notifyWatchonlyChanged, [this](bool showWatchOnly) {
            updateWatchOnlyLabels(showWatchOnly && !walletModel->privateKeysDisabled());
        });
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateOmniAlerts()
{
    updateAlerts(clientModel->getStatusBarWarnings());
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    QString alertString = warnings; // get current bitcoin alert/warning directly

    // get alert messages
    std::vector<std::string> omniAlerts = GetOmniCoreAlertMessages();
    for (std::vector<std::string>::iterator it = omniAlerts.begin(); it != omniAlerts.end(); it++) {
        if (!alertString.isEmpty()) alertString += "\n";
        alertString += QString::fromStdString(*it);
    }

    // get activations
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
        if (!alertString.isEmpty()) alertString += "\n";
        FeatureActivation pendingAct = *it;
        alertString += QString::fromStdString(strprintf("Feature %d ('%s') will go live at block %d",
                                                  pendingAct.featureId, pendingAct.featureName, pendingAct.activationBlock));
    }
    int currentHeight = GetHeight();
    std::vector<FeatureActivation> vecCompletedActivations = GetCompletedActivations();
    for (std::vector<FeatureActivation>::iterator it = vecCompletedActivations.begin(); it != vecCompletedActivations.end(); ++it) {
        if (currentHeight > (*it).activationBlock+1024) continue; // don't include after live+1024 blocks
        if (!alertString.isEmpty()) alertString += "\n";
        FeatureActivation completedAct = *it;
        alertString += QString::fromStdString(strprintf("Feature %d ('%s') is now live.", completedAct.featureId, completedAct.featureName));
    }

    if (!alertString.isEmpty()) {
        this->ui->labelAlerts->setVisible(true);
        this->ui->labelAlerts->setText(alertString);
    } else {
        this->ui->labelAlerts->setVisible(false);
        this->ui->labelAlerts->setText("");
    }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
