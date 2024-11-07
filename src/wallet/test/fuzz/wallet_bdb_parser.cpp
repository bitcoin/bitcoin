// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/bdb.h>
#include <wallet/db.h>
#include <wallet/dump.h>
#include <wallet/migrate.h>

#include <fstream>
#include <iostream>

// There is an inconsistency in BDB on Windows.
// See: https://github.com/bitcoin/bitcoin/pull/26606#issuecomment-2322763212
#undef USE_BDB_NON_MSVC
#if defined(USE_BDB) && !defined(_MSC_VER)
#define USE_BDB_NON_MSVC
#endif

using wallet::DatabaseOptions;
using wallet::DatabaseStatus;

namespace {
TestingSetup* g_setup;
} // namespace

void initialize_wallet_bdb_parser()
{
    static auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_bdb_parser, .init = initialize_wallet_bdb_parser)
{
    const auto wallet_path = g_setup->m_args.GetDataDirNet() / "fuzzed_wallet.dat";

    {
        AutoFile outfile{fsbridge::fopen(wallet_path, "wb")};
        outfile << Span{buffer};
    }

    const DatabaseOptions options{};
    DatabaseStatus status;
    bilingual_str error;

    fs::path bdb_ro_dumpfile{g_setup->m_args.GetDataDirNet() / "fuzzed_dumpfile_bdb_ro.dump"};
    if (fs::exists(bdb_ro_dumpfile)) { // Writing into an existing dump file will throw an exception
        remove(bdb_ro_dumpfile);
    }
    g_setup->m_args.ForceSetArg("-dumpfile", fs::PathToString(bdb_ro_dumpfile));

#ifdef USE_BDB_NON_MSVC
    bool bdb_ro_err = false;
    bool bdb_ro_strict_err = false;
#endif
    auto db{MakeBerkeleyRODatabase(wallet_path, options, status, error)};
    if (db) {
        assert(DumpWallet(g_setup->m_args, *db, error));
    } else {
#ifdef USE_BDB_NON_MSVC
        bdb_ro_err = true;
#endif
        if (error.original.starts_with("AutoFile::ignore: end of file") ||
            error.original.starts_with("AutoFile::read: end of file") ||
            error.original.starts_with("AutoFile::seek: ") ||
            error.original == "Not a BDB file" ||
            error.original == "Unexpected page type, should be 9 (BTree Metadata)" ||
            error.original == "Unexpected database flags, should only be 0x20 (subdatabases)" ||
            error.original == "Unexpected outer database root page type" ||
            error.original == "Unexpected number of entries in outer database root page" ||
            error.original == "Subdatabase page number has unexpected length" ||
            error.original == "Unknown record type in records page" ||
            error.original == "Unknown record type in internal page" ||
            error.original == "Unexpected page size" ||
            error.original == "Unexpected page type" ||
            error.original == "Page number mismatch" ||
            error.original == "Bad btree level" ||
            error.original == "Bad page size" ||
            error.original == "Meta page number mismatch" ||
            error.original == "Data record position not in page" ||
            error.original == "Internal record position not in page" ||
            error.original == "LSNs are not reset, this database is not completely flushed. Please reopen then close the database with a version that has BDB support" ||
            error.original == "Records page has odd number of records" ||
            error.original == "Bad overflow record page type") {
            // Do nothing
        } else if (error.original == "Subdatabase last page is greater than database last page" ||
                   error.original == "Page number is greater than database last page" ||
                   error.original == "Last page number could not fit in file" ||
                   error.original == "Subdatabase has an unexpected name" ||
                   error.original == "Unsupported BDB data file version number" ||
                   error.original == "BDB builtin encryption is not supported") {
#ifdef USE_BDB_NON_MSVC
            bdb_ro_strict_err = true;
#endif
        } else {
            throw std::runtime_error(error.original);
        }
    }

#ifdef USE_BDB_NON_MSVC
    // Try opening with BDB
    fs::path bdb_dumpfile{g_setup->m_args.GetDataDirNet() / "fuzzed_dumpfile_bdb.dump"};
    if (fs::exists(bdb_dumpfile)) { // Writing into an existing dump file will throw an exception
        remove(bdb_dumpfile);
    }
    g_setup->m_args.ForceSetArg("-dumpfile", fs::PathToString(bdb_dumpfile));

    try {
        auto db{MakeBerkeleyDatabase(wallet_path, options, status, error)};
        if (bdb_ro_err && !db) {
            return;
        }
        assert(db);
        if (bdb_ro_strict_err) {
            // BerkeleyRO will be stricter than BDB. Ignore when those specific errors are hit.
            return;
        }
        assert(!bdb_ro_err);
        assert(DumpWallet(g_setup->m_args, *db, error));
    } catch (const std::runtime_error& e) {
        if (bdb_ro_err) return;
        throw e;
    }

    // Make sure the dumpfiles match
    if (fs::exists(bdb_ro_dumpfile) && fs::exists(bdb_dumpfile)) {
        std::ifstream bdb_ro_dump(bdb_ro_dumpfile, std::ios_base::binary | std::ios_base::in);
        std::ifstream bdb_dump(bdb_dumpfile, std::ios_base::binary | std::ios_base::in);
        assert(std::equal(
            std::istreambuf_iterator<char>(bdb_ro_dump.rdbuf()),
            std::istreambuf_iterator<char>(),
            std::istreambuf_iterator<char>(bdb_dump.rdbuf())));
    }
#endif
}
