use std::{env, fs, io, io::Write, path::{Path, PathBuf}, process::{Command, Output}};

#[cfg(not(feature = "apple"))]
fn create_cross_cmake_command() -> Command {
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();

    let mut command = if target_arch.eq("wasm32") {
        Command::new("emcmake")
    } else {
        Command::new("cmake")
    };

    if target_arch.eq("wasm32") {
        command.arg("cmake");
    }

    command
}

fn handle_command_output(output: Output) {
    io::stdout()
        .write_all(&output.stdout)
        .expect("should write output");

    io::stderr()
        .write_all(&output.stderr)
        .expect("should write output");

    assert!(output.status.success());
}

#[cfg(not(feature = "apple"))]
fn main() {
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();

    // TODO: fix build for wasm32 on MacOS
    //   errors with `error: linking with `rust-lld` failed: exit status: 1`
    if target_arch.eq("wasm32") {
        println!("Build for wasm32 is not fully supported");
        return;
    }

    let root_path = Path::new("../..")
        .canonicalize()
        .expect("can't get abs path");

    let bls_dash_build_path = root_path.join("build");
    let bls_dash_src_path = root_path.join("src");
    let c_bindings_path = root_path.join("rust-bindings/bls-dash-sys/c-bindings");

    println!("root {}", root_path.display());
    println!("bls_dash_build_path {}", bls_dash_build_path.display());
    println!("bls_dash_src_path {}", bls_dash_src_path.display());
    // println!("c_bindings_path {}", c_bindings_path.display());

    // Run cmake

    println!("Run cmake:");

    if bls_dash_build_path.exists() {
        fs::remove_dir_all(&bls_dash_build_path).expect("can't clean build directory");
    }

    fs::create_dir_all(&bls_dash_build_path).expect("can't create build directory");

    let cmake_output = create_cross_cmake_command()
        .current_dir(&bls_dash_build_path)
        .arg("-DBUILD_BLS_PYTHON_BINDINGS=0")
        .arg("-DBUILD_BLS_TESTS=0")
        .arg("-DBUILD_BLS_BENCHMARKS=0")
        .arg("-DBUILD_BLS_JS_BINDINGS=0")
        .arg("..")
        .output()
        .expect("can't run cmake");

    handle_command_output(cmake_output);

    // Build deps for bls-signatures

    println!("Build dependencies:");

    let build_output = Command::new("cmake")
        .args(["--build", ".", "--", "-j", "6"])
        .current_dir(&bls_dash_build_path)
        .output()
        .expect("can't build bls-signatures deps");

    handle_command_output(build_output);

    // Collect include paths
    let include_paths_file_path = bls_dash_build_path.join("include_paths.txt");

    let include_paths =
        fs::read_to_string(include_paths_file_path).expect("should read include paths from file");

    let mut include_paths: Vec<_> = include_paths
        .split(';')
        .filter(|path| !path.is_empty())
        .map(|path| PathBuf::from(path))
        .collect();

    include_paths.extend([
        bls_dash_build_path.join("_deps/relic-src/include"),
        bls_dash_build_path.join("_deps/relic-build/include"),
        bls_dash_build_path.join("src"),
        root_path.join("include/dashbls"),
        bls_dash_build_path.join("depends/relic/include"),
        bls_dash_build_path.join("depends/mimalloc/include"),
        root_path.join("depends/relic/include"),
        root_path.join("depends/mimalloc/include"),
        bls_dash_src_path.clone(),
    ]);

    // Build c binding

    println!("Build C binding:");

    let mut cc = cc::Build::new();

    let cpp_files_mask = c_bindings_path.join("**/*.cpp");

    let cpp_files: Vec<_> = glob::glob(cpp_files_mask.to_str().unwrap())
        .expect("can't get list of cpp files")
        .filter_map(Result::ok)
        .collect();

    cc.files(cpp_files)
        .includes(&include_paths)
        .cpp(true)
        .flag_if_supported("-std=c++14");

    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();

    // Fix homebrew LLVM installation issue
    if env::consts::OS == "macos" && target_arch == "wasm32" {
        cc.archiver("llvm-ar");
    }

    if target_arch.eq("wasm32") {
        cc.flag_if_supported("-ffreestanding")
            .define("_LIBCPP_HAS_NO_THREADS", Some("1"));
    }

    if !cfg!(debug_assertions) {
        cc.opt_level(2);
    }

    cc.compile("bls-dash-sys");

    // // Link dependencies
    // println!(
    //     "cargo:rustc-link-search={}",
    //     bls_dash_build_path.join("_deps/sodium-build").display()
    // );

    // println!("cargo:rustc-link-lib=static=sodium");

    println!(
        "cargo:rustc-link-search={}",
        root_path.join("build/depends/relic/lib").display()
    );

    println!("cargo:rustc-link-lib=static=relic_s");

    println!(
        "cargo:rustc-link-search={}",
        root_path.join("build/depends/mimalloc").display()
    );

    println!("cargo:rustc-link-lib=static=mimalloc-secure");

    println!(
        "cargo:rustc-link-search={}",
        bls_dash_build_path.join("src").display()
    );

    println!("cargo:rustc-link-lib=static=dashbls");

    // Link GMP if exists
    let gmp_libraries_file_path = bls_dash_build_path.join("gmp_libraries.txt");

    if gmp_libraries_file_path.exists() {
        let gmp_libraries_path = PathBuf::from(
            fs::read_to_string(gmp_libraries_file_path)
                .expect("should read gmp includes from file"),
        );

        let gmp_libraries_parent_path = gmp_libraries_path
            .parent()
            .expect("can't get gmp libraries parent dir");

        println!(
            "cargo:rustc-link-search={}",
            gmp_libraries_parent_path.display()
        );

        println!("cargo:rustc-link-lib=static=gmp");
    }

    // Generate rust code for c binding to src/lib.rs
    // println!("Generate C binding for rust:");

    // let mut builder = bindgen::Builder::default()
    //     // .trust_clang_mangling(true)
    //     // .wasm_import_module_name()
    //     .size_t_is_usize(true)
    //     .parse_callbacks(Box::new(bindgen::CargoCallbacks));

    // let headers_to_process = [
    //     "blschia.h",
    //     "elements.h",
    //     "privatekey.h",
    //     "schemes.h",
    //     "threshold.h",
    //     "bip32/chaincode.h",
    //     "bip32/extendedprivatekey.h",
    //     "bip32/extendedpublickey.h",
    // ];

    // for header in headers_to_process {
    //     builder = builder.header(c_bindings_path.join(header).to_str().unwrap())
    // }

    // if target_arch == "wasm32" {
    //     builder = builder.clang_args(
    //         include_paths
    //             .iter()
    //             .map(|path| format!("-I{}", path.display())),
    //     );
    // }

    // let bindings = builder.generate().expect("Unable to generate bindings");

    // let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());

    // bindings
    //     .write_to_file(out_path.join("bindings.rs"))
    //     .expect("couldn't write bindings");

    // // Rerun build if files changed
    // println!("cargo:rerun-if-changed={}", c_bindings_path.display());
    println!("cargo:rerun-if-changed={}", bls_dash_src_path.display());
}

