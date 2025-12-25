// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/db.h>
#include <wallet/dump.h>
#include <wallet/migrate.h>

#include <fstream>
#include <iostream>

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
        outfile << std::span{buffer};
        assert(outfile.fclose() == 0);
    }

    const DatabaseOptions options{};
    DatabaseStatus status;
    bilingual_str error;

    fs::path bdb_ro_dumpfile{g_setup->m_args.GetDataDirNet() / "fuzzed_dumpfile_bdb_ro.dump"};
    if (fs::exists(bdb_ro_dumpfile)) { // Writing into an existing dump file will throw an exception
        remove(bdb_ro_dumpfile);
    }
    g_setup->m_args.ForceSetArg("-dumpfile", fs::PathToString(bdb_ro_dumpfile));

    auto db{MakeBerkeleyRODatabase(wallet_path, options, status, error)};
    if (db) {
        assert(DumpWallet(g_setup->m_args, *db, error));
    } else {
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
        } else {
            throw std::runtime_error(error.original);
        }
    }
}
