use alloc::vec::Vec;

pub trait Buffer {
    type Slice: AsMut<[u8]>;

    fn get_writable(&mut self, len: usize) -> &mut [u8];

    fn get_data_owned(&mut self) -> Self::Slice;

    fn get_data_by_ref(&mut self, header_size: usize) -> &mut [u8];

    fn len(&self) -> usize;
}

#[derive(Debug)]
pub struct SlowAndCorrect {
    inner: Vec<u8>,
    cursor: usize,
}

impl SlowAndCorrect {
    pub fn new() -> Self {
        Self {
            inner: Vec::new(),
            cursor: 0,
        }
    }
}

impl Default for SlowAndCorrect {
    fn default() -> Self {
        Self::new()
    }
}

impl Buffer for SlowAndCorrect {
    type Slice = Vec<u8>;

    #[inline]
    fn get_writable(&mut self, len: usize) -> &mut [u8] {
        let cursor = self.cursor;
        let len = self.cursor + len;

        if len > self.inner.len() {
            self.inner.resize(len, 0)
        };

        self.cursor = len;

        &mut self.inner[cursor..len]
    }

    #[inline]
    fn get_data_owned(&mut self) -> Vec<u8> {
        let mut tail = self.inner.split_off(self.cursor);
        core::mem::swap(&mut tail, &mut self.inner);
        let head = tail;
        self.cursor = 0;
        head
    }

    #[inline]
    fn get_data_by_ref(&mut self, header_size: usize) -> &mut [u8] {
        &mut self.inner[..usize::min(header_size, self.cursor)]
    }

    #[inline]
    fn len(&self) -> usize {
        self.cursor
    }
}
