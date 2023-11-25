// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::path::PathBuf;
use std::process::Command;
use std::process::ExitCode;

type LintError = String;
type LintResult = Result<(), LintError>;
type LintFn = fn() -> LintResult;

/// Return the git command
fn git() -> Command {
    Command::new("git")
}

/// Return stdout
fn check_output(cmd: &mut std::process::Command) -> Result<String, LintError> {
    let out = cmd.output().expect("command error");
    if !out.status.success() {
        return Err(String::from_utf8_lossy(&out.stderr).to_string());
    }
    Ok(String::from_utf8(out.stdout)
        .map_err(|e| format!("{e}"))?
        .trim()
        .to_string())
}

/// Return the git root as utf8, or panic
fn get_git_root() -> String {
    check_output(git().args(["rev-parse", "--show-toplevel"])).unwrap()
}

fn lint_subtree() -> LintResult {
    // This only checks that the trees are pure subtrees, it is not doing a full
    // check with -r to not have to fetch all the remotes.
    let mut good = true;
    for subtree in [
        "src/crypto/ctaes",
        "src/secp256k1",
        "src/minisketch",
        "src/leveldb",
        "src/crc32c",
    ] {
        good &= Command::new("test/lint/git-subtree-check.sh")
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

fn lint_std_filesystem() -> LintResult {
    let found = git()
        .args([
            "grep",
            "std::filesystem",
            "--",
            "./src/",
            ":(exclude)src/util/fs.h",
        ])
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

fn lint_doc() -> LintResult {
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

fn lint_all() -> LintResult {
    if Command::new("test/lint/all-lint.py")
        .status()
        .expect("command error")
        .success()
    {
        Ok(())
    } else {
        Err("".to_string())
    }
}

fn main() -> ExitCode {
    let test_list: Vec<(&str, LintFn)> = vec![
        ("subtree check", lint_subtree),
        ("std::filesystem check", lint_std_filesystem),
        ("-help=1 documentation check", lint_doc),
        ("all-lint.py script", lint_all),
    ];

    let git_root = PathBuf::from(get_git_root());

    let mut test_failed = false;
    for (lint_name, lint_fn) in test_list {
        // chdir to root before each lint test
        env::set_current_dir(&git_root).unwrap();
        if let Err(err) = lint_fn() {
            println!("{err}\n^---- Failure generated from {lint_name}!");
            test_failed = true;
        }
    }
    if test_failed {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}
