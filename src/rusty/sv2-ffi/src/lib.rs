#![cfg(not(feature = "with_serde"))]

use codec_sv2::{Encoder, Frame, StandardDecoder, StandardSv2Frame};
use common_messages_sv2::{
    CSetupConnection, CSetupConnectionError, ChannelEndpointChanged, SetupConnection,
    SetupConnectionError, SetupConnectionSuccess,
};
use template_distribution_sv2::{
    CNewTemplate, CRequestTransactionDataError, CRequestTransactionDataSuccess, CSetNewPrevHash,
    CSubmitSolution, CoinbaseOutputDataSize, NewTemplate, RequestTransactionData,
    RequestTransactionDataError, RequestTransactionDataSuccess, SetNewPrevHash, SubmitSolution,
};

use binary_sv2::{
    binary_codec_sv2::CVec, decodable::DecodableField, decodable::FieldMarker,
    encodable::EncodableField, from_bytes, Deserialize, Error,
};

use const_sv2::{
    MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGES, MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE,
    MESSAGE_TYPE_NEW_TEMPLATE, MESSAGE_TYPE_REQUEST_TRANSACTION_DATA,
    MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR, MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS,
    MESSAGE_TYPE_SETUP_CONNECTION, MESSAGE_TYPE_SETUP_CONNECTION_ERROR,
    MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS, MESSAGE_TYPE_SET_NEW_PREV_HASH,
    MESSAGE_TYPE_SUBMIT_SOLUTION,
};
use core::convert::{TryFrom, TryInto};

