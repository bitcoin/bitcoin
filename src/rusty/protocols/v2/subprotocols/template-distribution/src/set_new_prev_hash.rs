#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::binary_codec_sv2::{self, free_vec, CVec};
#[cfg(not(feature = "with_serde"))]
use binary_sv2::Error;
use binary_sv2::U256;
use binary_sv2::{Deserialize, Serialize};
#[cfg(not(feature = "with_serde"))]
use core::convert::TryInto;

/// ## SetNewPrevHash (Server -> Client)
/// Upon successful validation of a new best block, the server MUST immediately provide a
/// SetNewPrevHash message. If a [NewWork](TODO link) message has previously been sent with the
/// [future_job](TODO link) flag set, which is valid work based on the prev_hash contained in this message, the
/// template_id field SHOULD be set to the job_id present in that NewTemplate message
/// indicating the client MUST begin mining on that template as soon as possible.
/// TODO: Define how many previous works the client has to track (2? 3?), and require that the
/// server reference one of those in SetNewPrevHash.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct SetNewPrevHash<'decoder> {
    /// template_id referenced in a previous NewTemplate message.
    pub template_id: u64,
    /// Previous block’s hash, as it must appear in the next block’s header.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub prev_hash: U256<'decoder>,
    /// The nTime field in the block header at which the client should start
    /// (usually current time). This is NOT the minimum valid nTime value.
    pub header_timestamp: u32,
    /// Block header field.
    pub n_bits: u32,
    /// The maximum double-SHA256 hash value which would represent a valid
    /// block. Note that this may be lower than the target implied by nBits in
    /// several cases, including weak-block based block propagation.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub target: U256<'decoder>,
}

#[cfg(not(feature = "with_serde"))]
#[repr(C)]
pub struct CSetNewPrevHash {
    template_id: u64,
    prev_hash: CVec,
    header_timestamp: u32,
    n_bits: u32,
    target: CVec,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CSetNewPrevHash {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<SetNewPrevHash<'a>, Error> {
        let prev_hash: U256 = self.prev_hash.as_mut_slice().try_into()?;
        let target: U256 = self.target.as_mut_slice().try_into()?;

        Ok(SetNewPrevHash {
            template_id: self.template_id,
            prev_hash,
            header_timestamp: self.header_timestamp,
            n_bits: self.n_bits,
            target,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_set_new_prev_hash(s: CSetNewPrevHash) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CSetNewPrevHash {
    fn drop(&mut self) {
        free_vec(&mut self.target);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<SetNewPrevHash<'a>> for CSetNewPrevHash {
    fn from(v: SetNewPrevHash<'a>) -> Self {
        Self {
            template_id: v.template_id,
            prev_hash: v.prev_hash.into(),
            header_timestamp: v.header_timestamp,
            n_bits: v.n_bits,
            target: v.target.into(),
        }
    }
}
