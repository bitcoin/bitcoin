#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::binary_codec_sv2::{self, free_vec, free_vec_2, CVec, CVec2};
#[cfg(not(feature = "with_serde"))]
use binary_sv2::Error;
use binary_sv2::{Deserialize, Serialize};
use binary_sv2::{Seq0255, B0255, B064K, U256};
#[cfg(not(feature = "with_serde"))]
use core::convert::TryInto;

/// ## NewTemplate (Server -> Client)
/// The primary template-providing function. Note that the coinbase_tx_outputs bytes will appear
/// as is at the end of the coinbase transaction.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct NewTemplate<'decoder> {
    /// Server’s identification of the template. Strictly increasing, the
    /// current UNIX time may be used in place of an ID.
    pub template_id: u64,
    /// True if the template is intended for future [`crate::SetNewPrevHash`]
    /// message sent on the channel. If False, the job relates to the last
    /// sent [`crate::SetNewPrevHash`] message on the channel and the miner
    /// should start to work on the job immediately.
    pub future_template: bool,
    /// Valid header version field that reflects the current network
    /// consensus. The general purpose bits (as specified in [BIP320](TODO link)) can
    /// be freely manipulated by the downstream node. The downstream
    /// node MUST NOT rely on the upstream node to set the BIP320 bits
    /// to any particular value.
    pub version: u32,
    /// The coinbase transaction nVersion field.
    pub coinbase_tx_version: u32,
    /// Up to 8 bytes (not including the length byte) which are to be placed
    /// at the beginning of the coinbase field in the coinbase transaction.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub coinbase_prefix: B0255<'decoder>,
    ///bug
    /// The coinbase transaction input’s nSequence field.
    pub coinbase_tx_input_sequence: u32,
    /// The value, in satoshis, available for spending in coinbase outputs
    /// added by the client. Includes both transaction fees and block
    /// subsidy.
    pub coinbase_tx_value_remaining: u64,
    /// The number of transaction outputs included in coinbase_tx_outputs.
    pub coinbase_tx_outputs_count: u32,
    /// Bitcoin transaction outputs to be included as the last outputs in the
    /// coinbase transaction.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub coinbase_tx_outputs: B064K<'decoder>, //bug
    /// The locktime field in the coinbase transaction.
    pub coinbase_tx_locktime: u32,
    /// Merkle path hashes ordered from deepest.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub merkle_path: Seq0255<'decoder, U256<'decoder>>,
}

#[repr(C)]
#[cfg(not(feature = "with_serde"))]
pub struct CNewTemplate {
    template_id: u64,
    future_template: bool,
    version: u32,
    coinbase_tx_version: u32,
    coinbase_prefix: CVec,
    coinbase_tx_input_sequence: u32,
    coinbase_tx_value_remaining: u64,
    coinbase_tx_outputs_count: u32,
    coinbase_tx_outputs: CVec,
    coinbase_tx_locktime: u32,
    merkle_path: CVec2,
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_new_template(s: CNewTemplate) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CNewTemplate {
    fn drop(&mut self) {
        free_vec(&mut self.coinbase_prefix);
        free_vec(&mut self.coinbase_tx_outputs);
        free_vec_2(&mut self.merkle_path);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<NewTemplate<'a>> for CNewTemplate {
    fn from(v: NewTemplate<'a>) -> Self {
        Self {
            template_id: v.template_id,
            future_template: v.future_template,
            version: v.version,
            coinbase_tx_version: v.coinbase_tx_version,
            coinbase_prefix: v.coinbase_prefix.into(),
            coinbase_tx_input_sequence: v.coinbase_tx_input_sequence,
            coinbase_tx_value_remaining: v.coinbase_tx_value_remaining,
            coinbase_tx_outputs_count: v.coinbase_tx_outputs_count,
            coinbase_tx_outputs: v.coinbase_tx_outputs.into(),
            coinbase_tx_locktime: v.coinbase_tx_locktime,
            merkle_path: v.merkle_path.into(),
        }
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CNewTemplate {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<NewTemplate<'a>, Error> {
        let coinbase_prefix: B0255 = self.coinbase_prefix.as_mut_slice().try_into()?;
        let coinbase_tx_outputs: B064K = self.coinbase_tx_outputs.as_mut_slice().try_into()?;

        let merkle_path_ = self.merkle_path.as_mut_slice();
        let mut merkle_path: Vec<U256> = Vec::new();
        for cvec in merkle_path_ {
            merkle_path.push(cvec.as_mut_slice().try_into()?);
        }

        let merkle_path = Seq0255::new(merkle_path)?;
        Ok(NewTemplate {
            template_id: self.template_id,
            future_template: self.future_template,
            version: self.version,
            coinbase_tx_version: self.coinbase_tx_version,
            coinbase_prefix,
            coinbase_tx_input_sequence: self.coinbase_tx_input_sequence,
            coinbase_tx_value_remaining: self.coinbase_tx_value_remaining,
            coinbase_tx_outputs_count: self.coinbase_tx_outputs_count,
            coinbase_tx_outputs,
            coinbase_tx_locktime: self.coinbase_tx_locktime,
            merkle_path,
        })
    }
}
