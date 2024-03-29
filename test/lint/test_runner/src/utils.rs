// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::LintError;
use std::path::PathBuf;
use std::process::Command;

/// Return the git root as utf8, or panic
pub fn get_git_root() -> PathBuf {
    PathBuf::from(check_output(git().args(["rev-parse", "--show-toplevel"])).unwrap())
}

/// Return all subtree paths
pub fn get_subtrees() -> Vec<&'static str> {
    vec![
        "src/crc32c",
        "src/crypto/ctaes",
        "src/leveldb",
        "src/minisketch",
        "src/secp256k1",
    ]
}

/// Return the git command
pub fn git() -> Command {
    let mut git = Command::new("git");
    git.arg("--no-pager");
    git
}

/// Return stdout
pub fn check_output(cmd: &mut std::process::Command) -> Result<String, LintError> {
    let out = cmd.output().expect("command error");
    if !out.status.success() {
        Err(String::from_utf8_lossy(&out.stderr).to_string())
    } else {
        Ok(String::from_utf8(out.stdout)
            .map_err(|e| format!("{e}"))?
            .trim()
            .to_string())
    }
}
