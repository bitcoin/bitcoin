#![no_std]

//! # Template Distribution Protocol
//! The Template Distribution protocol is used to receive updates of the block template to use in
//! mining the next block. It effectively replaces BIPs [22](TODO) and [23](TODO) (getblocktemplate) and provides
//! a much more efficient API which allows Bitcoin Core (or some other full node software) to push
//! template updates at more appropriate times as well as provide a template which may be
//! mined on quickly for the block-after-next. While not recommended, the template update
//! protocol can be a remote server, and is thus authenticated and signed in the same way as all
//! other protocols ([using the same SetupConnection handshake][TODO]).
//! Like the [Job Negotiation](TODO) and [Job Distribution](TODO) (sub)protocols, all Template Distribution messages
//! have the channel_msg bit unset, and there is no concept of channels. After the initial common
//! handshake, the client MUST immediately send a [`CoinbaseOutputDataSize`] message to indicate
//! the space it requires for coinbase output addition, to which the server MUST immediately reply
//! with the current best block template it has available to the client. Thereafter, the server
//! SHOULD push new block templates to the client whenever the total fee in the current block
//! template increases materially, and MUST send updated block templates whenever it learns of
//! a new block.
//! Template Providers MUST attempt to broadcast blocks which are mined using work they
//! provided, and thus MUST track the work which they provided to clients.
extern crate alloc;

mod coinbase_output_data_size;
mod new_template;
mod request_transaction_data;
mod set_new_prev_hash;
mod submit_solution;
//
pub use coinbase_output_data_size::CoinbaseOutputDataSize;
#[cfg(not(feature = "with_serde"))]
pub use new_template::CNewTemplate;
pub use new_template::NewTemplate;
#[cfg(not(feature = "with_serde"))]
pub use request_transaction_data::{CRequestTransactionDataError, CRequestTransactionDataSuccess};
pub use request_transaction_data::{
    RequestTransactionData, RequestTransactionDataError, RequestTransactionDataSuccess,
};
#[cfg(not(feature = "with_serde"))]
pub use set_new_prev_hash::CSetNewPrevHash;
pub use set_new_prev_hash::SetNewPrevHash;
#[cfg(not(feature = "with_serde"))]
pub use submit_solution::CSubmitSolution;
pub use submit_solution::SubmitSolution;

#[no_mangle]
pub extern "C" fn _c_export_coinbase_out(_a: CoinbaseOutputDataSize) {}

#[no_mangle]
pub extern "C" fn _c_export_req_tx_data(_a: RequestTransactionData) {}
