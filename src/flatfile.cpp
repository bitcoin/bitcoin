// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdexcept>

#include <flatfile.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/fs_helpers.h>

FlatFileSeq::FlatFileSeq(fs::path dir, const char* prefix, size_t chunk_size) :
    m_dir(std::move(dir)),
    m_prefix(prefix),
    m_chunk_size(chunk_size)
{
    if (chunk_size == 0) {
        throw std::invalid_argument("chunk_size must be positive");
    }
}

std::string FlatFilePos::ToString() const
{
    return strprintf("FlatFilePos(nFile=%i, nPos=%i)", nFile, nPos);
}

fs::path FlatFileSeq::FileName(const FlatFilePos& pos) const
{
    return m_dir / fs::u8path(strprintf("%s%05u.dat", m_prefix, pos.nFile));
}

FILE* FlatFileSeq::Open(const FlatFilePos& pos, bool read_only) const
{
    if (pos.IsNull()) {
        return nullptr;
    }
    fs::path path = FileName(pos);
    fs::create_directories(path.parent_path());
    FILE* file = fsbridge::fopen(path, read_only ? "rb": "rb+");
    if (!file && !read_only)
        file = fsbridge::fopen(path, "wb+");
    if (!file) {
        LogPrintf("Unable to open file %s\n", fs::PathToString(path));
        return nullptr;
    }
    if (pos.nPos && fseek(file, pos.nPos, SEEK_SET)) {
        LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, fs::PathToString(path));
        fclose(file);
        return nullptr;
    }
    return file;
}

size_t FlatFileSeq::Allocate(const FlatFilePos& pos, size_t add_size, bool& out_of_space) const
{
    out_of_space = false;

    unsigned int n_old_chunks = (pos.nPos + m_chunk_size - 1) / m_chunk_size;
    unsigned int n_new_chunks = (pos.nPos + add_size + m_chunk_size - 1) / m_chunk_size;
    if (n_new_chunks > n_old_chunks) {
        size_t old_size = pos.nPos;
        size_t new_size = n_new_chunks * m_chunk_size;
        size_t inc_size = new_size - old_size;

        if (CheckDiskSpace(m_dir, inc_size)) {
            FILE *file = Open(pos);
            if (file) {
                LogPrint(BCLog::VALIDATION, "Pre-allocating up to position 0x%x in %s%05u.dat\n", new_size, m_prefix, pos.nFile);
                AllocateFileRange(file, pos.nPos, inc_size);
                fclose(file);
                return inc_size;
            }
        } else {
            out_of_space = true;
        }
    }
    return 0;
}

bool FlatFileSeq::Flush(const FlatFilePos& pos, bool finalize) const
{
    FILE* file = Open(FlatFilePos(pos.nFile, 0)); // Avoid fseek to nPos
    if (!file) {
        LogError("%s: failed to open file %d\n", __func__, pos.nFile);
        return false;
    }
    if (finalize && !TruncateFile(file, pos.nPos)) {
        fclose(file);
        LogError("%s: failed to truncate file %d\n", __func__, pos.nFile);
        return false;
    }
    if (!FileCommit(file)) {
        fclose(file);
        LogError("%s: failed to commit file %d\n", __func__, pos.nFile);
        return false;
    }
    DirectoryCommit(m_dir);

    fclose(file);
    return true;
}
