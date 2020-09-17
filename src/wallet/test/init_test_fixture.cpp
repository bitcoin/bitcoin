// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <util/check.h>
#include <util/system.h>

#include <wallet/test/init_test_fixture.h>

InitWalletDirTestingSetup::InitWalletDirTestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    m_wallet_client = MakeWalletClient(*m_chain, *Assert(m_node.args));

    std::string sep;
    sep += fs::path::preferred_separator;

    m_datadir = GetDataDir();
    m_cwd = fs::current_path();

    m_walletdir_path_cases["default"] = m_datadir / "wallets";
    m_walletdir_path_cases["custom"] = m_datadir / "my_wallets";
    m_walletdir_path_cases["nonexistent"] = m_datadir / "path_does_not_exist";
    m_walletdir_path_cases["file"] = m_datadir / "not_a_directory.dat";
    m_walletdir_path_cases["trailing"] = m_datadir / "wallets" / sep;
    m_walletdir_path_cases["trailing2"] = m_datadir / "wallets" / sep / sep;

    fs::current_path(m_datadir);
    m_walletdir_path_cases["relative"] = "wallets";

    fs::create_directories(m_walletdir_path_cases["default"]);
    fs::create_directories(m_walletdir_path_cases["custom"]);
    fs::create_directories(m_walletdir_path_cases["relative"]);
    std::ofstream f(m_walletdir_path_cases["file"].BOOST_FILESYSTEM_C_STR);
    f.close();
}

InitWalletDirTestingSetup::~InitWalletDirTestingSetup()
{
    fs::current_path(m_cwd);
}

void InitWalletDirTestingSetup::SetWalletDir(const fs::path& walletdir_path)
{
    gArgs.ForceSetArg("-walletdir", walletdir_path.string());
}
