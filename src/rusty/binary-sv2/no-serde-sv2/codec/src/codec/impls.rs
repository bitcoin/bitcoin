use crate::codec::decodable::{
    Decodable, DecodableField, DecodablePrimitive, FieldMarker, GetMarker, PrimitiveMarker,
};
use crate::codec::encodable::{EncodableField, EncodablePrimitive};
use crate::datatypes::*;
use crate::Error;
use alloc::vec::Vec;
use core::convert::{TryFrom, TryInto};

// IMPL GET MARKER FOR PRIMITIVES
impl GetMarker for bool {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::Bool)
    }
}
impl GetMarker for u8 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U8)
    }
}
impl GetMarker for u16 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U16)
    }
}
impl GetMarker for U24 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U24)
    }
}
impl GetMarker for u32 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U32)
    }
}
impl GetMarker for f32 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::F32)
    }
}
impl GetMarker for u64 {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U64)
    }
}
impl<'a> GetMarker for U256<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::U256)
    }
}
impl<'a> GetMarker for Signature<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::Signature)
    }
}
impl<'a> GetMarker for B032<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::B032)
    }
}
impl<'a> GetMarker for B0255<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::B0255)
    }
}
impl<'a> GetMarker for B064K<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::B064K)
    }
}
impl<'a> GetMarker for B016M<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::B016M)
    }
}
impl<'a> GetMarker for Bytes<'a> {
    fn get_marker() -> FieldMarker {
        FieldMarker::Primitive(PrimitiveMarker::Bytes)
    }
}

// IMPL DECODABLE FOR PRIMITIVES

impl<'a> Decodable<'a> for u8 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U8.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for u16 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U16.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for u32 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U32.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for f32 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::F32.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for u64 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U64.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for bool {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::Bool.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for U24 {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U24.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for U256<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::U256.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for Signature<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::Signature.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for B032<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::B032.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for B0255<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::B0255.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for B064K<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::B064K.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}
impl<'a> Decodable<'a> for B016M<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::B016M.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}

impl<'a> Decodable<'a> for Bytes<'a> {
    fn get_structure(_: &[u8]) -> Result<Vec<FieldMarker>, Error> {
        Ok(vec![PrimitiveMarker::Bytes.into()])
    }

    fn from_decoded_fields(mut data: Vec<DecodableField<'a>>) -> Result<Self, Error> {
        data.pop().unwrap().try_into()
    }
}

// IMPL TRY_FROM PRIMITIVE FOR PRIMITIVEs

