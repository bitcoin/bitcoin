// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <memusage.h>
#include <span.h>
#include <streams.h>
#include <util/fs_helpers.h>
#include <util/obfuscation.h>

#include <array>

AutoFile::AutoFile(std::FILE* file, const Obfuscation& obfuscation) : m_file{file}, m_obfuscation{obfuscation}
{
    if (!IsNull()) {
        auto pos{std::ftell(m_file)};
        if (pos >= 0) m_position = pos;
    }
}

std::size_t AutoFile::detail_fread(std::span<std::byte> dst)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::read: file handle is nullptr");
    const size_t ret = std::fread(dst.data(), 1, dst.size(), m_file);
    if (m_obfuscation) {
        if (!m_position) throw std::ios_base::failure("AutoFile::read: position unknown");
        m_obfuscation(dst.subspan(0, ret), *m_position);
    }
    if (m_position) *m_position += ret;
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

void AutoFile::read(std::span<std::byte> dst)
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

void AutoFile::write(std::span<const std::byte> src)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::write: file handle is nullptr");
    if (!m_obfuscation) {
        if (std::fwrite(src.data(), 1, src.size(), m_file) != src.size()) {
            throw std::ios_base::failure("AutoFile::write: write failed");
        }
        m_was_written = true;
        if (m_position.has_value()) *m_position += src.size();
    } else {
        std::array<std::byte, 4096> buf;
        while (src.size()) {
            auto buf_now{std::span{buf}.first(std::min<size_t>(src.size(), buf.size()))};
            std::copy_n(src.begin(), buf_now.size(), buf_now.begin());
            write_buffer(buf_now);
            src = src.subspan(buf_now.size());
        }
    }
}

void AutoFile::write_buffer(std::span<std::byte> src)
{
    if (!m_file) throw std::ios_base::failure("AutoFile::write_buffer: file handle is nullptr");
    if (m_obfuscation) {
        if (!m_position) throw std::ios_base::failure("AutoFile::write_buffer: obfuscation position unknown");
        m_obfuscation(src, *m_position); // obfuscate in-place
    }
    if (std::fwrite(src.data(), 1, src.size(), m_file) != src.size()) {
        throw std::ios_base::failure("AutoFile::write_buffer: write failed");
    }
    m_was_written = true;
    if (m_position) *m_position += src.size();
}

bool AutoFile::Commit()
{
    return ::FileCommit(m_file);
}

bool AutoFile::Truncate(unsigned size)
{
    m_was_written = true;
    return ::TruncateFile(m_file, size);
}

size_t DataStream::GetMemoryUsage() const noexcept
{
    return sizeof(*this) + memusage::DynamicUsage(vch);
}
