// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

mod exclude;
mod linters;
mod utils;

use linters::{get_linter_list, Linter};

use std::env;
use std::fs;
use std::process::exit;
use std::process::Command;
use std::process::ExitCode;

type LintError = String;
type LintResult = Result<(), LintError>;
type LintFn = fn() -> LintResult;

fn print_help_and_exit() {
    print!(
        r#"
Usage: test_runner [--lint=LINTER_TO_RUN]

Runs all linters in the lint test suite, printing any errors
they detect.

If you wish to only run some particular lint tests, pass
'--lint=' with the name of the lint test you wish to run.
You can set as many '--lint=' values as you wish, e.g.:
test_runner --lint=doc --lint=subtree

The individual linters available to run are:
"#
    );
    for linter in get_linter_list() {
        println!("{}: \"{}\"", linter.name, linter.description)
    }

    exit(1);
}

fn parse_lint_args(args: &[String]) -> Vec<&'static Linter> {
    let linter_list = get_linter_list();
    let mut lint_values = Vec::new();

    for arg in args {
        #[allow(clippy::if_same_then_else)]
        if arg.starts_with("--lint=") {
            let lint_name = arg
                .trim_start_matches("--lint=")
                .trim_matches('"')
                .trim_matches('\'');

            let try_find_linter = linter_list.iter().find(|linter| linter.name == lint_name);
            match try_find_linter {
                Some(linter) => {
                    lint_values.push(*linter);
                }
                None => {
                    println!("No linter {lint_name} found!");
                    print_help_and_exit();
                }
            }
        } else if arg.eq("--help") || arg.eq("-h") {
            print_help_and_exit();
        } else {
            print_help_and_exit();
        }
    }

    lint_values
}

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
            println!("^---- ⚠️ failure generated from {}", entry_fn);
        }
    }
    if good {
        Ok(())
    } else {
        Err("".to_string())
    }
}

fn main() -> ExitCode {
    let git_root = utils::get_git_root();

    let mut test_failed = false;

    let linters_to_run: Vec<&Linter> = if env::args().count() > 1 {
        let args: Vec<String> = env::args().skip(1).collect();
        parse_lint_args(&args)
    } else {
        // If no arguments are passed, run all linters.
        get_linter_list()
    };

    for linter in linters_to_run {
        // chdir to root before each lint test
        env::set_current_dir(&git_root).unwrap();
        if let Err(err) = (linter.lint_fn)() {
            println!("{err}\n^---- ⚠️ Failure generated from {}:", linter.name);
            println!("{}", linter.description);
            test_failed = true;
        }
    }

    if test_failed {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}
