// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_TEST_UTIL_H
#define BITCOIN_WALLET_TEST_UTIL_H

#include <addresstype.h>
#include <streams.h>
#include <wallet/db.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/sqlite.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

class ArgsManager;
class CChain;
class CKey;
enum class OutputType;
namespace interfaces {
class Chain;
} // namespace interfaces

namespace wallet {
class CWallet;
class WalletDatabase;
struct WalletContext;

inline constexpr DatabaseFormat DATABASE_FORMATS[] = {
       DatabaseFormat::SQLITE,
};

inline const std::string ADDRESS_BCRT1_UNSPENDABLE = "bcrt1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq3xueyj";

std::unique_ptr<CWallet> CreateSyncedWallet(interfaces::Chain& chain, CChain& cchain, const CKey& key);

std::shared_ptr<CWallet> TestCreateWallet(WalletContext& context);
std::shared_ptr<CWallet> TestCreateWallet(std::unique_ptr<WalletDatabase> database, WalletContext& context, uint64_t create_flags);
std::shared_ptr<CWallet> TestLoadWallet(WalletContext& context);
std::shared_ptr<CWallet> TestLoadWallet(std::unique_ptr<WalletDatabase> database, WalletContext& context);
void TestUnloadWallet(std::shared_ptr<CWallet>&& wallet);

/** Returns a new encoded destination from the wallet (hardcoded to BECH32) */
std::string getnewaddress(CWallet& w);
/** Returns a new destination, of an specific type, from the wallet */
CTxDestination getNewDestination(CWallet& w, OutputType output_type);

using MockableData = std::map<SerializeData, SerializeData, std::less<>>;


class MockableSQLiteBatch : public SQLiteBatch
{
public:
    using SQLiteBatch::SQLiteBatch;
    using SQLiteBatch::WriteKey;
};

/** A WalletDatabase whose contents and return values can be modified as needed for testing
 **/
class MockableSQLiteDatabase : public InMemoryWalletDatabase
{
public:
    MockableSQLiteDatabase();

    bool Backup(const std::string& strDest) const override { return true; }

    std::string Filename() override { return "mockable"; }
    std::string Format() override { return "sqlite-mock"; }
    std::unique_ptr<DatabaseBatch> MakeBatch() override { return std::make_unique<MockableSQLiteBatch>(*this); }
};

/** A SQLite wallet database that can fail selected operations for testing. */
class FaultInjectingDatabase : public MockableSQLiteDatabase
{
public:
    void FailNextWrite(std::string record_type, size_t matches_to_skip = 0)
    {
        m_fail_write = {std::move(record_type), matches_to_skip};
    }
    void FailNextErase(std::string record_type) { m_fail_erase = {std::move(record_type)}; }
    void FailNextCommit() { m_fail_commit = true; }

    std::optional<SerializeData> GetRecordValue(const std::string& record_type)
    {
        DataStream prefix;
        prefix << record_type;
        auto batch{MakeBatch()};
        if (auto cursor{batch->GetNewPrefixCursor(prefix)}) {
            DataStream key, value;
            if (cursor->Next(key, value) == DatabaseCursor::Status::MORE) {
                return SerializeData{value.begin(), value.end()};
            }
        }
        return std::nullopt;
    }

    bool HasRecordType(const std::string& record_type) { return GetRecordValue(record_type).has_value(); }

    std::unique_ptr<DatabaseBatch> MakeBatch() override { return std::make_unique<Batch>(*this); }

private:
    struct Failure {
        std::string record_type;
        size_t matches_to_skip{0};
    };

    static bool ShouldFail(std::optional<Failure>& failure, const DataStream& key)
    {
        if (!failure) return false;
        std::string record_type;
        SpanReader{MakeByteSpan(key)} >> record_type;
        if (failure->record_type != record_type) return false;
        if (failure->matches_to_skip > 0) {
            --failure->matches_to_skip;
            return false;
        }
        failure.reset();
        return true;
    }

    class Batch : public SQLiteBatch
    {
    public:
        explicit Batch(FaultInjectingDatabase& database) : SQLiteBatch(database), m_owner{database} {}

        bool TxnCommit() override
        {
            if (std::exchange(m_owner.m_fail_commit, false)) return false;
            return SQLiteBatch::TxnCommit();
        }

    protected:
        bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite = true) override
        {
            if (ShouldFail(m_owner.m_fail_write, key)) return false;
            return SQLiteBatch::WriteKey(std::move(key), std::move(value), overwrite);
        }

        bool EraseKey(DataStream&& key) override
        {
            if (ShouldFail(m_owner.m_fail_erase, key)) return false;
            return SQLiteBatch::EraseKey(std::move(key));
        }

    private:
        FaultInjectingDatabase& m_owner;
    };

    std::optional<Failure> m_fail_write{};
    std::optional<Failure> m_fail_erase{};
    bool m_fail_commit{false};
};

std::unique_ptr<WalletDatabase> CreateMockableWalletDatabase();
MockableSQLiteDatabase& GetMockableDatabase(CWallet& wallet);

DescriptorScriptPubKeyMan* CreateDescriptor(CWallet& keystore, const std::string& desc_str, bool success);
} // namespace wallet

#endif // BITCOIN_WALLET_TEST_UTIL_H
