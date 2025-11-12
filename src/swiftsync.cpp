// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <swiftsync.h>
#include <uint256.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <span>
#include <vector>

using namespace swiftsync;

Aggregate::Aggregate()
{
    std::span<unsigned char> salt;
    GetStrongRandBytes(salt);
    m_salted_hasher.write(std::as_writable_bytes(salt));
}

void Aggregate::Add(const COutPoint& outpoint)
{
    auto hash = (HashWriter(m_salted_hasher) << outpoint).GetSHA256();
    m_limb0 += hash.GetUint64(0);
    m_limb1 += hash.GetUint64(1);
    m_limb2 += hash.GetUint64(2);
    m_limb3 += hash.GetUint64(3);
}

void Aggregate::Spend(const COutPoint& outpoint)
{
    auto hash = (HashWriter(m_salted_hasher) << outpoint).GetSHA256();
    m_limb0 -= hash.GetUint64(0);
    m_limb1 -= hash.GetUint64(1);
    m_limb2 -= hash.GetUint64(2);
    m_limb3 -= hash.GetUint64(3);
}

HintsfileWriter::HintsfileWriter(AutoFile& file, const uint32_t& preallocate) : m_file(file.release())
{
    uint64_t dummy_file_pos{0};
    m_file << FILE_MAGIC;
    m_file << FILE_VERSION;
    m_file << preallocate;
    for (uint32_t height = 1; height <= preallocate; height++) {
        m_file << height;
        m_file << dummy_file_pos;
    }
}

bool HintsfileWriter::WriteNextUnspents(const std::vector<uint64_t>& unspent_offsets, const uint32_t& height)
{
    // First write the current file position for the current height in the header section.
    uint64_t curr_pos = m_file.size();
    uint64_t cursor = FILE_HEADER_LEN + (m_index * (sizeof(uint64_t) + sizeof(uint32_t)));
    m_file.seek(cursor, SEEK_SET);
    m_file << height;
    m_file << curr_pos;
    // Next append the positions of the unspent offsets in the block at this height.
    m_file.seek(curr_pos, SEEK_SET);
    WriteCompactSize(m_file, unspent_offsets.size());
    for (const auto& offset : unspent_offsets) {
        WriteCompactSize(m_file, offset);
    }
    m_index++;
    return m_file.Commit();
}

HintsfileReader::HintsfileReader(AutoFile& file) : m_file(file.release())
{
    std::array<uint8_t, 4> magic{};
    m_file >> magic;
    if (magic != FILE_MAGIC) {
        throw std::ios_base::failure("HintsfileReader: This is not a hint file.");
    }
    uint8_t version{};
    m_file >> version;
    if (version != FILE_VERSION) {
        throw std::ios_base::failure("HintsfileReader: Unsupported file version.");
    }
    m_file >> m_stop_height;
    for (uint32_t index = 1; index <= m_stop_height; index++) {
        uint32_t height;
        uint64_t file_pos;
        m_file >> height;
        m_file >> file_pos;
        m_height_to_file_pos.emplace(height, file_pos);
    }
}
HintsfileReader::HintsfileReader(HintsfileReader&& o) noexcept : m_file{o.m_file.release()}, m_stop_height{o.m_stop_height}, m_height_to_file_pos{std::move(o.m_height_to_file_pos)} {};

std::vector<uint64_t> HintsfileReader::ReadBlock(const uint32_t& height)
{
    uint64_t file_pos = m_height_to_file_pos.at(height);
    m_file.seek(file_pos, SEEK_SET);
    uint64_t num_unspents = ReadCompactSize(m_file);
    std::vector<uint64_t> offsets{};
    offsets.reserve(num_unspents);
    for (uint64_t i = 0; i < num_unspents; i++) {
        offsets.push_back(ReadCompactSize(m_file));
    }
    return offsets;
}
