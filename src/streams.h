// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STREAMS_H
#define BITCOIN_STREAMS_H

#include <serialize.h>
#include <span.h>
#include <support/allocators/zeroafterfree.h>
#include <util/overflow.h>

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <ios>
#include <limits>
#include <optional>
#include <stdint.h>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

namespace util {
inline void Xor(Span<std::byte> write, Span<const std::byte> key, size_t key_offset = 0)
{
    if (key.size() == 0) {
        return;
    }
    key_offset %= key.size();

    for (size_t i = 0, j = key_offset; i != write.size(); i++) {
        write[i] ^= key[j++];

        // This potentially acts on very many bytes of data, so it's
        // important that we calculate `j`, i.e. the `key` index in this
        // way instead of doing a %, which would effectively be a division
        // for each byte Xor'd -- much slower than need be.
        if (j == key.size())
            j = 0;
    }
}
} // namespace util

/* Minimal stream for overwriting and/or appending to an existing byte vector
 *
 * The referenced vector will grow as necessary
 */
class VectorWriter
{
public:
/*
 * @param[in]  vchDataIn  Referenced byte vector to overwrite/append
 * @param[in]  nPosIn Starting position. Vector index where writes should start. The vector will initially
 *                    grow as necessary to max(nPosIn, vec.size()). So to append, use vec.size().
*/
    VectorWriter(std::vector<unsigned char>& vchDataIn, size_t nPosIn) : vchData{vchDataIn}, nPos{nPosIn}
    {
        if(nPos > vchData.size())
            vchData.resize(nPos);
    }
/*
 * (other params same as above)
 * @param[in]  args  A list of items to serialize starting at nPosIn.
*/
    template <typename... Args>
    VectorWriter(std::vector<unsigned char>& vchDataIn, size_t nPosIn, Args&&... args) : VectorWriter{vchDataIn, nPosIn}
    {
        ::SerializeMany(*this, std::forward<Args>(args)...);
    }
    void write(Span<const std::byte> src)
    {
        assert(nPos <= vchData.size());
        size_t nOverwrite = std::min(src.size(), vchData.size() - nPos);
        if (nOverwrite) {
            memcpy(vchData.data() + nPos, src.data(), nOverwrite);
        }
        if (nOverwrite < src.size()) {
            vchData.insert(vchData.end(), UCharCast(src.data()) + nOverwrite, UCharCast(src.end()));
        }
        nPos += src.size();
    }
    template <typename T>
    VectorWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

private:
    std::vector<unsigned char>& vchData;
    size_t nPos;
};

/** Minimal stream for reading from an existing byte array by Span.
 */
class SpanReader
{
private:
    Span<const unsigned char> m_data;

public:
    /**
     * @param[in]  data Referenced byte vector to overwrite/append
     */
    explicit SpanReader(Span<const unsigned char> data) : m_data{data} {}

    template<typename T>
    SpanReader& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }

    size_t size() const { return m_data.size(); }
    bool empty() const { return m_data.empty(); }

    void read(Span<std::byte> dst)
    {
        if (dst.size() == 0) {
            return;
        }

        // Read from the beginning of the buffer
        if (dst.size() > m_data.size()) {
            throw std::ios_base::failure("SpanReader::read(): end of data");
        }
        memcpy(dst.data(), m_data.data(), dst.size());
        m_data = m_data.subspan(dst.size());
    }

    void ignore(size_t n)
    {
        m_data = m_data.subspan(n);
    }
};

/** Double ended buffer combining vector and stream-like interfaces.
 *
 * >> and << read and write unformatted data using the above serialization templates.
 * Fills with data in linear time; some stringstream implementations take N^2 time.
 */
class DataStream
{
protected:
    using vector_type = SerializeData;
    vector_type vch;
    vector_type::size_type m_read_pos{0};

public:
    typedef vector_type::allocator_type   allocator_type;
    typedef vector_type::size_type        size_type;
    typedef vector_type::difference_type  difference_type;
    typedef vector_type::reference        reference;
    typedef vector_type::const_reference  const_reference;
    typedef vector_type::value_type       value_type;
    typedef vector_type::iterator         iterator;
    typedef vector_type::const_iterator   const_iterator;
    typedef vector_type::reverse_iterator reverse_iterator;

    explicit DataStream() = default;
    explicit DataStream(Span<const uint8_t> sp) : DataStream{AsBytes(sp)} {}
    explicit DataStream(Span<const value_type> sp) : vch(sp.data(), sp.data() + sp.size()) {}

