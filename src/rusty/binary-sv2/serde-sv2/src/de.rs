//! Serde deserializer for [stratum v2][Sv2] implemented following [serde tutorial][tutorial]
//!
//! [Sv2]: https://docs.google.com/document/d/1FadCWj-57dvhxsnFM_7X806qyvhR0u3i85607bGHxvg/edit
//! [tutorial]: https://serde.rs/data-format.html
use alloc::vec::Vec;
use core::convert::TryInto;

use serde::de::{self, DeserializeSeed, SeqAccess, Visitor};
use serde::Deserialize;

use crate::error::{Error, Result};

//enum Sv2Seq {
//    S64k,
//    S255,
//}

#[derive(Debug)]
pub struct Deserializer<'de> {
    input: &'de [u8],
    len: u8,
}

impl<'de> Deserializer<'de> {
    fn _new(input: &'de mut [u8]) -> Self {
        Deserializer { input, len: 0 }
    }

    pub fn from_bytes(input: &'de mut [u8]) -> Self {
        Self::_new(input)
    }
}

pub fn from_bytes<'a, T>(b: &'a mut [u8]) -> Result<T>
where
    T: Deserialize<'a>,
{
    let mut deserializer = Deserializer::from_bytes(b);
    let t = T::deserialize(&mut deserializer)?;
    Ok(t)
}

impl<'de> Deserializer<'de> {
    fn as_vec(&self) -> Vec<u8> {
        self.input.to_vec()
    }

    #[inline]
    fn get_slice(&mut self, len: usize) -> Result<&'de [u8]> {
        if self.input.len() < len {
            return Err(Error::ReadError);
        };

        let (sl, rem) = &self.input.split_at(len);
        self.input = rem;

        Ok(sl)
    }

    #[inline]
    fn parse_bytes(&mut self) -> &'de [u8] {
        self.input
    }

    #[inline]
    fn parse_seq0255(&mut self, element_size: u8) -> Result<&'de [u8]> {
        let len = self.parse_u8()?;
        let len = len as usize * element_size as usize;
        self.get_slice(len as usize)
    }

    #[inline]
    fn parse_seq064k(&mut self, element_size: u8) -> Result<&'de [u8]> {
        let len = self.parse_u16()?;
        let len = len as usize * element_size as usize;
        self.get_slice(len as usize)
    }

    #[inline]
    fn parse_seq064k_variable(&mut self, element_size: u8) -> Result<&'de [u8]> {
        let element_size = element_size as usize;
        let len = self.parse_u16()?;
        let mut next_element_index: usize = 0;
        for _ in 0..len {
            let len = &self.input[next_element_index..next_element_index + element_size];
            let len = match element_size {
                1 => len[0] as u32,
                2 => u32::from_le_bytes([len[0], len[1], 0, 0]),
                3 => u32::from_le_bytes([len[0], len[1], len[2], 0]),
                _ => unreachable!(),
            };
            next_element_index += len as usize + element_size;
        }
        self.get_slice(next_element_index)
    }

    #[inline]
    fn parse_bool(&mut self) -> Result<bool> {
        let bool_ = self.get_slice(1)?;
        match bool_ {
            [0] => Ok(false),
            [1] => Ok(true),
            _ => Err(Error::InvalidBool(bool_[0])),
        }
    }

    #[inline]
    fn parse_u8(&mut self) -> Result<u8> {
        let u8_ = self.get_slice(1)?;
        Ok(u8_[0])
    }

    #[inline]
    fn parse_u16(&mut self) -> Result<u16> {
        let u16_ = self.get_slice(2)?;
        Ok(u16::from_le_bytes([u16_[0], u16_[1]]))
    }

    #[inline]
    fn parse_u24(&mut self) -> Result<u32> {
        let u24 = self.get_slice(3)?;
        Ok(u32::from_le_bytes([u24[0], u24[1], u24[2], 0]))
    }

    #[inline]
    fn parse_u32(&mut self) -> Result<u32> {
        let u32_ = self.get_slice(4)?;
        Ok(u32::from_le_bytes([u32_[0], u32_[1], u32_[2], u32_[3]]))
    }

    #[inline]
    fn parse_f32(&mut self) -> Result<f32> {
        let f32_ = self.get_slice(4)?;
        Ok(f32::from_le_bytes([f32_[0], f32_[1], f32_[2], f32_[3]]))
    }

    #[inline]
    fn parse_u256(&mut self) -> Result<&'de [u8; 32]> {
        // slice is 32 bytes so unwrap never called
        let u256: &[u8; 32] = self.get_slice(32)?.try_into().unwrap();
        Ok(u256)
    }

    #[inline]
    fn parse_signature(&mut self) -> Result<&'de [u8; 64]> {
        // slice is 64 bytes so unwrap never called
        let signature: &[u8; 64] = self.get_slice(64)?.try_into().unwrap();
        Ok(signature)
    }

    #[inline]
    fn parse_string(&mut self) -> Result<&'de str> {
        let len = self.parse_u8()?;
        let str_ = self.get_slice(len as usize)?;
        core::str::from_utf8(str_).map_err(|_| Error::InvalidUtf8)
    }

    #[inline]
    fn parse_b016m(&mut self) -> Result<&'de [u8]> {
        let len = self.parse_u24()?;
        self.get_slice(len as usize)
    }

    #[inline]
    fn parse_b064k(&mut self) -> Result<&'de [u8]> {
        let len = self.parse_u16()?;
        self.get_slice(len as usize)
    }

    #[inline]
    fn parse_b0255(&mut self) -> Result<&'de [u8]> {
        let len = self.parse_u8()?;
        self.get_slice(len as usize)
    }
    #[inline]
    fn parse_b032(&mut self) -> Result<&'de [u8]> {
        let len = self.parse_u8()?;
        self.get_slice(len as usize)
    }
}

