//! Useful comparison functions.

/// Compare two slices for equality in fixed time. Panics if the slices are of non-equal length.
///
/// This works by XOR'ing each byte of the two inputs together and keeping an OR counter of the
/// results.
///
/// Instead of doing fancy bit twiddling to try to outsmart the compiler and prevent early exits,
/// which is not guaranteed to remain stable as compilers get ever smarter, we take the hit of
/// writing each intermediate value to memory with a volatile write and then re-reading it with a
/// volatile read. This should remain stable across compiler upgrades, but is much slower.
///
/// As of rust 1.31.0 disassembly looks completely within reason for this, see
/// https://godbolt.org/z/mMbGQv
pub fn fixed_time_eq(a: &[u8], b: &[u8]) -> bool {
    assert!(a.len() == b.len());
    let count = a.len();
    let lhs = &a[..count];
    let rhs = &b[..count];

    let mut r: u8 = 0;
    for i in 0..count {
        let mut rs = unsafe { ::core::ptr::read_volatile(&r) };
        rs |= lhs[i] ^ rhs[i];
        unsafe { ::core::ptr::write_volatile(&mut r, rs); }
    }
    {
        let mut t = unsafe { ::core::ptr::read_volatile(&r) };
        t |= t >> 4;
        unsafe { ::core::ptr::write_volatile(&mut r, t); }
    }
    {
        let mut t = unsafe { ::core::ptr::read_volatile(&r) };
        t |= t >> 2;
        unsafe { ::core::ptr::write_volatile(&mut r, t); }
    }
    {
        let mut t = unsafe { ::core::ptr::read_volatile(&r) };
        t |= t >> 1;
        unsafe { ::core::ptr::write_volatile(&mut r, t); }
    }
    unsafe { (::core::ptr::read_volatile(&r) & 1) == 0 }
}

#[test]
fn eq_test() {
    assert!( fixed_time_eq(&[0b00000000], &[0b00000000]));
    assert!( fixed_time_eq(&[0b00000001], &[0b00000001]));
    assert!( fixed_time_eq(&[0b00000010], &[0b00000010]));
    assert!( fixed_time_eq(&[0b00000100], &[0b00000100]));
    assert!( fixed_time_eq(&[0b00001000], &[0b00001000]));
    assert!( fixed_time_eq(&[0b00010000], &[0b00010000]));
    assert!( fixed_time_eq(&[0b00100000], &[0b00100000]));
    assert!( fixed_time_eq(&[0b01000000], &[0b01000000]));
    assert!( fixed_time_eq(&[0b10000000], &[0b10000000]));
    assert!( fixed_time_eq(&[0b11111111], &[0b11111111]));

    assert!(!fixed_time_eq(&[0b00000001], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00000001], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b00000010], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00000010], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b00000100], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00000100], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b00001000], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00001000], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b00010000], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00010000], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b00100000], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b00100000], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b01000000], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b01000000], &[0b11111111]));
    assert!(!fixed_time_eq(&[0b10000000], &[0b00000000]));
    assert!(!fixed_time_eq(&[0b10000000], &[0b11111111]));

    assert!( fixed_time_eq(&[0b00000000, 0b00000000], &[0b00000000, 0b00000000]));
    assert!(!fixed_time_eq(&[0b00000001, 0b00000000], &[0b00000000, 0b00000000]));
    assert!(!fixed_time_eq(&[0b00000000, 0b00000001], &[0b00000000, 0b00000000]));
    assert!(!fixed_time_eq(&[0b00000000, 0b00000000], &[0b00000001, 0b00000000]));
    assert!(!fixed_time_eq(&[0b00000000, 0b00000000], &[0b00000001, 0b00000001]));
}

#[cfg(all(test, feature="unstable"))]
mod benches {
    use test::Bencher;

    use sha256;
    use sha512;
    use Hash;
    use cmp::fixed_time_eq;

    #[bench]
    fn bench_32b_constant_time_cmp_ne(bh: &mut Bencher) {
        let hash_a = sha256::Hash::hash(&[0; 1]);
        let hash_b = sha256::Hash::hash(&[1; 1]);
        bh.iter(|| {
            fixed_time_eq(&hash_a[..], &hash_b[..])
        })
    }

    #[bench]
    fn bench_32b_slice_cmp_ne(bh: &mut Bencher) {
        let hash_a = sha256::Hash::hash(&[0; 1]);
        let hash_b = sha256::Hash::hash(&[1; 1]);
        bh.iter(|| {
            &hash_a[..] == &hash_b[..]
        })
    }

    #[bench]
    fn bench_32b_constant_time_cmp_eq(bh: &mut Bencher) {
        let hash_a = sha256::Hash::hash(&[0; 1]);
        let hash_b = sha256::Hash::hash(&[0; 1]);
        bh.iter(|| {
            fixed_time_eq(&hash_a[..], &hash_b[..])
        })
    }

    #[bench]
    fn bench_32b_slice_cmp_eq(bh: &mut Bencher) {
        let hash_a = sha256::Hash::hash(&[0; 1]);
        let hash_b = sha256::Hash::hash(&[0; 1]);
        bh.iter(|| {
            &hash_a[..] == &hash_b[..]
        })
    }

    #[bench]
    fn bench_64b_constant_time_cmp_ne(bh: &mut Bencher) {
        let hash_a = sha512::Hash::hash(&[0; 1]);
        let hash_b = sha512::Hash::hash(&[1; 1]);
        bh.iter(|| {
            fixed_time_eq(&hash_a[..], &hash_b[..])
        })
    }

    #[bench]
    fn bench_64b_slice_cmp_ne(bh: &mut Bencher) {
        let hash_a = sha512::Hash::hash(&[0; 1]);
        let hash_b = sha512::Hash::hash(&[1; 1]);
        bh.iter(|| {
            &hash_a[..] == &hash_b[..]
        })
    }

    #[bench]
    fn bench_64b_constant_time_cmp_eq(bh: &mut Bencher) {
        let hash_a = sha512::Hash::hash(&[0; 1]);
        let hash_b = sha512::Hash::hash(&[0; 1]);
        bh.iter(|| {
            fixed_time_eq(&hash_a[..], &hash_b[..])
        })
    }

    #[bench]
    fn bench_64b_slice_cmp_eq(bh: &mut Bencher) {
        let hash_a = sha512::Hash::hash(&[0; 1]);
        let hash_b = sha512::Hash::hash(&[0; 1]);
        bh.iter(|| {
            &hash_a[..] == &hash_b[..]
        })
    }
}
