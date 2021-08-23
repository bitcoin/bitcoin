#![no_std]

extern crate alloc;

#[cfg(feature = "noise_sv2")]
use alloc::{boxed::Box, vec::Vec};

mod buffer;
mod decoder;
mod encoder;
mod error;

pub use error::Error;

pub use decoder::StandardEitherFrame;
pub use decoder::StandardSv2Frame;

pub use decoder::StandardDecoder;
#[cfg(feature = "noise_sv2")]
pub use decoder::StandardNoiseDecoder;

pub use encoder::Encoder;
#[cfg(feature = "noise_sv2")]
pub use encoder::NoiseEncoder;

pub use framing_sv2::framing2::{Frame, Sv2Frame};
#[cfg(feature = "noise_sv2")]
pub use framing_sv2::framing2::{HandShakeFrame, NoiseFrame};

#[cfg(feature = "noise_sv2")]
pub use noise_sv2::{self, handshake::Step, Initiator, Responder, TransportMode};

#[cfg(feature = "noise_sv2")]
#[derive(Debug)]
pub enum State {
    /// Not yet initialized
    NotInitialized,
    /// Handshake mode where codec is negotiating keys
    HandShake(Box<HandshakeRole>),
    /// Transport mode where AEAD is fully operational. The `TransportMode` object in this variant
    /// as able to perform encryption and decryption resp.
    Transport(TransportMode),
}

#[cfg(feature = "noise_sv2")]
#[derive(Debug)]
pub enum HandshakeRole {
    Initiator(noise_sv2::Initiator),
    Responder(noise_sv2::Responder),
}

#[cfg(feature = "noise_sv2")]
impl HandshakeRole {
    pub fn step(&mut self, in_msg: Option<Vec<u8>>) -> Result<HandShakeFrame, crate::Error> {
        match self {
            Self::Initiator(stepper) => {
                let message = stepper.step(in_msg).map_err(|_| ())?.inner();
                Ok(HandShakeFrame::from_message(message, 0, 0).ok_or(())?)
            }

            Self::Responder(stepper) => {
                let message = stepper.step(in_msg).map_err(|_| ())?.inner();
                Ok(HandShakeFrame::from_message(message, 0, 0).ok_or(())?)
            }
        }
    }

    pub fn into_transport(self) -> Result<TransportMode, crate::Error> {
        match self {
            Self::Initiator(stepper) => {
                let tp = stepper
                    .into_handshake_state()
                    .into_transport_mode()
                    .map_err(|_| ())?;
                Ok(TransportMode::new(tp))
            }

            Self::Responder(stepper) => {
                let tp = stepper
                    .into_handshake_state()
                    .into_transport_mode()
                    .map_err(|_| ())?;
                Ok(TransportMode::new(tp))
            }
        }
    }
}

#[cfg(feature = "noise_sv2")]
impl State {
    #[inline(always)]
    pub fn is_in_transport_mode(&self) -> bool {
        match self {
            Self::NotInitialized => false,
            Self::HandShake(_) => false,
            Self::Transport(_) => true,
        }
    }

    #[inline(always)]
    pub fn is_not_initialized(&self) -> bool {
        match self {
            Self::NotInitialized => true,
            Self::HandShake(_) => false,
            Self::Transport(_) => false,
        }
    }
}

#[cfg(feature = "noise_sv2")]
impl State {
    pub fn take(&mut self) -> Self {
        let mut new_me = Self::NotInitialized;
        core::mem::swap(&mut new_me, self);
        new_me
    }

    pub fn new() -> Self {
        Self::NotInitialized
    }

    pub fn initialize(inner: HandshakeRole) -> Self {
        Self::HandShake(Box::new(inner))
    }

    pub fn with_transport_mode(tm: TransportMode) -> Self {
        Self::Transport(tm)
    }

    pub fn step(&mut self, in_msg: Option<Vec<u8>>) -> Result<HandShakeFrame, crate::Error> {
        match self {
            Self::NotInitialized => Err(Error::Todo),
            Self::HandShake(stepper) => stepper.step(in_msg),
            Self::Transport(_) => Err(Error::Todo),
        }
    }

    pub fn into_transport_mode(self) -> Result<Self, Error> {
        match self {
            Self::NotInitialized => Err(Error::Todo),
            Self::HandShake(stepper) => {
                let tp = stepper.into_transport()?;

                Ok(Self::with_transport_mode(tp))
            }
            Self::Transport(_) => Ok(self),
        }
    }
}

#[cfg(feature = "noise_sv2")]
impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}
