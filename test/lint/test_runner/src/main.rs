// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

mod lint_cpp;
mod lint_docs;
mod lint_text_format;
mod util;

use std::env;
use std::fs;
use std::io::ErrorKind;
use std::process::{Command, ExitCode};

use lint_cpp::{
    lint_boost_assert, lint_includes_build_config, lint_rpc_assert, lint_std_filesystem,
};
use lint_docs::{lint_doc_args, lint_doc_release_note_snippets, lint_markdown};
use lint_text_format::{
    lint_commit_msg, lint_tabs_whitespace, lint_trailing_newline, lint_trailing_whitespace,
};
use util::{
    check_output, commit_range, get_git_root, get_pathspecs_default_excludes, get_subtrees, git,
    LintFn, LintResult,
};

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

fn lint_subtree() -> LintResult {
    // This only checks that the trees are pure subtrees, it is not doing a full
    // check with -r to not have to fetch all the remotes.
    let mut good = true;
    for subtree in get_subtrees() {
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

fn lint_scripted_diff() -> LintResult {
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

fn lint_py_lint() -> LintResult {
    let bin_name = "ruff";
    let checks = format!(
        "--select={}",
        [
            "B006", // mutable-argument-default
            "B008", // function-call-in-default-argument
            "E101", // indentation contains mixed spaces and tabs
            "E401", // multiple imports on one line
            "E402", // module level import not at top of file
            "E701", // multiple statements on one line (colon)
            "E702", // multiple statements on one line (semicolon)
            "E703", // statement ends with a semicolon
            "E711", // comparison to None should be 'if cond is None:'
            "E713", // test for membership should be "not in"
            "E714", // test for object identity should be "is not"
            "E721", // do not compare types, use "isinstance()"
            "E722", // do not use bare 'except'
            "E742", // do not define classes named "l", "O", or "I"
            "E743", // do not define functions named "l", "O", or "I"
            "F401", // module imported but unused
            "F402", // import module from line N shadowed by loop variable
            "F403", // 'from foo_module import *' used; unable to detect undefined names
            "F404", // future import(s) name after other statements
            "F405", // foo_function may be undefined, or defined from star imports: bar_module
            "F406", // "from module import *" only allowed at module level
            "F407", // an undefined __future__ feature name was imported
            "F541", // f-string without any placeholders
            "F601", // dictionary key name repeated with different values
            "F602", // dictionary key variable name repeated with different values
            "F621", // too many expressions in an assignment with star-unpacking
            "F631", // assertion test is a tuple, which are always True
            "F632", // use ==/!= to compare str, bytes, and int literals
            "F811", // redefinition of unused name from line N
            "F821", // undefined name 'Foo'
            "F822", // undefined name name in __all__
            "F823", // local variable name … referenced before assignment
            "F841", // local variable 'foo' is assigned to but never used
            "PLE",  // Pylint errors
            "W191", // indentation contains tabs
            "W291", // trailing whitespace
            "W292", // no newline at end of file
            "W293", // blank line contains whitespace
            "W605", // invalid escape sequence "x"
        ]
        .join(",")
    );
    let files = check_output(
        git()
            .args(["ls-files", "--", "*.py"])
            .args(get_pathspecs_default_excludes()),
    )?;

    let mut cmd = Command::new(bin_name);
    cmd.args(["check", &checks]).args(files.lines());

    match cmd.status() {
        Ok(status) if status.success() => Ok(()),
        Ok(_) => Err(format!("`{bin_name}` found errors!")),
        Err(e) if e.kind() == ErrorKind::NotFound => {
            println!(
                "`{bin_name}` was not found in $PATH, skipping those checks."
            );
            Ok(())
        }
        Err(e) => Err(format!("Error running `{bin_name}`: {e}")),
    }
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
