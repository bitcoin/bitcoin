// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

mod lint_cpp;
mod lint_docs;
mod lint_py;
mod lint_repo_hygiene;
mod lint_text_format;
mod util;

use std::env;
use std::fs;
use std::process::{Command, ExitCode};

use lint_cpp::{
    lint_boost_assert, lint_includes_build_config, lint_rpc_assert, lint_std_filesystem,
};
use lint_docs::{lint_doc_args, lint_doc_release_note_snippets, lint_markdown};
use lint_py::lint_py_lint;
use lint_repo_hygiene::{lint_scripted_diff, lint_subtree};
use lint_text_format::{
    lint_commit_msg, lint_tabs_whitespace, lint_trailing_newline, lint_trailing_whitespace,
};
use util::{check_output, commit_range, get_git_root, git, LintFn, LintResult};

struct Linter {
    pub description: &'static str,
    pub name: &'static str,
    pub lint_fn: LintFn,
}

fn get_linter_list() -> Vec<&'static Linter> {
    vec![
        &Linter {
            description: "Check that all command line arguments are documented.",
            name: "doc",
            lint_fn: lint_doc_args
        },
        &Linter {
            description: "Check that no symbol from bitcoin-build-config.h is used without the header being included",
            name: "includes_build_config",
            lint_fn: lint_includes_build_config
        },
        &Linter {
            description: "Check that markdown links resolve",
            name: "markdown",
            lint_fn: lint_markdown
        },
        &Linter {
            description: "Lint Python code",
            name: "py_lint",
            lint_fn: lint_py_lint,
        },
        &Linter {
            description: "Check that std::filesystem is not used directly",
            name: "std_filesystem",
            lint_fn: lint_std_filesystem
        },
        &Linter {
            description: "Check that fatal assertions are not used in RPC code",
            name: "rpc_assert",
            lint_fn: lint_rpc_assert
        },
        &Linter {
            description: "Check that boost assertions are not used",
            name: "boost_assert",
            lint_fn: lint_boost_assert
        },
        &Linter {
            description: "Check that release note snippets are in the right folder",
            name: "doc_release_note_snippets",
            lint_fn: lint_doc_release_note_snippets
        },
        &Linter {
            description: "Check that subtrees are pure subtrees",
            name: "subtree",
            lint_fn: lint_subtree
        },
        &Linter {
            description: "Check scripted-diffs",
            name: "scripted_diff",
            lint_fn: lint_scripted_diff
        },
        &Linter {
            description: "Check that commit messages have a new line before the body or no body at all.",
            name: "commit_msg",
            lint_fn: lint_commit_msg
        },
        &Linter {
            description: "Check that tabs are not used as whitespace",
            name: "tabs_whitespace",
            lint_fn: lint_tabs_whitespace
        },
        &Linter {
            description: "Check for trailing whitespace",
            name: "trailing_whitespace",
            lint_fn: lint_trailing_whitespace
        },
        &Linter {
            description: "Check for trailing newline",
            name: "trailing_newline",
            lint_fn: lint_trailing_newline
        },
        &Linter {
            description: "Run all linters of the form: test/lint/lint-*.py",
            name: "all_python_linters",
            lint_fn: run_all_python_linters
        },
    ]
}

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

    std::process::exit(1);
}

fn parse_lint_args(args: &[String]) -> Vec<&'static Linter> {
    let linter_list = get_linter_list();
    let mut lint_values = Vec::new();

    for arg in args {
        #[allow(clippy::if_same_then_else)]
        if arg.starts_with("--lint=") {
            let lint_arg_value = arg
                .trim_start_matches("--lint=")
                .trim_matches('"')
                .trim_matches('\'');

            let try_find_linter = linter_list
                .iter()
                .find(|linter| linter.name == lint_arg_value);
            match try_find_linter {
                Some(linter) => {
                    lint_values.push(*linter);
                }
                None => {
                    println!("No linter {lint_arg_value} found!");
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
    let lint_dir = get_git_root().join("test/lint");
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
            println!("^---- ⚠️ Failure generated from {entry_fn}");
        }
    }
    if good {
        Ok(())
    } else {
        Err("".to_string())
    }
}

fn main() -> ExitCode {
    let linters_to_run: Vec<&Linter> = if env::args().count() > 1 {
        let args: Vec<String> = env::args().skip(1).collect();
        parse_lint_args(&args)
    } else {
        // If no arguments are passed, run all linters.
        get_linter_list()
    };

    let git_root = get_git_root();
    let commit_range = commit_range();
    let commit_log = check_output(git().args(["log", "--no-merges", "--oneline", &commit_range]))
        .expect("check_output failed");
    println!("Checking commit range ({commit_range}):\n{commit_log}\n");

    let mut test_failed = false;
    for linter in linters_to_run {
        // chdir to root before each lint test
        env::set_current_dir(&git_root).unwrap();
        if let Err(err) = (linter.lint_fn)() {
            println!(
                "^^^\n{err}\n^---- ⚠️ Failure generated from lint check '{}' ({})!\n\n",
                linter.name, linter.description,
            );
            test_failed = true;
        }
    }
    if test_failed {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}
