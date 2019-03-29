extern crate cbindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();

    // either read from $LIBNAME_H the header file, or
    // set it equal to $TARGET/$(CARGO_PKG_NAME).h
    let header_file_name = env::var("HEADER_FILE_OVERRIDE")
        .or(env::var("CARGO_PKG_NAME").map(|f| {
            target_dir()
                .join(format!("{}.h", f))
                .display()
                .to_string()
        }))
        .unwrap();

    cbindgen::Builder::new()
        .with_crate(crate_dir)
        .with_language(cbindgen::Language::C)
        .with_parse_deps(true)
        .with_parse_include(&["cuckoofilter"])
        .generate()
        .expect("Unable to generate bindings")
        .write_to_file(&header_file_name);
}

/// Find the location of the `target/` directory.
fn target_dir() -> PathBuf {
    env::var("HEADER_TARGET_DIR_OVERRIDE")
        .or(env::var("CARGO_TARGET_DIR"))
        .map(PathBuf::from)
        .unwrap_or_else(|_| {
            PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap()).join("target")
        })
}
