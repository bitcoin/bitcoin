// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::io::ErrorKind;
use std::process::{Command, Stdio};

use crate::util::{check_output, get_subtrees, git, LintResult};

pub fn lint_doc_release_note_snippets() -> LintResult {
    let non_release_notes = check_output(git().args([
        "ls-files",
        "--",
        "doc/release-notes/",
        ":(exclude)doc/release-notes/*.*.md", // Assume that at least one dot implies a proper release note
    ]))?;
    if non_release_notes.is_empty() {
        Ok(())
    } else {
        println!("{non_release_notes}");
        Err(r#"
Release note snippets and other docs must be put into the doc/ folder directly.

The doc/release-notes/ folder is for archived release notes of previous releases only. Snippets are
expected to follow the naming "/doc/release-notes-<PR number>.md".
            "#
        .trim()
        .to_string())
    }
}

pub fn lint_doc_args() -> LintResult {
    if Command::new("test/lint/check-doc.py")
        .status()
        .expect("command error")
        .success()
    {
        Ok(())
    } else {
        Err("".to_string())
    }
}

pub fn lint_markdown() -> LintResult {
    let bin_name = "mlc";
    let mut md_ignore_paths = get_subtrees();
    md_ignore_paths.push("./doc/README_doxygen.md");
    let md_ignore_path_str = md_ignore_paths.join(",");

    let mut cmd = Command::new(bin_name);
    cmd.args([
        "--offline",
        "--ignore-path",
        md_ignore_path_str.as_str(),
        "--gitignore",
        "--gituntracked",
        "--root-dir",
        ".",
    ])
    .stdout(Stdio::null()); // Suppress overly-verbose output

    match cmd.output() {
        Ok(output) if output.status.success() => Ok(()),
        Ok(output) => {
            let stderr = String::from_utf8_lossy(&output.stderr);
            Err(format!(
                r#"
One or more markdown links are broken.

Note: relative links are preferred as jump-to-file works natively within Emacs, but they are not required.

Markdown link errors found:
{stderr}
                "#
            )
            .trim()
            .to_string())
        }
        Err(e) if e.kind() == ErrorKind::NotFound => {
            println!("`mlc` was not found in $PATH, skipping markdown lint check.");
            Ok(())
        }
        Err(e) => Err(format!("Error running mlc: {e}")), // Misc errors
    }
}
