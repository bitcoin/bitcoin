#pragma once

#include <mw/file/File.h>
#include <mw/file/FilePath.h>
#include <mw/file/MemMap.h>

class AppendOnlyFile
{
public:
    using Ptr = std::shared_ptr<AppendOnlyFile>;

    AppendOnlyFile(const File& file, const size_t fileSize) noexcept
        : m_file(file),
        m_mmap(file.GetPath()),
        m_fileSize(fileSize),
        m_bufferIndex(fileSize)
    {

    }
    virtual ~AppendOnlyFile() = default;

    static AppendOnlyFile::Ptr Load(const FilePath& path)
    {
        File file(path);
        file.Create();

        auto pAppendOnlyFile = std::make_shared<AppendOnlyFile>(file, file.GetSize());
        pAppendOnlyFile->m_mmap.Map();
        return pAppendOnlyFile;
    }

    void Commit(const FilePath& new_path)
    {
        if (m_fileSize < m_bufferIndex) {
            ThrowFile_F("Buffer index is past the end of {}", m_file);
        }

        m_mmap.Unmap();

        m_file.CopyTo(new_path);
        m_file = File(new_path);

        if (m_fileSize != m_bufferIndex || !m_buffer.empty()) {
            m_file.Write(m_bufferIndex, m_buffer, true);
            m_fileSize = m_file.GetSize();
        }

        m_bufferIndex = m_fileSize;
        m_buffer.clear();

        m_mmap = MemMap{ m_file };
        m_mmap.Map();
    }

    void Rollback() noexcept
    {
        m_bufferIndex = m_fileSize;
        m_buffer.clear();
    }

    void Append(const std::vector<uint8_t>& data)
    {
        m_buffer.insert(m_buffer.end(), data.cbegin(), data.cend());
    }

    void Rewind(const uint64_t nextPosition)
    {
        assert(m_fileSize == m_bufferIndex);

        if (nextPosition > (m_bufferIndex + m_buffer.size()))
        {
            ThrowFile_F("Tried to rewind past end of {}", m_file);
        }

        if (nextPosition <= m_bufferIndex)
        {
            m_buffer.clear();
            m_bufferIndex = nextPosition;
        }
        else
        {
            m_buffer.erase(m_buffer.begin() + nextPosition - m_bufferIndex, m_buffer.end());
        }
    }

    uint64_t GetSize() const noexcept
    {
        return m_bufferIndex + m_buffer.size();
    }

    std::vector<uint8_t> Read(const uint64_t position, const uint64_t numBytes) const
    {
        if ((position + numBytes) > (m_bufferIndex + m_buffer.size()))
        {
            ThrowFile_F("Tried to read past end of {}", m_file);
        }

        if (position < m_bufferIndex)
        {
            // FUTURE: Read from mapped and then from buffer, if necessary
            return m_mmap.Read(position, numBytes);
        }
        else
        {
            auto begin = m_buffer.cbegin() + position - m_bufferIndex;
            auto end = begin + numBytes;
            return std::vector<uint8_t>(begin, end);
        }
    }

private:
    File m_file;
    MemMap m_mmap;
    uint64_t m_fileSize;

    uint64_t m_bufferIndex;
    std::vector<uint8_t> m_buffer;
};