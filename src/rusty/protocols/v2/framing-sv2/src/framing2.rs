use crate::header::Header;
use crate::header::NoiseHeader;
use alloc::vec::Vec;
use binary_sv2::Serialize;
use binary_sv2::{to_writer, GetSize};
use core::convert::TryFrom;

const NOISE_MAX_LEN: usize = const_sv2::NOISE_FRAME_MAX_SIZE;

pub trait Frame<'a, T: Serialize + GetSize>: Sized {
    type Buffer: AsMut<[u8]>;
    type Deserialized;

    /// Serialize the frame into dst if the frame is already serialized it just swap dst with
    /// itself
    fn serialize(self, dst: &mut Self::Buffer) -> Result<(), binary_sv2::Error>;

    ///fn deserialize(&'a mut self) -> Result<Self::Deserialized, serde_sv2::Error>;
    fn payload(&'a mut self) -> &'a mut [u8];

    /// If is an Sv2 frame return the Some(header) if it is a noise frame return None
    fn get_header(&self) -> Option<crate::header::Header>;

    /// Try to build an Frame frame from raw bytes.
    /// It return the frame or the number of the bytes needed to complete the frame
    /// The resulting frame is just a header plus a payload with the right number of bytes nothing
    /// is said about the correctness of the payload
    fn from_bytes(bytes: Self::Buffer) -> Result<Self, isize>;

    fn from_bytes_unchecked(bytes: Self::Buffer) -> Self;

    fn size_hint(bytes: &[u8]) -> isize;

    fn encoded_length(&self) -> usize;

    /// Try to build an Frame frame from a serializable payload.
    /// It return a Frame if the size of the payload fit in the frame, if not it return None
    fn from_message(message: T, message_type: u8, extension_type: u16) -> Option<Self>;
}

#[derive(Debug)]
pub struct Sv2Frame<T, B> {
    header: Header,
    payload: Option<T>,
    serialized: Option<B>,
}

impl<T, B> Default for Sv2Frame<T, B> {
    fn default() -> Self {
        Sv2Frame {
            header: Header::default(),
            payload: None,
            serialized: None,
        }
    }
}

#[derive(Debug)]
pub struct NoiseFrame {
    header: u16,
    payload: Vec<u8>,
}

pub type HandShakeFrame = NoiseFrame;

impl<'a, T: Serialize + GetSize, B: AsMut<[u8]>> Frame<'a, T> for Sv2Frame<T, B> {
    type Buffer = B;
    type Deserialized = B;

    /// Serialize the frame into dst if the frame is already serialized it just swap dst with
    /// itself
    #[inline]
    fn serialize(self, dst: &mut Self::Buffer) -> Result<(), binary_sv2::Error> {
        if self.serialized.is_some() {
            *dst = self.serialized.unwrap();
            Ok(())
        } else {
            #[cfg(not(feature = "with_serde"))]
            to_writer(self.header, dst.as_mut())?;
            #[cfg(not(feature = "with_serde"))]
            to_writer(self.payload.unwrap(), &mut dst.as_mut()[Header::SIZE..])?;
            #[cfg(feature = "with_serde")]
            to_writer(&self.header, dst.as_mut())?;
            #[cfg(feature = "with_serde")]
            to_writer(&self.payload.unwrap(), &mut dst.as_mut()[Header::SIZE..])?;
            Ok(())
        }
    }

    fn payload(&'a mut self) -> &'a mut [u8] {
        if self.payload.is_some() {
            todo!()
        } else {
            &mut self.serialized.as_mut().unwrap().as_mut()[Header::SIZE..]
        }
    }

    /// If is an Sv2 frame return the Some(header) if it is a noise frame return None
    fn get_header(&self) -> Option<crate::header::Header> {
        Some(self.header)
    }

    /// Try to build a Frame frame from raw bytes.
    /// It return the frame or the number of the bytes needed to complete the frame
    /// The resulting frame is just a header plus a payload with the right number of bytes nothing
    /// is said about the correctness of the payload
    #[inline]
    fn from_bytes(mut bytes: Self::Buffer) -> Result<Self, isize> {
        let hint = Self::size_hint(bytes.as_mut());

        if hint == 0 {
            Ok(Self::from_bytes_unchecked(bytes))
        } else {
            Err(hint)
        }
    }

    #[inline]
    fn from_bytes_unchecked(mut bytes: Self::Buffer) -> Self {
        let header = Header::from_bytes(bytes.as_mut()).unwrap();
        Self {
            header,
            payload: None,
            serialized: Some(bytes),
        }
    }

    #[inline]
    fn size_hint(bytes: &[u8]) -> isize {
        match Header::from_bytes(bytes) {
            Err(i) => i,
            Ok(header) => {
                if bytes.len() - Header::SIZE == header.len() {
                    0
                } else {
                    (bytes.len() - Header::SIZE) as isize + header.len() as isize
                }
            }
        }
    }

    #[inline]
    fn encoded_length(&self) -> usize {
        if self.serialized.is_some() {
            unimplemented!()
        } else {
            self.payload.as_ref().unwrap().get_size() + Header::SIZE
        }
    }

    /// Try to build an Frame frame from a serializable payload.
    /// It returns a Frame if the size of the payload fits in the frame, if not it returns None
    fn from_message(message: T, message_type: u8, extension_type: u16) -> Option<Self> {
        let len = message.get_size() as u32;
        Header::from_len(len, message_type, extension_type).map(|header| Self {
            header,
            payload: Some(message),
            serialized: None,
        })
    }
}

#[inline]
pub fn build_noise_frame_header(frame: &mut Vec<u8>, len: u16) {
    frame.push(len.to_le_bytes()[0]);
    frame.push(len.to_le_bytes()[1]);
}

impl<'a> Frame<'a, Vec<u8>> for NoiseFrame {
    //impl<T: Serialize + GetSize> Frame<T> for NoiseFrame {