// fn main() {
//     let target = env::var("TARGET").unwrap();
//     println!("Building bls-signatures for apple target: {}", target);
//     let root_path = Path::new("../..")
//         .canonicalize()
//         .expect("can't get abs path");
//     let bls_dash_build_path = root_path.join("build");
//     let bls_dash_src_path = root_path.join("src");
//     let artefacts_path = bls_dash_build_path.join("artefacts");
//     let target_path = artefacts_path.join(&target);
//     let script = root_path.join("apple.rust.single.sh");
//     if bls_dash_build_path.exists() {
//         fs::remove_dir_all(&bls_dash_build_path).expect("can't clean build directory");
//     }
//     fs::create_dir_all(&bls_dash_build_path).expect("can't create build directory");
//     let output = Command::new("sh")
//         .current_dir(&root_path)
//         .arg(script)
//         .arg(target)
//         .output()
//         .expect("Failed to execute the shell script");
//     handle_command_output(output);
//     let library_path = target_path.join("libbls.a");
//     if !fs::metadata(&library_path).is_ok() {
//         panic!("Library file not found: {}", library_path.display());
//     }
//     println!("cargo:rustc-link-search={}", target_path.display());
//     println!("cargo:rustc-link-lib=static=gmp");
//     println!("cargo:rustc-link-lib=static=sodium");
//     println!("cargo:rustc-link-lib=static=relic_s");
//     println!("cargo:rustc-link-search={}", bls_dash_build_path.join("src").display());
//     println!("cargo:rustc-link-lib=static=bls");
//     println!("cargo:rerun-if-changed={}", bls_dash_src_path.display());
// }

