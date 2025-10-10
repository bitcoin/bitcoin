// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <qt/netwatch.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/networkstyle.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>

#include <chain.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <sync.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <memory>
#include <type_traits>

#include <QAbstractTableModel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {

bool IsDatacarrier(const CTxOut& txout)
{
    return (txout.scriptPubKey[0] == OP_RETURN && txout.nValue == 0);
}

size_t CountNonDatacarrierOutputs(const CTransactionRef& tx)
{
    size_t count = 0;
    for (const auto& txout : tx->vout) {
        if (IsDatacarrier(txout)) continue;
        ++count;
    }
    return count;
}

const CTxOut* GetNonDatacarrierOutput(const CTransactionRef& tx, const size_t txout_index)
{
    size_t count = 0;
    for (auto& txout : tx->vout) {
        if (IsDatacarrier(txout)) continue;
        if (count == txout_index) {
            return &txout;
        }
        ++count;
    }
    return nullptr;
}

} // namespace

QString LogEntry::LogEntryTypeAbbreviation(const Type log_entry_type)
{
    switch (log_entry_type) {
        case Type::Block:       return QObject::tr("Blk", "Tx Watch: Block type abbreviation");
        case Type::Transaction: return QObject::tr("Txn", "Tx Watch: Transaction type abbreviation");
    } // no default case, so the compiler can warn about missing cases

    assert(false);
}

void LogEntry::init(const LogEntry& other)
{
    if (!other.m_data) {
        m_data = nullptr;
        return;
    }
    const meta_t n = *(meta_t*)other.m_data;
    const int32_t relTimestamp = n & rel_ts_mask;
    switch (n >> 30) {
        case 1:  // CBlockIndex*
            init(relTimestamp, other.getBlockIndex());
            break;
        case 2:  // CTransactionRef
        {
            CTransactionWeakref ptx(*other.get<CTransactionRef>());
            init(relTimestamp, ptx, false);
            break;
        }
        case 3:  // CTransactionWeakref
            init(relTimestamp, *other.get<CTransactionWeakref>(), true);
            break;
        default: assert(false);
    }
}

LogEntry::LogEntry(const LogEntry& other)
{
    init(other);
}

void LogEntry::init(int32_t relTimestamp, const CBlockIndex& blockindex)
{
    relTimestamp &= rel_ts_mask;
    size_t alignment;
    const size_t sz = data_sizeof<CBlockIndex*>(alignment);
    m_data = (uint8_t*)::operator new(sz);
    *((meta_t*)m_data) = relTimestamp | (1 << 30);
    *((const CBlockIndex**)&m_data[alignment]) = &blockindex;
}

LogEntry::LogEntry(int32_t relTimestamp, const CBlockIndex& blockindex)
{
    init(relTimestamp, blockindex);
}

void LogEntry::init(int32_t relTimestamp, const CTransactionWeakref& tx, bool weak)
{
    relTimestamp &= rel_ts_mask;

    // Allocate enough space for either, so we can convert between them
    size_t alignment_shared, alignment_weak;
    const size_t sz = std::max(data_sizeof<CTransactionRef>(alignment_shared), data_sizeof<CTransactionWeakref>(alignment_weak));
    m_data = (uint8_t*)::operator new(sz);

    uint32_t type;
    if (weak) {
        //std::allocator<std::weak_ptr<CTransaction>>::construct(m_data + alignment_weak, tx);
        new (m_data + alignment_weak) CTransactionWeakref(tx);
        type = 3;
    } else {
        new (m_data + alignment_shared) CTransactionRef(tx.lock());
        type = 2;
    }
    *((meta_t*)m_data) = relTimestamp | (type << 30);
}

LogEntry::LogEntry(int32_t relTimestamp, const CTransactionWeakref& tx, bool weak)
{
    init(relTimestamp, tx, weak);
}

LogEntry::LogEntry(int32_t relTimestamp, const CTransactionRef& tx, bool weak)
{
    CTransactionWeakref ptx(tx);
    init(relTimestamp, ptx, weak);
}