impl<'a> TryFrom<DecodablePrimitive<'a>> for u8 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U8(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for u16 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U16(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for u32 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U32(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for f32 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::F32(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for u64 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U64(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for bool {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::Bool(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}

impl<'a> TryFrom<DecodablePrimitive<'a>> for U24 {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U24(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for U256<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::U256(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for Signature<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::Signature(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for B032<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::B032(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for B0255<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::B0255(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for B064K<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::B064K(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for B016M<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::B016M(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}
impl<'a> TryFrom<DecodablePrimitive<'a>> for Bytes<'a> {
    type Error = Error;

    fn try_from(value: DecodablePrimitive<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodablePrimitive::Bytes(val) => Ok(val),
            _ => Err(Error::PrimitiveConversionError),
        }
    }
}

// IMPL TRY_FROM DECODEC FIELD FOR PRIMITIVES

impl<'a> TryFrom<DecodableField<'a>> for u8 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for u16 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for u32 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for f32 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for u64 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for bool {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for U24 {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for U256<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for Signature<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for B032<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for B0255<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for B064K<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for B016M<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}
impl<'a> TryFrom<DecodableField<'a>> for Bytes<'a> {
    type Error = Error;

    fn try_from(value: DecodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            DecodableField::Primitive(p) => p.try_into(),
            _ => Err(Error::DecodableConversionError),
        }
    }
}

// IMPL FROM PRIMITIVES FOR ENCODED FIELD

impl<'a> From<bool> for EncodableField<'a> {
    fn from(v: bool) -> Self {
        EncodableField::Primitive(EncodablePrimitive::Bool(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for bool {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::Bool(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<u8> for EncodableField<'a> {
    fn from(v: u8) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U8(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for u8 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U8(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<u16> for EncodableField<'a> {
    fn from(v: u16) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U16(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for u16 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U16(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<U24> for EncodableField<'a> {
    fn from(v: U24) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U24(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for U24 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U24(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<u32> for EncodableField<'a> {
    fn from(v: u32) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U32(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for u32 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U32(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<f32> for EncodableField<'a> {
    fn from(v: f32) -> Self {
        EncodableField::Primitive(EncodablePrimitive::F32(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for f32 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::F32(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<u64> for EncodableField<'a> {
    fn from(v: u64) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U64(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for u64 {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U64(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<U256<'a>> for EncodableField<'a> {
    fn from(v: U256<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::U256(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for U256<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::U256(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<Signature<'a>> for EncodableField<'a> {
    fn from(v: Signature<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::Signature(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for Signature<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::Signature(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<B032<'a>> for EncodableField<'a> {
    fn from(v: B032<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::B032(v))
    }
}
impl<'a> From<B0255<'a>> for EncodableField<'a> {
    fn from(v: B0255<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::B0255(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for B032<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::B032(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> TryFrom<EncodableField<'a>> for B0255<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::B0255(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<B064K<'a>> for EncodableField<'a> {
    fn from(v: B064K<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::B064K(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for B064K<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::B064K(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<B016M<'a>> for EncodableField<'a> {
    fn from(v: B016M<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::B016M(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for B016M<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::B016M(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
impl<'a> From<Bytes<'a>> for EncodableField<'a> {
    fn from(v: Bytes<'a>) -> Self {
        EncodableField::Primitive(EncodablePrimitive::Bytes(v))
    }
}
impl<'a> TryFrom<EncodableField<'a>> for Bytes<'a> {
    type Error = Error;

    fn try_from(value: EncodableField<'a>) -> Result<Self, Self::Error> {
        match value {
            EncodableField::Primitive(EncodablePrimitive::Bytes(v)) => Ok(v),
            _ => Err(Error::Todo),
        }
    }
}
//impl<'a> From<&'a Seq0255<'a, U24>> for EncodableField<'a> {
//    fn from(v: &'a Seq0255<'a, U24>) -> Self {
//        EncodableField::Primitive(EncodablePrimitive::Seq0255u24(v))
//    }
//}

// IMPL INTO FIELD MARKER FOR PRIMITIVES
impl From<bool> for FieldMarker {
    fn from(_: bool) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::Bool)
    }
}
impl From<u8> for FieldMarker {
    fn from(_: u8) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U8)
    }
}

impl From<u16> for FieldMarker {
    fn from(_: u16) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U16)
    }
}

impl From<u32> for FieldMarker {
    fn from(_: u32) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U32)
    }
}

impl From<f32> for FieldMarker {
    fn from(_: f32) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::F32)
    }
}

impl From<u64> for FieldMarker {
    fn from(_: u64) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U64)
    }
}

impl From<U24> for FieldMarker {
    fn from(_: U24) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U24)
    }
}

impl<'a> From<Inner<'a, true, 32, 0, 0>> for FieldMarker {
    fn from(_: Inner<'a, true, 32, 0, 0>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::U256)
    }
}

impl<'a> From<Inner<'a, true, 64, 0, 0>> for FieldMarker {
    fn from(_: Inner<'a, true, 64, 0, 0>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::Signature)
    }
}

impl<'a> From<B032<'a>> for FieldMarker {
    fn from(_: B032<'a>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::B032)
    }
}

impl<'a> From<Inner<'a, false, 1, 1, 255>> for FieldMarker {
    fn from(_: Inner<'a, false, 1, 1, 255>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::B0255)
    }
}

impl<'a> From<Inner<'a, false, 1, 2, { 2_usize.pow(16) - 1 }>> for FieldMarker {
    fn from(_: Inner<'a, false, 1, 2, { 2_usize.pow(16) - 1 }>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::B064K)
    }
}

impl<'a> From<Inner<'a, false, 1, 3, { 2_usize.pow(24) - 1 }>> for FieldMarker {
    fn from(_: Inner<'a, false, 1, 3, { 2_usize.pow(24) - 1 }>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::B016M)
    }
}
impl<'a> From<Bytes<'a>> for FieldMarker {
    fn from(_: Inner<'a, false, 0, 0, { ((2_usize.pow(63) - 1) * 2) + 1 }>) -> Self {
        FieldMarker::Primitive(PrimitiveMarker::Bytes)
    }
}
