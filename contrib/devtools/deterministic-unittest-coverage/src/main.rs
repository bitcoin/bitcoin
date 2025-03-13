// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

use std::env;
use std::fs::File;
use std::path::Path;
use std::process::{exit, Command};
use std::str;

const LLVM_PROFDATA: &str = "llvm-profdata";
const LLVM_COV: &str = "llvm-cov";
const GIT: &str = "git";

fn exit_help(err: &str) -> ! {
    eprintln!("Error: {}", err);
    eprintln!();
    eprintln!("Usage: program ./build_dir boost_unittest_filter");
    eprintln!();
    eprintln!("Refer to the devtools/README.md for more details.");
    exit(1)
}

fn sanity_check(test_exe: &Path) {
    for tool in [LLVM_PROFDATA, LLVM_COV, GIT] {
        let output = Command::new(tool).arg("--help").output();
        match output {
            Ok(output) if output.status.success() => {}
            _ => {
                exit_help(&format!("The tool {} is not installed", tool));
            }
        }
    }
    if !test_exe.exists() {
        exit_help(&format!(
            "Test executable ({}) not found",
            test_exe.display()
        ));
    }
}

fn main() {
    // Parse args
    let args = env::args().collect::<Vec<_>>();
    let build_dir = args
        .get(1)
        .unwrap_or_else(|| exit_help("Must set build dir"));
    if build_dir == "--help" {
        exit_help("--help requested")
    }
    let filter = args
        .get(2)
        // Require filter for now. In the future it could be optional and the tool could provide a
        // default filter.
        .unwrap_or_else(|| exit_help("Must set boost test filter"));
    if args.get(3).is_some() {
        exit_help("Too many args")
    }

    let build_dir = Path::new(build_dir);
    let test_exe = build_dir.join("bin/test_bitcoin");

    sanity_check(&test_exe);

    deterministic_coverage(build_dir, &test_exe, filter);
}

fn deterministic_coverage(build_dir: &Path, test_exe: &Path, filter: &str) {
    let profraw_file = build_dir.join("test_det_cov.profraw");
    let profdata_file = build_dir.join("test_det_cov.profdata");
    let run_single = |run_id: u8| {
        let cov_txt_path = build_dir.join(format!("test_det_cov.show.{run_id}.txt"));
        assert!(Command::new(test_exe)
            .env("LLVM_PROFILE_FILE", &profraw_file)
            .env("BOOST_TEST_RUN_FILTERS", filter)
            .env("RANDOM_CTX_SEED", "21")
            .status()
            .expect("test failed")
            .success());
        assert!(Command::new(LLVM_PROFDATA)
            .arg("merge")
            .arg("--sparse")
            .arg(&profraw_file)
            .arg("-o")
            .arg(&profdata_file)
            .status()
            .expect("merge failed")
            .success());
        let cov_file = File::create(&cov_txt_path).expect("Failed to create coverage txt file");
        assert!(Command::new(LLVM_COV)
            .args([
                "show",
                "--show-line-counts-or-regions",
                "--show-branches=count",
                "--show-expansions",
                &format!("--instr-profile={}", profdata_file.display()),
            ])
            .arg(test_exe)
            .stdout(cov_file)
            .status()
            .expect("llvm-cov failed")
            .success());
        cov_txt_path
    };
    let check_diff = |a: &Path, b: &Path| {
        let same = Command::new(GIT)
            .args(["--no-pager", "diff", "--no-index"])
            .arg(a)
            .arg(b)
            .status()
            .expect("Failed to execute git command")
            .success();
        if !same {
            eprintln!();
            eprintln!("The coverage was not deterministic between runs.");
            eprintln!("Exiting.");
            exit(1);
        }
    };
    let r0 = run_single(0);
    let r1 = run_single(1);
    check_diff(&r0, &r1);
    println!("The coverage was deterministic across two runs.");
}