void LogEntry::clear()
{
    if (!m_data) {
        return;
    }
    const meta_t n = *(meta_t*)m_data;
    switch (n >> 30) {
        case 1:  // CBlockIndex*
            break;
        case 2:  // CTransactionRef
            get<CTransactionRef>()->~CTransactionRef();
            break;
        case 3:  // CTransactionWeakref
            get<CTransactionWeakref>()->~CTransactionWeakref();
            break;
        default: assert(false);
    }
    delete m_data;
}

LogEntry::~LogEntry()
{
    clear();
}

LogEntry& LogEntry::operator=(const LogEntry& other)
{
    if (this != &other) {
        clear();
        init(other);
    }
    return *this;
}

LogEntry::operator bool() const
{
    return m_data;
}

int32_t LogEntry::getRelTimestamp() const
{
    const meta_t n = *(meta_t*)m_data;
    return n & rel_ts_mask;
}

uint64_t LogEntry::getTimestamp(uint64_t now) const
{
    uint64_t ts = (now & ~rel_ts_mask64) | getRelTimestamp();
    if (ts > now) {
        ts -= (rel_ts_mask64 + 1);
    }
    return ts;
}

LogEntry::Type LogEntry::getType() const
{
    const meta_t n = *(meta_t*)m_data;
    return (n >> 31) ? Type::Transaction : Type::Block;
}

const CBlockIndex& LogEntry::getBlockIndex() const
{
    return **(get<const CBlockIndex*>());
}

CTransactionRef LogEntry::getTransactionRef() const
{
    const meta_t n = *(meta_t*)m_data;
    if ((n >> 30) & 1) {
        return get<CTransactionWeakref>()->lock();
    } else {
        return *get<CTransactionRef>();
    }
}

bool LogEntry::isWeak() const
{
    const meta_t n = *(meta_t*)m_data;
    return ((n >> 30) == 3);
}

bool LogEntry::expired() const
{
    if (isWeak()) {
        return get<CTransactionWeakref>()->expired();
    }
    return false;
}

void LogEntry::makeWeak()
{
    const meta_t n = *(meta_t*)m_data;
    if ((n >> 30) != 2) {
        return;
    }
    CTransactionRef * const ptx_old = get<CTransactionRef>();
    CTransactionRef tx = *ptx_old;  // save a copy
    ptx_old->~CTransactionRef();
    CTransactionWeakref * const ptx_new = get<CTransactionWeakref>();
    new (ptx_new) CTransactionWeakref(tx);
    *((meta_t*)m_data) |= (3 << 30);
}

class NetWatchValidationInterface final : public CValidationInterface {
private:
    NetWatchLogModel& model;

public:
    explicit NetWatchValidationInterface(NetWatchLogModel& model_in) : model(model_in) {}
    void ValidationInterfaceUnregistering() override;

    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override;
    void TransactionAddedToMempool(const NewMempoolTransactionInfo&, uint64_t mempool_sequence) override;
};

void NetWatchValidationInterface::ValidationInterfaceUnregistering()
{
    model.OrphanedValidationInterface();
}

void NetWatchValidationInterface::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
{
    model.LogBlock(pindex, block);
}

void NetWatchValidationInterface::TransactionAddedToMempool(const NewMempoolTransactionInfo& txinfo, uint64_t mempool_sequence)
{
    model.LogTransaction(txinfo.info.m_tx);
}

NetWatchLogModel::NetWatchLogModel(QWidget *parent) :
    QAbstractTableModel(parent),
    m_widget(parent)
{
}

NetWatchLogModel::~NetWatchLogModel()
{
    LOCK(cs);
    if (m_validation_interface) {
        Assert(m_client_model && m_client_model->node().context()->validation_signals);
        m_client_model->node().context()->validation_signals->UnregisterValidationInterface(m_validation_interface);
        delete m_validation_interface;
        m_validation_interface = nullptr;
    }
}

