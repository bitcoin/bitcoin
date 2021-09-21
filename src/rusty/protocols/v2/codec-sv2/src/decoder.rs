use binary_sv2::Serialize;
use core::marker::PhantomData;
use framing_sv2::framing2::{EitherFrame, Frame as F_, Sv2Frame};
use framing_sv2::header::Header;

use crate::buffer::{Buffer, SlowAndCorrect};
use crate::error::{Error, Result};

pub type StandardEitherFrame<T> = EitherFrame<T, <SlowAndCorrect as Buffer>::Slice>;
pub type StandardSv2Frame<T> = Sv2Frame<T, <SlowAndCorrect as Buffer>::Slice>;
pub type StandardDecoder<T> = WithoutNoise<SlowAndCorrect, T>;

#[derive(Debug)]
pub struct WithoutNoise<B: Buffer, T: Serialize + binary_sv2::GetSize> {
    frame: PhantomData<T>,
    missing_b: usize,
    buffer: B,
}

impl<T: Serialize + binary_sv2::GetSize, B: Buffer> WithoutNoise<B, T> {
    #[inline]
    pub fn next_frame(&mut self) -> Result<Sv2Frame<T, B::Slice>> {
        let len = self.buffer.len();
        let src = self.buffer.get_data_by_ref(len);
        let hint = Sv2Frame::<T, B::Slice>::size_hint(src) as usize;

        match hint {
            0 => {
                self.missing_b = Header::SIZE;
                let src = self.buffer.get_data_owned();
                let frame = Sv2Frame::<T, B::Slice>::from_bytes_unchecked(src);
                Ok(frame)
            }
            _ => {
                self.missing_b = hint;
                Err(Error::MissingBytes(self.missing_b))
            }
        }
    }

    pub fn writable(&mut self) -> &mut [u8] {
        self.buffer.get_writable(self.missing_b)
    }
}

impl<T: Serialize + binary_sv2::GetSize> WithoutNoise<SlowAndCorrect, T> {
    pub fn new() -> Self {
        Self {
            frame: PhantomData,
            missing_b: Header::SIZE,
            buffer: SlowAndCorrect::new(),
        }
    }
}

impl<T: Serialize + binary_sv2::GetSize> Default for WithoutNoise<SlowAndCorrect, T> {
    fn default() -> Self {
        Self::new()
    }
}
