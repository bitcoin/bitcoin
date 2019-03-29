#![feature(test)]

extern crate cuckoofilter;
#[cfg(feature = "farmhash")]
extern crate farmhash;
#[cfg(feature = "fnv")]
extern crate fnv;
extern crate rand;
extern crate test;

use self::cuckoofilter::*;
use std::error::Error;
use std::fs::File;
use std::io::prelude::*;
use std::path::Path;

fn get_words() -> String {
    let path = Path::new("/usr/share/dict/words");
    let display = path.display();

    // Open the path in read-only mode, returns `io::Result<File>`
    let mut file = match File::open(&path) {
        // The `description` method of `io::Error` returns a string that
        // describes the error
        Err(why) => panic!("couldn't open {}: {}", display, Error::description(&why)),
        Ok(file) => file,
    };

    let mut contents = String::new();
    if let Err(why) = file.read_to_string(&mut contents) {
        panic!("couldn't read {}: {}", display, Error::description(&why));
    }
    contents
}

fn perform_insertions<H: std::hash::Hasher + Default>(b: &mut test::Bencher) {
    let contents = get_words();
    let split: Vec<&str> = contents.split("\n").take(1000).collect();
    let mut cf = CuckooFilter::<H>::with_capacity(split.len() * 2);

    b.iter(|| {
        for s in &split {
            test::black_box(cf.test_and_add(s).unwrap());
        }
    });
}

#[bench]
fn bench_new(b: &mut test::Bencher) {
    b.iter(|| {
        test::black_box(CuckooFilter::new());
    });
}

#[bench]
fn bench_clear(b: &mut test::Bencher) {
    let mut cf = test::black_box(CuckooFilter::new());

    b.iter(|| {
        test::black_box(cf.clear());
    });
}

#[cfg(feature = "farmhash")]
#[bench]
fn bench_insertion_farmhash(b: &mut test::Bencher) {
    perform_insertions::<farmhash::FarmHasher>(b);
}

#[cfg(feature = "fnv")]
#[bench]
fn bench_insertion_fnv(b: &mut test::Bencher) {
    perform_insertions::<fnv::FnvHasher>(b);
}

#[bench]
fn bench_insertion_default(b: &mut test::Bencher) {
    perform_insertions::<std::collections::hash_map::DefaultHasher>(b);
}
