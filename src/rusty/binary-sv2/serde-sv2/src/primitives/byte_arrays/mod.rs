pub mod b016m;
pub mod b0255;
pub mod b032;
pub mod b064k;
pub mod bytes;

#[test]
fn test_b0_64k() {
    use crate::ser::to_bytes;
    use core::convert::TryInto;

    let test: b064k::B064K = (&[1, 2, 9][..])
        .try_into()
        .expect("vector smaller than 64K should not fail");

    let expected = vec![3, 0, 1, 2, 9];
    assert_eq!(to_bytes(&test).unwrap(), expected);
}

#[test]
fn test_b0_64k_2() {
    use crate::ser::to_bytes;
    use core::convert::TryInto;

    let test: b064k::B064K = (&[10; 754][..])
        .try_into()
        .expect("vector smaller than 64K should not fail");

    let mut expected = vec![10; 756];
    expected[0] = 242;
    expected[1] = 2;
    assert_eq!(to_bytes(&test).unwrap(), expected);
}

#[test]
fn test_b0_64k_3() {
    use crate::error::Error;
    use core::convert::TryInto;

    let test: Result<b064k::B064K, Error> = (&[10; 70000][..]).try_into();

    match test {
        Ok(_) => assert!(false, "vector bigger than 64K should return an error"),
        Err(_) => assert!(true),
    }
}

#[test]
fn test_b0_16m() {
    use crate::ser::to_bytes;
    use core::convert::TryInto;

    let test: b016m::B016M = (&[1 as u8, 2, 9][..])
        .try_into()
        .expect("vector smaller than 64K should not fail");

    let expected = vec![3, 0, 0, 1, 2, 9];
    assert_eq!(to_bytes(&test).unwrap(), expected);
}
