use alloc::string::String;
use core::fmt::{self, Display};

use serde::{de, ser};

pub type Result<T> = core::result::Result<T, Error>;

// TODO provode additional information in the error type:
// 1. byte offset into the input
// 2. ??
#[derive(Clone, Debug, PartialEq)]
pub enum Error {
    // One or more variants that can be created by data structures through the
    // `ser::Error` and `de::Error` traits. For example the Serialize impl for
    // Mutex<T> might return an error because the mutex is poisoned, or the
    // Deserialize impl for a struct may return an error because a required
    // field is missing.
    Message(String),

    StringLenBiggerThan256,
    InvalidUtf8,
    LenBiggerThan16M,
    LenBiggerThan255,
    LenBiggerThan32,
    LenBiggerThan64K,
    WriteError,
    ReadError,
    InvalidBoolSize(usize),
    InvalidBool(u8),
    InvalidU256(usize),
    InvalidSignatureSize(usize),
    InvalidU16Size(usize),
    InvalidU24Size(usize),
    InvalidU32Size(usize),
    InvalidU64Size(usize),
    U24TooBig(u32),
}

impl ser::Error for Error {
    fn custom<T: Display>(msg: T) -> Self {
        Error::Message(format!("{}", msg))
    }
}

impl de::Error for Error {
    fn custom<T: Display>(msg: T) -> Self {
        Error::Message(format!("{}", msg))
    }
}

impl Display for Error {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        match self {
            Error::Message(msg) => formatter.write_str(msg),
            Error::WriteError => formatter.write_str("write error"),
            Error::ReadError => formatter.write_str("read error"),
            _ => formatter.write_str("TODO display not implemented"),
        }
    }
}

// impl core::error::Error for Error {}
