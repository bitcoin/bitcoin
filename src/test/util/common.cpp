// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <test/util/common.h>

void SimulateFileSystemError(const fs::path& test_root_dir, const fs::path& path, const std::function<void()>& fn)
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
        fs::rename(backup, target);
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
