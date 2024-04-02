// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::{exclude, utils, LintResult};

// Make sure we explicitly open all text files using UTF-8 (or ASCII) encoding to
// avoid potential issues on the BSDs where the locale is not always set.
pub fn encoding() -> LintResult {
    let mut encoding_error = false;
    let bad_opens = String::from_utf8(
        utils::git()
            .args(["grep", "-E"])
            .args(["-e", r#" open\("#])
            .args(["--and", "--not"])
            .args(["-e", r#"open\(.*encoding=.(ascii|utf8|utf-8)."#])
            .args(["--and", "--not"])
            .args(["-e", r#"open\([^,]*, (\*\*kwargs|['"][^'"]*b.*['"])"#])
            .args(["--", "*.py"])
            .args(exclude::get_pathspecs_exclude_python_encoding())
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for bad_open in bad_opens.lines() {
        encoding_error = true;
        println!(
            r#"
Python's open(...) seems to be used to open text files without explicitly specifying encoding='(ascii|utf8|utf-8):'
{bad_open}
"#
        )
    }

    let bad_check_outputs = String::from_utf8(
        utils::git()
            .args(["grep", "-E"])
            .args(["-e", r#" check_output\(.*text=True"#])
            .args(["--and", "--not"])
            .args(["-e", r#"check_output\(.*encoding=.(ascii|utf8|utf-8)."#])
            .args(["--", "*.py"])
            .args(exclude::get_pathspecs_exclude_python_encoding())
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for bad_check_output in bad_check_outputs.lines() {
        encoding_error = true;
        println!(
            r#"
Python's check_output(...) seems to be used to get program outputs without explicitly specifying encoding='(ascii|utf8|utf-8):'
{bad_check_output}
"#
        )
    }

    if encoding_error {
        Err(r#"
^^^
The lint check: 'python_encoding' found one or more attempts to read text files or program output without setting an encoding.

Advice not applicable in this specific case? Add an exception by updating the exceptions list.
        "#
        .to_string())
    } else {
        Ok(())
    }
}
