// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::fs;
use std::io::ErrorKind;
use std::process::{Command, Stdio};

use crate::util::{check_output, get_pathspecs_default_excludes, get_subtrees, git, LintResult};

fn has_copyright_header(file: &str) -> bool {
    let text = fs::read_to_string(file).expect("File should be readable");
    let mut lines = text
        .lines()
        .map(|line| line.trim_start_matches("// ").trim_start_matches("# "));

    lines
        .find(|line| line.starts_with("Copyright") && line.ends_with("The Bitcoin Core developers"))
        .is_some()
        && lines
            .find(|line| line.starts_with("Distributed under the MIT software license"))
            .is_some()
        && lines
            .next()
            .is_some_and(|line| line.starts_with("file COPYING or http"))
}

pub fn lint_copyright_doc() -> LintResult {
    let files = check_output(
        git()
            .args([
                "ls-files",
                "--full-name",
                "--",
                "*.py",
                "*.cpp",
                "*.h",
                "*.sh",
                "*.rs",
            ])
            .args(get_pathspecs_default_excludes())
            .args([
                // External copyright.
                ":(exclude)contrib/devtools/clang-format-diff.py",
                ":(exclude)src/bench/nanobench.h",
                ":(exclude)src/test/fuzz/FuzzedDataProvider.h",
                ":(exclude)src/tinyformat.h",
                ":(exclude)src/univalue/lib/univalue_write.cpp",
                ":(exclude)src/util/subprocess.h",
                ":(exclude)test/functional/test_framework/authproxy.py",
                // Empty Python package marker files.
                ":(exclude,glob)**/__init__.py",
            ]),
    )
    .expect("Git should be callable");

    let missing_headers: Vec<_> = files
        .lines()
        .filter(|file| !has_copyright_header(file))
        .map(|file| format!("No copyright header found in {file}"))
        .collect();

    if !missing_headers.is_empty() {
        println!("{}", missing_headers.join("\n"));
        Err(r#"
Add the missing copyright headers, or exclude the above paths if not applicable.

Please also see the copyright section from
https://github.com/bitcoin/bitcoin/blob/master/CONTRIBUTING.md#copyright
            "#
        .trim()
        .to_string())
    } else {
        Ok(())
    }
}

pub fn lint_doc_release_note_snippets() -> LintResult {
    let non_release_notes = check_output(git().args([
        "ls-files",
        "--",
        "doc/release-notes/",
        ":(exclude)doc/release-notes/*.*.md", // Assume that at least one dot implies a proper release note
    ]))?;
    if !non_release_notes.is_empty() {
        println!("{non_release_notes}");
        return Err(r#"
Release note snippets and other docs must be put into the doc/ folder directly.

The doc/release-notes/ folder is for archived release notes of previous releases only. Snippets must
be uniquely named and placed directly in the doc/ folder.
            "#
        .trim()
        .to_string());
    }

    let snippets = check_output(git().args([
        "ls-files",
        "--",
        ":(glob)doc/release-notes-*.md",
        ":(exclude)doc/release-notes-empty-template.md",
    ]))?;
    let mut missing = Vec::new();
    for path in snippets.lines() {
        let note = fs::read_to_string(path).expect("must be able to read release note snippet");
        if !note.contains("(#") && !note.contains("(gui#") {
            missing.push(path);
        }
    }
    if !missing.is_empty() {
        return Err(format!(
            "Release note snippets must mention their PR as (#12345) or (gui#123):\n{}",
            missing.join("\n")
        ));
    }
    Ok(())
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
