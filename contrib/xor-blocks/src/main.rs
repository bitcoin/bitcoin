use std::{
    env, fs,
    io::{self, Read},
    path::Path,
    time::{Duration, Instant},
};

const MAGIC: [u8; 4] = [0xf9, 0xbe, 0xb4, 0xd9];

fn main() -> Result<(), io::Error> {
    let start = Instant::now();

    let blocks_path = if env::args().len() > 1 {
        env::args().nth(1).expect("the arg to exist").into()
    } else {
        #[allow(deprecated)]
        env::home_dir()
            .expect("to have a home directory")
            .join(match env::consts::OS {
                "macos" => "Library/Application Support/Bitcoin",
                "windows" => "AppData\\Local\\Bitcoin",
                "linux" => ".bitcoin",
                _ => {
                    println!("Unknown OS");
                    return Ok(());
                }
            })
            .join("blocks")
    };

    let paths = fs::read_dir(&blocks_path)?
        .map(|res| res.map(|e| e.path()))
        .collect::<Result<Vec<_>, io::Error>>()?;

    let xor_path: std::path::PathBuf = blocks_path.join("xor.dat");
    if !fs::exists(&xor_path)? {
        println!("No xor.dat file. Make sure you are running Bitcoin Core v28 or higher.");
        return Ok(());
    }

    println!("Xor'ing blocks dir. Do not start bitcoind until finished!");

    let key: [u8; 8] = fs::read(&xor_path)?
        .try_into()
        .expect("xor.dat to be 8 bytes");
    if key[0..4] == [0u8; 4] && key[4..] != [0u8; 4] {
        println!("This script doesn't work with a non-zero key with 4 bytes of leading zeros");
        return Ok(());
    }
    let key = if key == [0u8; 8] {
        loop {
            let key: [u8; 8] = rand::random();
            // Don't use keys with 4 bytes of leading zeros
            // They won't let us detect the first 4 bytes of magic in the files
            if key[0..4] == [0u8; 4] {
                continue;
            }
            break key;
        }
    } else {
        key
    };

    fs::write(xor_path, key)?;

    let total = paths.len();
    let mut done = 0;
    let mut timer = Instant::now();
    let duration = Duration::from_secs(5);

    paths.into_iter().for_each(|path| {
        if let Err(e) = xor_block(&path, key) {
            println!(
                "Error xor-ing file {:?}: {e:?}",
                path.iter()
                    .next_back()
                    .expect("path to have a last component")
            )
        };

        done += 1;
        if timer.elapsed() > duration {
            println!("Xor'd {done} / {total} files");
            timer = Instant::now();
        }
    });

    println!(
        "Done in {} seconds! Blocksdir is now xor'd.",
        start.elapsed().as_secs()
    );

    Ok(())
}

fn xor_block(path: &Path, key: [u8; 8]) -> Result<(), io::Error> {
    if path.extension().is_none_or(|f| f != "dat") {
        return Ok(());
    }

    let file_name = path
        .iter()
        .next_back()
        .expect("there to be a last path component");

    if file_name == "xor.dat" {
        return Ok(());
    }

    let mut file = fs::File::open(path)?;
    let mut buf = [0u8; 4];
    file.read_exact(&mut buf)?;

    if buf != MAGIC {
        return Ok(());
    }

    let mut block = fs::read(path)?;
    block
        .iter_mut()
        .enumerate()
        .for_each(|(i, b)| *b ^= key[i % key.len()]);

    let mut tmp_path = path.as_os_str().to_owned();
    tmp_path.push(".tmp");
    fs::write(&tmp_path, block)?;
    fs::rename(&tmp_path, path)?;

    Ok(())
}
