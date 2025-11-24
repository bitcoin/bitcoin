// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <swiftsync.h>
#include <uint256.h>
#include <util/fs.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <ios>
#include <utility>
#include <vector>

using namespace swiftsync;

void BlockHintsWriter::WriteBit(uint8_t bit) noexcept
{
    uint8_t shift = m_bits_written % 8;
    m_curr_byte |= bit << shift;
    ++m_bits_written;
    if (m_bits_written % 8 == 0) {
        m_hints.push_back(m_curr_byte);
        m_curr_byte = 0x00;
    }
}

bool BlockHintsWriter::EncodeToFile(AutoFile& file)
{
    if (m_bits_written % 8 != 0) {
        m_hints.push_back(m_curr_byte);
    }
    file << m_bits_written;
    for (const uint8_t packed : m_hints) {
        file << packed;
    }
    return file.Commit();
}

HintsfileWriter::HintsfileWriter(AutoFile& file, uint32_t preallocate) : m_file(file.release())
{
    uint64_t dummy_file_pos{};
    m_file << FILE_MAGIC;
    m_file << FILE_VERSION;
    m_file << preallocate;
    for (uint32_t height{}; height < preallocate; ++height) {
        m_file << height;
        m_file << dummy_file_pos;
    }
}

bool HintsfileWriter::WriteNextUnspents(BlockHintsWriter& hints, uint32_t height)
{
    // First write the current file position for the current height in the header section.
    uint64_t curr_pos = m_file.size();
    uint64_t cursor = FILE_HEADER_LEN + (m_index * (sizeof(uint64_t) + sizeof(uint32_t)));
    m_file.seek(cursor, SEEK_SET);
    m_file << height;
    m_file << curr_pos;
    // Next append the positions of the unspent offsets in the block at this height.
    ++m_index;
    m_file.seek(curr_pos, SEEK_SET);
    return hints.EncodeToFile(m_file);
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
    for (uint32_t index{}; index < m_stop_height; ++index) {
        uint32_t height;
        uint64_t file_pos;
        m_file >> height;
        m_file >> file_pos;
        m_height_to_file_pos.emplace(height, file_pos);
    }
}
HintsfileReader::HintsfileReader(HintsfileReader&& o) noexcept : m_file{o.m_file.release()}, m_stop_height{o.m_stop_height}, m_height_to_file_pos{std::move(o.m_height_to_file_pos)} {};

BlockHints HintsfileReader::ReadBlock(uint32_t height)
{
    uint64_t file_pos = m_height_to_file_pos.at(height);
    m_file.seek(file_pos, SEEK_SET);
    uint32_t num_hints{};
    m_file >> num_hints;
    std::vector<bool> block_hints{};
    block_hints.reserve(num_hints);
    uint8_t curr_byte{};
    for (uint32_t i{}; i < num_hints; ++i) {
        uint8_t leftover = i % 8;
        if (leftover == 0) {
            m_file >> curr_byte;
        }
        block_hints.push_back((curr_byte >> leftover) & 0x01);
    }
    return block_hints;
}

void Context::ApplyHints(HintsfileReader reader)
{
    m_hint_reader.emplace(std::move(reader));
}

BlockHints Context::ReadBlockHints(int nHeight)
{
    return m_hint_reader->ReadBlock(nHeight);
}
