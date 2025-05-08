// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

use std::{
    env,
    fmt::{self, Debug, Display, Formatter},
    fs::{self, File},
    hash::{BuildHasher, Hasher, RandomState},
    io::{self, BufReader, BufWriter, Read, Write},
    path::Path,
    time::{Duration, Instant},
};

const MAGIC: [u8; 4] = [0xf9, 0xbe, 0xb4, 0xd9];
const XOR_FILE_NAME: &str = "xor.dat";

#[derive(Debug)]
struct Error {
    msg: String,
}

impl Error {
    fn new(msg: String) -> Self {
        Self { msg }
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.msg)
    }
}

impl From<io::Error> for Error {
    fn from(value: io::Error) -> Self {
        Error::new(value.to_string())
    }
}

impl From<&str> for Error {
    fn from(value: &str) -> Self {
        Error::new(value.to_string())
    }
}

fn main() -> Result<(), Error> {
    let start = Instant::now();

    let data_dir_path = if env::args().len() == 2 {
        let arg = env::args().nth(1).expect("the arg to exist");
        if !arg.starts_with("--datadir=") && !arg.starts_with("-datadir=") {
            return Err(Error::new(format!(
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
                "windows" => "AppData\\Roaming\\Bitcoin",
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

    let key_path = blocks_path.join(XOR_FILE_NAME);
    let key: [u8; 8] = fs::read(&key_path)
        .map_err(|_| "No xor.dat file. Make sure you are running Bitcoin Core v28 or higher.")?
        .try_into()
        .map_err(|_| "Invalid xor.dat file.")?;

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

    let mut double_key = [0u8; 16];
    double_key[..8].copy_from_slice(&key);
    double_key[8..].copy_from_slice(&key);
    let key = u128::from_ne_bytes(double_key);

    paths.into_iter().for_each(|path| {
        if path.extension().is_none_or(|f| f != "dat") {
            return;
        }

        if path.file_name().expect("there to be a file name") == XOR_FILE_NAME {
            return;
        }

        if let Err(e) = xor_file(&path, key) {
            println!("Error obfuscating file {}: {e}", path.display())
        };

        done += 1;
        if timer.elapsed() > duration {
            let rate = done / start.elapsed().as_secs();
            println!("Obfuscated {done} / {total} files ({rate}/sec)");
            timer = Instant::now();
        }
    });

    println!(
        "Done in {} seconds! Blocksdir is now obfuscated.",
        start.elapsed().as_secs()
    );

    Ok(())
}

fn xor_file(path: &Path, key: u128) -> Result<(), io::Error> {
    let mut buf_u128 = 0u128;
    let buf = unsafe { (&mut buf_u128 as *mut _ as *mut [u8; 16]).as_mut() }.unwrap();

    let file = File::open(path)?;
    let mut reader = BufReader::new(file);

    reader.read_exact(buf)?;

    if buf[..4] != MAGIC {
        // This file is already obfuscated
        return Ok(());
    }

    let tmp_path = path.with_extension("dat.tmp");
    let file = File::options()
        .write(true)
        .create(true)
        .truncate(true)
        .open(&tmp_path)?;
    let mut writer = BufWriter::new(file);

    loop {
        buf_u128 ^= key;
        writer.write_all(buf)?;
        let n = reader.read(buf)?;
        if n < 16 {
            let key = key.to_ne_bytes();
            for i in 0..n {
                buf[i] ^= key[i];
            }
            writer.write_all(&buf[..n])?;
            break;
        }
    }

    writer.into_inner()?.sync_data()?;
    fs::rename(&tmp_path, path)?;

    Ok(())
}
