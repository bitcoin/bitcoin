// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::fs::{read_dir, File};
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

Usage: program ./build_dir ./qa-assets/fuzz_corpora fuzz_target_name

Refer to the devtools/README.md for more details."#
    )
}

fn sanity_check(corpora_dir: &Path, fuzz_exe: &Path) -> AppResult {
    for tool in [LLVM_PROFDATA, LLVM_COV, GIT] {
        let output = Command::new(tool).arg("--help").output();
        match output {
            Ok(output) if output.status.success() => {}
            _ => Err(exit_help(&format!("The tool {} is not installed", tool)))?,
        }
    }
    if !corpora_dir.is_dir() {
        Err(exit_help(&format!(
            "Fuzz corpora path ({}) must be a directory",
            corpora_dir.display()
        )))?;
    }
    if !fuzz_exe.exists() {
        Err(exit_help(&format!(
            "Fuzz executable ({}) not found",
            fuzz_exe.display()
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
    let corpora_dir = args.get(2).ok_or(exit_help("Must set fuzz corpora dir"))?;
    let fuzz_target = args
        .get(3)
        // Require fuzz target for now. In the future it could be optional and the tool could
        // iterate over all compiled fuzz targets
        .ok_or(exit_help("Must set fuzz target"))?;
    if args.get(4).is_some() {
        Err(exit_help("Too many args"))?;
    }

    let build_dir = Path::new(build_dir);
    let corpora_dir = Path::new(corpora_dir);
    let fuzz_exe = build_dir.join("bin/fuzz");

    sanity_check(corpora_dir, &fuzz_exe)?;

    deterministic_coverage(build_dir, corpora_dir, &fuzz_exe, fuzz_target)
}

fn using_libfuzzer(fuzz_exe: &Path) -> Result<bool, AppError> {
    println!("Check if using libFuzzer ...");
    let stderr = Command::new(fuzz_exe)
        .arg("-help=1") // Will be interpreted as option (libfuzzer) or as input file
        .env("FUZZ", "addition_overflow") // Any valid target
        .output()
        .map_err(|e| format!("fuzz failed with {e}"))?
        .stderr;
    let help_output = str::from_utf8(&stderr)
        .map_err(|e| format!("The libFuzzer -help=1 output must be valid text ({e})"))?;
    Ok(help_output.contains("libFuzzer"))
}

fn deterministic_coverage(
    build_dir: &Path,
    corpora_dir: &Path,
    fuzz_exe: &Path,
    fuzz_target: &str,
) -> AppResult {
    let using_libfuzzer = using_libfuzzer(fuzz_exe)?;
    let profraw_file = build_dir.join("fuzz_det_cov.profraw");
    let profdata_file = build_dir.join("fuzz_det_cov.profdata");
    let corpus_dir = corpora_dir.join(fuzz_target);
    let mut entries = read_dir(&corpus_dir)
        .map_err(|err| {
            exit_help(&format!(
                "The fuzz target's input directory must exist! ({}; {})",
                corpus_dir.display(),
                err
            ))
        })?
        .map(|entry| entry.expect("IO error"))
        .collect::<Vec<_>>();
    entries.sort_by_key(|entry| entry.file_name());
    let run_single = |run_id: u8, entry: &Path| -> Result<PathBuf, AppError> {
        let cov_txt_path = build_dir.join(format!("fuzz_det_cov.show.{run_id}.txt"));
        if !{
            {
                let mut cmd = Command::new(fuzz_exe);
                if using_libfuzzer {
                    cmd.arg("-runs=1");
                }
                cmd
            }
            .env("LLVM_PROFILE_FILE", &profraw_file)
            .env("FUZZ", fuzz_target)
            .arg(entry)
            .status()
            .map_err(|e| format!("fuzz failed with {e}"))?
            .success()
        } {
            Err("fuzz failed".to_string())?;
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
                &format!("--instr-profile={}", profdata_file.display()),
            ])
            .arg(fuzz_exe)
            .stdout(cov_file)
            .spawn()
            .map_err(|e| format!("{LLVM_COV} show failed with {e}"))?
            .wait()
            .map_err(|e| format!("{LLVM_COV} show failed with {e}"))?
            .success()
        {
            Err(format!("{LLVM_COV} show failed"))?;
        };
        Ok(cov_txt_path)
    };
    let check_diff = |a: &Path, b: &Path, err: &str| -> AppResult {
        let same = Command::new(GIT)
            .args(["--no-pager", "diff", "--no-index"])
            .arg(a)
            .arg(b)
            .status()
            .map_err(|e| format!("{GIT} diff failed with {e}"))?
            .success();
        if !same {
            Err(format!(
                r#"
The coverage was not deterministic between runs.
{err}"#
            ))?;
        }
        Ok(())
    };
    // First, check that each fuzz input is deterministic running by itself in a process.
    //
    // This can catch issues and isolate where a single fuzz input triggers non-determinism, but
    // all other fuzz inputs are deterministic.
    //
    // Also, This can catch issues where several fuzz inputs are non-deterministic, but the sum of
    // their overall coverage trace remains the same across runs and thus remains undetected.
    println!("Check each fuzz input individually ...");
    for entry in entries {
        let entry = entry.path();
        if !entry.is_file() {
            Err(format!("{} should be a file", entry.display()))?;
        }
        let cov_txt_base = run_single(0, &entry)?;
        let cov_txt_repeat = run_single(1, &entry)?;
        check_diff(
            &cov_txt_base,
            &cov_txt_repeat,
            &format!("The fuzz target input was {}.", entry.display()),
        )?;
    }
    // Finally, check that running over all fuzz inputs in one process is deterministic as well.
    // This can catch issues where mutable global state is leaked from one fuzz input execution to
    // the next.
    println!("Check all fuzz inputs in one go ...");
    {
        if !corpus_dir.is_dir() {
            Err(format!("{} should be a folder", corpus_dir.display()))?;
        }
        let cov_txt_base = run_single(0, &corpus_dir)?;
        let cov_txt_repeat = run_single(1, &corpus_dir)?;
        check_diff(
            &cov_txt_base,
            &cov_txt_repeat,
            &format!("All fuzz inputs in {} were used.", corpus_dir.display()),
        )?;
    }
    println!("Coverage test passed for {fuzz_target}.");
    Ok(())
}

fn main() -> ExitCode {
    match app() {
        Ok(()) => ExitCode::SUCCESS,
        Err(err) => {
            eprintln!("{}", err);
            ExitCode::FAILURE
        }
    }
}
