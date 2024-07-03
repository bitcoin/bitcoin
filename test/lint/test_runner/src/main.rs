// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

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

/// Return the pathspecs to exclude all subtrees
fn get_pathspecs_exclude_subtrees() -> Vec<String> {
    get_subtrees()
        .iter()
        .map(|s| format!(":(exclude){}", s))
        .collect()
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

/// Return the pathspecs for whitespace related excludes
fn get_pathspecs_exclude_whitespace() -> Vec<String> {
    let mut list = get_pathspecs_exclude_subtrees();
    list.extend(
        [
            // Permanent excludes
            "*.patch",
            "src/qt/locale",
            "contrib/windeploy/win-codesign.cert",
            "doc/README_windows.txt",
            // Temporary excludes, or existing violations
            "doc/release-notes/release-notes-0.*",
            "contrib/init/bitcoind.openrc",
            "contrib/macdeploy/macdeployqtplus",
            "src/crypto/sha256_sse4.cpp",
            "src/qt/res/src/*.svg",
            "test/functional/test_framework/crypto/ellswift_decode_test_vectors.csv",
            "test/functional/test_framework/crypto/xswiftec_inv_test_vectors.csv",
            "contrib/qos/tc.sh",
            "contrib/verify-commits/gpg.sh",
            "src/univalue/include/univalue_escapes.h",
            "src/univalue/test/object.cpp",
            "test/lint/git-subtree-check.sh",
        ]
        .iter()
        .map(|s| format!(":(exclude){}", s)),
    );
    list
}

fn lint_trailing_whitespace() -> LintResult {
    let trailing_space = git()
        .args(["grep", "-I", "--line-number", "\\s$", "--"])
        .args(get_pathspecs_exclude_whitespace())
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
        .args(get_pathspecs_exclude_whitespace())
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
                .args(get_pathspecs_exclude_subtrees())
                .args([
                    // These are exceptions which don't use bitcoin-config.h, rather the Makefile.am adds
                    // these cppflags manually.
                    ":(exclude)src/crypto/sha256_arm_shani.cpp",
                    ":(exclude)src/crypto/sha256_avx2.cpp",
                    ":(exclude)src/crypto/sha256_sse41.cpp",
                    ":(exclude)src/crypto/sha256_x86_shani.cpp",
                ]),
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
        "--gitignore",
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

Relative links are preferred (but not required) as jumping to file works natively within Emacs.

Markdown link errors found:
{}
                "#,
                stderr
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
