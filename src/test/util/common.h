// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COMMON_H
#define BITCOIN_TEST_UTIL_COMMON_H

#include <util/fs.h>

#include <ostream>
#include <optional>
#include <string>

#ifndef WIN32
#include <unistd.h>
#endif

/**
 * BOOST_CHECK_EXCEPTION predicates to check the specific validation error.
 * Use as
 * BOOST_CHECK_EXCEPTION(code that throws, exception type, HasReason("foo"));
 */
class HasReason
{
public:
    explicit HasReason(std::string_view reason) : m_reason(reason) {}
    bool operator()(std::string_view s) const { return s.find(m_reason) != std::string_view::npos; }
    bool operator()(const std::exception& e) const { return (*this)(e.what()); }

private:
    const std::string m_reason;
};

// Make types usable in BOOST_CHECK_* @{
namespace std {
template <typename T> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const T& e)
{
    return os << static_cast<std::underlying_type_t<T>>(e);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    return v ? os << *v
             : os << "std::nullopt";
}
} // namespace std

template <typename T>
concept HasToString = requires(const T& t) { t.ToString(); };

template <HasToString T>
inline std::ostream& operator<<(std::ostream& os, const T& obj)
{
    return os << obj.ToString();
}

// @}

/**
 * Simulate a filesystem access failure for `path` by temporarily making it
 * unavailable. The target is removed and its parent made inaccessible while
 * `fn()` executes, then everything is restored.
 */
template <typename Fn>
void SimulateFileSystemError(const fs::path& test_root_dir, const fs::path& path, Fn&& fn)
{
#ifdef WIN32
    // On Windows, any open file (such as the db) prevents directory renaming,
    // so we can't simulate a filesystem error in this platform. Skip it.
    return;
#else
    // This check relies on filesystem permission manipulation to simulate I/O
    // failures. Running as root bypasses permission checks, so the OS will
    // allow directory creation and file writes even when perms are set to
    // none, making it impossible to simulate the expected failures.
    if (getuid() == 0) return;
 #endif

    const fs::path root = fs::weakly_canonical(test_root_dir);
    const fs::path target = fs::weakly_canonical(path);

    // Simple sanity check: ensure target is inside the test directory.
    if (!fs::PathToString(target).starts_with(fs::PathToString(root))) {
        throw std::runtime_error("Path escapes test root directory");
    }

    if (!fs::exists(target)) {
        throw std::runtime_error("Path does not exist");
    }

    const fs::path parent = target.parent_path();
    const fs::path backup = parent / fs::PathFromString(target.filename().string() + "_bak");

    // Make the target disappear (file or directory)
    fs::rename(target, backup);

    const auto old_perms = fs::status(parent).permissions();

    // Helper to restore state (permissions + original path)
    auto restore = [&]() {
        fs::permissions(parent, old_perms, fs::perm_options::replace);
        if (fs::exists(backup) && !fs::exists(target)) {
            fs::rename(backup, target);
        }
    };

    // Block any access and dir recreation under the parent directory
    fs::permissions(parent, fs::perms::none, fs::perm_options::replace);

    try {
        fn(); // run test under simulated failure
    } catch (...) {
        restore();
        throw;
    }

    restore();
}

#endif // BITCOIN_TEST_UTIL_COMMON_H
