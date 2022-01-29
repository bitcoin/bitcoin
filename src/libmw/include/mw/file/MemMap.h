#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4334)
#pragma warning(disable:4018)
#endif

#include <mio/mmap.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <mw/file/File.h>
#include <cassert>

class MemMap
{
public:
    MemMap(File file) noexcept
        : m_file(std::move(file)), m_mapped(false) { }

    void Map()
    {
        assert(!m_mapped);

        if (m_file.GetSize() > 0) {
            std::error_code error;
            m_mmap = mio::make_mmap_source(m_file.GetPath().ToString(), error);
            if (error) {
                ThrowFile_F("Failed to mmap file: {}", error);
            }

            m_mapped = true;
        }
    }

    void Unmap()
    {
        if (m_mapped) {
            m_mmap.unmap();
            m_mapped = false;
        }
    }

    std::vector<uint8_t> Read(const size_t position, const size_t numBytes) const
    {
        assert(m_mapped);
        return std::vector<uint8_t>(m_mmap.cbegin() + position, m_mmap.cbegin() + position + numBytes);
    }

    uint8_t ReadByte(const size_t position) const
    {
        assert(m_mapped);
        return *((uint8_t*)(m_mmap.cbegin() + position));
    }

    bool empty() const noexcept
    {
        assert(m_mapped);
        return m_mmap.empty();
    }

    size_t size() const noexcept
    {
        return m_mmap.size();
    }

    const File& GetFile() const noexcept { return m_file; }
    File& GetFile() noexcept { return m_file; }

private:
    File m_file;
    mio::mmap_source m_mmap;
    bool m_mapped;
};