void NetWatchLogModel::OrphanedValidationInterface()
{
    LOCK(cs);
    delete m_validation_interface;
    m_validation_interface = nullptr;
}

int NetWatchLogModel::rowCount(const QModelIndex& parent) const
{
    LOCK(cs);
    return m_log.size() - m_logskip;
}

int NetWatchLogModel::columnCount(const QModelIndex& parent) const
{
    return HeaderCount;
}

QVariant NetWatchLogModel::data(const CBlockIndex& blockindex, int txout_index, const Header header) const
{
    switch (header) {
        case Header::Type:
            return LogEntry::LogEntryTypeAbbreviation(LogEntry::Type::Block);
        case Header::Id:
            return QString::fromStdString(blockindex.GetBlockHash().GetHex());
        case Header::Address:
        case Header::Value: {
            if (blockindex.nTx == 0 || !(WITH_LOCK(::cs_main, return blockindex.nStatus) & BLOCK_HAVE_DATA)) {
                return QVariant();
            }
            CBlock block;
            Assert(m_client_model && m_client_model->node().context());
            if (!m_client_model->node().context()->chainman->m_blockman.ReadBlock(block, blockindex)) {
                // Indicate error somehow?
                return QVariant();
            }
            assert(block.vtx.size());
            return data(block.vtx[0], txout_index, header);
        }
        case Header::Time:  // Not valid here
            assert(false);
    }
    return QVariant();
}

QVariant NetWatchLogModel::data(const CTransactionRef& tx, int txout_index, const Header header) const
{
    switch (header) {
        case Header::Type:
            return LogEntry::LogEntryTypeAbbreviation(LogEntry::Type::Transaction);
        case Header::Id:
            return QString::fromStdString(tx->GetHash().GetHex());
        case Header::Address: {
            const CTxOut *ptxout = GetNonDatacarrierOutput(tx, txout_index);
            if (!ptxout) {
                // Only datacarriers
                ptxout = &tx->vout[0];
            }
            CTxDestination txdest;
            if (ptxout->scriptPubKey[0] == OP_RETURN && ptxout->nValue > 0) {
                return tr("(Burn)", "Tx Watch: Provably burned value in transaction");
            } else if (!ExtractDestination(ptxout->scriptPubKey, txdest)) {
                return tr("(Unknown)", "Tx Watch: Unknown transaction output type");
            }
            return QString::fromStdString(EncodeDestination(txdest));
        }
        case Header::Value: {
            const CTxOut *ptxout = GetNonDatacarrierOutput(tx, txout_index);
            if (!ptxout) {
                ptxout = &tx->vout[0];
            }
            if (m_client_model) {
                return BitcoinUnits::format(m_client_model->getOptionsModel()->getDisplayUnit(), ptxout->nValue);
            } else {
                return qlonglong(ptxout->nValue);
            }
        }
        case Header::Time:  // Not valid here
            assert(false);
    }
    return QVariant();
}

const LogEntry& NetWatchLogModel::getLogEntryRow(int row) const
{
    AssertLockHeld(cs);
    size_t pos = (m_logpos + row) % m_log.size();
    return m_log[pos];
}

LogEntry& NetWatchLogModel::getLogEntryRow(int row)
{
    AssertLockHeld(cs);
    size_t pos = (m_logpos + row) % m_log.size();
    return m_log[pos];
}

bool NetWatchLogModel::isLogRowContinuation(int row) const
{
    AssertLockHeld(cs);
    return !getLogEntryRow(row);
}

const LogEntry& NetWatchLogModel::findLogEntry(int row, int& out_entry_row) const
{
    AssertLockHeld(cs);
    out_entry_row = 0;
    while (row && isLogRowContinuation(row)) {
        --row;
        ++out_entry_row;
    }
    return getLogEntryRow(row);
}