impl<'de, 'a> de::Deserializer<'de> for &'a mut Deserializer<'de> {
    type Error = Error;

    #[inline]
    fn deserialize_u8<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_u8(self.parse_u8()?)
    }

    #[inline]
    fn deserialize_u16<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_u16(self.parse_u16()?)
    }

    #[inline]
    fn deserialize_u32<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_u32(self.parse_u32()?)
    }

    #[inline]
    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_str(self.parse_string()?)
    }

    #[inline]
    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_bytes<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        self.deserialize_byte_buf(visitor)
    }

    fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_byte_buf(self.as_vec())
    }

    #[inline]
    fn deserialize_struct<V>(
        self,
        _name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_seq(Struct::new(self, fields.len()))
    }

    // Each Sv2 primitive type is implemented as a new type struct.
    fn deserialize_newtype_struct<V>(self, _name: &'static str, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        match _name {
            "U24" => visitor.visit_u32(self.parse_u24()?),
            "U256" => visitor.visit_borrowed_bytes(self.parse_u256()?),
            "Signature" => visitor.visit_borrowed_bytes(self.parse_signature()?),
            "B016M" => visitor.visit_borrowed_bytes(self.parse_b016m()?),
            "B064K" => visitor.visit_borrowed_bytes(self.parse_b064k()?),
            "B0255" => visitor.visit_borrowed_bytes(self.parse_b0255()?),
            "B032" => visitor.visit_borrowed_bytes(self.parse_b032()?),
            "Seq_0255_U256" => visitor.visit_borrowed_bytes(self.parse_seq0255(32)?),
            "Seq_0255_Bool" => visitor.visit_borrowed_bytes(self.parse_seq0255(1)?),
            "Seq_0255_U16" => visitor.visit_borrowed_bytes(self.parse_seq0255(2)?),
            "Seq_0255_U24" => visitor.visit_borrowed_bytes(self.parse_seq0255(3)?),
            "Seq_0255_U32" => visitor.visit_borrowed_bytes(self.parse_seq0255(4)?),
            "Seq_0255_Signature" => visitor.visit_borrowed_bytes(self.parse_seq0255(64)?),
            "Seq_064K_U256" => visitor.visit_borrowed_bytes(self.parse_seq064k(32)?),
            "Seq_064K_Bool" => visitor.visit_borrowed_bytes(self.parse_seq064k(1)?),
            "Seq_064K_U16" => visitor.visit_borrowed_bytes(self.parse_seq064k(2)?),
            "Seq_064K_U24" => visitor.visit_borrowed_bytes(self.parse_seq064k(3)?),
            "Seq_064K_U32" => visitor.visit_borrowed_bytes(self.parse_seq064k(4)?),
            "Seq_064K_U64" => visitor.visit_borrowed_bytes(self.parse_seq064k(8)?),
            "Seq_064K_Signature" => visitor.visit_borrowed_bytes(self.parse_seq064k(64)?),
            "Seq_064K_B064K" => visitor.visit_borrowed_bytes(self.parse_seq064k_variable(2)?),
            "Seq_064K_B016M" => visitor.visit_borrowed_bytes(self.parse_seq064k_variable(3)?),
            "Bytes" => visitor.visit_borrowed_bytes(self.parse_bytes()),
            _ => unreachable!("Invalid type"),
            //_ => visitor.visit_newtype_struct(self),
        }
    }

    #[inline]
    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_bool(self.parse_bool()?)
    }

    ///// UNIMPLEMENTED /////

    fn deserialize_option<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_unit<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_unit_struct<V>(self, _name: &'static str, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_any<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_i8<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_i16<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_i32<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_i64<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_u64<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_f32<V>(self, visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        visitor.visit_f32(self.parse_f32()?)
    }

    // Float parsing is stupidly hard.
    fn deserialize_f64<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_char<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_seq<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        todo!()
    }

    fn deserialize_tuple<V>(self, _len: usize, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_tuple_struct<V>(
        self,
        _name: &'static str,
        _len: usize,
        _visitor: V,
    ) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_map<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_enum<V>(
        self,
        _name: &'static str,
        _variants: &'static [&'static str],
        _visitor: V,
    ) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_identifier<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn deserialize_ignored_any<V>(self, _visitor: V) -> Result<V::Value>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }
}

