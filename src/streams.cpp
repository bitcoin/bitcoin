// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <span.h>
#include <streams.h>
#include <util/fs_helpers.h>

#include <array>

AutoFile::AutoFile(std::FILE* file, std::vector<std::byte> data_xor)
    : m_file{file}, m_xor{std::move(data_xor)}
{
    if (!IsNull()) {
        auto pos{std::ftell(m_file)};
        if (pos >= 0) m_position = pos;
    }
}

std::size_t AutoFile::detail_fread(Span<std::byte> dst)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::read: file handle is nullptr");
    size_t ret = std::fread(dst.data(), 1, dst.size(), m_file);
    if (!m_xor.empty()) {
        if (!m_position.has_value()) throw std::ios_base::failure("AutoFile::read: position unknown");
        util::Xor(dst.subspan(0, ret), m_xor, *m_position);
    }
    if (m_position.has_value()) *m_position += ret;
    return ret;
}

void AutoFile::seek(int64_t offset, int origin)
{
    if (IsNull()) {
        throw std::ios_base::failure("AutoFile::seek: file handle is nullptr");
    }
    if (std::fseek(m_file, offset, origin) != 0) {
        throw std::ios_base::failure(feof() ? "AutoFile::seek: end of file" : "AutoFile::seek: fseek failed");
    }
    if (origin == SEEK_SET) {
        m_position = offset;
    } else if (origin == SEEK_CUR && m_position.has_value()) {
        *m_position += offset;
    } else {
        int64_t r{std::ftell(m_file)};
        if (r < 0) {
            throw std::ios_base::failure("AutoFile::seek: ftell failed");
        }
        m_position = r;
    }
}

int64_t AutoFile::tell()
{
    if (!m_position.has_value()) throw std::ios_base::failure("AutoFile::tell: position unknown");
    return *m_position;
}

void AutoFile::read(Span<std::byte> dst)
{
    if (detail_fread(dst) != dst.size()) {
        throw std::ios_base::failure(feof() ? "AutoFile::read: end of file" : "AutoFile::read: fread failed");
    }
}

void AutoFile::ignore(size_t nSize)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::ignore: file handle is nullptr");
    unsigned char data[4096];
    while (nSize > 0) {
        size_t nNow = std::min<size_t>(nSize, sizeof(data));
        if (std::fread(data, 1, nNow, m_file) != nNow) {
            throw std::ios_base::failure(feof() ? "AutoFile::ignore: end of file" : "AutoFile::ignore: fread failed");
        }
        nSize -= nNow;
        if (m_position.has_value()) *m_position += nNow;
    }
}

void AutoFile::write(Span<const std::byte> src)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::write: file handle is nullptr");
    if (m_xor.empty()) {
        if (std::fwrite(src.data(), 1, src.size(), m_file) != src.size()) {
            throw std::ios_base::failure("AutoFile::write: write failed");
        }
        if (m_position.has_value()) *m_position += src.size();
    } else {
        if (!m_position.has_value()) throw std::ios_base::failure("AutoFile::write: position unknown");
        std::array<std::byte, 4096> buf;
        while (src.size() > 0) {
            auto buf_now{Span{buf}.first(std::min<size_t>(src.size(), buf.size()))};
            std::copy(src.begin(), src.begin() + buf_now.size(), buf_now.begin());
            util::Xor(buf_now, m_xor, *m_position);
            if (std::fwrite(buf_now.data(), 1, buf_now.size(), m_file) != buf_now.size()) {
                throw std::ios_base::failure{"XorFile::write: failed"};
            }
            src = src.subspan(buf_now.size());
            *m_position += buf_now.size();
        }
    }
}

bool AutoFile::Commit()
{
    return ::FileCommit(m_file);
}

bool AutoFile::Truncate(unsigned size)
{
    return ::TruncateFile(m_file, size);
}
