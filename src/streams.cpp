// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <span.h>
#include <streams.h>

std::size_t AutoFile::detail_fread(Span<std::byte> dst)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::read: file handle is nullptr");
    return std::fread(dst.data(), 1, dst.size(), m_file);
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
    }
}

void AutoFile::write(Span<const std::byte> src)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::write: file handle is nullptr");
    if (std::fwrite(src.data(), 1, src.size(), m_file) != src.size()) {
        throw std::ios_base::failure("AutoFile::write: write failed");
    }
}
