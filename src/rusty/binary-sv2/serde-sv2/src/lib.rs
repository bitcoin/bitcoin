//! Serde serializer/deserializer for [stratum v2][Sv2] implemented following [serde tutorial][tutorial]
//!
//! ```txt
//! SERDE    <-> Sv2
//! bool     <-> BOOL
//! u8       <-> U8
//! u16      <-> U16
//! U24      <-> U24
//! u32      <-> u32
//! f32      <-> f32 // todo not in the spec but used
//! u64      <-> u64 // TODO not in the spec but used
//! U256     <-> U256
//! String   <-> STRO_255
//! Signature<-> SIGNATURE
//! B032     <-> B0_32 // todo not in the spec but used
//! B032     <-> STR0_32 // todo not in the spec but used
//! B0255    <-> B0_255
//! B064K    <-> B0_64K
//! B016M    <-> B0_16M
//! [u8]     <-> BYTES
//! Pubkey   <-> PUBKEY
//! Seq0255  <-> SEQ0_255[T]
//! Seq064K  <-> SEQ0_64K[T]
//! ```
//! Serilalizer and Deserializer are implemented using slices in order to reduce copies:
//!
//! ## Fixed length primitives:
//! Each fixed length Sv2 primitive type when deserialized contains a view in the input buffer so
//! no copy is needed to deserialize raw bytes.
//!
//! ## Non fixed legth primitives
//! Non fixed lenght primitives can be diveded in strings, byte sequences and general sequences.
//!
//! ### Strings
//! Strings are automatically handled by Serde
//!
//! ### Generic sequences
//! Non byte sequences `SEQ0_255[T]` and  `SEQ0_64K[T]` are implemented as a
//! struct that contains two optional field:
//! * an optional view in the input buffer
//! * an optional slice of T
//!
//! When the sequence is constructed from a serialized message we just get a view in the input
//! buffer and no data is copyed.
//!
//! When the sequence is constructed from [T] the sequnce can safely point to them without
//! transmute.
//!
//! ### Bytes sequences
//! Byte sequences can be limited lenght sequences or unlimited length sequences, the latter are
//! automatically handled by Serde.
//! Limited lenght byte sequences are not implemented as a specific new type of generic sequences cause:
//! * For the rust type system a serialized byte array and a deserialized byte array are the same
//!   thing, that is not true for generic sequences.
//! * In order to not copy data around generic sequences need to be implemented as struct containing
//!   two optional field, a slice of byte and a slice of T.
//! * This dicotomy is not needed for byte sequences so they are implemented as specific smaller
//! struct.
//!
//! ## Why not rkyv?
//! [rkyv][rkyv1] is a a zero-copy deserialization framework for Rust. I do not know rkyv but it
//! seems that the objective of this library could have been readched with less effort and
//! [better][rkyv2] using rykv instad then serder.
//!
//! Btw Serde is like standard for rust code, very safe and stable. The deserialization/serialization
//! part will be in the code that need to be added to bitcoin core, so it must use the most safe and
//! stable option. That do not exclude that a serialization/deserialization backend can be implemented
//! with rykv and then used by the subprotocols crates via a conditional compilation flag!
//!
//! [Sv2]: https://docs.google.com/document/d/1FadCWj-57dvhxsnFM_7X806qyvhR0u3i85607bGHxvg/edit
//! [tutorial]: https://serde.rs/data-format.html
//! [rkyv1]: https://docs.rs/rkyv/0.4.3/rkyv
//! [rkyv2]: https://davidkoloski.me/blog/rkyv-is-faster-than/

#![no_std]

#[macro_use]
extern crate alloc;

mod de;
mod error;
mod primitives;
mod ser;

pub use de::{from_bytes, Deserializer};
pub use error::{Error, Result};
pub use primitives::{
    Bool, Bytes, GetSize, Pubkey, Seq0255, Seq064K, Signature, Str0255, B016M, B0255, B032, B064K,
    U16, U24, U256, U32, U64, U8,
};
pub use ser::{to_bytes, to_writer, Serializer};
pub type Str032<'a> = B032<'a>;