struct Seq<'de, 'a> {
    de: &'a mut Deserializer<'de>,
    len: usize,
}

//impl<'de, 'a> Seq<'de, 'a> {
//    fn new(de: &'a mut Deserializer<'de>, type_: Sv2Seq) -> std::result::Result<Self, Error> {
//        let len = match type_ {
//            Sv2Seq::S255 => de.parse_u8()? as usize,
//            Sv2Seq::S64k => de.parse_u16()? as usize,
//        };
//        Ok(Self { de, len })
//    }
//}

impl<'de, 'a> SeqAccess<'de> for Seq<'de, 'a> {
    type Error = Error;

    #[inline]
    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>>
    where
        T: DeserializeSeed<'de>,
    {
        // Check if there are no more elements.
        if self.len == 0 {
            return Ok(None);
        }

        self.len -= 1;

        seed.deserialize(&mut *self.de).map(Some)
    }
}

struct Struct<'de, 'a> {
    de: &'a mut Deserializer<'de>,
    len: usize,
}

impl<'de, 'a> Struct<'de, 'a> {
    fn new(de: &'a mut Deserializer<'de>, len: usize) -> Self {
        Self { de, len }
    }
}

impl<'de, 'a> SeqAccess<'de> for Struct<'de, 'a> {
    type Error = Error;

    #[inline]
    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>>
    where
        T: DeserializeSeed<'de>,
    {
        // Check if there are no more elements.
        if self.len == 0 {
            return Ok(None);
        }

        self.len -= 1;

        seed.deserialize(&mut *self.de).map(Some)
    }
}

///// TEST /////

#[test]
fn test_struct() {
    use serde::Serialize;

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test {
        a: u32,
        b: u8,
        c: crate::primitives::U24,
    }

    let expected = Test {
        a: 456,
        b: 9,
        c: 67_u32.try_into().unwrap(),
    };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_b0255() {
    use serde::Serialize;

    let b0255: crate::primitives::B0255 = (&[6; 3][..]).try_into().unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::B0255<'a>,
    }

    let expected = Test { a: b0255 };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_u256() {
    use serde::Serialize;

    let u256: crate::primitives::U256 = (&[6; 32][..]).try_into().unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::U256<'a>,
    }

    let expected = Test { a: u256 };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_signature() {
    use serde::Serialize;

    let s: crate::primitives::Signature = (&[6; 64][..]).try_into().unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Signature<'a>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_b016m() {
    use serde::Serialize;

    let b: crate::primitives::B016M = (&[0; 70000][..]).try_into().unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        b: bool,
        #[serde(borrow)]
        a: crate::primitives::B016M<'a>,
    }

    let expected = Test { a: b, b: true };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_b064k() {
    use core::convert::TryInto;
    use serde::Serialize;

    let b: crate::primitives::B064K = (&[1, 2, 9][..])
        .try_into()
        .expect("vector smaller than 64K should not fail");

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        b: bool,
        #[serde(borrow)]
        a: crate::primitives::B064K<'a>,
    }

    let expected = Test { a: b, b: true };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq0255_u256() {
    use crate::primitives::Seq0255;
    use crate::primitives::U256;
    use serde::Serialize;

    let u256_1: crate::primitives::U256 = (&[6; 32][..]).try_into().unwrap();
    let u256_2: crate::primitives::U256 = (&[5; 32][..]).try_into().unwrap();
    let u256_3: crate::primitives::U256 = (&[0; 32][..]).try_into().unwrap();

    let val = vec![u256_1, u256_2, u256_3];
    let s = Seq0255::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, U256<'a>>,
    }

    let test = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&test).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    let bytes_2 = crate::ser::to_bytes(&deserialized).unwrap();

    assert_eq!(bytes, bytes_2);
}