// NOLINTNEXTLINE(misc-no-recursion)
QVariant NetWatchLogModel::data(const QModelIndex& index, int role) const
{
    const Header header = Header(index.column());
    switch (role) {
        case Qt::DisplayRole:
            break;
        case Qt::BackgroundRole: {
            if (!data(index, Qt::DisplayRole).isValid()) {
                return m_widget->palette().brush(QPalette::WindowText);
            }
            LogEntry::Type type;
            {
                int entry_row;
                LOCK(cs);
                const LogEntry& le = findLogEntry(index.row(), entry_row);
                type = le.getType();
            }
            if (type == LogEntry::Type::Block) {
                return m_widget->palette().brush(QPalette::AlternateBase);
            }
            return QVariant();
        }
        case Qt::ForegroundRole: {
            if (index.column() < 3) {
                bool iscont;
                {
                    LOCK(cs);
                    iscont = isLogRowContinuation(index.row());
                }
                if (iscont) {
                    QBrush brush = m_widget->palette().brush(QPalette::WindowText);
                    QColor color = brush.color();
                    color.setAlpha(color.alpha() / 2);
                    brush.setColor(color);
                    return brush;
                }
            }
            return QVariant();
        }
        case Qt::TextAlignmentRole:
            if (header == Header::Value) {
                return QVariant(Qt::AlignRight | Qt::AlignVCenter);
            }
            return QVariant();
        case Qt::FontRole:
            if (header == Header::Id) {
                return GUIUtil::fixedPitchFont();
            }
            if (header == Header::Value) {
                return m_client_model->getOptionsModel()->getFontForMoney();
            }
            return QVariant();
        default:
            return QVariant();
    }
    int entry_row;
    LOCK(cs);
    const LogEntry& le = findLogEntry(index.row(), entry_row);
    if (header == Header::Time) {
        return GUIUtil::dateTimeStr(le.getTimestamp(GetTime()));
    }
    const LogEntry::Type type = le.getType();
    if (type == LogEntry::Type::Block) {
        return data(le.getBlockIndex(), entry_row, header);
    }
    if (header == Header::Type) {
        return LogEntry::LogEntryTypeAbbreviation(LogEntry::Type::Transaction);
    }

    if (le.expired()) {
        return QVariant();
    }
    return data(le.getTransactionRef(), entry_row, header);
}

QVariant NetWatchLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    switch (Header(section)) {
        case Header::Time:     return tr("Time"   , "NetWatch: Time header");
        case Header::Type:     return tr("Type"   , "NetWatch: Type header");
        case Header::Id:       return tr("Id"     , "NetWatch: Block hash / Txid header");
        case Header::Address:  return tr("Address", "NetWatch: Address header");
        case Header::Value:
            if (m_client_model) {
                return BitcoinUnits::getAmountColumnTitle(m_client_model->getOptionsModel()->getDisplayUnit());
            } else {
                // Used only for sizing of the column
                return BitcoinUnits::getAmountColumnTitle(BitcoinUnits::Unit::mBTC);
            }
    }
    return QVariant();
}

