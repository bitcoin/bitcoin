// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::fs::File;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitCode};
use std::str;

/// A type for a complete and readable error message.
type AppError = String;
type AppResult = Result<(), AppError>;

const LLVM_PROFDATA: &str = "llvm-profdata";
const LLVM_COV: &str = "llvm-cov";
const GIT: &str = "git";

fn exit_help(err: &str) -> AppError {
    format!(
        r#"
Error: {err}

Usage: program ./build_dir boost_unittest_filter

Refer to the devtools/README.md for more details."#
    )
}

fn sanity_check(test_exe: &Path) -> AppResult {
    for tool in [LLVM_PROFDATA, LLVM_COV, GIT] {
        let output = Command::new(tool).arg("--help").output();
        match output {
            Ok(output) if output.status.success() => {}
            _ => Err(exit_help(&format!("The tool {} is not installed", tool)))?,
        }
    }
    if !test_exe.exists() {
        Err(exit_help(&format!(
            "Test executable ({}) not found",
            test_exe.display()
        )))?;
    }
    Ok(())
}

fn app() -> AppResult {
    // Parse args
    let args = env::args().collect::<Vec<_>>();
    let build_dir = args.get(1).ok_or(exit_help("Must set build dir"))?;
    if build_dir == "--help" {
        Err(exit_help("--help requested"))?;
    }
    let filter = args
        .get(2)
        // Require filter for now. In the future it could be optional and the tool could provide a
        // default filter.
        .ok_or(exit_help("Must set boost test filter"))?;
    if args.get(3).is_some() {
        Err(exit_help("Too many args"))?;
    }

    let build_dir = Path::new(build_dir);
    let test_exe = build_dir.join("bin/test_bitcoin");

    sanity_check(&test_exe)?;

    deterministic_coverage(build_dir, &test_exe, filter)
}

fn deterministic_coverage(build_dir: &Path, test_exe: &Path, filter: &str) -> AppResult {
    let profraw_file = build_dir.join("test_det_cov.profraw");
    let profdata_file = build_dir.join("test_det_cov.profdata");
    let run_single = |run_id: char| -> Result<PathBuf, AppError> {
        println!("Run with id {run_id}");
        let cov_txt_path = build_dir.join(format!("test_det_cov.show.{run_id}.txt"));
        if !Command::new(test_exe)
            .env("LLVM_PROFILE_FILE", &profraw_file)
            .env("BOOST_TEST_RUN_FILTERS", filter)
            .env("RANDOM_CTX_SEED", "21")
            .status()
            .map_err(|e| format!("test failed with {e}"))?
            .success()
        {
            Err("test failed".to_string())?;
        }
        if !Command::new(LLVM_PROFDATA)
            .arg("merge")
            .arg("--sparse")
            .arg(&profraw_file)
            .arg("-o")
            .arg(&profdata_file)
            .status()
            .map_err(|e| format!("{LLVM_PROFDATA} merge failed with {e}"))?
            .success()
        {
            Err(format!("{LLVM_PROFDATA} merge failed. This can be a sign of compiling without code coverage support."))?;
        }
        let cov_file = File::create(&cov_txt_path)
            .map_err(|e| format!("Failed to create coverage txt file ({e})"))?;
        if !Command::new(LLVM_COV)
            .args([
                "show",
                "--show-line-counts-or-regions",
                "--show-branches=count",
                "--show-expansions",
                "--show-instantiation-summary",
                "-Xdemangler=llvm-cxxfilt",
                &format!("--instr-profile={}", profdata_file.display()),
            ])
            .arg(test_exe)
            .stdout(cov_file)
            .status()
            .map_err(|e| format!("{LLVM_COV} show failed with {e}"))?
            .success()
        {
            Err(format!("{LLVM_COV} show failed"))?;
        }
        Ok(cov_txt_path)
    };
    let check_diff = |a: &Path, b: &Path| -> AppResult {
        let same = Command::new(GIT)
            .args(["--no-pager", "diff", "--no-index"])
            .arg(a)
            .arg(b)
            .status()
            .map_err(|e| format!("{GIT} diff failed with {e}"))?
            .success();
        if !same {
            Err("The coverage was not deterministic between runs.".to_string())?;
        }
        Ok(())
    };
    let r0 = run_single('a')?;
    let r1 = run_single('b')?;
    check_diff(&r0, &r1)?;
    println!("✨ The coverage was deterministic across two runs. ✨");
    Ok(())
}

fn main() -> ExitCode {
    match app() {
        Ok(()) => ExitCode::SUCCESS,
        Err(err) => {
            eprintln!("⚠️\n{}", err);
            ExitCode::FAILURE
        }
    }
}
