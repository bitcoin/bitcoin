use alloc::vec::Vec;
use binary_sv2::GetSize;
use binary_sv2::Serialize;
#[cfg(feature = "noise_sv2")]
use core::cmp::min;
#[cfg(feature = "noise_sv2")]
use core::convert::TryInto;
use core::marker::PhantomData;
#[cfg(feature = "noise_sv2")]
use framing_sv2::framing2::{build_noise_frame_header, EitherFrame, HandShakeFrame};
use framing_sv2::framing2::{Frame as F_, Sv2Frame};
#[cfg(feature = "noise_sv2")]
use framing_sv2::header::NoiseHeader;

#[cfg(feature = "noise_sv2")]
use crate::{State, TransportMode};

#[cfg(feature = "noise_sv2")]
const TAGLEN: usize = const_sv2::SNOW_TAGLEN;
#[cfg(feature = "noise_sv2")]
const MAX_M_L: usize = const_sv2::NOISE_FRAME_MAX_SIZE;
#[cfg(feature = "noise_sv2")]
const M: usize = MAX_M_L - TAGLEN;

#[cfg(feature = "noise_sv2")]
pub struct NoiseEncoder<T: Serialize + binary_sv2::GetSize> {
    noise_buffer: Vec<u8>,
    sv2_buffer: Vec<u8>,
    frame: PhantomData<T>,
}

#[cfg(feature = "noise_sv2")]
impl<T: Serialize + GetSize> NoiseEncoder<T> {
    #[inline]
    pub fn encode(
        &mut self,
        item: EitherFrame<T, Vec<u8>>,
        state: &mut State,
    ) -> Result<&[u8], crate::Error> {
        match state {
            State::Transport(transport_mode) => {
                let len = item.encoded_length();
                self.sv2_buffer.resize(len, 0);

                // ENCODE THE SV2 FRAME
                let i: Sv2Frame<T, Vec<u8>> = item.try_into().map_err(|_| ())?;
                i.serialize(&mut self.sv2_buffer).map_err(|_| ())?;

                // IF THE MESSAGE FIT INTO A NOISE FRAME ENCODE IT HOT PATH
                if len <= M {
                    self.encode_single_frame(transport_mode)?;

                // IF LEN IS BIGGER THAN NOISE PAYLOAD MAX SIZE MESSAGE IS ENCODED AS SEVERAL NOISE
                // MESSAGES COLD PATH
                } else {
                    self.encode_multiple_frame(transport_mode)?;
                }
            }
            State::HandShake(_) => self.while_handshaking(item)?,
            State::NotInitialized => self.while_handshaking(item)?,
        };

        Ok(&self.noise_buffer[..])
    }

    #[inline(always)]
    fn encode_single_frame(&mut self, transport_mode: &mut TransportMode) -> Result<(), ()> {
        // RESERVE ENAUGH SPACE TO ENCODE THE NOISE MESSAGE
        let len = TransportMode::size_hint_encrypt(self.sv2_buffer[..].len());
        let len_with_header = len + NoiseHeader::SIZE;

        let to_reserve = if self.noise_buffer.len() > len_with_header {
            0
        } else {
            len_with_header - self.noise_buffer.len()
        };
        self.noise_buffer.reserve(to_reserve);
        self.noise_buffer.clear();

        // PREPEND THE NOISE FRAME HEADER
        build_noise_frame_header(&mut self.noise_buffer, len as u16);

        // RESIZE THE BUFFER SO TRANSPORT MODE CAN WRITE IN IT
        self.noise_buffer.resize(len_with_header, 0);

        // ENCRYPT THE SV2 FRAME AND ENCODE THE NOISE FRAME
        transport_mode
            .write(
                &self.sv2_buffer[..],
                &mut self.noise_buffer[NoiseHeader::SIZE..],
            )
            .map_err(|_| ())
    }

    #[inline(never)]
    fn encode_multiple_frame(&mut self, transport_mode: &mut TransportMode) -> Result<(), ()> {
        self.noise_buffer.clear();

        let buffer_len: usize = self.sv2_buffer.len();
        let mut start: usize = 0;
        let mut end: usize = M;

        loop {
            let last_len = self.noise_buffer.len();

            end = min(end, buffer_len);

            let buf = &self.sv2_buffer[start..end];

            // PREPEND THE NOISE FRAME HEADER
            let len = TransportMode::size_hint_encrypt(buf.len());
            build_noise_frame_header(&mut self.noise_buffer, len as u16);

            // RESIZE THE BUFFER SO TRANSPORT MODE CAN WRITE IN IT
            self.noise_buffer.resize(self.noise_buffer.len() + len, 0);

            // ENCRYPT THE SV2 FRAGMENT
            transport_mode
                .write(
                    buf,
                    &mut self.noise_buffer
                        [last_len + NoiseHeader::SIZE..last_len + NoiseHeader::SIZE + len],
                )
                .map_err(|_| ())?;

            if end == buffer_len {
                break;
            }

            start += end;
            end += end;
        }
        Ok(())
    }

    #[inline(never)]
    fn while_handshaking(&mut self, item: EitherFrame<T, Vec<u8>>) -> Result<(), ()> {
        // ENCODE THE SV2 FRAME
        let i: HandShakeFrame = item.try_into().map_err(|_| ())?;
        i.serialize(&mut self.sv2_buffer).map_err(|_| ())?;

        self.noise_buffer.clear();
        self.noise_buffer.extend_from_slice(&self.sv2_buffer[..]);

        Ok(())
    }
}

#[cfg(feature = "noise_sv2")]
impl<T: Serialize + binary_sv2::GetSize> NoiseEncoder<T> {
    pub fn new() -> Self {
        Self {
            // TODO which capacity??
            sv2_buffer: Vec::with_capacity(512),
            noise_buffer: Vec::with_capacity(512),
            frame: core::marker::PhantomData,
        }
    }
}

#[cfg(feature = "noise_sv2")]
impl<T: Serialize + GetSize> Default for NoiseEncoder<T> {
    fn default() -> Self {
        Self::new()
    }
}

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
