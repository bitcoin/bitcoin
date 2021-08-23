mod inner;
mod seq_inner;

trait IntoOwned {
    fn into_owned(self) -> Self;
}

pub use inner::Inner;
pub use seq_inner::{Seq0255, Seq064K};

pub type U256<'a> = Inner<'a, true, 32, 0, 0>;
pub type PubKey<'a> = Inner<'a, true, 32, 0, 0>;
pub type Signature<'a> = Inner<'a, true, 64, 0, 0>;
pub type B032<'a> = Inner<'a, false, 1, 1, 32>;
pub type B0255<'a> = Inner<'a, false, 1, 1, 255>;
pub type Str032<'a> = Inner<'a, false, 1, 1, 32>;
pub type Str0255<'a> = Inner<'a, false, 1, 1, 255>;
pub type B064K<'a> = Inner<'a, false, 1, 2, { u16::MAX as usize }>;
pub type B016M<'a> = Inner<'a, false, 1, 3, { 2_usize.pow(24) - 1 }>;
pub type Bytes<'a> = Inner<'a, false, 0, 0, { usize::MAX }>;

impl<'decoder> From<[u8; 32]> for U256<'decoder> {
    fn from(v: [u8; 32]) -> Self {
        Inner::Owned(v.into())
    }
}
