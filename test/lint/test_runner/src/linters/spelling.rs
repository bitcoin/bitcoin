// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::{exclude, utils, LintResult};

use std::io;
use std::process::Command;

const SPELLING_IGNORE_WORDS_FILE: &str = "test/lint/spelling.ignore-words.txt";

/// Prints warnings for spelling errors.
/// Note: Will always return Ok(()), even if there are spelling errors.
pub fn spelling() -> LintResult {
    // Check that we have codespell installed
    let codespell_check = Command::new("codespell").arg("--version").output();

    match codespell_check {
        Ok(_) => (),
        Err(err) => {
            if err.kind() == io::ErrorKind::NotFound {
                println!("Skipping spell check linting since codespell is not installed.");
                return Ok(());
            } else {
                panic!("command error")
            }
        }
    }

    let files_list_string = String::from_utf8(
        utils::git()
            .args(["ls-files", "--"])
            .args(exclude::get_pathspecs_exclude_spelling())
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    let files_list: Vec<&str> = files_list_string.split('\n').collect::<Vec<_>>();

    // Using Command::status() here let's codespell's output pass through to the user.
    let codespell_result = Command::new("codespell")
        .args(["--check-filenames", "--disable-colors", "--quiet-level=7"])
        .args([format!("--ignore-words={SPELLING_IGNORE_WORDS_FILE}")])
        .args(files_list)
        .status()
        .expect("command error");

    if !codespell_result.success() {
        println!(
            r#"
^^^ Warning: `codespell` identified likely spelling errors. Any false positives? Add them to the list of ignored words in {SPELLING_IGNORE_WORDS_FILE}
"#
        );
    }

    Ok(())
}
