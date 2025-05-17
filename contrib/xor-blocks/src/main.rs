// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

use std::{
    env,
    fmt::{Debug, Display},
    fs::{self, File},
    hash::{BuildHasher, Hasher, RandomState},
    io::{self, BufReader, BufWriter, Read, Write},
    path::Path,
    time::{Duration, Instant},
};

const MAGIC: [u8; 4] = [0xf9, 0xbe, 0xb4, 0xd9];
const XOR_FILE_NAME: &str = "xor.dat";

#[derive(Debug)]
enum Error {
    Io(io::Error),
    Custom(String),
}

impl Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Error::Io(e) => write!(f, "{e}"),
            Error::Custom(e) => write!(f, "{e}"),
        }
    }
}

impl From<io::Error> for Error {
    fn from(value: io::Error) -> Self {
        Error::Io(value)
    }
}

impl From<&str> for Error {
    fn from(value: &str) -> Self {
        Error::Custom(value.to_string())
    }
}

fn main() -> Result<(), Error> {
    let start = Instant::now();

    let data_dir_path = if env::args().len() == 2 {
        let arg = env::args().nth(1).expect("the arg to exist");
        if !arg.starts_with("--datadir=") && !arg.starts_with("-datadir=") {
            return Err(Error::Custom(format!(
                "Error parsing command line arguments: Invalid parameter {arg}"
            )));
        }
        let (_, datadir) = arg.split_once("-datadir=").expect("split to be some");
        datadir.into()
    } else if env::args().len() == 1 {
        #[allow(deprecated)]
        env::home_dir()
            .expect("to have a home directory")
            .join(match env::consts::OS {
                "macos" => "Library/Application Support/Bitcoin",
                "windows" => "AppData\\Local\\Bitcoin",
                "linux" => ".bitcoin",
                _ => {
                    return Err("Unknown OS".into());
                }
            })
    } else {
        return Err("Invalid number of arguments. Run with `cargo run --release` if using default datadir. Otherwise run with `cargo run --release -- -datadir=<custom datadir>`.".into());
    };

    let blocks_path = data_dir_path.join("blocks");

    let paths = fs::read_dir(&blocks_path)?
        .map(|res| res.map(|e| e.path()))
        .collect::<Result<Vec<_>, io::Error>>()?;

    let key_path: std::path::PathBuf = blocks_path.join(XOR_FILE_NAME);
    let Ok::<[u8; 8], _>(key) = fs::read(&key_path)?.try_into() else {
        return Err(
            "No xor.dat file. Make sure you are running Bitcoin Core v28 or higher.".into(),
        );
    };

    if key[..4] == [0u8; 4] && key[4..] != [0u8; 4] {
        return Err(
            "This tool doesn't work with a non-zero key with 4 bytes of leading zeros".into(),
        );
    }

    let key = if key == [0u8; 8] {
        // The key in the file is zero, so overwrite with a new random one
        loop {
            let key: [u8; 8] = RandomState::new().build_hasher().finish().to_le_bytes();
            // Don't use keys with 4 bytes of leading zeros
            // They won't let us detect the first 4 bytes of magic in the files
            if key[..4] == [0u8; 4] {
                continue;
            }
            let mut out_file = File::options().write(true).open(&key_path)?;
            out_file.write_all(&key)?;
            out_file.sync_data()?;
            break key;
        }
    } else {
        // Use the existing random key
        key
    };

    println!("Obfuscating blocks dir. Do not start bitcoind until finished!");

    let total = paths.len();
    let mut done = 0;
    let mut timer = Instant::now();
    let duration = Duration::from_secs(5);

    paths.into_iter().for_each(|path| {
        if let Err(e) = xor_file(&path, key) {
            println!("Error obfuscating file {}: {e}", path.display())
        };

        done += 1;
        if timer.elapsed() > duration {
            println!("Obfuscated {done} / {total} files");
            timer = Instant::now();
        }
    });

    println!(
        "Done in {} seconds! Blocksdir is now obfuscated.",
        start.elapsed().as_secs()
    );

    Ok(())
}

fn xor_file(path: &Path, key: [u8; 8]) -> Result<(), io::Error> {
    if path.extension().is_none_or(|f| f != "dat") {
        return Ok(());
    }

    if path.file_name().expect("there to be a file name") == XOR_FILE_NAME {
        return Ok(());
    }

    let in_file = File::open(path)?;
    let mut reader = BufReader::new(in_file);

    let mut buf = [0u8; 16];
    reader.read_exact(&mut buf)?;

    if buf[..4] != MAGIC {
        // This file is already obfuscated
        return Ok(());
    }

    let mut tmp_path = path.as_os_str().to_owned();
    tmp_path.push(".tmp");
    let out_file = File::options()
        .write(true)
        .create(true)
        .truncate(true)
        .open(&tmp_path)?;
    let mut writer = BufWriter::new(out_file);

    let mut double_key = [0u8; 16];
    double_key[..8].copy_from_slice(&key);
    double_key[8..].copy_from_slice(&key);
    let key_u128 = &double_key as *const _ as *const u128;

    loop {
        let chunk = &mut buf as *mut _ as *mut u128;
        unsafe {
            *chunk ^= *key_u128;
            writer.write_all(&*(chunk as *const [u8; 16]))?;
        }
        if reader.read_exact(&mut buf).is_err() {
            break;
        }
    }

    let mut buf = vec![0u8; 15];
    reader.read_to_end(&mut buf)?;
    for i in 0..buf.len() {
        buf[i] ^= key[i % key.len()];
    }
    writer.write_all(&buf)?;

    writer.flush()?;
    fs::rename(&tmp_path, path)?;

    Ok(())
}
