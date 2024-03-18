// Copyright (c) 2023 The Bitcoin Core developers
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
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

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

    try {
        auto db{MakeBerkeleyRODatabase(wallet_path, options, status, error)};
        assert(db);
        assert(DumpWallet(g_setup->m_args, *db, error));
    }
    catch (const std::runtime_error& e) {
        if (std::string(e.what()) == "AutoFile::ignore: end of file: iostream error" ||
            std::string(e.what()) == "AutoFile::read: end of file: iostream error" ||
            std::string(e.what()) == "Not a BDB file" ||
            std::string(e.what()) == "Unsupported BDB data file version number" ||
            std::string(e.what()) == "Unexpected page type, should be 9 (BTree Metadata)" ||
            std::string(e.what()) == "Unexpected database flags, should only be 0x20 (subdatabases)" ||
            std::string(e.what()) == "Unexpected outer database root page type" ||
            std::string(e.what()) == "Unexpected number of entries in outer database root page" ||
            std::string(e.what()) == "Subdatabase has an unexpected name" ||
            std::string(e.what()) == "Subdatabase page number has unexpected length" ||
            std::string(e.what()) == "Unexpected inner database page type" ||
            std::string(e.what()) == "Unknown record type in records page" ||
            std::string(e.what()) == "Unknown record type in internal page" ||
            std::string(e.what()) == "Unexpected page size" ||
            std::string(e.what()) == "Unexpected page type" ||
            std::string(e.what()) == "Page number mismatch" ||
            std::string(e.what()) == "Bad btree level" ||
            std::string(e.what()) == "Bad page size" ||
            std::string(e.what()) == "File size is not a multiple of page size" ||
            std::string(e.what()) == "Meta page number mismatch")
        {
            // Do nothing
        } else if (std::string(e.what()) == "Subdatabase last page is greater than database last page" ||
                   std::string(e.what()) == "Page number is greater than database last page" ||
                   std::string(e.what()) == "Page number is greater than subdatabase last page" ||
                   std::string(e.what()) == "Last page number could not fit in file")
        {
        } else {
            throw e;
        }
    }
}
