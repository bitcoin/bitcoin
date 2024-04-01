// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use crate::{exclude, utils, LintResult};

use std::fs;
use std::path::Path;
use std::process::Command;

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