    type Buffer = Vec<u8>;
    type Deserialized = &'a mut [u8];

    /// Serialize the frame into dst if the frame is already serialized it just swap dst with
    /// itself
    #[inline]
    fn serialize(self, dst: &mut Self::Buffer) -> Result<(), binary_sv2::Error> {
        *dst = self.payload;
        Ok(())
    }

    #[inline]
    fn payload(&'a mut self) -> &'a mut [u8] {
        &mut self.payload[NoiseHeader::SIZE..]
    }

    /// If is an Sv2 frame return the Some(header) if it is a noise frame return None
    fn get_header(&self) -> Option<crate::header::Header> {
        None
    }

    /// Try to build a Frame frame from raw bytes.
    /// It return the frame or the number of the bytes needed to complete the frame
    /// The resulting frame is just a header plus a payload with the right number of bytes nothing
    /// is said about the correctness of the payload
    fn from_bytes(_bytes: Self::Buffer) -> Result<Self, isize> {
        unimplemented!()
    }

    #[inline]
    fn from_bytes_unchecked(bytes: Self::Buffer) -> Self {
        let len_b = &bytes[NoiseHeader::LEN_OFFSET..NoiseHeader::SIZE];
        let expected_len = u16::from_le_bytes([len_b[0], len_b[1]]) as usize;

        Self {
            header: expected_len as u16,
            payload: bytes,
        }
    }

    #[inline]
    fn size_hint(bytes: &[u8]) -> isize {
        if bytes.len() < NoiseHeader::SIZE {
            return (NoiseHeader::SIZE - bytes.len()) as isize;
        };

        let len_b = &bytes[NoiseHeader::LEN_OFFSET..NoiseHeader::SIZE];
        let expected_len = u16::from_le_bytes([len_b[0], len_b[1]]) as usize;

        if bytes.len() - NoiseHeader::SIZE == expected_len {
            0
        } else {
            expected_len as isize - (bytes.len() - NoiseHeader::SIZE) as isize
        }
    }

    #[inline]
    fn encoded_length(&self) -> usize {
        self.payload.len()
    }

    /// Try to build a `Frame` frame from a serializable payload.
    /// It returns a Frame if the size of the payload fits in the frame, if not it returns None
    /// Inneficient should be used only to build `HandShakeFrames`
    fn from_message(message: Vec<u8>, _message_type: u8, _extension_type: u16) -> Option<Self> {
        if message.len() <= NOISE_MAX_LEN {
            let header = message.len() as u16;
            let payload = [&header.to_le_bytes()[..], &message[..]].concat();
            Some(Self { header, payload })
        } else {
            None
        }
    }
}

/// A frame can be either
/// 1: Sv2Frame
/// 2: NoiseFrame
/// 3: HandashakeFrame
///
#[derive(Debug)]
pub enum EitherFrame<T, B> {
    HandShake(HandShakeFrame),
    Sv2(Sv2Frame<T, B>),
}

impl<T: Serialize + GetSize, B: AsMut<[u8]>> EitherFrame<T, B> {
    //pub fn serialize(mut self, dst: &mut B) -> Result<(), serde_sv2::Error> {
    //    match self {
    //        Self::HandShake(frame) => todo!(),
    //        Self::Sv2(frame) => frame.serialize(dst),
    //    }
    //}

    pub fn encoded_length(&self) -> usize {
        match &self {
            Self::HandShake(frame) => frame.encoded_length(),
            Self::Sv2(frame) => frame.encoded_length(),
        }
    }
}

impl<T, B> TryFrom<EitherFrame<T, B>> for HandShakeFrame {
    type Error = ();

    fn try_from(v: EitherFrame<T, B>) -> Result<Self, Self::Error> {
        match v {
            EitherFrame::HandShake(frame) => Ok(frame),
            EitherFrame::Sv2(_) => Err(()),
        }
    }
}

impl<T, B> TryFrom<EitherFrame<T, B>> for Sv2Frame<T, B> {
    type Error = ();

    fn try_from(v: EitherFrame<T, B>) -> Result<Self, Self::Error> {
        match v {
            EitherFrame::Sv2(frame) => Ok(frame),
            EitherFrame::HandShake(_) => Err(()),
        }
    }
}

impl<T, B> From<HandShakeFrame> for EitherFrame<T, B> {
    fn from(v: HandShakeFrame) -> Self {
        Self::HandShake(v)
    }
}

impl<T, B> From<Sv2Frame<T, B>> for EitherFrame<T, B> {
    fn from(v: Sv2Frame<T, B>) -> Self {
        Self::Sv2(v)
    }
}
