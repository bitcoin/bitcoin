// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FLATFILE_H
#define BITCOIN_FLATFILE_H

#include <string>

#include <fs.h>
#include <serialize.h>

struct FlatFilePos
{
    int nFile;
    unsigned int nPos;

    SERIALIZE_METHODS(FlatFilePos, obj)
    {
        READWRITE(VARINT(obj.nFile, VarIntMode::NONNEGATIVE_SIGNED), VARINT(obj.nPos));
    }

    FlatFilePos() : nFile(-1), nPos(0) {}

    FlatFilePos(int nFileIn, unsigned int nPosIn) :
        nFile(nFileIn),
        nPos(nPosIn)
    {}

    friend bool operator==(const FlatFilePos &a, const FlatFilePos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

    friend bool operator!=(const FlatFilePos &a, const FlatFilePos &b) {
        return !(a == b);
    }

    void SetNull() { nFile = -1; nPos = 0; }
    bool IsNull() const { return (nFile == -1); }

    std::string ToString() const;
};

/**
 * FlatFileSeq represents a sequence of numbered files storing raw data. This class facilitates
 * access to and efficient management of these files.
 */
class FlatFileSeq
{
private:
    const fs::path m_dir;
    const char* const m_prefix;
    const size_t m_chunk_size;

public:
    /**
     * Constructor
     *
     * @param dir The base directory that all files live in.
     * @param prefix A short prefix given to all file names.
     * @param chunk_size Disk space is pre-allocated in multiples of this amount.
     */
    FlatFileSeq(fs::path dir, const char* prefix, size_t chunk_size);

    /** Get the name of the file at the given position. */
    fs::path FileName(const FlatFilePos& pos) const;

    /** Open a handle to the file at the given position. */
    FILE* Open(const FlatFilePos& pos, bool read_only = false);

    /**
     * Allocate additional space in a file after the given starting position. The amount allocated
     * will be the minimum multiple of the sequence chunk size greater than add_size.
     *
     * @param[in] pos The starting position that bytes will be allocated after.
     * @param[in] add_size The minimum number of bytes to be allocated.
     * @param[out] out_of_space Whether the allocation failed due to insufficient disk space.
     * @return The number of bytes successfully allocated.
     */
    size_t Allocate(const FlatFilePos& pos, size_t add_size, bool& out_of_space);

    /**
     * Commit a file to disk, and optionally truncate off extra pre-allocated bytes if final.
     *
     * @param[in] pos The first unwritten position in the file to be flushed.
     * @param[in] finalize True if no more data will be written to this file.
     * @return true on success, false on failure.
     */
    bool Flush(const FlatFilePos& pos, bool finalize = false);
};

#endif // BITCOIN_FLATFILE_H
