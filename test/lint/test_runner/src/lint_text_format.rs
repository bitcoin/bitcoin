// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::fs::File;
use std::io::{Read, Seek, SeekFrom};

use crate::util::{check_output, commit_range, get_pathspecs_default_excludes, git, LintResult};

/// Return the pathspecs for whitespace related excludes
fn get_pathspecs_exclude_whitespace() -> Vec<String> {
    let mut list = get_pathspecs_default_excludes();
    list.extend(
        [
            // Permanent excludes
            "*.patch",
            "src/qt/locale",
            "contrib/windeploy/win-codesign.cert",
            "doc/README_windows.txt",
            // Temporary excludes, or existing violations
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
        ]
        .iter()
        .map(|s| format!(":(exclude){s}")),
    );
    list
}

pub fn lint_trailing_whitespace() -> LintResult {
    let trailing_space = git()
        .args(["grep", "-I", "--line-number", "\\s$", "--"])
        .args(get_pathspecs_exclude_whitespace())
        .status()
        .expect("command error")
        .success();
    if trailing_space {
        Err(r#"
Trailing whitespace (including Windows line endings [CR LF]) is problematic, because git may warn
about it, or editors may remove it by default, forcing developers in the future to either undo the
changes manually or spend time on review.

Thus, it is best to remove the trailing space now.

Please add any false positives, such as subtrees, Windows-related files, patch files, or externally
sourced files to the exclude list.
            "#
        .trim()
        .to_string())
    } else {
        Ok(())
    }
}

pub fn lint_trailing_newline() -> LintResult {
    let files = check_output(
        git()
            .args([
                "ls-files", "--", "*.py", "*.cpp", "*.h", "*.md", "*.rs", "*.sh", "*.cmake",
            ])
            .args(get_pathspecs_default_excludes()),
    )?;
    let mut missing_newline = false;
    for path in files.lines() {
        let mut file = File::open(path).expect("must be able to open file");
        if file.seek(SeekFrom::End(-1)).is_err() {
            continue; // Allow fully empty files
        }
        let mut buffer = [0u8; 1];
        file.read_exact(&mut buffer)
            .expect("must be able to read the last byte");
        if buffer[0] != b'\n' {
            missing_newline = true;
            println!("{path}");
        }
    }
    if missing_newline {
        Err(r#"
A trailing newline is required, because git may warn about it missing. Also, it can make diffs
verbose and can break git blame after appending lines.

Thus, it is best to add the trailing newline now.

Please add any false positives to the exclude list.
            "#
        .trim()
        .to_string())
    } else {
        Ok(())
    }
}

pub fn lint_tabs_whitespace() -> LintResult {
    let tabs = git()
        .args(["grep", "-I", "--line-number", "--perl-regexp", "^\\t", "--"])
        .args(["*.cpp", "*.h", "*.md", "*.py", "*.sh"])
        .args(get_pathspecs_exclude_whitespace())
        .status()
        .expect("command error")
        .success();
    if tabs {
        Err(r#"
Use of tabs in this codebase is problematic, because existing code uses spaces and tabs will cause
display issues and conflict with editor settings.

Please remove the tabs.

Please add any false positives, such as subtrees, or externally sourced files to the exclude list.
            "#
        .trim()
        .to_string())
    } else {
        Ok(())
    }
}

pub fn lint_commit_msg() -> LintResult {
    let mut good = true;
    let commit_hashes = check_output(git().args([
        "-c",
        "log.showSignature=false",
        "log",
        &commit_range(),
        "--format=%H",
    ]))?;
    for hash in commit_hashes.lines() {
        let commit_info = check_output(git().args([
            "-c",
            "log.showSignature=false",
            "log",
            "--format=%B",
            "-n",
            "1",
            hash,
        ]))?;
        if let Some(line) = commit_info.lines().nth(1) {
            if !line.is_empty() {
                println!(
                        "The subject line of commit hash {hash} is followed by a non-empty line. Subject lines should always be followed by a blank line."
                    );
                good = false;
            }
        }
    }
    if good {
        Ok(())
    } else {
        Err("".to_string())
    }
}
