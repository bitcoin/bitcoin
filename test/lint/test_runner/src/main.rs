// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

mod exclude;
mod utils;

mod linters;

use std::env;
use std::fs;
use std::process::Command;
use std::process::ExitCode;

type LintError = String;
type LintResult = Result<(), LintError>;
type LintFn = fn() -> LintResult;

fn run_all_python_linters() -> LintResult {
    let mut good = true;
    let lint_dir = utils::get_git_root().join("test/lint");
    for entry in fs::read_dir(lint_dir).unwrap() {
        let entry = entry.unwrap();
        let entry_fn = entry.file_name().into_string().unwrap();
        if entry_fn.starts_with("lint-")
            && entry_fn.ends_with(".py")
            && !Command::new("python3")
                .arg(entry.path())
                .status()
                .expect("command error")
                .success()
        {
            good = false;
            println!("^---- failure generated from {}", entry_fn);
        }
    }
    if good {
        Ok(())
    } else {
        Err("".to_string())
    }
}

fn main() -> ExitCode {
    let test_list: Vec<(&str, LintFn)> = vec![
        ("subtree check", linters::subtree::subtree),
        ("std::filesystem check", linters::std_filesystem::std_filesystem),
        ("trailing whitespace check", linters::whitespace::trailing_whitespace),
        ("no-tabs check", linters::whitespace::tabs_whitespace),
        ("build config includes check", linters::includes::includes_build_config),
        ("-help=1 documentation check", linters::doc::doc),
        ("lint-*.py scripts", run_all_python_linters),
    ];

    let git_root = utils::get_git_root();

    let mut test_failed = false;
    for (lint_name, lint_fn) in test_list {
        // chdir to root before each lint test
        env::set_current_dir(&git_root).unwrap();
        if let Err(err) = lint_fn() {
            println!("{err}\n^---- ⚠️ Failure generated from {lint_name}!");
            test_failed = true;
        }
    }
    if test_failed {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}
