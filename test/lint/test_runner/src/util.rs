// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::path::PathBuf;
use std::process::Command;

/// A possible error returned by any of the linters.
///
/// The error string should explain the failure type and list all violations.
pub type LintError = String;
pub type LintResult = Result<(), LintError>;
pub type LintFn = fn() -> LintResult;

/// Return the git command
///
/// Lint functions should use this command, so that only files tracked by git are considered and
/// temporary and untracked files are ignored. For example, instead of 'grep', 'git grep' should be
/// used.
pub fn git() -> Command {
    let mut git = Command::new("git");
    git.arg("--no-pager");
    git
}

/// Return stdout on success and a LintError on failure, when invalid UTF8 was detected or the
/// command did not succeed.
pub fn check_output(cmd: &mut Command) -> Result<String, LintError> {
    let out = cmd.output().expect("command error");
    if !out.status.success() {
        return Err(String::from_utf8_lossy(&out.stderr).to_string());
    }
    Ok(String::from_utf8(out.stdout)
        .map_err(|e| {
            format!("All path names, source code, messages, and output must be valid UTF8!\n{e}")
        })?
        .trim()
        .to_string())
}

/// Return the git root as utf8, or panic
pub fn get_git_root() -> PathBuf {
    PathBuf::from(check_output(git().args(["rev-parse", "--show-toplevel"])).unwrap())
}

/// Return the commit range, or panic
pub fn commit_range() -> String {
    // Use the env var, if set. E.g. COMMIT_RANGE='HEAD~n..HEAD' for the last 'n' commits.
    env::var("COMMIT_RANGE").unwrap_or_else(|_| {
        // Otherwise, assume that a merge commit exists. This merge commit is assumed
        // to be the base, after which linting will be done. If the merge commit is
        // HEAD, the range will be empty.
        format!(
            "{}..HEAD",
            check_output(git().args(["rev-list", "--max-count=1", "--merges", "HEAD"]))
                .expect("check_output failed")
        )
    })
}

/// Return all subtree paths
pub fn get_subtrees() -> Vec<&'static str> {
    // Keep in sync with [test/lint/README.md#git-subtree-checksh]
    vec![
        "src/crc32c",
        "src/crypto/ctaes",
        "src/ipc/libmultiprocess",
        "src/leveldb",
        "src/minisketch",
        "src/secp256k1",
    ]
}

/// Return the pathspecs to exclude by default
pub fn get_pathspecs_default_excludes() -> Vec<String> {
    get_subtrees()
        .iter()
        .chain(&[
            "doc/release-notes/release-notes-*", // archived notes
        ])
        .map(|s| format!(":(exclude){s}"))
        .collect()
}