#[test]
fn test_seq0255_bool() {
    use crate::primitives::Seq0255;
    use serde::Serialize;

    let s: crate::primitives::Seq0255<bool> = Seq0255::new(vec![true, false, true]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, bool>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq0255_u16() {
    use crate::primitives::Seq0255;
    use crate::primitives::U16;
    use serde::Serialize;

    let s: crate::primitives::Seq0255<U16> = Seq0255::new(vec![10, 43, 89]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, U16>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq0255_u24() {
    use crate::primitives::Seq0255;
    use crate::primitives::U24;
    use serde::Serialize;

    let u24_1 = U24(56);
    let u24_2 = U24(59);
    let u24_3 = U24(70999);

    let val = vec![u24_1, u24_2, u24_3];
    let s: crate::primitives::Seq0255<U24> = Seq0255::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, U24>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq0255_u32() {
    use crate::primitives::Seq0255;
    use serde::Serialize;

    let s: crate::primitives::Seq0255<u32> = Seq0255::new(vec![546, 99999, 87, 32]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, u32>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq0255_signature() {
    use crate::primitives::Seq0255;
    use crate::primitives::Signature;
    use serde::Serialize;

    let siganture_1: Signature = (&[88; 64][..]).try_into().unwrap();
    let siganture_2: Signature = (&[99; 64][..]).try_into().unwrap();
    let siganture_3: Signature = (&[220; 64][..]).try_into().unwrap();

    let val = vec![siganture_1, siganture_2, siganture_3];
    let s: crate::primitives::Seq0255<Signature> = Seq0255::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: crate::primitives::Seq0255<'a, Signature<'a>>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq064k_u256() {
    use crate::primitives::Seq064K;
    use crate::primitives::U256;
    use serde::Serialize;

    let u256_1: crate::primitives::U256 = (&[6; 32][..]).try_into().unwrap();
    let u256_2: crate::primitives::U256 = (&[5; 32][..]).try_into().unwrap();
    let u256_3: crate::primitives::U256 = (&[0; 32][..]).try_into().unwrap();

    let val = vec![u256_1, u256_2, u256_3];
    let s = Seq064K::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, U256<'a>>,
    }

    let test = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&test).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    let bytes_2 = crate::ser::to_bytes(&deserialized).unwrap();

    assert_eq!(bytes, bytes_2);
}

#[test]
fn test_seq064k_bool() {
    use crate::primitives::Seq064K;
    use serde::Serialize;

    let s: Seq064K<bool> = Seq064K::new(vec![true, false, true]).unwrap();
    let s2: Seq064K<bool> = Seq064K::new(vec![true; 64000]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, bool>,
    }

    let expected = Test { a: s };
    let expected2 = Test { a: s2 };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let mut bytes2 = crate::ser::to_bytes(&expected2).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();
    let deserialized2: Test = from_bytes(&mut bytes2[..]).unwrap();

    assert_eq!(deserialized, expected);
    assert_eq!(deserialized2, expected2);
}

#[test]
fn test_seq064k_u16() {
    use crate::primitives::Seq064K;
    use crate::primitives::U16;
    use serde::Serialize;

    let s: Seq064K<U16> = Seq064K::new(vec![10, 43, 89]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, U16>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();
    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq064k_u24() {
    use crate::primitives::Seq064K;
    use crate::primitives::U24;
    use serde::Serialize;

    let u24_1 = U24(56);
    let u24_2 = U24(59);
    let u24_3 = U24(70999);

    let val = vec![u24_1, u24_2, u24_3];
    let s: Seq064K<U24> = Seq064K::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, U24>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq064k_u32() {
    use crate::primitives::Seq064K;
    use serde::Serialize;

    let s: Seq064K<u32> = Seq064K::new(vec![546, 99999, 87, 32]).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, u32>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq064k_signature() {
    use crate::primitives::Seq064K;
    use crate::primitives::Signature;
    use serde::Serialize;

    let siganture_1: Signature = (&[88_u8; 64][..]).try_into().unwrap();
    let siganture_2: Signature = (&[99_u8; 64][..]).try_into().unwrap();
    let siganture_3: Signature = (&[220_u8; 64][..]).try_into().unwrap();

    let val = vec![siganture_1, siganture_2, siganture_3];
    let s: Seq064K<Signature> = Seq064K::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, Signature<'a>>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}

#[test]
fn test_seq064k_b016m() {
    use crate::primitives::Seq064K;
    use crate::primitives::B016M;
    use serde::Serialize;

    let bytes_1: B016M = (&[88_u8; 64][..]).try_into().unwrap();
    let bytes_2: B016M = (&[99_u8; 64][..]).try_into().unwrap();
    let bytes_3: B016M = (&[220_u8; 64][..]).try_into().unwrap();

    let val = vec![bytes_1, bytes_2, bytes_3];
    let s: Seq064K<B016M> = Seq064K::new(val).unwrap();

    #[derive(Deserialize, Serialize, PartialEq, Debug)]
    struct Test<'a> {
        #[serde(borrow)]
        a: Seq064K<'a, B016M<'a>>,
    }

    let expected = Test { a: s };

    let mut bytes = crate::ser::to_bytes(&expected).unwrap();

    let deserialized: Test = from_bytes(&mut bytes[..]).unwrap();

    assert_eq!(deserialized, expected);
}
