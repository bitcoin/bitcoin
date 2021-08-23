// At lower level I generally prefer to work with slices as more efficient than Read/Write streams
// eg: Read for & [u8] is implemented with memcpy but here is more than enough to just get a
// pointer to the original data

// ANche se enum decode sarebbe faclie da implementare non viene fatto dato che ogni messaggio puo
// essere derivato dal suo numero!
use crate::Error;
pub mod decodable;
pub mod encodable;
mod impls;

/// Return the encoded byte size or a `Decodable`
pub trait SizeHint {
    fn size_hint(data: &[u8], offset: usize) -> Result<usize, Error>;
    fn size_hint_(&self, data: &[u8], offset: usize) -> Result<usize, Error>;
}

/// Return the encoded byte size of an `Encodable` comprehensive of the header, if any
pub trait GetSize {
    fn get_size(&self) -> usize;
}

/// Implemented by all the primitives with a fixed size
pub trait Fixed {
    const SIZE: usize;
}

pub trait Variable {
    const HEADER_SIZE: usize;
    //const ELEMENT_SIZE: usize;
    const MAX_SIZE: usize;

    fn inner_size(&self) -> usize;

    // TODO use [u8; HEADER_SIZE] instead of Vec
    fn get_header(&self) -> Vec<u8>;
}

impl<T: Fixed> SizeHint for T {
    /// Total size of the encoded data type compreensive of the header when present
    fn size_hint(_data: &[u8], _offset: usize) -> Result<usize, Error> {
        Ok(Self::SIZE)
    }

    fn size_hint_(&self, _: &[u8], _offset: usize) -> Result<usize, Error> {
        Ok(Self::SIZE)
    }
}

impl<T: Fixed> GetSize for T {
    fn get_size(&self) -> usize {
        Self::SIZE
    }
}
