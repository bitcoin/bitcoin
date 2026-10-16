// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::process::Command;

use crate::util::{commit_range, get_subtrees, LintResult};

pub fn lint_subtree() -> LintResult {
    // This only checks that the trees are pure subtrees, it is not doing a full
    // check with --remote to not have to fetch all the remotes.
    //
    // Pass --incompatible to skip the "stacked on the previous subtree merge"
    // check: the linter cannot know whether an update had to be based on a later
    // commit, so it does not enforce stacking.
    let mut good = true;
    for subtree in get_subtrees() {
        good &= Command::new("test/lint/git-subtree-check.sh")
            .arg("--incompatible")
            .arg(subtree)
            .status()
            .expect("command_error")
            .success();
    }
    if good {
        Ok(())
    } else {
        Err("".to_string())
    }
}

pub fn lint_scripted_diff() -> LintResult {
    if Command::new("test/lint/commit-script-check.sh")
        .arg(commit_range())
        .status()
        .expect("command error")
        .success()
    {
        Ok(())
    } else {
        Err("".to_string())
    }
}