NetWatchLogSearch::NetWatchLogSearch(const QString& query, BitcoinUnit display_unit) :
    m_query(query)
{
    const QRegularExpression reHex("^[\\da-f]+$", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression reType("^(T(xn?)?|B(lk?)?)$", QRegularExpression::CaseInsensitiveOption);

    m_check_type = m_query.length() < 4 && reType.match(m_query).hasMatch();
    m_check_id = m_query.length() <= 64 && reHex.match(m_query).hasMatch();
    m_check_addr = m_query.length() <= LONGEST_BECH32_ADDRESS;
    CAmount val;
    m_check_value = BitcoinUnits::parse(display_unit, m_query, &val) && val >= 0 && val <= BitcoinUnits::maxMoney();
}

bool NetWatchLogSearch::match(const NetWatchLogModel& model, int row) const
{
    if (model.data(model.index(row, int(NetWatchLogModel::Header::Time))).toString().contains(m_query)) {
        return true;
    } else if (m_check_type && model.data(model.index(row, int(NetWatchLogModel::Header::Type))).toString().contains(m_query, Qt::CaseInsensitive)) {
        return true;
    } else if (m_check_id && model.data(model.index(row, int(NetWatchLogModel::Header::Id))).toString().contains(m_query, Qt::CaseInsensitive)) {
        return true;
    } else if (m_check_addr && model.data(model.index(row, int(NetWatchLogModel::Header::Address))).toString().contains(m_query, Qt::CaseInsensitive)) {
        return true;
    } else if (m_check_value && model.data(model.index(row, int(NetWatchLogModel::Header::Value))).toString().contains(m_query)) {
        return true;
    }
    return false;
}

void NetWatchLogModel::searchRows(const QString& query, QList<int>& results)
{
    const auto currentUnit = m_client_model->getOptionsModel()->getDisplayUnit();
    NetWatchLogSearch *newsearch = new NetWatchLogSearch(query, currentUnit);
    LOCK(cs);
    delete m_current_search;
    m_current_search = newsearch;

    bool fAdding = false;
    const size_t rows_used = rowCount();
    for (size_t row = 0; row < rows_used; ++row) {
        if (isLogRowContinuation(row) || newsearch->m_check_addr || newsearch->m_check_value) {
            // Check for a match
            fAdding = newsearch->match(*this, row);
        }
        if (fAdding) {
            results.append(row);
        }
    }
}

void NetWatchLogModel::searchDisable() {
    LOCK(cs);
    delete m_current_search;
    m_current_search = nullptr;
}

void NetWatchLogModel::log_append(const LogEntry& le, size_t& rows_used)
{
    AssertLockHeld(cs);
    if (m_log.size() < logsizelimit) {
        // Haven't filled up yet, so just push_back
        // Ensure push_back will append the current circular buffer, not go in the middle somewhere
        // NOTE: m_logpos and m_logskip can be non-zero here, when further outputs will be overwriting
        assert(m_logpos == m_logskip);
        m_log.push_back(le);
    } else {
        // Replace a deleted row
        assert(m_logskip);
        getLogEntryRow(rows_used) = le;
        --m_logskip;
    }
    ++rows_used;
    if (rows_used > max_nonweak_txouts) {
        LogEntry& old_le = getLogEntryRow(rows_used - max_nonweak_txouts - 1);
        if (old_le) {
            old_le.makeWeak();
        }
    }
}

void NetWatchLogModel::LogAddEntry(const LogEntry& le, size_t vout_count)
{
    if (vout_count < 1) {
        vout_count = 1;
    }
    const QModelIndex dummy;
    LOCK(cs);
    size_t rows_to_remove = 0;
    if (vout_count >= max_vout_per_tx) {
        vout_count = max_vout_per_tx;
    }
    size_t rows_used = rowCount();
    if (rows_used > logsizelimit - vout_count) {
        rows_to_remove = (rows_used + vout_count) - logsizelimit;
    }
    if (rows_to_remove) {
        // Don't orphan continuation entries
        while (isLogRowContinuation(rows_to_remove)) {
            ++rows_to_remove;
        }

        beginRemoveRows(dummy, 0, rows_to_remove - 1);
        m_logpos = (m_logpos + rows_to_remove) % m_log.size();
        m_logskip += rows_to_remove;
        endRemoveRows();

        rows_used = rowCount();
    }

    const LogEntry cont_le;
    const int first_new_row = rows_used, last_new_row = rows_used + vout_count - 1;
    beginInsertRows(dummy, first_new_row, last_new_row);
    log_append(le, rows_used);
    for (size_t i = 1; i < vout_count; ++i) {
        log_append(cont_le, rows_used);
    }
    endInsertRows();

    if (m_current_search) {
        QList<int> new_matches;
        if (m_current_search->m_check_addr || m_current_search->m_check_value) {
            for (int row = first_new_row; row <= last_new_row; ++row) {
                if (m_current_search->match(*this, row)) {
                    new_matches.append(row);
                }
            }
        } else if (m_current_search->match(*this, first_new_row)) {
            for (int row = first_new_row; row <= last_new_row; ++row) {
                new_matches.append(row);
            }
        }
        if (!new_matches.isEmpty()) {
            Q_EMIT moreSearchResults(new_matches);
        }
    }
}

void NetWatchLogModel::LogBlock(const CBlockIndex* pblockindex, const std::shared_ptr<const CBlock>& block_cached)
{
    std::shared_ptr<const CBlock> block = block_cached;
    if (!block) {
        std::shared_ptr<CBlock> pblock = std::make_shared<CBlock>();
        Assert(m_client_model && m_client_model->node().context());
        if (!m_client_model->node().context()->chainman->m_blockman.ReadBlock(*pblock, *pblockindex)) {
            // Indicate error somehow?
            return;
        }
        block = pblock;
    }
    assert(block->vtx.size());
    const size_t vout_count = CountNonDatacarrierOutputs(block->vtx[0]);
    LogAddEntry(LogEntry(GetTime(), *pblockindex), vout_count);
}

void NetWatchLogModel::LogTransaction(const CTransactionRef& tx)
{
    const size_t vout_count = CountNonDatacarrierOutputs(tx);
    LogAddEntry(LogEntry(GetTime(), tx), vout_count);
}

void NetWatchLogModel::setClientModel(ClientModel *model)
{
    if (m_client_model) {
        delete m_validation_interface;
        m_validation_interface = nullptr;

        disconnect(m_client_model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &NetWatchLogModel::updateDisplayUnit);
        disconnect(m_client_model->getOptionsModel(), &OptionsModel::fontForMoneyChanged, this, &NetWatchLogModel::updateDisplayUnit);
    }
    m_client_model = model;
    if (model) {
        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &NetWatchLogModel::updateDisplayUnit);
        connect(model->getOptionsModel(), &OptionsModel::fontForMoneyChanged, this, &NetWatchLogModel::updateDisplayUnit);

        Assert(model->node().context()->validation_signals);
        m_validation_interface = new NetWatchValidationInterface(*this);
        model->node().context()->validation_signals->RegisterValidationInterface(m_validation_interface);
    }
    updateDisplayUnit();
}

