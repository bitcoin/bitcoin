#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::binary_codec_sv2::{self, free_vec, CVec};
#[cfg(not(feature = "with_serde"))]
use binary_sv2::Error;
use binary_sv2::B064K;
use binary_sv2::{Deserialize, Serialize};
#[cfg(not(feature = "with_serde"))]
use core::convert::TryInto;

/// ## SubmitSolution (Client -> Server)
/// Upon finding a coinbase transaction/nonce pair which double-SHA256 hashes at or below
/// [`crate::SetNewPrevHash.target`], the client MUST immediately send this message, and the server
/// MUST then immediately construct the corresponding full block and attempt to propagate it to
/// the Bitcoin network.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct SubmitSolution<'decoder> {
    /// The template_id field as it appeared in NewTemplate.
    pub template_id: u64,
    /// The version field in the block header. Bits not defined by [BIP320](TODO link) as
    /// additional nonce MUST be the same as they appear in the [NewWork](TODO link)
    /// message, other bits may be set to any value.
    pub version: u32,
    /// The nTime field in the block header. This MUST be greater than or equal
    /// to the header_timestamp field in the latest [`crate::SetNewPrevHash`] message
    /// and lower than or equal to that value plus the number of seconds since
    /// the receipt of that message.
    pub header_timestamp: u32,
    /// The nonce field in the header.
    pub header_nonce: u32,
    /// The full serialized coinbase transaction, meeting all the requirements of
    /// the NewWork message, above.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub coinbase_tx: B064K<'decoder>,
}

#[cfg(not(feature = "with_serde"))]
#[repr(C)]
pub struct CSubmitSolution {
    template_id: u64,
    version: u32,
    header_timestamp: u32,
    header_nonce: u32,
    coinbase_tx: CVec,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CSubmitSolution {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<SubmitSolution<'a>, Error> {
        let coinbase_tx: B064K = self.coinbase_tx.as_mut_slice().try_into()?;

        Ok(SubmitSolution {
            template_id: self.template_id,
            version: self.version,
            header_timestamp: self.header_timestamp,
            header_nonce: self.header_nonce,
            coinbase_tx,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_submit_solution(s: CSubmitSolution) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CSubmitSolution {
    fn drop(&mut self) {
        free_vec(&mut self.coinbase_tx);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<SubmitSolution<'a>> for CSubmitSolution {
    fn from(v: SubmitSolution<'a>) -> Self {
        Self {
            template_id: v.template_id,
            version: v.version,
            header_timestamp: v.header_timestamp,
            header_nonce: v.header_nonce,
            coinbase_tx: v.coinbase_tx.into(),
        }
    }
}
