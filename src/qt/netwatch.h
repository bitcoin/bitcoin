// Copyright (c) 2017-2021 The Bitcoin Core developers
// Copyright (c) 2011-2013 David Krauss (std::align substitute from c-plus)
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NETWATCH_H
#define BITCOIN_QT_NETWATCH_H

#include <primitives/transaction.h>
#include <sync.h>
#include <validationinterface.h>

#include <QAbstractTableModel>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTableView;
QT_END_NAMESPACE

class CBlock;
class CBlockIndex;
class ClientModel;
class LogEntry;
class NetworkStyle;
class PlatformStyle;

static constexpr int LONGEST_BASE58_ADDRESS{35};
static constexpr int LONGEST_BECH32_ADDRESS{62};  // NOTE: Up to 74 in theory, but 62 in practice

class LogEntry {
private:
    /**
     * m_data is a uint32_t (meta_t) with the top two bits used for:
     *   1: CBlockIndex*
     *   2: CTransactionRef
     *   3: weak_ptr<const CTransaction>
     * The subsequent 30 bits are the timestamp, assumed to be most recent to the current time.
     * Following this, and any padding necessary for alignment, the object itself is stored
     */
    uint8_t *m_data{nullptr};

    typedef uint32_t meta_t;
    typedef std::weak_ptr<const CTransaction> CTransactionWeakref;
    static constexpr uint32_t rel_ts_mask{0x3fffffff};
    static constexpr uint64_t rel_ts_mask64{rel_ts_mask};

    template<typename T> static size_t data_sizeof(size_t& offset) {
        void *p = (void *)intptr_t(sizeof(meta_t));
        size_t dummy_bufsize = sizeof(T) * 2;
        p = align(std::alignment_of<T>::value, sizeof(T), p, dummy_bufsize);
        assert(p);
        offset = size_t(p);
        return offset + sizeof(T);
    }

    // std::align (missing in at least GCC 4.9) substitute from c-plus
    static inline void *align(std::size_t alignment, std::size_t size, void *&ptr, std::size_t &space) {
        auto pn = reinterpret_cast<std::uintptr_t>(ptr);
        auto aligned = (pn + alignment - 1) & -alignment;
        auto new_space = space - (aligned - pn);
        if (new_space < size) {
            return nullptr;
        }
        space = new_space;
        return ptr = reinterpret_cast<void *>(aligned);
    }

    void init(const LogEntry&);
    void init(int32_t relTimestamp, const CBlockIndex&);
    void init(int32_t relTimestamp, const CTransactionWeakref&, bool weak);
    void clear();

public:
    enum class Type {
        Block,
        Transaction,
    };
    static const QString LogEntryTypeAbbreviation(Type);

    LogEntry(const LogEntry&);
    LogEntry() { }
    explicit LogEntry(int32_t relTimestamp, const CBlockIndex&);
    explicit LogEntry(int32_t relTimestamp, const CTransactionWeakref&, bool weak = true);
    explicit LogEntry(int32_t relTimestamp, const CTransactionRef&, bool weak = false);
    ~LogEntry();

    LogEntry& operator=(const LogEntry& other);

    explicit operator bool() const;
    int32_t getRelTimestamp() const;
    uint64_t getTimestamp(uint64_t now) const;
    Type getType() const;

    template <typename T> T* get() const {
        void *p = m_data + sizeof(meta_t);
        size_t dummy_bufsize = sizeof(T) * 2;
        p = align(std::alignment_of<T>::value, sizeof(T), p, dummy_bufsize);
        assert(p);
        return (T*)p;
    }

    const CBlockIndex& getBlockIndex() const;
    CTransactionRef getTransactionRef() const;
    bool isWeak() const;
    bool expired() const;
    void makeWeak();
};

class NetWatchLogModel;

class NetWatchLogSearch {
public:
    QString m_query;

    bool m_check_type;
    bool m_check_id;
    bool m_check_addr;
    bool m_check_value;

    NetWatchLogSearch(const QString& query, int display_unit);
    bool match(const NetWatchLogModel& model, int row) const;
};

class NetWatchValidationInterface;

class NetWatchLogModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    QWidget * const m_widget;
    ClientModel *m_client_model{nullptr};

    NetWatchValidationInterface *m_validation_interface;

    mutable RecursiveMutex cs;
    std::vector<LogEntry> m_log GUARDED_BY(cs);
    static constexpr size_t logsizelimit{0x400};
    size_t m_logpos GUARDED_BY(cs) {0};
    size_t m_logskip GUARDED_BY(cs) {0};

    static constexpr size_t max_nonweak_txouts{0x200};
    static constexpr size_t max_vout_per_tx{0x100};

    NetWatchLogSearch *m_current_search GUARDED_BY(cs) {nullptr};

    const LogEntry& getLogEntryRow(int row) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    LogEntry& getLogEntryRow(int row) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void log_append(const LogEntry&, size_t& rows_used) EXCLUSIVE_LOCKS_REQUIRED(cs);

public:
    static constexpr int HeaderCount{5};
    enum class Header {
        Time,
        Type,
        Id,
        Address,
        Value,
    };

    explicit NetWatchLogModel(QWidget *parent);
    ~NetWatchLogModel();

    void setClientModel(ClientModel *model);
    void OrphanedValidationInterface();

    bool isLogRowContinuation(int row) const EXCLUSIVE_LOCKS_REQUIRED(cs);
    const LogEntry& findLogEntry(int row, int& out_entry_row) const EXCLUSIVE_LOCKS_REQUIRED(cs);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const CBlockIndex&, int txout_index, const Header) const;
    QVariant data(const CTransactionRef&, int txout_index, const Header) const;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    void searchRows(const QString& query, QList<int>& results);
    void searchDisable();

    void LogAddEntry(const LogEntry& le, size_t vout_count);
    void LogBlock(const CBlockIndex*);
    void LogTransaction(const CTransactionRef&);

Q_SIGNALS:
    void moreSearchResults(const QList<int>& rows);

public Q_SLOTS:
    void updateDisplayUnit();
};

class NetWatchLogTestModel : public NetWatchLogModel
{
    Q_OBJECT

public:
    NetWatchLogTestModel() : NetWatchLogModel(nullptr) { }

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
};

class GuiNetWatch: public QWidget
{
    Q_OBJECT

private:
    bool m_adjust_scroll;

public:
    GuiNetWatch(const PlatformStyle *, const NetworkStyle *, QWidget * parent = nullptr);

    void setClientModel(ClientModel *model);

    NetWatchLogModel *log_model;
    bool m_dont_cancel_search{false};

    QLineEdit *m_search_editor;
    QTableView *m_log_view;

public Q_SLOTS:
    void rowsRemoved(const QModelIndex& parent, int start, int end);
    void aboutToInsert();
    void maybeScrollToBottom();
    void doSearch(const QString& query);
    void moreSearchResults(const QList<int>& rows);
    void maybeCancelSearch();
};

#endif // BITCOIN_QT_NETWATCH_H
