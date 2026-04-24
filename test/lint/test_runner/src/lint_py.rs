// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::io::ErrorKind;
use std::process::Command;

use crate::util::{check_output, get_pathspecs_default_excludes, git, LintResult};

pub fn lint_py_lint() -> LintResult {
    let bin_name = "ruff";
    let files = check_output(
        git()
            .args(["ls-files", "--", "*.py"])
            .args(get_pathspecs_default_excludes()),
    )?;

    let mut cmd = Command::new(bin_name);
    cmd.arg("check").args(files.lines());

    match cmd.status() {
        Ok(status) if status.success() => Ok(()),
        Ok(_) => Err(format!("`{bin_name}` found errors!")),
        Err(e) if e.kind() == ErrorKind::NotFound => {
            println!("`{bin_name}` was not found in $PATH, skipping those checks.");
            Ok(())
        }
        Err(e) => Err(format!("Error running `{bin_name}`: {e}")),
    }
}

pub fn lint_rmtree() -> LintResult {
    let found = git()
        .args([
            "grep",
            "--line-number",
            "rmtree",
            "--",
            "test/functional/",
            ":(exclude)test/functional/test_framework/test_framework.py",
        ])
        .status()
        .expect("command error")
        .success();
    if found {
        Err(r#"
Use of shutil.rmtree() is dangerous and should be avoided. If it
is really required for the test, use self.cleanup_folder(_).
            "#
        .trim()
        .to_string())
    } else {
        Ok(())
    }
}