#[cfg(feature = "apple")]
fn main() {
    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();

    // TODO: fix build for wasm32 on MacOS
    //   errors with `error: linking with `rust-lld` failed: exit status: 1`
    if target_arch.eq("wasm32") {
        println!("Build for wasm32 is not fully supported");
        return;
    }


    let target = env::var("TARGET").unwrap();
    println!("Building bls-signatures for apple target: {}", target);
    let root_path = Path::new("../..")
        .canonicalize()
        .expect("can't get abs path");
    let bls_dash_build_path = root_path.join("build");
    let bls_dash_src_path = root_path.join("src");
    let bls_dash_src_include_path = root_path.join("include/dashbls");
    let c_bindings_path = root_path.join("rust-bindings/bls-dash-sys/c-bindings");
    let artefacts_path = bls_dash_build_path.join("artefacts");
    let target_path = artefacts_path.join(&target);
    let script = root_path.join("apple.rust.deps.sh");
    if bls_dash_build_path.exists() {
        fs::remove_dir_all(&bls_dash_build_path).expect("can't clean build directory");
    }
    fs::create_dir_all(&bls_dash_build_path).expect("can't create build directory");
    let output = Command::new("sh")
        .current_dir(&root_path)
        .arg(script)
        .arg(target.as_str())
        .output()
        .expect("Failed to execute the shell script");
    handle_command_output(output);
    let (arch, platform) = match target.as_str() {
        "x86_64-apple-ios" => ("x86_64", "iphonesimulator"),
        "aarch64-apple-ios" => ("arm64", "iphoneos"),
        "aarch64-apple-ios-sim" => ("arm64", "iphonesimulator"),
        "x86_64-apple-darwin" => ("x86_64", "macosx"),
        "aarch64-apple-darwin" => ("arm64", "macosx"),
        _ => panic!("Target {} not supported", target.as_str())
    };
    env::set_var("IPHONEOS_DEPLOYMENT_TARGET", "13.0");

    // Collect include paths
    let include_paths_file_path = bls_dash_build_path.join("include_paths.txt");

    let include_paths =
        fs::read_to_string(include_paths_file_path).expect("should read include paths from file");

    let mut include_paths: Vec<_> = include_paths
        .split(';')
        .filter(|path| !path.is_empty())
        .map(|path| PathBuf::from(path))
        .collect();

    include_paths.extend([
        bls_dash_build_path.join(format!("relic-{}-{}/_deps/relic-src/include", platform, arch)),
        bls_dash_build_path.join(format!("relic-{}-{}/_deps/relic-build/include", platform, arch)),
        bls_dash_build_path.join("contrib/relic/src"),
        root_path.join("src"),
        root_path.join("include/dashbls"),
        root_path.join("depends/relic/include"),
        root_path.join("depends/mimalloc/include"),
        root_path.join("depends/catch2/include"),
        bls_dash_src_path.clone(),
        bls_dash_src_include_path.clone()
    ]);

    let cpp_files: Vec<_> = glob::glob(c_bindings_path.join("**/*.cpp").to_str().unwrap())
        .expect("can't get list of cpp files")
        .filter_map(Result::ok)
        .collect();

    let mut cc = cc::Build::new();
    cc.files(cpp_files)
        .includes(&include_paths)
        .cpp(true)
        .flag("-Wno-unused-parameter")
        .flag("-Wno-sign-compare")
        .flag("-Wno-delete-non-abstract-non-virtual-dtor")
        .flag("-std=c++14");

    cc.compile("dashbls");

    println!("cargo:rustc-link-search={}", target_path.display());
    println!("cargo:rustc-link-lib=static=gmp");
    // println!("cargo:rustc-link-lib=static=sodium");
    // println!("cargo:rustc-link-lib=static=relic_s");
    println!("cargo:rustc-link-lib=static=bls");
    println!("cargo:rustc-link-search={}", bls_dash_src_path.display());
    println!("cargo:rustc-link-lib=static=dashbls");
    println!("cargo:rerun-if-changed={}", bls_dash_src_path.display());
}
