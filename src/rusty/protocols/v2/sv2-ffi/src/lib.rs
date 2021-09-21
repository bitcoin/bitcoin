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
            //CSv2Message::CoinbaseOutputDataSize(v) => {Ok(Sv2Message::CoinbaseOutputDataSize(*v))}
            CSv2Message::NewTemplate(v) => Ok(Sv2Message::NewTemplate(v.to_rust_rep_mut()?)),
            //CSv2Message::RequestTransactionData(v) => {Ok(Sv2Message::RequestTransactionData(*v))}
            //CSv2Message::RequestTransactionDataError(mut v) => {Ok(Sv2Message::RequestTransactionDataError(v.to_rust_rep_mut()?))}
            //CSv2Message::RequestTransactionDataSuccess(mut v) => {Ok(Sv2Message::RequestTransactionDataSuccess(v.to_rust_rep_mut()?))}
            CSv2Message::SetNewPrevHash(v) => Ok(Sv2Message::SetNewPrevHash(v.to_rust_rep_mut()?)),
            CSv2Message::SubmitSolution(v) => Ok(Sv2Message::SubmitSolution(v.to_rust_rep_mut()?)),
            //CSv2Message::ChannelEndpointChanged(v) => {Ok(Sv2Message::ChannelEndpointChanged(*v))}
            CSv2Message::SetupConnection(v) => {
                Ok(Sv2Message::SetupConnection(v.to_rust_rep_mut()?))
            }
            CSv2Message::SetupConnectionError(v) => {
                Ok(Sv2Message::SetupConnectionError(v.to_rust_rep_mut()?))
            }
            //CSv2Message::SetupConnectionSuccess(v) => {Ok(Sv2Message::SetupConnectionSuccess(*v))}
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
#[cfg(test)]
#[cfg(feature = "prop_test")]
mod tests {
    use super::*;
    use core::convert::TryInto;
    use quickcheck::{Arbitrary, Gen};
    use quickcheck_macros;

    #[derive(Clone, Debug)]
    pub struct RandomCoinbaseOutputDataSize(pub CoinbaseOutputDataSize);

