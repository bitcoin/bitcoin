use alloc::vec::Vec;
use binary_sv2::GetSize;
use binary_sv2::Serialize;
use core::marker::PhantomData;
use framing_sv2::framing2::{Frame as F_, Sv2Frame};


#[derive(Debug)]
pub struct Encoder<T> {
    buffer: Vec<u8>,
    frame: PhantomData<T>,
}

impl<T: Serialize + GetSize> Encoder<T> {
    pub fn encode(&mut self, item: Sv2Frame<T, Vec<u8>>) -> Result<&[u8], crate::Error> {
        let len = item.encoded_length();

        self.buffer.resize(len, 0);

        item.serialize(&mut self.buffer).map_err(|_| ())?;

        Ok(&self.buffer[..])
    }

    pub fn new() -> Self {
        Self {
            // TODO which capacity??
            buffer: Vec::with_capacity(512),
            frame: core::marker::PhantomData,
        }
    }
}

impl<T: Serialize + GetSize> Default for Encoder<T> {
    fn default() -> Self {
        Self::new()
    }
}
