use std::env;
use std::process::Command;
use std::str;

fn main() {
    let rustc_minor_ver =
        rustc_minor_version().expect("Failed to get rustc version");
    let rustc_dep_of_std = env::var("CARGO_FEATURE_RUSTC_DEP_OF_STD").is_ok();
    let align_cargo_feature = env::var("CARGO_FEATURE_ALIGN").is_ok();
    let libc_ci = env::var("LIBC_CI").is_ok();

    if env::var("CARGO_FEATURE_USE_STD").is_ok() {
        println!(
            "cargo:warning=\"libc's use_std cargo feature is deprecated since libc 0.2.55; \
             please consider using the `std` cargo feature instead\""
        );
    }

    // The ABI of libc used by libstd is backward compatible with FreeBSD 10.
    // The ABI of libc from crates.io is backward compatible with FreeBSD 11.
    //
    // On CI, we detect the actual FreeBSD version and match its ABI exactly,
    // running tests to ensure that the ABI is correct.
    match which_freebsd() {
        Some(10) if libc_ci || rustc_dep_of_std => {
            println!("cargo:rustc-cfg=freebsd10")
        }
        Some(11) if libc_ci => println!("cargo:rustc-cfg=freebsd11"),
        Some(12) if libc_ci => println!("cargo:rustc-cfg=freebsd12"),
        Some(13) if libc_ci => println!("cargo:rustc-cfg=freebsd13"),
        Some(_) | None => println!("cargo:rustc-cfg=freebsd11"),
    }

    // On CI: deny all warnings
    if libc_ci {
        println!("cargo:rustc-cfg=libc_deny_warnings");
    }

    // Rust >= 1.15 supports private module use:
    if rustc_minor_ver >= 15 || rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_priv_mod_use");
    }

    // Rust >= 1.19 supports unions:
    if rustc_minor_ver >= 19 || rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_union");
    }

    // Rust >= 1.24 supports const mem::size_of:
    if rustc_minor_ver >= 24 || rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_const_size_of");
    }

    // Rust >= 1.25 supports repr(align):
    if rustc_minor_ver >= 25 || rustc_dep_of_std || align_cargo_feature {
        println!("cargo:rustc-cfg=libc_align");
    }

    // Rust >= 1.30 supports `core::ffi::c_void`, so libc can just re-export it.
    // Otherwise, it defines an incompatible type to retaining
    // backwards-compatibility.
    if rustc_minor_ver >= 30 || rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_core_cvoid");
    }

    // Rust >= 1.33 supports repr(packed(N))
    if rustc_minor_ver >= 33 || rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_packedN");
    }

    // #[thread_local] is currently unstable
    if rustc_dep_of_std {
        println!("cargo:rustc-cfg=libc_thread_local");
    }
}

fn rustc_minor_version() -> Option<u32> {
    macro_rules! otry {
        ($e:expr) => {
            match $e {
                Some(e) => e,
                None => return None,
            }
        };
    }

    let rustc = otry!(env::var_os("RUSTC"));
    let output = otry!(Command::new(rustc).arg("--version").output().ok());
    let version = otry!(str::from_utf8(&output.stdout).ok());
    let mut pieces = version.split('.');

    if pieces.next() != Some("rustc 1") {
        return None;
    }

    otry!(pieces.next()).parse().ok()
}

fn which_freebsd() -> Option<i32> {
    let output = std::process::Command::new("freebsd-version").output().ok();
    if output.is_none() {
        return None;
    }
    let output = output.unwrap();
    if !output.status.success() {
        return None;
    }

    let stdout = String::from_utf8(output.stdout).ok();
    if stdout.is_none() {
        return None;
    }
    let stdout = stdout.unwrap();

    match &stdout {
        s if s.starts_with("10") => Some(10),
        s if s.starts_with("11") => Some(11),
        s if s.starts_with("12") => Some(12),
        s if s.starts_with("13") => Some(13),
        _ => None,
    }
}