#[derive(Clone, Debug)]
pub enum Sv2Message<'a> {
    CoinbaseOutputDataSize(CoinbaseOutputDataSize),
    NewTemplate(NewTemplate<'a>),
    RequestTransactionData(RequestTransactionData),
    RequestTransactionDataError(RequestTransactionDataError<'a>),
    RequestTransactionDataSuccess(RequestTransactionDataSuccess<'a>),
    SetNewPrevHash(SetNewPrevHash<'a>),
    SubmitSolution(SubmitSolution<'a>),
    ChannelEndpointChanged(ChannelEndpointChanged),
    SetupConnection(SetupConnection<'a>),
    SetupConnectionError(SetupConnectionError<'a>),
    SetupConnectionSuccess(SetupConnectionSuccess),
}

impl<'a> Sv2Message<'a> {
    pub fn message_type(&self) -> u8 {
        match self {
            Sv2Message::CoinbaseOutputDataSize(_) => MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE,
            Sv2Message::NewTemplate(_) => MESSAGE_TYPE_NEW_TEMPLATE,
            Sv2Message::RequestTransactionData(_) => MESSAGE_TYPE_REQUEST_TRANSACTION_DATA,
            Sv2Message::RequestTransactionDataError(_) => {
                MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR
            }
            Sv2Message::RequestTransactionDataSuccess(_) => {
                MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS
            }
            Sv2Message::SetNewPrevHash(_) => MESSAGE_TYPE_SET_NEW_PREV_HASH,
            Sv2Message::SubmitSolution(_) => MESSAGE_TYPE_SUBMIT_SOLUTION,
            Sv2Message::ChannelEndpointChanged(_) => MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGES,
            Sv2Message::SetupConnection(_) => MESSAGE_TYPE_SETUP_CONNECTION,
            Sv2Message::SetupConnectionError(_) => MESSAGE_TYPE_SETUP_CONNECTION_ERROR,
            Sv2Message::SetupConnectionSuccess(_) => MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS,
        }
    }
}

#[repr(C)]
pub enum CSv2Message {
    CoinbaseOutputDataSize(CoinbaseOutputDataSize),
    NewTemplate(CNewTemplate),
    RequestTransactionData(RequestTransactionData),
    RequestTransactionDataError(CRequestTransactionDataError),
    RequestTransactionDataSuccess(CRequestTransactionDataSuccess),
    SetNewPrevHash(CSetNewPrevHash),
    SubmitSolution(CSubmitSolution),
    ChannelEndpointChanged(ChannelEndpointChanged),
    SetupConnection(CSetupConnection),
    SetupConnectionError(CSetupConnectionError),
    SetupConnectionSuccess(SetupConnectionSuccess),
}

#[no_mangle]
pub extern "C" fn drop_sv2_message(s: CSv2Message) {
    match s {
        CSv2Message::CoinbaseOutputDataSize(_) => (),
        CSv2Message::NewTemplate(a) => drop(a),
        CSv2Message::RequestTransactionData(a) => drop(a),
        CSv2Message::RequestTransactionDataError(a) => drop(a),
        CSv2Message::RequestTransactionDataSuccess(a) => drop(a),
        CSv2Message::SetNewPrevHash(a) => drop(a),
        CSv2Message::SubmitSolution(a) => drop(a),
        CSv2Message::ChannelEndpointChanged(_) => (),
        CSv2Message::SetupConnection(_) => (),
        CSv2Message::SetupConnectionError(a) => drop(a),
        CSv2Message::SetupConnectionSuccess(a) => drop(a),
    }
}

impl<'a> From<Sv2Message<'a>> for CSv2Message {
    fn from(v: Sv2Message<'a>) -> Self {
        match v {
            Sv2Message::CoinbaseOutputDataSize(a) => Self::CoinbaseOutputDataSize(a),
            Sv2Message::NewTemplate(a) => Self::NewTemplate(a.into()),
            Sv2Message::RequestTransactionData(a) => Self::RequestTransactionData(a),
            Sv2Message::RequestTransactionDataError(a) => {
                Self::RequestTransactionDataError(a.into())
            }
            Sv2Message::RequestTransactionDataSuccess(a) => {
                Self::RequestTransactionDataSuccess(a.into())
            }
            Sv2Message::SetNewPrevHash(a) => Self::SetNewPrevHash(a.into()),
            Sv2Message::SubmitSolution(a) => Self::SubmitSolution(a.into()),
            Sv2Message::ChannelEndpointChanged(a) => Self::ChannelEndpointChanged(a),
            Sv2Message::SetupConnection(a) => Self::SetupConnection(a.into()),
            Sv2Message::SetupConnectionError(a) => Self::SetupConnectionError(a.into()),
            Sv2Message::SetupConnectionSuccess(a) => Self::SetupConnectionSuccess(a),
        }
    }
}

impl<'a> CSv2Message {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<Sv2Message<'a>, Error> {
        match self {
            CSv2Message::NewTemplate(v) => Ok(Sv2Message::NewTemplate(v.to_rust_rep_mut()?)),
            CSv2Message::SetNewPrevHash(v) => Ok(Sv2Message::SetNewPrevHash(v.to_rust_rep_mut()?)),
            CSv2Message::SubmitSolution(v) => Ok(Sv2Message::SubmitSolution(v.to_rust_rep_mut()?)),
            CSv2Message::SetupConnection(v) => {
                Ok(Sv2Message::SetupConnection(v.to_rust_rep_mut()?))
            }
            CSv2Message::SetupConnectionError(v) => {
                Ok(Sv2Message::SetupConnectionError(v.to_rust_rep_mut()?))
            }
            _ => todo!(),
        }
    }
}

impl<'decoder> From<Sv2Message<'decoder>> for EncodableField<'decoder> {
    fn from(m: Sv2Message<'decoder>) -> Self {
        match m {
            Sv2Message::CoinbaseOutputDataSize(a) => a.into(),
            Sv2Message::NewTemplate(a) => a.into(),
            Sv2Message::RequestTransactionData(a) => a.into(),
            Sv2Message::RequestTransactionDataError(a) => a.into(),
            Sv2Message::RequestTransactionDataSuccess(a) => a.into(),
            Sv2Message::SetNewPrevHash(a) => a.into(),
            Sv2Message::SubmitSolution(a) => a.into(),
            Sv2Message::ChannelEndpointChanged(a) => a.into(),
            Sv2Message::SetupConnection(a) => a.into(),
            Sv2Message::SetupConnectionError(a) => a.into(),
            Sv2Message::SetupConnectionSuccess(a) => a.into(),
        }
    }
}

impl binary_sv2::GetSize for Sv2Message<'_> {
    fn get_size(&self) -> usize {
        match self {
            Sv2Message::CoinbaseOutputDataSize(a) => a.get_size(),
            Sv2Message::NewTemplate(a) => a.get_size(),
            Sv2Message::RequestTransactionData(a) => a.get_size(),
            Sv2Message::RequestTransactionDataError(a) => a.get_size(),
            Sv2Message::RequestTransactionDataSuccess(a) => a.get_size(),
            Sv2Message::SetNewPrevHash(a) => a.get_size(),
            Sv2Message::SubmitSolution(a) => a.get_size(),
            Sv2Message::ChannelEndpointChanged(a) => a.get_size(),
            Sv2Message::SetupConnection(a) => a.get_size(),
            Sv2Message::SetupConnectionError(a) => a.get_size(),
            Sv2Message::SetupConnectionSuccess(a) => a.get_size(),
        }
    }
}

impl<'decoder> Deserialize<'decoder> for Sv2Message<'decoder> {
    fn get_structure(_v: &[u8]) -> std::result::Result<Vec<FieldMarker>, binary_sv2::Error> {
        unimplemented!()
    }
    fn from_decoded_fields(
        _v: Vec<DecodableField<'decoder>>,
    ) -> std::result::Result<Self, binary_sv2::Error> {
        unimplemented!()
    }
}

impl<'a> TryFrom<(u8, &'a mut [u8])> for Sv2Message<'a> {
    type Error = Error;

    fn try_from(v: (u8, &'a mut [u8])) -> Result<Self, Self::Error> {
        let msg_type = v.0;
        match msg_type {
            MESSAGE_TYPE_SETUP_CONNECTION => {
                let message: SetupConnection<'a> = from_bytes(v.1)?;
                Ok(Sv2Message::SetupConnection(message))
            }
            MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS => {
                let message: SetupConnectionSuccess = from_bytes(v.1)?;
                Ok(Sv2Message::SetupConnectionSuccess(message))
            }
            MESSAGE_TYPE_SETUP_CONNECTION_ERROR => {
                let message: SetupConnectionError<'a> = from_bytes(v.1)?;
                Ok(Sv2Message::SetupConnectionError(message))
            }
            MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGES => {
                let message: ChannelEndpointChanged = from_bytes(v.1)?;
                Ok(Sv2Message::ChannelEndpointChanged(message))
            }
            MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE => {
                let message: CoinbaseOutputDataSize = from_bytes(v.1)?;
                Ok(Sv2Message::CoinbaseOutputDataSize(message))
            }
            MESSAGE_TYPE_NEW_TEMPLATE => {
                let message: NewTemplate<'a> = from_bytes(v.1)?;
                Ok(Sv2Message::NewTemplate(message))
            }
            MESSAGE_TYPE_SET_NEW_PREV_HASH => {
                let message: SetNewPrevHash<'a> = from_bytes(v.1)?;
                Ok(Sv2Message::SetNewPrevHash(message))
            }
            MESSAGE_TYPE_REQUEST_TRANSACTION_DATA => {
                let message: RequestTransactionData = from_bytes(v.1)?;
                Ok(Sv2Message::RequestTransactionData(message))
            }
            MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS => {
                let message: RequestTransactionDataSuccess = from_bytes(v.1)?;
                Ok(Sv2Message::RequestTransactionDataSuccess(message))
            }
            MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR => {
                let message: RequestTransactionDataError = from_bytes(v.1)?;
                Ok(Sv2Message::RequestTransactionDataError(message))
            }
            MESSAGE_TYPE_SUBMIT_SOLUTION => {
                let message: SubmitSolution = from_bytes(v.1)?;
                Ok(Sv2Message::SubmitSolution(message))
            }
            _ => panic!(),
        }
    }
}

#[repr(C)]
pub enum CResult<T, E> {
    Ok(T),
    Err(E),
}

#[repr(C)]
pub enum Sv2Error {
    MissingBytes,
    EncoderBusy,
    Todo,
    Unknown,
}

#[no_mangle]
pub extern "C" fn is_ok(cresult: &CResult<CSv2Message, Sv2Error>) -> bool {
    match cresult {
        CResult::Ok(_) => true,
        CResult::Err(_) => false,
    }
}

impl<T, E> From<Result<T, E>> for CResult<T, E> {
    fn from(v: Result<T, E>) -> Self {
        match v {
            Ok(v) => Self::Ok(v),
            Err(e) => Self::Err(e),
        }
    }
}

#[derive(Debug)]
pub struct EncoderWrapper {
    encoder: Encoder<Sv2Message<'static>>,
    free: bool,
}

#[no_mangle]
pub extern "C" fn new_encoder() -> *mut EncoderWrapper {
    let encoder: Encoder<Sv2Message<'static>> = Encoder::new();
    let s = Box::new(EncoderWrapper {
        encoder,
        free: true,
    });
    Box::into_raw(s)
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "C" fn free_encoder(encoder: *mut EncoderWrapper) {
    let mut encoder = unsafe { Box::from_raw(encoder) };
    encoder.free = true;
    Box::into_raw(encoder);
}

fn encode_(message: &'static mut CSv2Message, encoder: &mut EncoderWrapper) -> Result<CVec, Error> {
    let message: Sv2Message = message.to_rust_rep_mut()?;
    let m_type = message.message_type();
    let frame = StandardSv2Frame::<Sv2Message<'static>>::from_message(message, m_type, 0)
        .ok_or(Error::Todo)?;
    encoder
        .encoder
        .encode(frame)
        .map_err(|_| Error::Todo)
        .map(|x| x.into())
}

/// # Safety
///
/// TODO
#[no_mangle]
pub unsafe extern "C" fn encode(
    message: &'static mut CSv2Message,
    encoder: *mut EncoderWrapper,
) -> CResult<CVec, Sv2Error> {
    let mut encoder = Box::from_raw(encoder);
    if encoder.free {
        let result = encode_(message, &mut encoder)
            .map_err(|_| Sv2Error::Todo)
            .into();
        encoder.free = false;
        Box::into_raw(encoder);
        result
    } else {
        CResult::Err(Sv2Error::EncoderBusy)
    }
}

#[derive(Debug)]
pub struct DecoderWrapper(StandardDecoder<Sv2Message<'static>>);

#[no_mangle]
pub extern "C" fn new_decoder() -> *mut DecoderWrapper {
    let s = Box::new(DecoderWrapper(StandardDecoder::new()));
    Box::into_raw(s)
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "C" fn get_writable(decoder: *mut DecoderWrapper) -> CVec {
    let mut decoder = unsafe { Box::from_raw(decoder) };
    let writable = decoder.0.writable();
    let res = CVec::as_shared_buffer(writable);
    Box::into_raw(decoder);
    res
}

#[no_mangle]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub extern "C" fn next_frame(decoder: *mut DecoderWrapper) -> CResult<CSv2Message, Sv2Error> {
    let mut decoder = unsafe { Box::from_raw(decoder) };

    match decoder.0.next_frame() {
        Ok(mut f) => {
            let msg_type = f.get_header().unwrap().msg_type();
            let payload = f.payload();
            let len = payload.len();
            let ptr = payload.as_mut_ptr();
            let payload = unsafe { std::slice::from_raw_parts_mut(ptr, len) };
            Box::into_raw(decoder);
            (msg_type, payload)
                .try_into()
                .map(|x: Sv2Message| x.into())
                .map_err(|_| Sv2Error::Unknown)
                .into()
        }
        Err(_) => {
            Box::into_raw(decoder);
            CResult::Err(Sv2Error::MissingBytes)
        }
    }
}
