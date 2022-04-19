// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <dbwrapper.h>

#include <memory>
#include <random.h>

#include <memenv.h>
#include <stdint.h>
#include <algorithm>

CDBWrapper::CDBWrapper(const fs::path& path, size_t nCacheSize, bool fMemory, bool fWipe, bool obfuscate)
{
    impl = InternalDBFactories::MakeDBImpl(GetSelectedDBType(), path, nCacheSize, fMemory, fWipe);

    // The base-case obfuscation key, which is a noop.
    obfuscate_key = std::vector<unsigned char>(OBFUSCATE_KEY_NUM_BYTES, '\000');

    bool key_exists = Read(OBFUSCATE_KEY_KEY, obfuscate_key);

    if (!key_exists && obfuscate && IsEmpty()) {
        // Initialize non-degenerate obfuscation if it won't upset
        // existing, non-obfuscated data.
        std::vector<unsigned char> new_key = CreateObfuscateKey();

        // Write `new_key` so we don't obfuscate the key with itself
        Write(OBFUSCATE_KEY_KEY, new_key);
        obfuscate_key = new_key;

        LogPrintf("Wrote new obfuscate key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
    }

    LogPrintf("Using obfuscation key for %s: %s\n", fs::PathToString(path), HexStr(obfuscate_key));
}

CDBWrapper::~CDBWrapper()
{
}

bool CDBWrapper::WriteBatch(CDBBatch &batch, bool fSync)
{
    return batch.FlushToParent(fSync);
}

size_t CDBWrapper::DynamicMemoryUsage() const
{
    return impl->DynamicMemoryUsage();
}

CDBIterator *CDBWrapper::NewIterator()
{
    return new CDBIterator(*this);
}

IDB *CDBWrapper::GetRawDB()
{
    return impl.get();
}

// Prefixed with null character to avoid collisions with other keys
//
// We must use a string constructor which specifies length so that we copy
// past the null-terminator.
const std::string CDBWrapper::OBFUSCATE_KEY_KEY("\000obfuscate_key", 14);

const unsigned int CDBWrapper::OBFUSCATE_KEY_NUM_BYTES = 8;

/**
 * Returns a string (consisting of 8 random bytes) suitable for use as an
 * obfuscating XOR key.
 */
std::vector<unsigned char> CDBWrapper::CreateObfuscateKey() const
{
    std::vector<uint8_t> ret(OBFUSCATE_KEY_NUM_BYTES);
    GetRandBytes(ret);
    return ret;
}

CDBIterator::~CDBIterator() { }
bool CDBIterator::Valid() const { return piter->Valid(); }
void CDBIterator::SeekToFirst() { piter->SeekToFirst(); }
void CDBIterator::Next() { piter->Next(); }

uint32_t CDBIterator::GetValueSize() {
    return piter->GetValueSize();
}

namespace dbwrapper_private {

const std::vector<unsigned char>& GetObfuscateKey(const CDBWrapper &w)
{
    return w.obfuscate_key;
}

} // namespace dbwrapper_private

std::unique_ptr<IDB> InternalDBFactories::MakeDBImpl(DatabaseBackend tp, const fs::path &path, size_t nCacheSize, bool fMemory, bool fWipe) {
    switch (tp) {
    case DatabaseBackend::LevelDB:
        return std::make_unique<CLevelDBImpl>(path, nCacheSize, fMemory, fWipe);
    }
    throw std::runtime_error(strprintf("Unrecognized database type: %d", static_cast<int>(tp)));
}

std::unique_ptr<IDBBatch> InternalDBFactories::MakeDBBatchImpl(DatabaseBackend tp, CDBWrapper &parent) {
    switch (tp) {
    case DatabaseBackend::LevelDB:
        return std::make_unique<CLevelDBBatch>(*static_cast<CLevelDBImpl*>(parent.GetRawDB()));
    }
    throw std::runtime_error(strprintf("Unrecognized database type: %d", static_cast<int>(tp)));
}

std::unique_ptr<IDBIterator> InternalDBFactories::MakeDBIteratorImpl(DatabaseBackend tp, CDBWrapper &parent)
{
    switch (tp) {
    case DatabaseBackend::LevelDB:
        return std::make_unique<CLevelDBIterator>(*static_cast<CLevelDBImpl*>(parent.GetRawDB()));
    }
    throw std::runtime_error(strprintf("Unrecognized database type: %d", static_cast<int>(tp)));
}

CDBBatch::CDBBatch(CDBWrapper &_parent) : parent(_parent), ssKey(SER_DISK, CLIENT_VERSION), ssValue(SER_DISK, CLIENT_VERSION), batch_impl(InternalDBFactories::MakeDBBatchImpl(GetSelectedDBType(), parent)) { }

void CDBBatch::Clear()
{
    batch_impl->Clear();
}

size_t CDBBatch::SizeEstimate() const { return batch_impl->SizeEstimate(); }

bool CDBBatch::FlushToParent(bool fSync)
{
    return batch_impl->FlushToParent(fSync);
}
