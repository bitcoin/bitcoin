#[cfg(not(feature = "with_serde"))]
use alloc::vec::Vec;
#[cfg(not(feature = "with_serde"))]
use binary_sv2::binary_codec_sv2::{self, free_vec, free_vec_2, CVec, CVec2};
#[cfg(not(feature = "with_serde"))]
use binary_sv2::Error;
use binary_sv2::{Deserialize, Serialize};
use binary_sv2::{Seq064K, Str0255, B016M, B064K};
#[cfg(not(feature = "with_serde"))]
use core::convert::TryInto;

/// ## RequestTransactionData (Client -> Server)
/// A request sent by the Job Negotiator to the Template Provider which requests the set of
/// transaction data for all transactions (excluding the coinbase transaction) included in a block, as
/// well as any additional data which may be required by the Pool to validate the work.
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
#[repr(C)]
pub struct RequestTransactionData {
    /// The template_id corresponding to a NewTemplate message.
    pub template_id: u64,
}

/// ## RequestTransactionData.Success (Server->Client)
/// A response to [`RequestTransactionData`] which contains the set of full transaction data and
/// excess data required for validation. For practical purposes, the excess data is usually the
/// SegWit commitment, however the Job Negotiator MUST NOT parse or interpret the excess data
/// in any way. Note that the transaction data MUST be treated as opaque blobs and MUST include
/// any SegWit or other data which the Pool may require to verify the transaction. For practical
/// purposes, the transaction data is likely the witness-encoded transaction today. However, to
/// ensure backward compatibility, the transaction data MAY be encoded in a way that is different
/// from the consensus serialization of Bitcoin transactions.
/// Ultimately, having some method of negotiating the specific format of transactions between the
/// Template Provider and the Pool’s Template verification node would be overly burdensome,
/// thus the following requirements are made explicit. The RequestTransactionData.Success
/// sender MUST ensure that the data is provided in a forwards- and backwards-compatible way to
/// ensure the end receiver of the data can interpret it, even in the face of new,
/// consensus-optional data. This allows significantly more flexibility on both the
/// RequestTransactionData.Success-generating and -interpreting sides during upgrades, at the
/// cost of breaking some potential optimizations which would require version negotiation to
/// provide support for previous versions. For practical purposes, and as a non-normative
/// suggested implementation for Bitcoin Core, this implies that additional consensus-optional
/// data be appended at the end of transaction data. It will simply be ignored by versions which do
/// not understand it.
/// To work around the limitation of not being able to negotiate e.g. a transaction compression
/// scheme, the format of the opaque data in RequestTransactionData.Success messages MAY be
/// changed in non-compatible ways at the time a fork activates, given sufficient time from
/// code-release to activation (as any sane fork would have to have) and there being some
/// in-Template Negotiation Protocol signaling of support for the new fork (e.g. for soft-forks
/// activated using [BIP 9](TODO link)).
#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct RequestTransactionDataSuccess<'decoder> {
    /// The template_id corresponding to a NewTemplate/RequestTransactionData message.
    pub template_id: u64,
    /// Extra data which the Pool may require to validate the work.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub excess_data: B064K<'decoder>,
    /// The transaction data, serialized as a series of B0_16M byte arrays.
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub transaction_list: Seq064K<'decoder, B016M<'decoder>>,
}

#[repr(C)]
#[cfg(not(feature = "with_serde"))]
pub struct CRequestTransactionDataSuccess {
    template_id: u64,
    excess_data: CVec,
    transaction_list: CVec2,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CRequestTransactionDataSuccess {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<RequestTransactionDataSuccess<'a>, Error> {
        let excess_data: B064K = self.excess_data.as_mut_slice().try_into()?;
        let transaction_list_ = self.transaction_list.as_mut_slice();
        let mut transaction_list: Vec<B016M> = Vec::new();
        for cvec in transaction_list_ {
            transaction_list.push(cvec.as_mut_slice().try_into()?);
        }
        let transaction_list = Seq064K::new(transaction_list)?;
        Ok(RequestTransactionDataSuccess {
            template_id: self.template_id,
            excess_data,
            transaction_list,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_request_tx_data_success(s: CRequestTransactionDataSuccess) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CRequestTransactionDataSuccess {
    fn drop(&mut self) {
        free_vec(&mut self.excess_data);
        free_vec_2(&mut self.transaction_list);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<RequestTransactionDataSuccess<'a>> for CRequestTransactionDataSuccess {
    fn from(v: RequestTransactionDataSuccess<'a>) -> Self {
        Self {
            template_id: v.template_id,
            excess_data: v.excess_data.into(),
            transaction_list: v.transaction_list.into(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, PartialEq, Eq)]
pub struct RequestTransactionDataError<'decoder> {
    /// The template_id corresponding to a NewTemplate/RequestTransactionData message.
    pub template_id: u64,
    /// Reason why no transaction data has been provided
    /// Possible error codes:
    /// * template-id-not-found
    #[cfg_attr(feature = "with_serde", serde(borrow))]
    pub error_code: Str0255<'decoder>,
}

#[repr(C)]
#[cfg(not(feature = "with_serde"))]
pub struct CRequestTransactionDataError {
    template_id: u64,
    error_code: CVec,
}

#[cfg(not(feature = "with_serde"))]
impl<'a> CRequestTransactionDataError {
    #[cfg(not(feature = "with_serde"))]
    pub fn to_rust_rep_mut(&'a mut self) -> Result<RequestTransactionDataError<'a>, Error> {
        let error_code: Str0255 = self.error_code.as_mut_slice().try_into()?;
        Ok(RequestTransactionDataError {
            template_id: self.template_id,
            error_code,
        })
    }
}

#[no_mangle]
#[cfg(not(feature = "with_serde"))]
pub extern "C" fn free_request_tx_data_error(s: CRequestTransactionDataError) {
    drop(s)
}

#[cfg(not(feature = "with_serde"))]
impl Drop for CRequestTransactionDataError {
    fn drop(&mut self) {
        free_vec(&mut self.error_code);
    }
}

#[cfg(not(feature = "with_serde"))]
impl<'a> From<RequestTransactionDataError<'a>> for CRequestTransactionDataError {
    fn from(v: RequestTransactionDataError<'a>) -> Self {
        Self {
            template_id: v.template_id,
            error_code: v.error_code.into(),
        }
    }
}
