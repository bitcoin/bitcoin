// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

/// Return the pathspecs for excludes
pub fn get_pathspecs_exclude(excludes: Option<&[&str]>) -> Vec<String> {
    // Exclude all subtrees
    let mut list = super::get_subtrees()
        .iter()
        .map(|s| format!(":(exclude){}", s))
        .collect::<Vec<String>>();

    if let Some(excludes) = excludes {
        list.extend(excludes.iter().map(|s| format!(":(exclude){}", s)));
    }

    list
}

/// Return the pathspecs for std::filesystem related excludes
pub fn get_pathspecs_exclude_std_filesystem() -> Vec<String> {
    get_pathspecs_exclude(Some(&["src/util/fs.h"]))
}

/// Return the pathspecs for whitespace related excludes
pub fn get_pathspecs_exclude_whitespace() -> Vec<String> {
    get_pathspecs_exclude(Some(&[
        // Permanent excludes
        "*.patch",
        "src/qt/locale",
        "contrib/windeploy/win-codesign.cert",
        "doc/README_windows.txt",
        // Temporary excludes, or existing violations
        "doc/release-notes/release-notes-0.*",
        "contrib/init/bitcoind.openrc",
        "contrib/macdeploy/macdeployqtplus",
        "src/crypto/sha256_sse4.cpp",
        "src/qt/res/src/*.svg",
        "test/functional/test_framework/crypto/ellswift_decode_test_vectors.csv",
        "test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv",
        "contrib/qos/tc.sh",
        "contrib/verify-commits/gpg.sh",
        "src/univalue/include/univalue_escapes.h",
        "src/univalue/test/object.cpp",
        "test/lint/git-subtree-check.sh",
    ]))
}

/// Return the pathspecs for include guard related excludes
pub fn get_pathspecs_exclude_include_guards() -> Vec<String> {
    get_pathspecs_exclude(Some(&[
        "contrib/devtools/bitcoin-tidy",
        "src/tinyformat.h",
        "src/bench/nanobench.h",
        "src/test/fuzz/FuzzedDataProvider.h",
    ]))
}

/// Return the pathspecs for includes check related excludes
pub fn get_pathspecs_exclude_includes() -> Vec<String> {
    get_pathspecs_exclude(Some(&["contrib/devtools/bitcoin-tidy"]))
}

/// Return the pathspecs for build config includes related excludes
pub fn get_pathspecs_exclude_includes_build_config() -> Vec<String> {
    get_pathspecs_exclude(Some(&[
        // These are exceptions which don't use bitcoin-config.h, rather the Makefile.am adds
        // these cppflags manually.
        "src/crypto/sha256_arm_shani.cpp",
        "src/crypto/sha256_avx2.cpp",
        "src/crypto/sha256_sse41.cpp",
        "src/crypto/sha256_x86_shani.cpp",
    ]))
}

/// Return the pathspecs for locale dependence related excludes
pub fn get_pathspecs_exclude_locale_dependence() -> Vec<String> {
    get_pathspecs_exclude(Some(&["src/tinyformat.h"]))
}

/// Return the pathspecs for python encoding related excludes
pub fn get_pathspecs_exclude_python_encoding() -> Vec<String> {
    get_pathspecs_exclude(None)
}

/// Return the pathspecs for spelling check related excludes
pub fn get_pathspecs_exclude_spelling() -> Vec<String> {
    get_pathspecs_exclude(Some(&[
        "build-aux/m4/",
        "contrib/seeds/*.txt",
        "depends/",
        "doc/release-notes/",
        "src/qt/locale/",
        "src/qt/*.qrc",
        "contrib/guix/patches",
    ]))
}
