//! This module contains type aliases for C's fixed-width integer types .
//!
//! These aliases are deprecated: use the Rust types instead.

#[deprecated(since = "0.2.55", note = "Use i8 instead.")]
pub type int8_t = i8;
#[deprecated(since = "0.2.55", note = "Use i16 instead.")]
pub type int16_t = i16;
#[deprecated(since = "0.2.55", note = "Use i32 instead.")]
pub type int32_t = i32;
#[deprecated(since = "0.2.55", note = "Use i64 instead.")]
pub type int64_t = i64;
#[deprecated(since = "0.2.55", note = "Use u8 instead.")]
pub type uint8_t = u8;
#[deprecated(since = "0.2.55", note = "Use u16 instead.")]
pub type uint16_t = u16;
#[deprecated(since = "0.2.55", note = "Use u32 instead.")]
pub type uint32_t = u32;
#[deprecated(since = "0.2.55", note = "Use u64 instead.")]
pub type uint64_t = u64;