    std::string str() const
    {
        return std::string{UCharCast(data()), UCharCast(data() + size())};
    }


    //
    // Vector subset
    //
    const_iterator begin() const                     { return vch.begin() + m_read_pos; }
    iterator begin()                                 { return vch.begin() + m_read_pos; }
    const_iterator end() const                       { return vch.end(); }
    iterator end()                                   { return vch.end(); }
    size_type size() const                           { return vch.size() - m_read_pos; }
    bool empty() const                               { return vch.size() == m_read_pos; }
    void resize(size_type n, value_type c = value_type{}) { vch.resize(n + m_read_pos, c); }
    void reserve(size_type n)                        { vch.reserve(n + m_read_pos); }
    const_reference operator[](size_type pos) const  { return vch[pos + m_read_pos]; }
    reference operator[](size_type pos)              { return vch[pos + m_read_pos]; }
    void clear()                                     { vch.clear(); m_read_pos = 0; }
    value_type* data()                               { return vch.data() + m_read_pos; }
    const value_type* data() const                   { return vch.data() + m_read_pos; }

    inline void Compact()
    {
        vch.erase(vch.begin(), vch.begin() + m_read_pos);
        m_read_pos = 0;
    }

    bool Rewind(std::optional<size_type> n = std::nullopt)
    {
        // Total rewind if no size is passed
        if (!n) {
            m_read_pos = 0;
            return true;
        }
        // Rewind by n characters if the buffer hasn't been compacted yet
        if (*n > m_read_pos)
            return false;
        m_read_pos -= *n;
        return true;
    }


    //
    // Stream subset
    //
    bool eof() const             { return size() == 0; }
    int in_avail() const         { return size(); }

    void read(Span<value_type> dst)
    {
        if (dst.size() == 0) return;

        // Read from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, dst.size())};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::read(): end of data");
        }
        memcpy(dst.data(), &vch[m_read_pos], dst.size());
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void ignore(size_t num_ignore)
    {
        // Ignore from the beginning of the buffer
        auto next_read_pos{CheckedAdd(m_read_pos, num_ignore)};
        if (!next_read_pos.has_value() || next_read_pos.value() > vch.size()) {
            throw std::ios_base::failure("DataStream::ignore(): end of data");
        }
        if (next_read_pos.value() == vch.size()) {
            m_read_pos = 0;
            vch.clear();
            return;
        }
        m_read_pos = next_read_pos.value();
    }

    void write(Span<const value_type> src)
    {
        // Write to the end of the buffer
        vch.insert(vch.end(), src.begin(), src.end());
    }

    template<typename T>
    DataStream& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

    template<typename T>
    DataStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return (*this);
    }

    /**
     * XOR the contents of this stream with a certain key.
     *
     * @param[in] key    The key used to XOR the data in this stream.
     */
    void Xor(const std::vector<unsigned char>& key)
    {
        util::Xor(MakeWritableByteSpan(*this), MakeByteSpan(key));
    }
};

template <typename IStream>
class BitStreamReader
{
private:
    IStream& m_istream;

    /// Buffered byte read in from the input stream. A new byte is read into the
    /// buffer when m_offset reaches 8.
    uint8_t m_buffer{0};

    /// Number of high order bits in m_buffer already returned by previous
    /// Read() calls. The next bit to be returned is at this offset from the
    /// most significant bit position.
    int m_offset{8};

public:
    explicit BitStreamReader(IStream& istream) : m_istream(istream) {}

    /** Read the specified number of bits from the stream. The data is returned
     * in the nbits least significant bits of a 64-bit uint.
     */
    uint64_t Read(int nbits) {
        if (nbits < 0 || nbits > 64) {
            throw std::out_of_range("nbits must be between 0 and 64");
        }

        uint64_t data = 0;
        while (nbits > 0) {
            if (m_offset == 8) {
                m_istream >> m_buffer;
                m_offset = 0;
            }

            int bits = std::min(8 - m_offset, nbits);
            data <<= bits;
            data |= static_cast<uint8_t>(m_buffer << m_offset) >> (8 - bits);
            m_offset += bits;
            nbits -= bits;
        }
        return data;
    }
};

template <typename OStream>
class BitStreamWriter
{
private:
    OStream& m_ostream;

    /// Buffered byte waiting to be written to the output stream. The byte is
    /// written buffer when m_offset reaches 8 or Flush() is called.
    uint8_t m_buffer{0};

