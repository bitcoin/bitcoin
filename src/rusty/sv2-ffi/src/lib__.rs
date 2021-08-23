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
mod tests {
    use super::*;
    use common_messages_sv2::Protocol;

    //use quickcheck::{Arbitrary, Gen};

    use quickcheck_macros;

    fn get_setup_connection() -> SetupConnection<'static> {
        let setup_connection = SetupConnection {
            protocol: Protocol::TemplateDistributionProtocol, // 2
            min_version: 2,
            max_version: 2,
            flags: 0,
            endpoint_host: "0.0.0.0".to_string().into_bytes().try_into().unwrap(),
            endpoint_port: 8081,
            vendor: "Bitmain".to_string().into_bytes().try_into().unwrap(),
            hardware_version: "901".to_string().into_bytes().try_into().unwrap(),
            firmware: "abcX".to_string().into_bytes().try_into().unwrap(),
            device_id: "89567".to_string().into_bytes().try_into().unwrap(),
        };
        return setup_connection;
    }

    #[test]
    fn test_message_type_cb_output_data_size() {
        let expected = MESSAGE_TYPE_COINBASE_OUTPUT_DATA_SIZE;
        let cb_output_data_size = CoinbaseOutputDataSize {
            coinbase_output_max_additional_size: 0,
        };
        let sv2_message = Sv2Message::CoinbaseOutputDataSize(cb_output_data_size);
        let actual = sv2_message.message_type();

        assert_eq!(expected, actual);
    }

    #[test]
    fn test_message_type_new_template() {
        let expected = MESSAGE_TYPE_NEW_TEMPLATE;
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

        assert_eq!(expected, actual);
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

    #[test]
    fn test_encode() {
        let mut encoder = Encoder::<SetupConnection>::new();
        let setup_connection = get_setup_connection();
        let setup_connection =
            StandardSv2Frame::from_message(setup_connection, MESSAGE_TYPE_SETUP_CONNECTION, 0)
                .unwrap();
        // let setup_connection = Sv2Frame {
        //     header: Header {
        //         extesion_type: 0,
        //         msg_type: 0,
        //         msg_length: U24(42),
        //     },
        //     payload: Some(SetupConnection {
        //         protocol: TemplateDistributionProtocol, // 2
        //         min_version: 2,
        //         max_version: 2,
        //         flags: 0,
        //         endpoint_host: Owned([48, 46, 48, 46, 48, 46, 48]),
        //         endpoint_port: 8081,
        //         vendor: Owned([66, 105, 116, 109, 97, 105, 110]),
        //         hardware_version: Owned([57, 48, 49]),
        //         firmware: Owned([97, 98, 99, 88]),
        //         device_id: Owned([56, 57, 53, 54, 55]),
        //     }),
        //     serialized: None,
        // };
        let setup_connection = encoder.encode(setup_connection).unwrap();
        let expected = [
            0, 0, 0, 42, 0, 0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 7, 48, 46, 48, 46, 48, 46, 48, 145, 31,
            7, 66, 105, 116, 109, 97, 105, 110, 3, 57, 48, 49, 4, 97, 98, 99, 88, 5, 56, 57, 53,
            54, 55,
        ]
        .to_vec();
        // [0(?), 0 (extension_type), 0 (msg_type), 42 (msg_length), 0 (?), 0 (?), 2 (protocol?), 2 (min_version?),  0 (?), 2 (?), 0 (?), 0 (?), 0 (?), 0 (?), 0 (?), 7 (len), 48 (0), 46(.), 48 (0), 46(.), 48 (0), 46(.), 48 (0), 145 (?), 31 (?), 7 (len),  66 (B), 105 (i), 116 (t), 109 (m), 97 (a), 105(i), 110 (n), 3 (len), 57 (9), 48 (0), 49 (1), 4 (len), 97 (a), 98 (b), 99 (c), 88 (X), 5(len), 56 (8), 57 (9), 53 (5), 54 (6), 55 (7)]
        assert_eq!(expected, setup_connection);
    }
}
