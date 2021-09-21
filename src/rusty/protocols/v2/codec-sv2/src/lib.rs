#![no_std]

extern crate alloc;

mod buffer;
mod decoder;
mod encoder;
mod error;

pub use error::Error;

pub use decoder::StandardEitherFrame;
pub use decoder::StandardSv2Frame;

pub use decoder::StandardDecoder;

pub use encoder::Encoder;

pub use framing_sv2::framing2::{Frame, Sv2Frame};