    /// Number of high order bits in m_buffer already written by previous
    /// Write() calls and not yet flushed to the stream. The next bit to be
    /// written to is at this offset from the most significant bit position.
    int m_offset{0};

public:
    explicit BitStreamWriter(OStream& ostream) : m_ostream(ostream) {}

    ~BitStreamWriter()
    {
        Flush();
    }

    /** Write the nbits least significant bits of a 64-bit int to the output
     * stream. Data is buffered until it completes an octet.
     */
    void Write(uint64_t data, int nbits) {
        if (nbits < 0 || nbits > 64) {
            throw std::out_of_range("nbits must be between 0 and 64");
        }

        while (nbits > 0) {
            int bits = std::min(8 - m_offset, nbits);
            m_buffer |= (data << (64 - nbits)) >> (64 - 8 + m_offset);
            m_offset += bits;
            nbits -= bits;

            if (m_offset == 8) {
                Flush();
            }
        }
    }

    /** Flush any unwritten bits to the output stream, padding with 0's to the
     * next byte boundary.
     */
    void Flush() {
        if (m_offset == 0) {
            return;
        }

        m_ostream << m_buffer;
        m_buffer = 0;
        m_offset = 0;
    }
};

/** Non-refcounted RAII wrapper for FILE*
 *
 * Will automatically close the file when it goes out of scope if not null.
 * If you're returning the file pointer, return file.release().
 * If you need to close the file early, use file.fclose() instead of fclose(file).
 */
class AutoFile
{
protected:
    std::FILE* m_file;
    std::vector<std::byte> m_xor;
    std::optional<int64_t> m_position;

public:
    explicit AutoFile(std::FILE* file, std::vector<std::byte> data_xor={});

    ~AutoFile() { fclose(); }

    // Disallow copies
    AutoFile(const AutoFile&) = delete;
    AutoFile& operator=(const AutoFile&) = delete;

    bool feof() const { return std::feof(m_file); }

    int fclose()
    {
        if (auto rel{release()}) return std::fclose(rel);
        return 0;
    }

    /** Get wrapped FILE* with transfer of ownership.
     * @note This will invalidate the AutoFile object, and makes it the responsibility of the caller
     * of this function to clean up the returned FILE*.
     */
    std::FILE* release()
    {
        std::FILE* ret{m_file};
        m_file = nullptr;
        return ret;
    }

    /** Return true if the wrapped FILE* is nullptr, false otherwise.
     */
    bool IsNull() const { return m_file == nullptr; }

    /** Continue with a different XOR key */
    void SetXor(std::vector<std::byte> data_xor) { m_xor = data_xor; }

    /** Implementation detail, only used internally. */
    std::size_t detail_fread(Span<std::byte> dst);

    /** Wrapper around fseek(). Will throw if seeking is not possible. */
    void seek(int64_t offset, int origin);

    /** Find position within the file. Will throw if unknown. */
    int64_t tell();

    /** Wrapper around FileCommit(). */
    bool Commit();

    /** Wrapper around TruncateFile(). */
    bool Truncate(unsigned size);

    //
    // Stream subset
    //
    void read(Span<std::byte> dst);
    void ignore(size_t nSize);
    void write(Span<const std::byte> src);

    template <typename T>
    AutoFile& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }

    template <typename T>
    AutoFile& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }
};

/** Wrapper around an AutoFile& that implements a ring buffer to
 *  deserialize from. It guarantees the ability to rewind a given number of bytes.
 *
 *  Will automatically close the file when it goes out of scope if not null.
 *  If you need to close the file early, use file.fclose() instead of fclose(file).
 */
class BufferedFile
{
private:
    AutoFile& m_src;
    uint64_t nSrcPos{0};  //!< how many bytes have been read from source
    uint64_t m_read_pos{0}; //!< how many bytes have been read from this
    uint64_t nReadLimit;  //!< up to which position we're allowed to read
    uint64_t nRewind;     //!< how many bytes we guarantee to rewind
    std::vector<std::byte> vchBuf; //!< the buffer

    //! read data from the source to fill the buffer
    bool Fill() {
        unsigned int pos = nSrcPos % vchBuf.size();
        unsigned int readNow = vchBuf.size() - pos;
        unsigned int nAvail = vchBuf.size() - (nSrcPos - m_read_pos) - nRewind;
        if (nAvail < readNow)
            readNow = nAvail;
        if (readNow == 0)
            return false;
        size_t nBytes{m_src.detail_fread(Span{vchBuf}.subspan(pos, readNow))};
        if (nBytes == 0) {
            throw std::ios_base::failure{m_src.feof() ? "BufferedFile::Fill: end of file" : "BufferedFile::Fill: fread failed"};
        }
        nSrcPos += nBytes;
        return true;
    }

