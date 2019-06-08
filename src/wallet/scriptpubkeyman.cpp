// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/scriptpubkeyman.h>

bool LegacyScriptPubKeyMan::GetNewDestination(const OutputType type, CTxDestination& dest, std::string& error)
{
    return false;
}

isminetype LegacyScriptPubKeyMan::IsMine(const CScript& script) const
{
    return ISMINE_NO;
}

bool LegacyScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key, bool accept_no_keys)
{
    return false;
}

bool LegacyScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    return false;
}

bool LegacyScriptPubKeyMan::GetReservedDestination(const OutputType type, bool internal, CTxDestination& address, int64_t& index, CKeyPool& keypool)
{
    return false;
}

void LegacyScriptPubKeyMan::KeepDestination(int64_t index)
{
}

void LegacyScriptPubKeyMan::ReturnDestination(int64_t index, bool internal, const CTxDestination& addr)
{
}

bool LegacyScriptPubKeyMan::TopUp(unsigned int size)
{
    return false;
}

void LegacyScriptPubKeyMan::MarkUnusedAddresses(const CScript& script)
{
}

void LegacyScriptPubKeyMan::UpgradeKeyMetadata()
{
}

bool LegacyScriptPubKeyMan::SetupGeneration(bool force)
{
    return false;
}

bool LegacyScriptPubKeyMan::IsHDEnabled() const
{
    return false;
}

bool LegacyScriptPubKeyMan::CanGetAddresses(bool internal)
{
    return false;
}

bool LegacyScriptPubKeyMan::Upgrade(int prev_version, std::string& error)
{
    return false;
}

bool LegacyScriptPubKeyMan::HavePrivateKeys() const
{
    return false;
}

void LegacyScriptPubKeyMan::RewriteDB()
{
}

int64_t LegacyScriptPubKeyMan::GetOldestKeyPoolTime()
{
    return GetTime();
}

size_t LegacyScriptPubKeyMan::KeypoolCountExternalKeys()
{
    return 0;
}

unsigned int LegacyScriptPubKeyMan::GetKeyPoolSize() const
{
    return 0;
}

int64_t LegacyScriptPubKeyMan::GetTimeFirstKey() const
{
    return 0;
}

std::unique_ptr<SigningProvider> LegacyScriptPubKeyMan::GetSigningProvider(const CScript& script) const
{
    return nullptr;
}

bool LegacyScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    return false;
}

const CKeyMetadata* LegacyScriptPubKeyMan::GetMetadata(uint160 id) const
{
    return nullptr;
}

uint256 LegacyScriptPubKeyMan::GetID() const
{
    return uint256S("0000000000000000000000000000000000000000000000000000000000000001");
}
