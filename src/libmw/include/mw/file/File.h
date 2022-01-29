#pragma once

#include <mw/common/Traits.h>
#include <mw/file/FilePath.h>
#include <unordered_map>

class File : public Traits::IPrintable
{
public:
    File(FilePath path) : m_path(std::move(path)) { }
    virtual ~File() = default;

    // Creates an empty file if it doesn't already exist
    void Create();

    const FilePath& GetPath() const noexcept { return m_path; }
    size_t GetSize() const;
    bool Exists() const;

    std::vector<uint8_t> ReadBytes() const;
    std::vector<uint8_t> ReadBytes(const size_t startIndex, const size_t numBytes) const;

    void Write(const std::vector<uint8_t>& bytes);
    void Write(const size_t startIndex, const std::vector<uint8_t>& bytes, const bool truncate);
    void WriteBytes(const std::unordered_map<uint64_t, uint8_t>& bytes);
    void Truncate(const uint64_t size);

    void CopyTo(const FilePath& new_path) const;

    //
    // Traits
    //
    std::string Format() const final { return m_path.Format(); }

private:
    FilePath m_path;
};