    //! Advance the stream's read pointer (m_read_pos) by up to 'length' bytes,
    //! filling the buffer from the file so that at least one byte is available.
    //! Return a pointer to the available buffer data and the number of bytes
    //! (which may be less than the requested length) that may be accessed
    //! beginning at that pointer.
    std::pair<std::byte*, size_t> AdvanceStream(size_t length)
    {
        assert(m_read_pos <= nSrcPos);
        if (m_read_pos + length > nReadLimit) {
            throw std::ios_base::failure("Attempt to position past buffer limit");
        }
        // If there are no bytes available, read from the file.
        if (m_read_pos == nSrcPos && length > 0) Fill();

        size_t buffer_offset{static_cast<size_t>(m_read_pos % vchBuf.size())};
        size_t buffer_available{static_cast<size_t>(vchBuf.size() - buffer_offset)};
        size_t bytes_until_source_pos{static_cast<size_t>(nSrcPos - m_read_pos)};
        size_t advance{std::min({length, buffer_available, bytes_until_source_pos})};
        m_read_pos += advance;
        return std::make_pair(&vchBuf[buffer_offset], advance);
    }

public:
    BufferedFile(AutoFile& file, uint64_t nBufSize, uint64_t nRewindIn)
        : m_src{file}, nReadLimit{std::numeric_limits<uint64_t>::max()}, nRewind{nRewindIn}, vchBuf(nBufSize, std::byte{0})
    {
        if (nRewindIn >= nBufSize)
            throw std::ios_base::failure("Rewind limit must be less than buffer size");
    }

    //! check whether we're at the end of the source file
    bool eof() const {
        return m_read_pos == nSrcPos && m_src.feof();
    }

    //! read a number of bytes
    void read(Span<std::byte> dst)
    {
        while (dst.size() > 0) {
            auto [buffer_pointer, length]{AdvanceStream(dst.size())};
            memcpy(dst.data(), buffer_pointer, length);
            dst = dst.subspan(length);
        }
    }

    //! Move the read position ahead in the stream to the given position.
    //! Use SetPos() to back up in the stream, not SkipTo().
    void SkipTo(const uint64_t file_pos)
    {
        assert(file_pos >= m_read_pos);
        while (m_read_pos < file_pos) AdvanceStream(file_pos - m_read_pos);
    }

    //! return the current reading position
    uint64_t GetPos() const {
        return m_read_pos;
    }

    //! rewind to a given reading position
    bool SetPos(uint64_t nPos) {
        size_t bufsize = vchBuf.size();
        if (nPos + bufsize < nSrcPos) {
            // rewinding too far, rewind as far as possible
            m_read_pos = nSrcPos - bufsize;
            return false;
        }
        if (nPos > nSrcPos) {
            // can't go this far forward, go as far as possible
            m_read_pos = nSrcPos;
            return false;
        }
        m_read_pos = nPos;
        return true;
    }

    //! prevent reading beyond a certain position
    //! no argument removes the limit
    bool SetLimit(uint64_t nPos = std::numeric_limits<uint64_t>::max()) {
        if (nPos < m_read_pos)
            return false;
        nReadLimit = nPos;
        return true;
    }

    template<typename T>
    BufferedFile& operator>>(T&& obj) {
        ::Unserialize(*this, obj);
        return (*this);
    }

    //! search for a given byte in the stream, and remain positioned on it
    void FindByte(std::byte byte)
    {
        // For best performance, avoid mod operation within the loop.
        size_t buf_offset{size_t(m_read_pos % uint64_t(vchBuf.size()))};
        while (true) {
            if (m_read_pos == nSrcPos) {
                // No more bytes available; read from the file into the buffer,
                // setting nSrcPos to one beyond the end of the new data.
                // Throws exception if end-of-file reached.
                Fill();
            }
            const size_t len{std::min<size_t>(vchBuf.size() - buf_offset, nSrcPos - m_read_pos)};
            const auto it_start{vchBuf.begin() + buf_offset};
            const auto it_find{std::find(it_start, it_start + len, byte)};
            const size_t inc{size_t(std::distance(it_start, it_find))};
            m_read_pos += inc;
            if (inc < len) break;
            buf_offset += inc;
            if (buf_offset >= vchBuf.size()) buf_offset = 0;
        }
    }
};

#endif // BITCOIN_STREAMS_H
