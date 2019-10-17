// Rust Bitcoin Library
// Written by
//   The Rust Bitcoin developers
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to
// the public domain worldwide. This software is distributed without
// any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//

use std::error;
use std::fmt;

use blockdata::transaction::Transaction;
use util::psbt::raw;

/// Ways that a Partially Signed Transaction might fail.
#[derive(Debug)]
pub enum Error {
    /// Magic bytes for a PSBT must be the ASCII for "psbt" serialized in most
    /// significant byte order.
    InvalidMagic,
    /// The separator for a PSBT must be `0xff`.
    InvalidSeparator,
    /// Known keys must be according to spec.
    InvalidKey(raw::Key),
    /// Keys within key-value map should never be duplicated.
    DuplicateKey(raw::Key),
    /// The scriptSigs for the unsigned transaction must be empty.
    UnsignedTxHasScriptSigs,
    /// The scriptWitnesses for the unsigned transaction must be empty.
    UnsignedTxHasScriptWitnesses,
    /// A PSBT must have an unsigned transaction.
    MustHaveUnsignedTx,
    /// Signals that there are no more key-value pairs in a key-value map.
    NoMorePairs,
    /// Attempting to merge with a PSBT describing a different unsigned
    /// transaction.
    UnexpectedUnsignedTx {
        /// Expected
        expected: Transaction,
        /// Actual
        actual: Transaction,
    },
    /// Unable to parse as a standard SigHash type.
    NonStandardSigHashType(u32),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Error::InvalidKey(ref rkey) => write!(f, "{}: {}", error::Error::description(self), rkey),
            Error::DuplicateKey(ref rkey) => write!(f, "{}: {}", error::Error::description(self), rkey),
            Error::UnexpectedUnsignedTx { expected: ref e, actual: ref a } => write!(f, "{}: expected {}, actual {}", error::Error::description(self), e.txid(), a.txid()),
            Error::NonStandardSigHashType(ref sht) => write!(f, "{}: {}", error::Error::description(self), sht),
            Error::InvalidMagic
            | Error::InvalidSeparator
            | Error::UnsignedTxHasScriptSigs
            | Error::UnsignedTxHasScriptWitnesses
            | Error::MustHaveUnsignedTx
            | Error::NoMorePairs => f.write_str(error::Error::description(self))
        }
    }
}

impl error::Error for Error {
    fn description(&self) -> &str {
        match *self {
            Error::InvalidMagic => "invalid magic",
            Error::InvalidSeparator => "invalid separator",
            Error::InvalidKey(..) => "invalid key",
            Error::DuplicateKey(..) => "duplicate key",
            Error::UnsignedTxHasScriptSigs => "the unsigned transaction has script sigs",
            Error::UnsignedTxHasScriptWitnesses => "the unsigned transaction has script witnesses",
            Error::MustHaveUnsignedTx => {
                "partially signed transactions must have an unsigned transaction"
            }
            Error::NoMorePairs => "no more key-value pairs for this psbt map",
            Error::UnexpectedUnsignedTx { .. } => "different unsigned transaction",
            Error::NonStandardSigHashType(..) =>  "non-standard sighash type",
        }
    }
}
