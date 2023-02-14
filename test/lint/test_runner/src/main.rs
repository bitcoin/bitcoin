// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

use std::env;
use std::fs;
use std::fs::File;
use std::io::BufRead;
use std::io::BufReader;
use std::path::PathBuf;
use std::process::Command;
use std::process::ExitCode;

use String as LintError;

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

/// Return the all pathspecs from the given spec files
fn get_pathspecs(spec_files: &[&str]) -> Result<Vec<String>, LintError> {
    let git_root = get_git_root();
    let mut pathspecs = Vec::new();
    for pspec in spec_files {
        let pspec = fs::read_to_string(format!("{git_root}/test/lint/pathspec-{pspec}.txt"))
            .map_err(|e| format!("Could not open {pspec}: {e}"))?;
        for line in pspec.lines() {
            if !line.is_empty() && !line.starts_with('#') {
                pathspecs.push(line.to_string());
            }
        }
    }
    Ok(pathspecs)
}

/// Ensure copyright headers according to the CLA
fn lint_cla() -> Result<(), LintError> {
    let pathspecs = get_pathspecs(&["subtrees", "cla"])?;
    let files = check_output(git().args(["ls-files", "--full-name"]).args(pathspecs))?;
    let mut test_failed = false;
    for file in files.lines() {
        let reader =
            BufReader::new(File::open(file).map_err(|e| format!("Could not open {file}: {e}"))?);
        let mut remaining = 3;
        for line in reader.lines() {
            let line = line.map_err(|e| format!("Could not read line in {file}: {e}"))?;
            let line = line.trim_start_matches("// ").trim_start_matches("# ");
            if remaining == 3
                && line.starts_with("Copyright")
                && line.ends_with("The Bitcoin Core developers")
            {
                remaining = 2;
                continue;
            }
            if remaining == 2
                && line == "Distributed under the MIT software license, see the accompanying"
            {
                remaining = 1;
                continue;
            }
            if remaining == 1
                && line == "file COPYING or http://www.opensource.org/licenses/mit-license.php."
            {
                remaining = 0;
                break;
            }
        }
        if remaining == 0 {
            continue;
        } else {
            println!("No copyright header found in {file}");
            test_failed = true;
        }
    }
    if test_failed {
        Err(r#"
Exclude the above paths via test/lint/pathspec-*.txt or add the missing copyright headers.

Please also see the copyright section from
https://github.com/bitcoin/bitcoin/blob/master/CONTRIBUTING.md#copyright
            "#
        .to_string())
    } else {
        Ok(())
    }
}

fn main() -> ExitCode {
    let test_list = [("CLA check", lint_cla)];

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
