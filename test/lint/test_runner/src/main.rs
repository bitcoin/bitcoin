// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

mod exclude;

use std::collections::HashSet;
use std::env;
use std::fs;
use std::io::ErrorKind;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitCode, Stdio};

type LintError = String;
type LintResult = Result<(), LintError>;
type LintFn = fn() -> LintResult;

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
            lint_fn: lint_doc
        },
        &Linter {
            description: "Check that header files have include guards",
            name: "include_guards",
            lint_fn: lint_include_guards
        },
        &Linter {
            description: "Check for duplicate includes, includes of .cpp files, new boost includes, and quote syntax includes.",
            name: "includes",
            lint_fn: lint_includes
        },
        &Linter {
            description: "Check that no symbol from bitcoin-config.h is used without the header being included",
            name: "includes_build_config",
            lint_fn: lint_includes_build_config
        },
        &Linter {
            description: "Check that markdown links resolve",
            name: "markdown",
            lint_fn: lint_markdown
        },
        &Linter {
            description: "Check that std::filesystem is not used directly",
            name: "std_filesystem",
            lint_fn: lint_std_filesystem
        },
        &Linter {
            description: "Check that subtrees are pure subtrees",
            name: "subtree",
            lint_fn: lint_subtree
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

/// Return the git command
fn git() -> Command {
    let mut git = Command::new("git");
    git.arg("--no-pager");
    git
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
fn get_git_root() -> PathBuf {
    PathBuf::from(check_output(git().args(["rev-parse", "--show-toplevel"])).unwrap())
}

/// Return all subtree paths
fn get_subtrees() -> Vec<&'static str> {
    vec![
        "src/crc32c",
        "src/crypto/ctaes",
        "src/leveldb",
        "src/minisketch",
        "src/secp256k1",
    ]
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

fn lint_std_filesystem() -> LintResult {
    let found = git()
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

fn lint_trailing_whitespace() -> LintResult {
    let trailing_space = git()
        .args(["grep", "-I", "--line-number", "\\s$", "--"])
        .args(exclude::get_pathspecs_exclude_whitespace())
        .status()
        .expect("command error")
        .success();
    if trailing_space {
        Err(r#"
^^^
Trailing whitespace (including Windows line endings [CR LF]) is problematic, because git may warn
about it, or editors may remove it by default, forcing developers in the future to either undo the
changes manually or spend time on review.

Thus, it is best to remove the trailing space now.

Please add any false positives, such as subtrees, Windows-related files, patch files, or externally
sourced files to the exclude list.
            "#
        .to_string())
    } else {
        Ok(())
    }
}

fn lint_tabs_whitespace() -> LintResult {
    let tabs = git()
        .args(["grep", "-I", "--line-number", "--perl-regexp", "^\\t", "--"])
        .args(["*.cpp", "*.h", "*.md", "*.py", "*.sh"])
        .args(exclude::get_pathspecs_exclude_whitespace())
        .status()
        .expect("command error")
        .success();
    if tabs {
        Err(r#"
^^^
Use of tabs in this codebase is problematic, because existing code uses spaces and tabs will cause
display issues and conflict with editor settings.

Please remove the tabs.

Please add any false positives, such as subtrees, or externally sourced files to the exclude list.
            "#
        .to_string())
    } else {
        Ok(())
    }
}

// Check include guards
fn lint_include_guards() -> LintResult {
    let mut include_guard_error = false;
    let header_files = String::from_utf8(
        git()
            .args(["ls-files", "--", "*.h"])
            .args(exclude::get_pathspecs_exclude_include_guards())
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for header_file in header_files.lines() {
        // src/wallet/fees.h -> BITCOIN_WALLET_FEES_H
        let header_id = format!(
            "BITCOIN_{}",
            header_file.split('/').collect::<Vec<_>>()[1..]
                .join("_")
                .replace(['-', '.'], "_")
                .to_uppercase()
        );

        let header_file_body =
            fs::read_to_string(header_file).expect("Failure opening header file");

        let include_guards = [
            format!("#ifndef {header_id}"),
            format!("#define {header_id}"),
            format!("#endif // {header_id}"),
        ];

        let mut guard_index = 0;
        for header_file_line in header_file_body.lines() {
            if guard_index >= 3 {
                break;
            }
            if header_file_line == include_guards[guard_index] {
                guard_index += 1;
            }
        }

        if guard_index != 3 {
            include_guard_error = true;
            println!(
                r#"
{header_file} seems to be missing the expected include guard:
{}
{}
{}
                "#,
                include_guards[0], include_guards[1], include_guards[2]
            )
        }
    }

    if include_guard_error {
        Err(r#"
^^^
One or more include guards are missing.
"#
        .to_string())
    } else {
        Ok(())
    }
}

// These are the boost files we expect to be included, if any other boost files
// are included `includes()` will complain
fn get_expected_boost_includes() -> Vec<String> {
    vec![
        "boost/date_time/posix_time/posix_time.hpp",
        "boost/multi_index/detail/hash_index_iterator.hpp",
        "boost/multi_index/hashed_index.hpp",
        "boost/multi_index/identity.hpp",
        "boost/multi_index/indexed_by.hpp",
        "boost/multi_index/ordered_index.hpp",
        "boost/multi_index/sequenced_index.hpp",
        "boost/multi_index/tag.hpp",
        "boost/multi_index_container.hpp",
        "boost/operators.hpp",
        "boost/signals2/connection.hpp",
        "boost/signals2/optional_last_value.hpp",
        "boost/signals2/signal.hpp",
        "boost/test/included/unit_test.hpp",
        "boost/test/unit_test.hpp",
        "boost/tuple/tuple.hpp",
    ]
    .iter()
    .map(|s| format!("#include <{s}>"))
    .collect()
}

// Check for duplicate includes, and includes of .cpp files.
// Guard against accidental introduction of new Boost dependencies.
// Enforce bracket syntax includes.
fn lint_includes() -> LintResult {
    let lint_includes_pathspec_excludes = &exclude::get_pathspecs_exclude_includes();
    let mut includes_error = false;
    let source_files = String::from_utf8(
        git()
            .args(["ls-files", "src/*.h", "src/*.cpp", "--"])
            .args(lint_includes_pathspec_excludes)
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    // Ensure any include only appears once in a source file.
    for source_file in source_files.lines() {
        let source_file_body =
            fs::read_to_string(source_file).expect("Failure opening source file");

        let mut include_set = HashSet::new();
        for source_line in source_file_body.lines() {
            if source_line.starts_with("#include") && !include_set.insert(source_line) {
                includes_error = true;
                println!("Duplicate include found in {source_file}:");
                println!("{source_line}\n");
            }
        }
    }

    // Ensure no `.cpp` files are included.
    let cpp_includes = String::from_utf8(
        git()
            .args(["grep", "-E"])
            .args([r#"^#include [<\"][^>\"]+\.cpp[>\"]"#])
            .args(["--", "*.cpp", "*.h"])
            .args(lint_includes_pathspec_excludes)
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for cpp_include in cpp_includes.lines() {
        includes_error = true;
        println!(".cpp file was used in an #include directive:");
        println!("{cpp_include}\n");
    }

    // Enforce bracket syntax includes.
    let quote_syntax_includes = String::from_utf8(
        git()
            .args(["grep", "-E"])
            .args([r#"^#include \""#])
            .args(["--", "*.cpp", "*.h"])
            .args(lint_includes_pathspec_excludes)
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for quote_syntax_include in quote_syntax_includes.lines() {
        includes_error = true;
        println!(
            r#"
Please use bracket syntax includes ("\#include <foo.h>") instead of quote syntax includes:
{quote_syntax_include}
"#
        );
    }

    let expected_boost_includes = get_expected_boost_includes();

    // Ensure no new Boost dependencies are introduced
    let boost_includes = String::from_utf8(
        git()
            .args(["grep", "-E"])
            .args([r#"^#include <boost/"#])
            .args(["--", "*.cpp", "*.h"])
            .args(lint_includes_pathspec_excludes)
            .output()
            .expect("command error")
            .stdout,
    )
    .expect("error reading stdout");

    for boost_include in boost_includes.lines() {
        // Trim to "#include <...>"
        let trimmed_boost_include = boost_include
            .trim_start_matches(|c| c != '#')
            .trim_end_matches(|c| c != '>')
            .to_string();

        if !expected_boost_includes.contains(&trimmed_boost_include) {
            includes_error = true;
            println!(
                r#"
A new boost dependency {trimmed_boost_include} was introduced:
{boost_include}
"#
            );
        }
    }

    // Check if a Boost dependency has been removed
    for expected_boost_include in &expected_boost_includes {
        let expected_boost_include_status = git()
            .args(["grep", "-q"])
            .args([format!("^{expected_boost_include}")])
            .args(["--", "*.cpp", "*.h"])
            .args(lint_includes_pathspec_excludes)
            .status()
            .expect("command error");

        // git grep -q signals that a match exists with an exit code of zero,
        // Otherwise it exits with non-zero code.
        if !expected_boost_include_status.success() {
            includes_error = true;
            println!(
                r#"
Good job! The Boost dependency "{expected_boost_include}" is no longer used.
Please remove it from get_expected_boost_includes` in `test/lint/test_runner/src/main.rs`
to make sure this dependency is not accidentally reintroduced.\n)
"#
            );
        }
    }

    if includes_error {
        Err(r#"
^^^
The linter found one or more errors related to the use of includes.
        "#
        .to_string())
    } else {
        Ok(())
    }
}

fn lint_includes_build_config() -> LintResult {
    let config_path = "./src/config/bitcoin-config.h.in";
    if !Path::new(config_path).is_file() {
        assert!(Command::new("./autogen.sh")
            .status()
            .expect("command error")
            .success());
    }
    let defines_regex = format!(
        r"^\s*(?!//).*({})",
        check_output(Command::new("grep").args(["undef ", "--", config_path]))
            .expect("grep failed")
            .lines()
            .map(|line| {
                line.split("undef ")
                    .nth(1)
                    .unwrap_or_else(|| panic!("Could not extract name in line: {line}"))
            })
            .collect::<Vec<_>>()
            .join("|")
    );
    let print_affected_files = |mode: bool| {
        // * mode==true: Print files which use the define, but lack the include
        // * mode==false: Print files which lack the define, but use the include
        let defines_files = check_output(
            git()
                .args([
                    "grep",
                    "--perl-regexp",
                    if mode {
                        "--files-with-matches"
                    } else {
                        "--files-without-match"
                    },
                    &defines_regex,
                    "--",
                    "*.cpp",
                    "*.h",
                ])
                .args(exclude::get_pathspecs_exclude_includes_build_config()),
        )
        .expect("grep failed");
        git()
            .args([
                "grep",
                if mode {
                    "--files-without-match"
                } else {
                    "--files-with-matches"
                },
                if mode {
                    "^#include <config/bitcoin-config.h> // IWYU pragma: keep$"
                } else {
                    "#include <config/bitcoin-config.h>" // Catch redundant includes with and without the IWYU pragma
                },
                "--",
            ])
            .args(defines_files.lines())
            .status()
            .expect("command error")
            .success()
    };
    let missing = print_affected_files(true);
    if missing {
        return Err(format!(
            r#"
^^^
One or more files use a symbol declared in the bitcoin-config.h header. However, they are not
including the header. This is problematic, because the header may or may not be indirectly
included. If the indirect include were to be intentionally or accidentally removed, the build could
still succeed, but silently be buggy. For example, a slower fallback algorithm could be picked,
even though bitcoin-config.h indicates that a faster feature is available and should be used.

If you are unsure which symbol is used, you can find it with this command:
git grep --perl-regexp '{}' -- file_name

Make sure to include it with the IWYU pragma. Otherwise, IWYU may falsely instruct to remove the
include again.

#include <config/bitcoin-config.h> // IWYU pragma: keep
            "#,
            defines_regex
        ));
    }
    let redundant = print_affected_files(false);
    if redundant {
        return Err(r#"
^^^
None of the files use a symbol declared in the bitcoin-config.h header. However, they are including
the header. Consider removing the unused include.
            "#
        .to_string());
    }
    Ok(())
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

fn lint_markdown() -> LintResult {
    let bin_name = "mlc";
    let mut md_ignore_paths = get_subtrees();
    md_ignore_paths.push("./doc/README_doxygen.md");
    let md_ignore_path_str = md_ignore_paths.join(",");

    let mut cmd = Command::new(bin_name);
    cmd.args([
        "--offline",
        "--ignore-path",
        md_ignore_path_str.as_str(),
        "--root-dir",
        ".",
    ])
    .stdout(Stdio::null()); // Suppress overly-verbose output

    match cmd.output() {
        Ok(output) if output.status.success() => Ok(()),
        Ok(output) => {
            let stderr = String::from_utf8_lossy(&output.stderr);
            let filtered_stderr: String = stderr // Filter out this annoying trailing line
                .lines()
                .filter(|&line| line != "The following links could not be resolved:")
                .collect::<Vec<&str>>()
                .join("\n");
            Err(format!(
                r#"
One or more markdown links are broken.

Relative links are preferred (but not required) as jumping to file works natively within Emacs.

Markdown link errors found:
{}
                "#,
                filtered_stderr
            ))
        }
        Err(e) if e.kind() == ErrorKind::NotFound => {
            println!("`mlc` was not found in $PATH, skipping markdown lint check.");
            Ok(())
        }
        Err(e) => Err(format!("Error running mlc: {}", e)), // Misc errors
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
            println!("^---- ⚠️ Failure generated from {}", entry_fn);
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

    let mut test_failed = false;
    for linter in linters_to_run {
        // chdir to root before each lint test
        env::set_current_dir(&git_root).unwrap();
        if let Err(err) = (linter.lint_fn)() {
            println!(
                "{err}\n^---- ⚠️ Failure generated from lint check '{}'!",
                linter.name
            );
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
