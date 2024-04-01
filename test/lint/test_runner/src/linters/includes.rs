// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::{exclude, utils, LintResult};

use std::collections::HashSet;
use std::fs;
use std::path::Path;
use std::process::Command;

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
        "boost/process.hpp",
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
pub fn includes() -> LintResult {
    let lint_includes_pathspec_excludes = &exclude::get_pathspecs_exclude_includes();
    let mut includes_error = false;
    let source_files = String::from_utf8(
        utils::git()
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
        utils::git()
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
        utils::git()
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
        utils::git()
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
        let expected_boost_include_status = utils::git()
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

// Check include guards
pub fn include_guards() -> LintResult {
    let mut include_guard_error = false;
    let header_files = String::from_utf8(
        utils::git()
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

pub fn includes_build_config() -> LintResult {
    let config_path = "./src/config/bitcoin-config.h.in";
    let include_directive = "#include <config/bitcoin-config.h>";
    if !Path::new(config_path).is_file() {
        assert!(Command::new("./autogen.sh")
            .status()
            .expect("command error")
            .success());
    }
    let defines_regex = format!(
        r"^\s*(?!//).*({})",
        utils::check_output(Command::new("grep").args(["undef ", "--", config_path]))
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
        let defines_files = utils::check_output(
            utils::git()
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
        utils::git()
            .args([
                "grep",
                if mode {
                    "--files-without-match"
                } else {
                    "--files-with-matches"
                },
                include_directive,
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