    impl Arbitrary for RandomCoinbaseOutputDataSize {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomCoinbaseOutputDataSize(CoinbaseOutputDataSize::from_gen(g))
        }
    }

    fn get_setup_connection() -> SetupConnection<'static> {
        get_setup_connection_w_params(
            common_messages_sv2::Protocol::TemplateDistributionProtocol,
            2,
            2,
            0,
            "0.0.0.0".to_string(),
            8081,
            "Bitmain".to_string(),
            "901".to_string(),
            "abcX".to_string(),
            "89567".to_string(),
        )
    }

    fn get_setup_connection_w_params(
        protocol: common_messages_sv2::Protocol,
        min_version: u16,
        max_version: u16,
        flags: u32,
        endpoint_host: String,
        endpoint_port: u16,
        vendor: String,
        hardware_version: String,
        firmware: String,
        device_id: String,
    ) -> SetupConnection<'static> {
        SetupConnection {
            protocol,
            min_version,
            max_version,
            flags,
            endpoint_host: endpoint_host.into_bytes().try_into().unwrap(),
            endpoint_port,
            vendor: vendor.into_bytes().try_into().unwrap(),
            hardware_version: hardware_version.into_bytes().try_into().unwrap(),
            firmware: firmware.into_bytes().try_into().unwrap(),
            device_id: device_id.into_bytes().try_into().unwrap(),
        }
    }

    #[test]
    fn test_message_type_cb_output_data_size() {
        let expect = MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE;
        let cb_output_data_size = CoinbaseOutputDataSize {
            coinbase_output_max_additional_size: 0,
        };
        let sv2_message = Sv2Message::CoinbaseOutputDataSize(cb_output_data_size);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_new_template() {
        let expect = MESSAGE_TYPE_NEW_TEMPLATE;
        let new_template = NewTemplate {
            template_id: 0,
            future_template: false,
            version: 0x01000000,
            coinbase_tx_version: 0x01000000,
            coinbase_prefix: "0".to_string().into_bytes().try_into().unwrap(),
            coinbase_tx_input_sequence: 0xffffffff,
            coinbase_tx_value_remaining: 0x00f2052a,
            coinbase_tx_outputs_count: 1,
            coinbase_tx_outputs: "0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"
                .to_string()
                .into_bytes()
                .try_into()
                .unwrap(),
                coinbase_tx_locktime: 0x00000000,
                merkle_path: binary_sv2::Seq0255::new(Vec::<binary_sv2::U256>::new()).unwrap(),

        };
        let sv2_message = Sv2Message::NewTemplate(new_template);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_request_transaction_data() {
        let expect = MESSAGE_TYPE_REQUEST_TRANSACTION_DATA;
        let request_tx_data = RequestTransactionData { template_id: 0 };
        let sv2_message = Sv2Message::RequestTransactionData(request_tx_data);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_request_transaction_data_error() {
        let expect = MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR;
        let request_tx_data_err = RequestTransactionDataError {
            template_id: 0,
            error_code: "an error code".to_string().into_bytes().try_into().unwrap(),
        };
        let sv2_message = Sv2Message::RequestTransactionDataError(request_tx_data_err);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_request_transaction_data_success() {
        let expect = MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS;

        let request_tx_data_success = RequestTransactionDataSuccess {
            template_id: 0,
            excess_data: "some_excess_data"
                .to_string()
                .into_bytes()
                .try_into()
                .unwrap(),
            transaction_list: binary_sv2::Seq064K::new(Vec::new()).unwrap(),
        };
        let sv2_message = Sv2Message::RequestTransactionDataSuccess(request_tx_data_success);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_set_new_prev_hash() {
        let expect = MESSAGE_TYPE_SET_NEW_PREV_HASH;

        let mut u256 = [0_u8; 32];
        let u256_prev_hash: binary_sv2::U256 = (&mut u256[..]).try_into().unwrap();

        let mut u256 = [0_u8; 32];
        let u256_target: binary_sv2::U256 = (&mut u256[..]).try_into().unwrap();

        let set_new_prev_hash = SetNewPrevHash {
            template_id: 0,
            prev_hash: u256_prev_hash,
            header_timestamp: 0x29ab5f49,
            n_bits: 0xffff001d,
            target: u256_target,
        };
        let sv2_message = Sv2Message::SetNewPrevHash(set_new_prev_hash);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_submit_solution() {
        let expect = MESSAGE_TYPE_SUBMIT_SOLUTION;

        let submit_solution = SubmitSolution {
            template_id: 0,
            version: 0x01000000,
            header_timestamp: 0x29ab5f49,
            header_nonce: 0x1dac2b7c,
            coinbase_tx: "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000"
                .to_string()
                .into_bytes()
                .try_into()
                .unwrap(),
        };

        let sv2_message = Sv2Message::SubmitSolution(submit_solution);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_channel_endpoint_changed() {
        let expect = MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGES;

        let channel_endpoint_changed = ChannelEndpointChanged { channel_id: 0 };

        let sv2_message = Sv2Message::ChannelEndpointChanged(channel_endpoint_changed);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_setup_connection() {
        let expect = MESSAGE_TYPE_SETUP_CONNECTION;

        let setup_connection = get_setup_connection();

        let sv2_message = Sv2Message::SetupConnection(setup_connection);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_setup_connection_error() {
        let expect = MESSAGE_TYPE_SETUP_CONNECTION_ERROR;

        let setup_connection_err = SetupConnectionError {
            flags: 0,
            error_code: "an error code".to_string().into_bytes().try_into().unwrap(),
        };

        let sv2_message = Sv2Message::SetupConnectionError(setup_connection_err);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    fn test_message_type_setup_connection_success() {
        let expect = MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS;

        let setup_connection_success = SetupConnectionSuccess {
            used_version: 1,
            flags: 0,
        };

        let sv2_message = Sv2Message::SetupConnectionSuccess(setup_connection_success);
        let actual = sv2_message.message_type();

        assert_eq!(expect, actual);
    }

    #[test]
    #[ignore]
    fn test_next_frame() {
        let decoder = StandardDecoder::<Sv2Message<'static>>::new();
        println!("DECODER: {:?}", &decoder);
        println!("DECODER 2: {:?}", &decoder);
        let mut decoder_wrapper = DecoderWrapper(decoder);
        let _res = next_frame(&mut decoder_wrapper);
    }

    // RR

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_coinbase_output_data_size(message: RandomCoinbaseOutputDataSize) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<CoinbaseOutputDataSize>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE, 0)
                .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::CoinbaseOutputDataSize(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomNewTemplate(pub NewTemplate<'static>);
    impl Arbitrary for RandomNewTemplate {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomNewTemplate(NewTemplate::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_new_template_id(message: RandomNewTemplate) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<NewTemplate>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        // Create frame
        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_NEW_TEMPLATE, 0).unwrap();
        // Encode frame
        let encoded_frame = encoder.encode(frame).unwrap();

        // Decode encoded frame
        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        // Puts decoder in the next state (next 6 bytes). If frame is incomplete, returns an error
        // prompting to add more bytes to decode the frame
        // Required between two writes because of how this is intended to use the decoder in a loop
        // read from a stream.
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        // Decoded frame, complete frame is filled
        let mut decoded = decoder.next_frame().unwrap();

        // Extract payload of the frame which is the NewTemplate message
        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::NewTemplate(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomRequestTransactionData(pub RequestTransactionData);
    impl Arbitrary for RandomRequestTransactionData {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomRequestTransactionData(RequestTransactionData::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_request_transaction_data(message: RandomRequestTransactionData) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<RequestTransactionData>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_REQUEST_TRANSACTION_DATA, 0)
                .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::RequestTransactionData(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomRequestTransactionDataError(pub RequestTransactionDataError<'static>);

    impl Arbitrary for RandomRequestTransactionDataError {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomRequestTransactionDataError(RequestTransactionDataError::from_gen(g))
        }
    }
    #[quickcheck_macros::quickcheck]
    fn encode_with_c_request_transaction_data_error(
        message: RandomRequestTransactionDataError,
    ) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<RequestTransactionDataError>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame = StandardSv2Frame::from_message(
            message.0,
            MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_ERROR,
            0,
        )
        .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::RequestTransactionDataError(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomRequestTransactionDataSuccess(pub RequestTransactionDataSuccess<'static>);

    impl Arbitrary for RandomRequestTransactionDataSuccess {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomRequestTransactionDataSuccess(RequestTransactionDataSuccess::from_gen(g))
        }
    }
    #[quickcheck_macros::quickcheck]
    fn encode_with_c_request_transaction_data_success(
        message: RandomRequestTransactionDataSuccess,
    ) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<RequestTransactionDataSuccess>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame = StandardSv2Frame::from_message(
            message.0,
            MESSAGE_TYPE_REQUEST_TRANSACTION_DATA_SUCCESS,
            0,
        )
        .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::RequestTransactionDataSuccess(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomSetNewPrevHash(pub SetNewPrevHash<'static>);

    impl Arbitrary for RandomSetNewPrevHash {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomSetNewPrevHash(SetNewPrevHash::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_set_new_prev_hash(message: RandomSetNewPrevHash) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<SetNewPrevHash>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_SET_NEW_PREV_HASH, 0).unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::SetNewPrevHash(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomSubmitSolution(pub SubmitSolution<'static>);

    impl Arbitrary for RandomSubmitSolution {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomSubmitSolution(SubmitSolution::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_submit_solution(message: RandomSubmitSolution) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<SubmitSolution>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_SUBMIT_SOLUTION, 0).unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::SubmitSolution(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomSetupConnection(pub SetupConnection<'static>);

    impl Arbitrary for RandomSetupConnection {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomSetupConnection(SetupConnection::from_gen(g))
        }
    }

    #[derive(Clone, Debug)]
    pub struct RandomChannelEndpointChanged(pub ChannelEndpointChanged);

    impl Arbitrary for RandomChannelEndpointChanged {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomChannelEndpointChanged(ChannelEndpointChanged::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_channel_endpoint_changed(message: RandomChannelEndpointChanged) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<ChannelEndpointChanged>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_CHANNEL_ENDPOINT_CHANGES, 0)
                .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::ChannelEndpointChanged(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_setup_connection(message: RandomSetupConnection) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<SetupConnection>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_SETUP_CONNECTION, 0).unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::SetupConnection(m) => m,
            _ => panic!(),
        };

        decoded_message == expected
    }

    #[derive(Clone, Debug)]
    pub struct RandomSetupConnectionError(pub SetupConnectionError<'static>);

    impl Arbitrary for RandomSetupConnectionError {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomSetupConnectionError(SetupConnectionError::from_gen(g))
        }
    }

    #[quickcheck_macros::quickcheck]
    fn encode_with_c_setup_connection_error(message: RandomSetupConnectionError) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<SetupConnectionError>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_SETUP_CONNECTION_ERROR, 0)
                .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::SetupConnectionError(m) => m,
            _ => panic!(),
        };

        decoded_message.flags == expected.flags
    }

    #[derive(Clone, Debug)]
    pub struct RandomSetupConnectionSuccess(pub SetupConnectionSuccess);

    #[cfg(feature = "prop_test")]
    impl Arbitrary for RandomSetupConnectionSuccess {
        fn arbitrary(g: &mut Gen) -> Self {
            RandomSetupConnectionSuccess(SetupConnectionSuccess::from_gen(g))
        }
    }
    #[quickcheck_macros::quickcheck]
    fn encode_with_c_setup_connection_success(message: RandomSetupConnectionSuccess) -> bool {
        let expected = message.clone().0;

        let mut encoder = Encoder::<SetupConnectionSuccess>::new();
        let mut decoder = StandardDecoder::<Sv2Message<'static>>::new();

        let frame =
            StandardSv2Frame::from_message(message.0, MESSAGE_TYPE_SETUP_CONNECTION_SUCCESS, 0)
                .unwrap();
        let encoded_frame = encoder.encode(frame).unwrap();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i]
        }
        let _ = decoder.next_frame();

        let buffer = decoder.writable();
        for i in 0..buffer.len() {
            buffer[i] = encoded_frame[i + 6]
        }

        let mut decoded = decoder.next_frame().unwrap();

        let msg_type = decoded.get_header().unwrap().msg_type();
        let payload = decoded.payload();
        let decoded_message: Sv2Message = (msg_type, payload).try_into().unwrap();
        let decoded_message = match decoded_message {
            Sv2Message::SetupConnectionSuccess(m) => m,
            _ => panic!(),
        };

        decoded_message.flags == expected.flags
    }
}