void NetWatchLogModel::updateDisplayUnit()
{
    Q_EMIT headerDataChanged(Qt::Horizontal, int(Header::Value), int(Header::Value));
    Q_EMIT dataChanged(index(0, int(Header::Value)), index(rowCount() - 1, int(Header::Value)));
}

int NetWatchLogTestModel::rowCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant NetWatchLogTestModel::data(const QModelIndex& index, int role) const
{
    const NetWatchLogModel::Header header = NetWatchLogModel::Header(index.column());
    if (role == Qt::FontRole && header == Header::Id) {
        return GUIUtil::fixedPitchFont();
    } else if (role != Qt::DisplayRole) {
        return QVariant();
    }
    switch (header) {
        case Header::Time:
            return QString{GUIUtil::dateTimeStr(GetTime()) + "4"};
        case Header::Type:
            return LogEntry::LogEntryTypeAbbreviation(LogEntry::Type(index.row()));
        case Header::Id:
            return QString(64, '0');
        case Header::Address:
            if (index.row()) {
                return QString{"bc1" + QString(LONGEST_BECH32_ADDRESS-3, 'x')};
            } else {
                return QString(LONGEST_BASE58_ADDRESS, 'W');
            }
        case Header::Value:
            return "20000000.00000000";
    }
    return QVariant();
}

