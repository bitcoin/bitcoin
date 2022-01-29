#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#if defined(__APPLE__)
#undef _GLIBCXX_HAVE_TIMESPEC_GET
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4127 4244)
#endif

#include <boost/filesystem.hpp>
#include <ghc/filesystem.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

using error_code = std::error_code;

#include <mw/common/Traits.h>
#include <mw/exceptions/FileException.h>

#include <fstream>

class FilePath : public Traits::IPrintable
{
    friend class File;
public:
    //
    // Constructors
    //
    FilePath(const FilePath& other) = default;
    FilePath(FilePath&& other) = default;
    FilePath(const boost::filesystem::path& path) : m_path(path.string()) {}
    FilePath(const ghc::filesystem::path& path) : m_path(path) {}
    FilePath(const char* path) : m_path(path) {}
    FilePath(const std::string& u8str) : m_path(u8str) {}

    //
    // Destructor
    //
    virtual ~FilePath() = default;

    //
    // Operators
    //
    FilePath& operator=(const FilePath& other) = default;
    FilePath& operator=(FilePath&& other) noexcept = default;
    bool operator==(const FilePath& rhs) const noexcept { return m_path == rhs.m_path; }

    FilePath GetChild(const ghc::filesystem::path& filename) const { return FilePath(m_path / filename); }
    FilePath GetChild(const char* filename) const { return FilePath(m_path / ghc::filesystem::path(filename)); }
    FilePath GetChild(const std::string& filename) const { return FilePath(m_path / ghc::filesystem::path(filename)); }

    FilePath GetParent() const
    {
        if (!m_path.has_parent_path()) {
            ThrowFile_F("Can't find parent path for {}", *this);
        }

        return FilePath(m_path.parent_path());
    }

    bool Exists() const
    {
        error_code ec;
        const bool exists = ghc::filesystem::exists(m_path, ec);
        if (ec) {
            ThrowFile_F("Error ({}) while checking if {} exists", ec.message(), *this);
        }

        return exists;
    }

    bool Exists_Safe() const noexcept
    {
        error_code ec;
        return ghc::filesystem::exists(m_path, ec);
    }

    bool IsDirectory() const
    {
        error_code ec;
        const bool isDirectory = ghc::filesystem::is_directory(m_path, ec);
        if (ec) {
            ThrowFile_F("Error ({}) while checking if {} is a directory", ec.message(), *this);
        }

        return isDirectory;
    }

    bool IsDirectory_Safe() const noexcept
    {
        error_code ec;
        return ghc::filesystem::is_directory(m_path, ec);
    }

    FilePath CreateDir() const
    {
        error_code ec;
        ghc::filesystem::create_directories(m_path, ec);
        if (ec && (!Exists_Safe() || !IsDirectory_Safe())) {
            ThrowFile_F("Error ({}) while trying to create directory {}", ec.message(), *this);
        }

        return *this;
    }

    void Remove() const
    {
        error_code ec;
        ghc::filesystem::remove_all(m_path, ec);
        if (ec && Exists_Safe()) {
            ThrowFile_F("Error ({}) while trying to remove {}", ec.message(), *this);
        }
    }

    std::string ToString() const { return m_path.u8string(); }
    boost::filesystem::path ToBoost() const { return boost::filesystem::path(m_path.u8string()); }

    //
    // Traits
    //
    std::string Format() const final { return m_path.u8string(); }

private:
    ghc::filesystem::path m_path;
};