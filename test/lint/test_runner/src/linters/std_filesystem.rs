// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::{exclude, utils, LintResult};

pub fn std_filesystem() -> LintResult {
    let found = utils::git()
        .args(["grep", "std::filesystem", "--"])
        .args(["--", "./src"])
        .args(exclude::get_pathspecs_exclude_std_filesystem())
        .status()
        .expect("command error")
        .success();
    if found {
        Err(r#"
^^^
Direct use of std::filesystem may be dangerous and buggy. Please include <util/fs.h> and use the
fs:: namespace, which has unsafe filesystem functions marked as deleted.
            "#
        .to_string())
    } else {
        Ok(())
    }
}