GuiNetWatch::GuiNetWatch(const PlatformStyle *platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QWidget(parent)
{
    QVBoxLayout * const layout = new QVBoxLayout(this);

    m_search_editor = new QLineEdit(this);
    m_search_editor->setPlaceholderText("Search");
    layout->addWidget(m_search_editor);

    m_log_view = new QTableView(this);
    m_log_view->verticalHeader()->hide();
    m_log_view->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    m_log_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_log_view->setTabKeyNavigation(false);
    m_log_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    {
        NetWatchLogTestModel testmodel;
        m_log_view->setModel(&testmodel);
        m_log_view->resizeColumnsToContents();

        log_model = new NetWatchLogModel(this);
        m_log_view->setModel(log_model);
    }
    layout->addWidget(m_log_view);

    setWindowTitle(tr(CLIENT_NAME) + " - " + tr("Network Watch") + " " + networkStyle->getTitleAddText());
    setMinimumSize(640, 480);
    resize(layout->contentsMargins().left() + (m_log_view->frameWidth() * 2) + m_log_view->columnViewportPosition(NetWatchLogModel::HeaderCount-1) + m_log_view->columnWidth(NetWatchLogModel::HeaderCount-1) + m_log_view->verticalScrollBar()->size().width() + layout->contentsMargins().right(), 480);
    setWindowIcon(networkStyle->getTrayAndWindowIcon());

    connect(m_search_editor, &QLineEdit::textChanged, this, &GuiNetWatch::doSearch);

    connect(log_model, &NetWatchLogModel::rowsRemoved, this, &GuiNetWatch::rowsRemoved);
    connect(log_model, &NetWatchLogModel::rowsAboutToBeInserted, this, &GuiNetWatch::aboutToInsert);
    connect(log_model, &NetWatchLogModel::rowsInserted, this, &GuiNetWatch::maybeScrollToBottom);
    connect(log_model, &NetWatchLogModel::moreSearchResults, this, &GuiNetWatch::moreSearchResults);

    connect(m_log_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &GuiNetWatch::maybeCancelSearch);

    setLayout(layout);
}

void GuiNetWatch::setClientModel(ClientModel *model)
{
    log_model->setClientModel(model);
}

void GuiNetWatch::rowsRemoved(const QModelIndex& parent, int start, int end)
{
    if (m_log_view->verticalScrollBar()->value() >= m_log_view->verticalScrollBar()->maximum()) {
        return;
    }
    // Maintain the current viewed entries in place
    int scrollpos = m_log_view->verticalScrollBar()->value();
    if (start < scrollpos) {
        scrollpos -= std::max(0, 1 + end - start);
        m_log_view->verticalScrollBar()->setValue(scrollpos);
    }
}

void GuiNetWatch::aboutToInsert()
{
    m_adjust_scroll = (m_log_view->verticalScrollBar()->value() >= m_log_view->verticalScrollBar()->maximum());
}

void GuiNetWatch::maybeScrollToBottom()
{
    if (m_adjust_scroll) {
        m_log_view->scrollToBottom();
    }
}

void GuiNetWatch::doSearch(const QString& query)
{
    if (query.isEmpty()) {
        log_model->searchDisable();
        m_search_editor->setStyleSheet("");
        return;
    }
    QList<int> results;
    log_model->searchRows(query, results);
    if (results.isEmpty()) {
        m_search_editor->setStyleSheet(STYLE_INVALID);
        return;
    }
    m_search_editor->setStyleSheet(STYLE_ACTIVE);
    QItemSelectionModel& sel = *m_log_view->selectionModel();
    m_dont_cancel_search = true;
    sel.clear();
    Q_FOREACH (int row, results) {
        sel.select(log_model->index(row, 0), QItemSelectionModel::Rows | QItemSelectionModel::Select);
    }
    m_dont_cancel_search = false;
    m_log_view->scrollTo(log_model->index(results.back(), int(NetWatchLogModel::Header::Id)));
}

void GuiNetWatch::moreSearchResults(const QList<int>& rows)
{
    m_search_editor->setStyleSheet(STYLE_ACTIVE);
    QItemSelectionModel& sel = *m_log_view->selectionModel();
    m_dont_cancel_search = true;
    Q_FOREACH (int row, rows) {
        sel.select(log_model->index(row, 0), QItemSelectionModel::Rows | QItemSelectionModel::Select);
    }
    m_dont_cancel_search = false;
}

void GuiNetWatch::maybeCancelSearch()
{
    if (m_dont_cancel_search) {
        return;
    }
    m_search_editor->setStyleSheet("");
    log_model->searchDisable();
}
