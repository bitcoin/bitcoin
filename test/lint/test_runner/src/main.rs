// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::process::ExitCode;

type LintError = String;
type LintResult = Result<(), LintError>;
type LintFn = fn() -> LintResult;

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

fn lint_all() -> LintResult {
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
        ("subtree check", lint_subtree),
        ("std::filesystem check", lint_std_filesystem),
        ("trailing whitespace check", lint_trailing_whitespace),
        ("no-tabs check", lint_tabs_whitespace),
        ("build config includes check", lint_includes_build_config),
        ("-help=1 documentation check", lint_doc),
        ("lint-*.py scripts", lint_all),
    ];

    let git_root = get_git_root();

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
