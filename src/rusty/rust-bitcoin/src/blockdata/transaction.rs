// Rust Bitcoin Library
// Written in 2014 by
//     Andrew Poelstra <apoelstra@wpsoftware.net>
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

//! Bitcoin Transaction
//!
//! A transaction describes a transfer of money. It consumes previously-unspent
//! transaction outputs and produces new ones, satisfying the condition to spend
//! the old outputs (typically a digital signature with a specific key must be
//! provided) and defining the condition to spend the new ones. The use of digital
//! signatures ensures that coins cannot be spent by unauthorized parties.
//!
//! This module provides the structures and functions needed to support transactions.
//!

use std::default::Default;
use std::{fmt, io};

use hashes::{self, sha256d, Hash};
use hashes::hex::FromHex;

use util::endian;
use util::hash::BitcoinHash;
#[cfg(feature="bitcoinconsensus")] use blockdata::script;
use blockdata::script::Script;
use consensus::{encode, serialize, Decodable, Encodable};
use VarInt;

/// A reference to a transaction output
#[derive(Copy, Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct OutPoint {
    /// The referenced transaction's txid
    pub txid: sha256d::Hash,
    /// The index of the referenced output in its transaction's vout
    pub vout: u32,
}
serde_struct_human_string_impl!(OutPoint, "an OutPoint", txid, vout);

impl OutPoint {
    /// Create a new [OutPoint].
    #[inline]
    pub fn new(txid: sha256d::Hash, vout: u32) -> OutPoint {
        OutPoint {
            txid: txid,
            vout: vout,
        }
    }

    /// Creates a "null" `OutPoint`.
    ///
    /// This value is used for coinbase transactions because they don't have
    /// any previous outputs.
    #[inline]
    pub fn null() -> OutPoint {
        OutPoint {
            txid: Default::default(),
            vout: u32::max_value(),
        }
    }

    /// Checks if an `OutPoint` is "null".
    ///
    /// # Examples
    ///
    /// ```rust
    /// use bitcoin::blockdata::constants::genesis_block;
    /// use bitcoin::network::constants::Network;
    ///
    /// let block = genesis_block(Network::Bitcoin);
    /// let tx = &block.txdata[0];
    ///
    /// // Coinbase transactions don't have any previous output.
    /// assert_eq!(tx.input[0].previous_output.is_null(), true);
    /// ```
    #[inline]
    pub fn is_null(&self) -> bool {
        *self == OutPoint::null()
    }
}

impl Default for OutPoint {
    fn default() -> Self {
        OutPoint::null()
    }
}

impl fmt::Display for OutPoint {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}:{}", self.txid, self.vout)
    }
}

/// An error in parsing an OutPoint.
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum ParseOutPointError {
    /// Error in TXID part.
    Txid(hashes::hex::Error),
    /// Error in vout part.
    Vout(::std::num::ParseIntError),
    /// Error in general format.
    Format,
    /// Size exceeds max.
    TooLong,
    /// Vout part is not strictly numeric without leading zeroes.
    VoutNotCanonical,
}

impl fmt::Display for ParseOutPointError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            ParseOutPointError::Txid(ref e) => write!(f, "error parsing TXID: {}", e),
            ParseOutPointError::Vout(ref e) => write!(f, "error parsing vout: {}", e),
            ParseOutPointError::Format => write!(f, "OutPoint not in <txid>:<vout> format"),
            ParseOutPointError::TooLong => write!(f, "vout should be at most 10 digits"),
            ParseOutPointError::VoutNotCanonical => write!(f, "no leading zeroes or + allowed in vout part"),
        }
    }
}

impl ::std::error::Error for ParseOutPointError {
    fn description(&self) -> &str {
        match *self {
            ParseOutPointError::Txid(_) => "TXID parse error",
            ParseOutPointError::Vout(_) => "vout parse error",
            ParseOutPointError::Format => "outpoint format error",
            ParseOutPointError::TooLong => "size error",
            ParseOutPointError::VoutNotCanonical => "vout canonical error",
        }
    }

    fn cause(&self) -> Option<&::std::error::Error> {
        match *self {
            ParseOutPointError::Txid(ref e) => Some(e),
            ParseOutPointError::Vout(ref e) => Some(e),
            _ => None,
        }
    }
}

/// Parses a string-encoded transaction index (vout).
/// It does not permit leading zeroes or non-digit characters.
fn parse_vout(s: &str) -> Result<u32, ParseOutPointError> {
    if s.len() > 1 {
        let first = s.chars().nth(0).unwrap();
        if first == '0' || first == '+' {
            return Err(ParseOutPointError::VoutNotCanonical);
        }
    }
    Ok(s.parse().map_err(ParseOutPointError::Vout)?)
}

impl ::std::str::FromStr for OutPoint {
    type Err = ParseOutPointError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if s.len() > 75 { // 64 + 1 + 10
            return Err(ParseOutPointError::TooLong);
        }
        let find = s.find(':');
        if find == None || find != s.rfind(':') {
            return Err(ParseOutPointError::Format);
        }
        let colon = find.unwrap();
        if colon == 0 || colon == s.len() - 1 {
            return Err(ParseOutPointError::Format);
        }
        Ok(OutPoint {
            txid: sha256d::Hash::from_hex(&s[..colon]).map_err(ParseOutPointError::Txid)?,
            vout: parse_vout(&s[colon+1..])?,
        })
    }
}

/// A transaction input, which defines old coins to be consumed
#[derive(Clone, PartialEq, Eq, Debug, Hash)]
pub struct TxIn {
    /// The reference to the previous output that is being used an an input
    pub previous_output: OutPoint,
    /// The script which pushes values on the stack which will cause
    /// the referenced output's script to accept
    pub script_sig: Script,
    /// The sequence number, which suggests to miners which of two
    /// conflicting transactions should be preferred, or 0xFFFFFFFF
    /// to ignore this feature. This is generally never used since
    /// the miner behaviour cannot be enforced.
    pub sequence: u32,
    /// Witness data: an array of byte-arrays.
    /// Note that this field is *not* (de)serialized with the rest of the TxIn in
    /// Encodable/Decodable, as it is (de)serialized at the end of the full
    /// Transaction. It *is* (de)serialized with the rest of the TxIn in other
    /// (de)serialization routines.
    pub witness: Vec<Vec<u8>>
}
serde_struct_impl!(TxIn, previous_output, script_sig, sequence, witness);

/// A transaction output, which defines new coins to be created from old ones.
#[derive(Clone, PartialEq, Eq, Debug, Hash)]
pub struct TxOut {
    /// The value of the output, in satoshis
    pub value: u64,
    /// The script which must satisfy for the output to be spent
    pub script_pubkey: Script
}
serde_struct_impl!(TxOut, value, script_pubkey);

// This is used as a "null txout" in consensus signing code
impl Default for TxOut {
    fn default() -> TxOut {
        TxOut { value: 0xffffffffffffffff, script_pubkey: Script::new() }
    }
}

/// A Bitcoin transaction, which describes an authenticated movement of coins.
///
/// If any inputs have nonempty witnesses, the entire transaction is serialized
/// in the post-BIP141 Segwit format which includes a list of witnesses. If all
/// inputs have empty witnesses, the transaction is serialized in the pre-BIP141
/// format.
///
/// There is one major exception to this: to avoid deserialization ambiguity,
/// if the transaction has no inputs, it is serialized in the BIP141 style. Be
/// aware that this differs from the transaction format in PSBT, which _never_
/// uses BIP141. (Ordinarily there is no conflict, since in PSBT transactions
/// are always unsigned and therefore their inputs have empty witnesses.)
///
/// The specific ambiguity is that Segwit uses the flag bytes `0001` where an old
/// serializer would read the number of transaction inputs. The old serializer
/// would interpret this as "no inputs, one output", which means the transaction
/// is invalid, and simply reject it. Segwit further specifies that this encoding
/// should *only* be used when some input has a nonempty witness; that is,
/// witness-less transactions should be encoded in the traditional format.
///
/// However, in protocols where transactions may legitimately have 0 inputs, e.g.
/// when parties are cooperatively funding a transaction, the "00 means Segwit"
/// heuristic does not work. Since Segwit requires such a transaction be encoded
/// in the original transaction format (since it has no inputs and therefore
/// no input witnesses), a traditionally encoded transaction may have the `0001`
/// Segwit flag in it, which confuses most Segwit parsers including the one in
/// Bitcoin Core.
///
/// We therefore deviate from the spec by always using the Segwit witness encoding
/// for 0-input transactions, which results in unambiguously parseable transactions.
#[derive(Clone, PartialEq, Eq, Debug, Hash)]
pub struct Transaction {
    /// The protocol version, is currently expected to be 1 or 2 (BIP 68).
    pub version: u32,
    /// Block number before which this transaction is valid, or 0 for
    /// valid immediately.
    pub lock_time: u32,
    /// List of inputs
    pub input: Vec<TxIn>,
    /// List of outputs
    pub output: Vec<TxOut>,
}
serde_struct_impl!(Transaction, version, lock_time, input, output);

impl Transaction {
    /// Computes a "normalized TXID" which does not include any signatures.
    /// This gives a way to identify a transaction that is ``the same'' as
    /// another in the sense of having same inputs and outputs.
    pub fn ntxid(&self) -> sha256d::Hash {
        let cloned_tx = Transaction {
            version: self.version,
            lock_time: self.lock_time,
            input: self.input.iter().map(|txin| TxIn { script_sig: Script::new(), witness: vec![], .. *txin }).collect(),
            output: self.output.clone(),
        };
        cloned_tx.bitcoin_hash()
    }

    /// Computes the txid. For non-segwit transactions this will be identical
    /// to the output of `BitcoinHash::bitcoin_hash()`, but for segwit transactions,
    /// this will give the correct txid (not including witnesses) while `bitcoin_hash`
    /// will also hash witnesses.
    pub fn txid(&self) -> sha256d::Hash {
        let mut enc = sha256d::Hash::engine();
        self.version.consensus_encode(&mut enc).unwrap();
        self.input.consensus_encode(&mut enc).unwrap();
        self.output.consensus_encode(&mut enc).unwrap();
        self.lock_time.consensus_encode(&mut enc).unwrap();
        sha256d::Hash::from_engine(enc)
    }

    /// Computes a signature hash for a given input index with a given sighash flag.
    /// To actually produce a scriptSig, this hash needs to be run through an
    /// ECDSA signer, the SigHashType appended to the resulting sig, and a
    /// script written around this, but this is the general (and hard) part.
    ///
    /// *Warning* This does NOT attempt to support OP_CODESEPARATOR. In general
    /// this would require evaluating `script_pubkey` to determine which separators
    /// get evaluated and which don't, which we don't have the information to
    /// determine.
    ///
    /// # Panics
    /// Panics if `input_index` is greater than or equal to `self.input.len()`
    ///
    pub fn signature_hash(&self, input_index: usize, script_pubkey: &Script, sighash_u32: u32) -> sha256d::Hash {
        assert!(input_index < self.input.len());  // Panic on OOB

        let (sighash, anyone_can_pay) = SigHashType::from_u32(sighash_u32).split_anyonecanpay_flag();

        // Special-case sighash_single bug because this is easy enough.
        if sighash == SigHashType::Single && input_index >= self.output.len() {
            return sha256d::Hash::from_slice(&[1, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0,
                                               0, 0, 0, 0, 0, 0, 0, 0]).unwrap();
        }

        // Build tx to sign
        let mut tx = Transaction {
            version: self.version,
            lock_time: self.lock_time,
            input: vec![],
            output: vec![],
        };
        // Add all inputs necessary..
        if anyone_can_pay {
            tx.input = vec![TxIn {
                previous_output: self.input[input_index].previous_output,
                script_sig: script_pubkey.clone(),
                sequence: self.input[input_index].sequence,
                witness: vec![],
            }];
        } else {
            tx.input = Vec::with_capacity(self.input.len());
            for (n, input) in self.input.iter().enumerate() {
                tx.input.push(TxIn {
                    previous_output: input.previous_output,
                    script_sig: if n == input_index { script_pubkey.clone() } else { Script::new() },
                    sequence: if n != input_index && (sighash == SigHashType::Single || sighash == SigHashType::None) { 0 } else { input.sequence },
                    witness: vec![],
                });
            }
        }
        // ..then all outputs
        tx.output = match sighash {
            SigHashType::All => self.output.clone(),
            SigHashType::Single => {
                let output_iter = self.output.iter()
                                      .take(input_index + 1)  // sign all outputs up to and including this one, but erase
                                      .enumerate()            // all of them except for this one
                                      .map(|(n, out)| if n == input_index { out.clone() } else { TxOut::default() });
                output_iter.collect()
            }
            SigHashType::None => vec![],
            _ => unreachable!()
        };
        // hash the result
        let mut raw_vec = serialize(&tx);
        raw_vec.extend_from_slice(&endian::u32_to_array_le(sighash_u32));
        sha256d::Hash::hash(&raw_vec)
    }

    /// Gets the "weight" of this transaction, as defined by BIP141. For transactions with an empty
    /// witness, this is simply the consensus-serialized size times 4. For transactions with a
    /// witness, this is the non-witness consensus-serialized size multiplied by 3 plus the
    /// with-witness consensus-serialized size.
    #[inline]
    pub fn get_weight(&self) -> usize {
        let mut input_weight = 0;
        let mut inputs_with_witnesses = 0;
        for input in &self.input {
            input_weight += 4*(32 + 4 + 4 + // outpoint (32+4) + nSequence
                VarInt(input.script_sig.len() as u64).len() +
                input.script_sig.len());
            if !input.witness.is_empty() {
                inputs_with_witnesses += 1;
                input_weight += VarInt(input.witness.len() as u64).len();
                for elem in &input.witness {
                    input_weight += VarInt(elem.len() as u64).len() + elem.len();
                }
            }
        }
        let mut output_size = 0;
        for output in &self.output {
            output_size += 8 + // value
                VarInt(output.script_pubkey.len() as u64).len() +
                output.script_pubkey.len();
        }
        let non_input_size =
        // version:
        4 +
        // count varints:
        VarInt(self.input.len() as u64).len() +
        VarInt(self.output.len() as u64).len() +
        output_size +
        // lock_time
        4;
        if inputs_with_witnesses == 0 {
            non_input_size * 4 + input_weight
        } else {
            non_input_size * 4 + input_weight + self.input.len() - inputs_with_witnesses + 2
        }
    }

    #[cfg(feature="bitcoinconsensus")]
    /// Verify that this transaction is able to spend its inputs
    /// The lambda spent should not return the same TxOut twice!
    pub fn verify<S>(&self, mut spent: S) -> Result<(), script::Error>
        where S: FnMut(&OutPoint) -> Option<TxOut> {
        let tx = serialize(&*self);
        for (idx, input) in self.input.iter().enumerate() {
            if let Some(output) = spent(&input.previous_output) {
                output.script_pubkey.verify(idx, output.value, tx.as_slice())?;
            } else {
                return Err(script::Error::UnknownSpentOutput(input.previous_output.clone()));
            }
        }
        Ok(())
    }

    /// Is this a coin base transaction?
    pub fn is_coin_base(&self) -> bool {
        self.input.len() == 1 && self.input[0].previous_output.is_null()
    }
}

impl BitcoinHash for Transaction {
    fn bitcoin_hash(&self) -> sha256d::Hash {
        let mut enc = sha256d::Hash::engine();
        self.consensus_encode(&mut enc).unwrap();
        sha256d::Hash::from_engine(enc)
    }
}

impl_consensus_encoding!(TxOut, value, script_pubkey);

impl Encodable for OutPoint {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let len = self.txid.consensus_encode(&mut s)?;
        Ok(len + self.vout.consensus_encode(s)?)
    }
}
impl Decodable for OutPoint {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(OutPoint {
            txid: Decodable::consensus_decode(&mut d)?,
            vout: Decodable::consensus_decode(d)?,
        })
    }
}

impl Encodable for TxIn {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let mut len = 0;
        len += self.previous_output.consensus_encode(&mut s)?;
        len += self.script_sig.consensus_encode(&mut s)?;
        len += self.sequence.consensus_encode(s)?;
        Ok(len)
    }
}
impl Decodable for TxIn {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        Ok(TxIn {
            previous_output: Decodable::consensus_decode(&mut d)?,
            script_sig: Decodable::consensus_decode(&mut d)?,
            sequence: Decodable::consensus_decode(d)?,
            witness: vec![],
        })
    }
}

impl Encodable for Transaction {
    fn consensus_encode<S: io::Write>(
        &self,
        mut s: S,
    ) -> Result<usize, encode::Error> {
        let mut len = 0;
        len += self.version.consensus_encode(&mut s)?;
        let mut have_witness = self.input.is_empty();
        for input in &self.input {
            if !input.witness.is_empty() {
                have_witness = true;
                break;
            }
        }
        if !have_witness {
            len += self.input.consensus_encode(&mut s)?;
            len += self.output.consensus_encode(&mut s)?;
        } else {
            len += 0u8.consensus_encode(&mut s)?;
            len += 1u8.consensus_encode(&mut s)?;
            len += self.input.consensus_encode(&mut s)?;
            len += self.output.consensus_encode(&mut s)?;
            for input in &self.input {
                len += input.witness.consensus_encode(&mut s)?;
            }
        }
        len += self.lock_time.consensus_encode(s)?;
        Ok(len)
    }
}

impl Decodable for Transaction {
    fn consensus_decode<D: io::Read>(mut d: D) -> Result<Self, encode::Error> {
        let version = u32::consensus_decode(&mut d)?;
        let input = Vec::<TxIn>::consensus_decode(&mut d)?;
        // segwit
        if input.is_empty() {
            let segwit_flag = u8::consensus_decode(&mut d)?;
            match segwit_flag {
                // BIP144 input witnesses
                1 => {
                    let mut input = Vec::<TxIn>::consensus_decode(&mut d)?;
                    let output = Vec::<TxOut>::consensus_decode(&mut d)?;
                    for txin in input.iter_mut() {
                        txin.witness = Decodable::consensus_decode(&mut d)?;
                    }
                    if !input.is_empty() && input.iter().all(|input| input.witness.is_empty()) {
                        Err(encode::Error::ParseFailed("witness flag set but no witnesses present"))
                    } else {
                        Ok(Transaction {
                            version: version,
                            input: input,
                            output: output,
                            lock_time: Decodable::consensus_decode(d)?,
                        })
                    }
                }
                // We don't support anything else
                x => {
                    Err(encode::Error::UnsupportedSegwitFlag(x))
                }
            }
        // non-segwit
        } else {
            Ok(Transaction {
                version: version,
                input: input,
                output: Decodable::consensus_decode(&mut d)?,
                lock_time: Decodable::consensus_decode(d)?,
            })
        }
    }
}

/// Hashtype of a transaction, encoded in the last byte of a signature
/// Fixed values so they can be casted as integer types for encoding
#[derive(PartialEq, Eq, Debug, Copy, Clone)]
pub enum SigHashType {
    /// 0x1: Sign all outputs
    All		= 0x01,
    /// 0x2: Sign no outputs --- anyone can choose the destination
    None	= 0x02,
    /// 0x3: Sign the output whose index matches this input's index. If none exists,
    /// sign the hash `0000000000000000000000000000000000000000000000000000000000000001`.
    /// (This rule is probably an unintentional C++ism, but it's consensus so we have
    /// to follow it.)
    Single	= 0x03,
    /// 0x81: Sign all outputs but only this input
    AllPlusAnyoneCanPay		= 0x81,
    /// 0x82: Sign no outputs and only this input
    NonePlusAnyoneCanPay	= 0x82,
    /// 0x83: Sign one output and only this input (see `Single` for what "one output" means)
    SinglePlusAnyoneCanPay	= 0x83
}

impl SigHashType {
     /// Break the sighash flag into the "real" sighash flag and the ANYONECANPAY boolean
     fn split_anyonecanpay_flag(&self) -> (SigHashType, bool) {
         match *self {
             SigHashType::All		=> (SigHashType::All, false),
             SigHashType::None		=> (SigHashType::None, false),
             SigHashType::Single	=> (SigHashType::Single, false),
             SigHashType::AllPlusAnyoneCanPay		=> (SigHashType::All, true),
             SigHashType::NonePlusAnyoneCanPay		=> (SigHashType::None, true),
             SigHashType::SinglePlusAnyoneCanPay	=> (SigHashType::Single, true)
         }
     }

     /// Reads a 4-byte uint32 as a sighash type
     pub fn from_u32(n: u32) -> SigHashType {
         match n & 0x9f {
             // "real" sighashes
             0x01 => SigHashType::All,
             0x02 => SigHashType::None,
             0x03 => SigHashType::Single,
             0x81 => SigHashType::AllPlusAnyoneCanPay,
             0x82 => SigHashType::NonePlusAnyoneCanPay,
             0x83 => SigHashType::SinglePlusAnyoneCanPay,
             // catchalls
             x if x & 0x80 == 0x80 => SigHashType::AllPlusAnyoneCanPay,
             _ => SigHashType::All
         }
     }

     /// Converts to a u32
     pub fn as_u32(&self) -> u32 { *self as u32 }
}


#[cfg(test)]
mod tests {
    use super::{OutPoint, ParseOutPointError, Transaction, TxIn};

    use std::str::FromStr;
    use blockdata::script::Script;
    use consensus::encode::serialize;
    use consensus::encode::deserialize;
    use util::hash::BitcoinHash;
    use util::misc::hex_bytes;

    use hashes::{sha256d, Hash};
    use hashes::hex::FromHex;

    #[test]
    fn test_outpoint() {
        assert_eq!(OutPoint::from_str("i don't care"),
                   Err(ParseOutPointError::Format));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:1:1"),
                   Err(ParseOutPointError::Format));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:"),
                   Err(ParseOutPointError::Format));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:11111111111"),
                   Err(ParseOutPointError::TooLong));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:01"),
                   Err(ParseOutPointError::VoutNotCanonical));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:+42"),
                   Err(ParseOutPointError::VoutNotCanonical));
        assert_eq!(OutPoint::from_str("i don't care:1"),
                   Err(ParseOutPointError::Txid(sha256d::Hash::from_hex("i don't care").unwrap_err())));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c945X:1"),
                   Err(ParseOutPointError::Txid(sha256d::Hash::from_hex("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c945X").unwrap_err())));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:lol"),
                   Err(ParseOutPointError::Vout(u32::from_str("lol").unwrap_err())));
 
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:42"),
                   Ok(OutPoint{
                       txid: sha256d::Hash::from_hex("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456").unwrap(),
                       vout: 42,
                   }));
        assert_eq!(OutPoint::from_str("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456:0"),
                   Ok(OutPoint{
                       txid: sha256d::Hash::from_hex("5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456").unwrap(),
                       vout: 0,
                   }));
    }

    #[test]
    fn test_txin() {
        let txin: Result<TxIn, _> = deserialize(&hex_bytes("a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff").unwrap());
        assert!(txin.is_ok());
    }

    #[test]
    fn test_is_coinbase () {
        use network::constants::Network;
        use blockdata::constants;

        let genesis = constants::genesis_block(Network::Bitcoin);
        assert! (genesis.txdata[0].is_coin_base());
        let hex_tx = hex_bytes("0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000").unwrap();
        let tx: Transaction = deserialize(&hex_tx).unwrap();
        assert!(!tx.is_coin_base());
    }

    #[test]
    fn test_transaction() {
        let hex_tx = hex_bytes("0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000").unwrap();
        let tx: Result<Transaction, _> = deserialize(&hex_tx);
        assert!(tx.is_ok());
        let realtx = tx.unwrap();
        // All these tests aren't really needed because if they fail, the hash check at the end
        // will also fail. But these will show you where the failure is so I'll leave them in.
        assert_eq!(realtx.version, 1);
        assert_eq!(realtx.input.len(), 1);
        // In particular this one is easy to get backward -- in bitcoin hashes are encoded
        // as little-endian 256-bit numbers rather than as data strings.
        assert_eq!(format!("{:x}", realtx.input[0].previous_output.txid),
                   "ce9ea9f6f5e422c6a9dbcddb3b9a14d1c78fab9ab520cb281aa2a74a09575da1".to_string());
        assert_eq!(realtx.input[0].previous_output.vout, 1);
        assert_eq!(realtx.output.len(), 1);
        assert_eq!(realtx.lock_time, 0);

        assert_eq!(format!("{:x}", realtx.bitcoin_hash()),
                   "a6eab3c14ab5272a58a5ba91505ba1a4b6d7a3a9fcbd187b6cd99a7b6d548cb7".to_string());
        assert_eq!(realtx.get_weight(), 193*4);
    }

    #[test]
    fn tx_no_input_deserialization() {
        let hex_tx = hex_bytes(
            "010000000001000100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000"
        ).unwrap();
        let tx: Transaction = deserialize(&hex_tx).expect("deserialize tx");

        assert_eq!(tx.input.len(), 0);
        assert_eq!(tx.output.len(), 1);

        let reser = serialize(&tx);
        assert_eq!(hex_tx, reser);
    }

    #[test]
    fn test_ntxid() {
        let hex_tx = hex_bytes("0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000").unwrap();
        let mut tx: Transaction = deserialize(&hex_tx).unwrap();

        let old_ntxid = tx.ntxid();
        assert_eq!(format!("{:x}", old_ntxid), "c3573dbea28ce24425c59a189391937e00d255150fa973d59d61caf3a06b601d");
        // changing sigs does not affect it
        tx.input[0].script_sig = Script::new();
        assert_eq!(old_ntxid, tx.ntxid());
        // changing pks does
        tx.output[0].script_pubkey = Script::new();
        assert!(old_ntxid != tx.ntxid());
    }

    #[test]
    fn test_txid() {
        // segwit tx from Liquid integration tests, txid/hash from Core decoderawtransaction
        let hex_tx = hex_bytes(
            "01000000000102ff34f95a672bb6a4f6ff4a7e90fa8c7b3be7e70ffc39bc99be3bda67942e836c00000000\
             23220020cde476664d3fa347b8d54ef3aee33dcb686a65ced2b5207cbf4ec5eda6b9b46e4f414d4c934ad8\
             1d330314e888888e3bd22c7dde8aac2ca9227b30d7c40093248af7812201000000232200200af6f6a071a6\
             9d5417e592ed99d256ddfd8b3b2238ac73f5da1b06fc0b2e79d54f414d4c0ba0c8f505000000001976a914\
             dcb5898d9036afad9209e6ff0086772795b1441088ac033c0f000000000017a914889f8c10ff2bd4bb9dab\
             b68c5c0d700a46925e6c87033c0f000000000017a914889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c87\
             033c0f000000000017a914889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c87033c0f000000000017a914\
             889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c87033c0f000000000017a914889f8c10ff2bd4bb9dabb6\
             8c5c0d700a46925e6c87033c0f000000000017a914889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c8703\
             3c0f000000000017a914889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c87033c0f000000000017a91488\
             9f8c10ff2bd4bb9dabb68c5c0d700a46925e6c87033c0f000000000017a914889f8c10ff2bd4bb9dabb68c\
             5c0d700a46925e6c87033c0f000000000017a914889f8c10ff2bd4bb9dabb68c5c0d700a46925e6c870500\
             47304402200380b8663e727d7e8d773530ef85d5f82c0b067c97ae927800a0876a1f01d8e2022021ee611e\
             f6507dfd217add2cd60a8aea3cbcfec034da0bebf3312d19577b8c290147304402207bd9943ce1c2c5547b\
             120683fd05d78d23d73be1a5b5a2074ff586b9c853ed4202202881dcf435088d663c9af7b23efb3c03b9db\
             c0c899b247aa94a74d9b4b3c84f501483045022100ba12bba745af3f18f6e56be70f8382ca8e107d1ed5ce\
             aa3e8c360d5ecf78886f022069b38ebaac8fe6a6b97b497cbbb115f3176f7213540bef08f9292e5a72de52\
             de01695321023c9cd9c6950ffee24772be948a45dc5ef1986271e46b686cb52007bac214395a2102756e27\
             cb004af05a6e9faed81fd68ff69959e3c64ac8c9f6cd0e08fd0ad0e75d2103fa40da236bd82202a985a910\
             4e851080b5940812685769202a3b43e4a8b13e6a53ae050048304502210098b9687b81d725a7970d1eee91\
             ff6b89bc9832c2e0e3fb0d10eec143930b006f02206f77ce19dc58ecbfef9221f81daad90bb4f468df3912\
             12abc4f084fe2cc9bdef01483045022100e5479f81a3ad564103da5e2ec8e12f61f3ac8d312ab68763c1dd\
             d7bae94c20610220789b81b7220b27b681b1b2e87198897376ba9d033bc387f084c8b8310c8539c2014830\
             45022100aa1cc48a2d256c0e556616444cc08ae4959d464e5ffff2ae09e3550bdab6ce9f02207192d5e332\
             9a56ba7b1ead724634d104f1c3f8749fe6081e6233aee3e855817a016953210260de9cc68658c61af984e3\
             ab0281d17cfca1cc035966d335f474932d5e6c5422210355fbb768ce3ce39360277345dbb5f376e706459e\
             5a2b5e0e09a535e61690647021023222ceec58b94bd25925dd9743dae6b928737491bd940fc5dd7c6f5d5f\
             2adc1e53ae00000000"
        ).unwrap();
        let tx: Transaction = deserialize(&hex_tx).unwrap();

        assert_eq!(format!("{:x}", tx.bitcoin_hash()), "d6ac4a5e61657c4c604dcde855a1db74ec6b3e54f32695d72c5e11c7761ea1b4");
        assert_eq!(format!("{:x}", tx.txid()), "9652aa62b0e748caeec40c4cb7bc17c6792435cc3dfe447dd1ca24f912a1c6ec");
        assert_eq!(tx.get_weight(), 2718);

        // non-segwit tx from my mempool
        let hex_tx = hex_bytes(
            "01000000010c7196428403d8b0c88fcb3ee8d64f56f55c8973c9ab7dd106bb4f3527f5888d000000006a47\
             30440220503a696f55f2c00eee2ac5e65b17767cd88ed04866b5637d3c1d5d996a70656d02202c9aff698f\
             343abb6d176704beda63fcdec503133ea4f6a5216b7f925fa9910c0121024d89b5a13d6521388969209df2\
             7a8469bd565aff10e8d42cef931fad5121bfb8ffffffff02b825b404000000001976a914ef79e7ee9fff98\
             bcfd08473d2b76b02a48f8c69088ac0000000000000000296a273236303039343836393731373233313237\
             3633313032313332353630353838373931323132373000000000"
        ).unwrap();
        let tx: Transaction = deserialize(&hex_tx).unwrap();

        assert_eq!(format!("{:x}", tx.bitcoin_hash()), "971ed48a62c143bbd9c87f4bafa2ef213cfa106c6e140f111931d0be307468dd");
        assert_eq!(format!("{:x}", tx.txid()), "971ed48a62c143bbd9c87f4bafa2ef213cfa106c6e140f111931d0be307468dd");
    }

    #[test]
    #[cfg(feature = "serde")]
    fn test_txn_encode_decode() {
        let hex_tx = hex_bytes("0100000001a15d57094aa7a21a28cb20b59aab8fc7d1149a3bdbcddba9c622e4f5f6a99ece010000006c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52ffffffff0100e1f505000000001976a9140389035a9225b3839e2bbf32d826a1e222031fd888ac00000000").unwrap();
        let tx: Transaction = deserialize(&hex_tx).unwrap();
        serde_round_trip!(tx);
    }

    fn run_test_sighash(tx: &str, script: &str, input_index: usize, hash_type: i32, expected_result: &str) {
        let tx: Transaction = deserialize(&hex_bytes(tx).unwrap()[..]).unwrap();
        let script = Script::from(hex_bytes(script).unwrap());
        let mut raw_expected = hex_bytes(expected_result).unwrap();
        raw_expected.reverse();
        let expected_result = sha256d::Hash::from_slice(&raw_expected[..]).unwrap();

        let actual_result = tx.signature_hash(input_index, &script, hash_type as u32);
        assert_eq!(actual_result, expected_result);
    }

    // Test decoding transaction `4be105f158ea44aec57bf12c5817d073a712ab131df6f37786872cfc70734188`
    // from testnet, which is the first BIP144-encoded transaction I encountered.
    #[test]
    #[cfg(feature = "serde")]
    fn test_segwit_tx_decode() {
        let hex_tx = hex_bytes("010000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff3603da1b0e00045503bd5704c7dd8a0d0ced13bb5785010800000000000a636b706f6f6c122f4e696e6a61506f6f6c2f5345475749542fffffffff02b4e5a212000000001976a914876fbb82ec05caa6af7a3b5e5a983aae6c6cc6d688ac0000000000000000266a24aa21a9edf91c46b49eb8a29089980f02ee6b57e7d63d33b18b4fddac2bcd7db2a39837040120000000000000000000000000000000000000000000000000000000000000000000000000").unwrap();
        let tx: Transaction = deserialize(&hex_tx).unwrap();
        assert_eq!(tx.get_weight(), 780);
        serde_round_trip!(tx);

        let consensus_encoded = serialize(&tx);
        assert_eq!(consensus_encoded, hex_tx);
    }


    // These test vectors were stolen from libbtc, which is Copyright 2014 Jonas Schnelli MIT
    // They were transformed by replacing {...} with run_test_sighash(...), then the ones containing
    // OP_CODESEPARATOR in their pubkeys were removed
    #[test]
    fn test_sighash() {
        run_test_sighash("907c2bc503ade11cc3b04eb2918b6f547b0630ab569273824748c87ea14b0696526c66ba740200000004ab65ababfd1f9bdd4ef073c7afc4ae00da8a66f429c917a0081ad1e1dabce28d373eab81d8628de802000000096aab5253ab52000052ad042b5f25efb33beec9f3364e8a9139e8439d9d7e26529c3c30b6c3fd89f8684cfd68ea0200000009ab53526500636a52ab599ac2fe02a526ed040000000008535300516352515164370e010000000003006300ab2ec229", "", 2, 1864164639, "31af167a6cf3f9d5f6875caa4d31704ceb0eba078d132b78dab52c3b8997317e");
        run_test_sighash("a0aa3126041621a6dea5b800141aa696daf28408959dfb2df96095db9fa425ad3f427f2f6103000000015360290e9c6063fa26912c2e7fb6a0ad80f1c5fea1771d42f12976092e7a85a4229fdb6e890000000001abc109f6e47688ac0e4682988785744602b8c87228fcef0695085edf19088af1a9db126e93000000000665516aac536affffffff8fe53e0806e12dfd05d67ac68f4768fdbe23fc48ace22a5aa8ba04c96d58e2750300000009ac51abac63ab5153650524aa680455ce7b000000000000499e50030000000008636a00ac526563ac5051ee030000000003abacabd2b6fe000000000003516563910fb6b5", "65", 0, -1391424484, "48d6a1bd2cd9eec54eb866fc71209418a950402b5d7e52363bfb75c98e141175");
        run_test_sighash("73107cbd025c22ebc8c3e0a47b2a760739216a528de8d4dab5d45cbeb3051cebae73b01ca10200000007ab6353656a636affffffffe26816dffc670841e6a6c8c61c586da401df1261a330a6c6b3dd9f9a0789bc9e000000000800ac6552ac6aac51ffffffff0174a8f0010000000004ac52515100000000", "5163ac63635151ac", 1, 1190874345, "06e328de263a87b09beabe222a21627a6ea5c7f560030da31610c4611f4a46bc");
        run_test_sighash("50818f4c01b464538b1e7e7f5ae4ed96ad23c68c830e78da9a845bc19b5c3b0b20bb82e5e9030000000763526a63655352ffffffff023b3f9c040000000008630051516a6a5163a83caf01000000000553ab65510000000000", "6aac", 0, 946795545, "746306f322de2b4b58ffe7faae83f6a72433c22f88062cdde881d4dd8a5a4e2d");
        run_test_sighash("a93e93440250f97012d466a6cc24839f572def241c814fe6ae94442cf58ea33eb0fdd9bcc1030000000600636a0065acffffffff5dee3a6e7e5ad6310dea3e5b3ddda1a56bf8de7d3b75889fc024b5e233ec10f80300000007ac53635253ab53ffffffff0160468b04000000000800526a5300ac526a00000000", "ac00636a53", 1, 1773442520, "5c9d3a2ce9365bb72cfabbaa4579c843bb8abf200944612cf8ae4b56a908bcbd");
        run_test_sighash("d3b7421e011f4de0f1cea9ba7458bf3486bee722519efab711a963fa8c100970cf7488b7bb0200000003525352dcd61b300148be5d05000000000000000000", "535251536aac536a", 0, -1960128125, "29aa6d2d752d3310eba20442770ad345b7f6a35f96161ede5f07b33e92053e2a");
        run_test_sighash("04bac8c5033460235919a9c63c42b2db884c7c8f2ed8fcd69ff683a0a2cccd9796346a04050200000003655351fcad3a2c5a7cbadeb4ec7acc9836c3f5c3e776e5c566220f7f965cf194f8ef98efb5e3530200000007526a006552526526a2f55ba5f69699ece76692552b399ba908301907c5763d28a15b08581b23179cb01eac03000000075363ab6a516351073942c2025aa98a05000000000765006aabac65abd7ffa6030000000004516a655200000000", "53ac6365ac526a", 1, 764174870, "bf5fdc314ded2372a0ad078568d76c5064bf2affbde0764c335009e56634481b");
        run_test_sighash("c363a70c01ab174230bbe4afe0c3efa2d7f2feaf179431359adedccf30d1f69efe0c86ed390200000002ab51558648fe0231318b04000000000151662170000000000008ac5300006a63acac00000000", "", 0, 2146479410, "191ab180b0d753763671717d051f138d4866b7cb0d1d4811472e64de595d2c70");
        run_test_sighash("8d437a7304d8772210a923fd81187c425fc28c17a5052571501db05c7e89b11448b36618cd02000000026a6340fec14ad2c9298fde1477f1e8325e5747b61b7e2ff2a549f3d132689560ab6c45dd43c3010000000963ac00ac000051516a447ed907a7efffebeb103988bf5f947fc688aab2c6a7914f48238cf92c337fad4a79348102000000085352ac526a5152517436edf2d80e3ef06725227c970a816b25d0b58d2cd3c187a7af2cea66d6b27ba69bf33a0300000007000063ab526553f3f0d6140386815d030000000003ab6300de138f00000000000900525153515265abac1f87040300000000036aac6500000000", "51", 3, -315779667, "b6632ac53578a741ae8c36d8b69e79f39b89913a2c781cdf1bf47a8c29d997a5");
        run_test_sighash("fd878840031e82fdbe1ad1d745d1185622b0060ac56638290ec4f66b1beef4450817114a2c0000000009516a63ab53650051abffffffff37b7a10322b5418bfd64fb09cd8a27ddf57731aeb1f1f920ffde7cb2dfb6cdb70300000008536a5365ac53515369ecc034f1594690dbe189094dc816d6d57ea75917de764cbf8eccce4632cbabe7e116cd0100000003515352ffffffff035777fc000000000003515200abe9140300000000050063005165bed6d10200000000076300536363ab65195e9110", "635265", 0, 1729787658, "6e3735d37a4b28c45919543aabcb732e7a3e1874db5315abb7cc6b143d62ff10");
        run_test_sighash("a63bc673049c75211aa2c09ecc38e360eaa571435fedd2af1116b5c1fa3d0629c269ecccbf0000000008ac65ab516352ac52ffffffffbf1a76fdda7f451a5f0baff0f9ccd0fe9136444c094bb8c544b1af0fa2774b06010000000463535253ffffffff13d6b7c3ddceef255d680d87181e100864eeb11a5bb6a3528cb0d70d7ee2bbbc02000000056a0052abab951241809623313b198bb520645c15ec96bfcc74a2b0f3db7ad61d455cc32db04afc5cc702000000016309c9ae25014d9473020000000004abab6aac3bb1e803", "", 3, -232881718, "6e48f3da3a4ac07eb4043a232df9f84e110485d7c7669dd114f679c27d15b97e");
        run_test_sighash("4c565efe04e7d32bac03ae358d63140c1cfe95de15e30c5b84f31bb0b65bb542d637f49e0f010000000551abab536348ae32b31c7d3132030a510a1b1aacf7b7c3f19ce8dc49944ef93e5fa5fe2d356b4a73a00100000009abac635163ac00ab514c8bc57b6b844e04555c0a4f4fb426df139475cd2396ae418bc7015820e852f711519bc202000000086a00510000abac52488ff4aec72cbcfcc98759c58e20a8d2d9725aa4a80f83964e69bc4e793a4ff25cd75dc701000000086a52ac6aac5351532ec6b10802463e0200000000000553005265523e08680100000000002f39a6b0", "", 3, 70712784, "c6076b6a45e6fcfba14d3df47a34f6aadbacfba107e95621d8d7c9c0e40518ed");
        run_test_sighash("fd22692802db8ae6ab095aeae3867305a954278f7c076c542f0344b2591789e7e33e4d29f4020000000151ffffffffb9409129cfed9d3226f3b6bab7a2c83f99f48d039100eeb5796f00903b0e5e5e0100000006656552ac63abd226abac0403e649000000000007abab51ac5100ac8035f10000000000095165006a63526a52510d42db030000000007635365ac6a63ab24ef5901000000000453ab6a0000000000", "536a52516aac6a", 1, 309309168, "7ca0f75e6530ec9f80d031fc3513ca4ecd67f20cb38b4dacc6a1d825c3cdbfdb");
        run_test_sighash("a43f85f701ffa54a3cc57177510f3ea28ecb6db0d4431fc79171cad708a6054f6e5b4f89170000000008ac6a006a536551652bebeaa2013e779c05000000000665ac5363635100000000", "ac", 0, 2028978692, "58294f0d7f2e68fe1fd30c01764fe1619bcc7961d68968944a0e263af6550437");
        run_test_sighash("c2b0b99001acfecf7da736de0ffaef8134a9676811602a6299ba5a2563a23bb09e8cbedf9300000000026300ffffffff042997c50300000000045252536a272437030000000007655353ab6363ac663752030000000002ab6a6d5c900000000000066a6a5265abab00000000", "52ac525163515251", 0, -894181723, "8b300032a1915a4ac05cea2f7d44c26f2a08d109a71602636f15866563eaafdc");
        run_test_sighash("82f9f10304c17a9d954cf3380db817814a8c738d2c811f0412284b2c791ec75515f38c4f8c020000000265ab5729ca7db1b79abee66c8a757221f29280d0681355cb522149525f36da760548dbd7080a0100000001510b477bd9ce9ad5bb81c0306273a3a7d051e053f04ecf3a1dbeda543e20601a5755c0cfae030000000451ac656affffffff71141a04134f6c292c2e0d415e6705dfd8dcee892b0d0807828d5aeb7d11f5ef0300000001520b6c6dc802a6f3dd0000000000056aab515163bfb6800300000000015300000000", "", 3, -635779440, "d55ed1e6c53510f2608716c12132a11fb5e662ec67421a513c074537eeccc34b");
        run_test_sighash("2074bad5011847f14df5ea7b4afd80cd56b02b99634893c6e3d5aaad41ca7c8ee8e5098df003000000026a6affffffff018ad59700000000000900ac656a526551635300000000", "65635265", 0, -1804671183, "663c999a52288c9999bff36c9da2f8b78d5c61b8347538f76c164ccba9868d0a");
        run_test_sighash("7100b11302e554d4ef249ee416e7510a485e43b2ba4b8812d8fe5529fe33ea75f36d392c4403000000020000ffffffff3d01a37e075e9a7715a657ae1bdf1e44b46e236ad16fd2f4c74eb9bf370368810000000007636553ac536365ffffffff01db696a0400000000065200ac656aac00000000", "63005151", 0, -1210499507, "b9c3aee8515a4a3b439de1ffc9c156824bda12cb75bfe5bc863164e8fd31bd7a");
        run_test_sighash("02c1017802091d1cb08fec512db7b012fe4220d57a5f15f9e7676358b012786e1209bcff950100000004acab6352ffffffff799bc282724a970a6fea1828984d0aeb0f16b67776fa213cbdc4838a2f1961a3010000000951516a536552ab6aabffffffff016c7b4b03000000000865abac5253ac5352b70195ad", "65655200516a", 0, -241626954, "be567cb47170b34ff81c66c1142cb9d27f9b6898a384d6dfc4fce16b75b6cb14");
        run_test_sighash("4504cb1904c7a4acf375ddae431a74de72d5436efc73312cf8e9921f431267ea6852f9714a01000000066a656a656553a2fbd587c098b3a1c5bd1d6480f730a0d6d9b537966e20efc0e352d971576d0f87df0d6d01000000016321aeec3c4dcc819f1290edb463a737118f39ab5765800547522708c425306ebfca3f396603000000055300ac656a1d09281d05bfac57b5eb17eb3fa81ffcedfbcd3a917f1be0985c944d473d2c34d245eb350300000007656a51525152ac263078d9032f470f0500000000066aac00000052e12da60200000000003488410200000000076365006300ab539981e432", "52536a52526a", 1, -31909119, "f0a2deee7fd8a3a9fad6927e763ded11c940ee47e9e6d410f94fda5001f82e0c");
        run_test_sighash("14bc7c3e03322ec0f1311f4327e93059c996275302554473104f3f7b46ca179bfac9ef753503000000016affffffff9d405eaeffa1ca54d9a05441a296e5cc3a3e32bb8307afaf167f7b57190b07e00300000008abab51ab5263abab45533aa242c61bca90dd15d46079a0ab0841d85df67b29ba87f2393cd764a6997c372b55030000000452005263ffffffff0250f40e02000000000651516a0063630e95ab0000000000046a5151ac00000000", "6a65005151", 0, -1460947095, "aa418d096929394c9147be8818d8c9dafe6d105945ab9cd7ec682df537b5dd79");
        run_test_sighash("a08ff466049fb7619e25502ec22fedfb229eaa1fe275aa0b5a23154b318441bf547989d0510000000005ab5363636affffffff2b0e335cb5383886751cdbd993dc0720817745a6b1c9b8ab3d15547fc9aafd03000000000965656a536a52656a532b53d10584c290d3ac1ab74ab0a19201a4a039cb59dc58719821c024f6bf2eb26322b33f010000000965ac6aac0053ab6353ffffffff048decba6ebbd2db81e416e39dde1f821ba69329725e702bcdea20c5cc0ecc6402000000086363ab5351ac6551466e377b0468c0fa00000000000651ab53ac6a513461c6010000000008636a636365535100eeb3dc010000000006526a52ac516a43f362010000000005000063536500000000", "0063516a", 1, -1158911348, "f6a1ecb50bd7c2594ebecea5a1aa23c905087553e40486dade793c2f127fdfae");
        run_test_sighash("f10a0356031cd569d652dbca8e7a4d36c8da33cdff428d003338602b7764fe2c96c505175b010000000465ac516affffffffbb54563c71136fa944ee20452d78dc87073ac2365ba07e638dce29a5d179da600000000003635152ffffffff9a411d8e2d421b1e6085540ee2809901e590940bbb41532fa38bd7a16b68cc350100000007535251635365636195df1603b61c45010000000002ab65bf6a310400000000026352fcbba10200000000016aa30b7ff0", "5351", 0, 1552495929, "9eb8adf2caecb4bf9ac59d7f46bd20e83258472db2f569ee91aba4cf5ee78e29");
        run_test_sighash("c3325c9b012f659466626ca8f3c61dfd36f34670abc054476b7516a1839ec43cd0870aa0c0000000000753525265005351e7e3f04b0112650500000000000363ac6300000000", "acac", 0, -68961433, "5ca70e727d91b1a42b78488af2ed551642c32d3de4712a51679f60f1456a8647");
        run_test_sighash("b500ca48011ec57c2e5252e5da6432089130603245ffbafb0e4c5ffe6090feb629207eeb0e010000000652ab6a636aab8302c9d2042b44f40500000000015278c05a050000000004ac5251524be080020000000007636aac63ac5252c93a9a04000000000965ab6553636aab5352d91f9ddb", "52005100", 0, -2024394677, "49c8a6940a461cc7225637f1e512cdd174c99f96ec05935a59637ededc77124c");
        run_test_sighash("f52ff64b02ee91adb01f3936cc42e41e1672778962b68cf013293d649536b519bc3271dd2c00000000020065afee11313784849a7c15f44a61cd5fd51ccfcdae707e5896d131b082dc9322a19e12858501000000036aac654e8ca882022deb7c020000000006006a515352abd3defc0000000000016300000000", "63520063", 0, 1130989496, "7f208df9a5507e98c62cebc5c1e2445eb632e95527594929b9577b53363e96f6");
        run_test_sighash("ab7d6f36027a7adc36a5cf7528fe4fb5d94b2c96803a4b38a83a675d7806dda62b380df86a0000000003000000ffffffff5bc00131e29e22057c04be854794b4877dda42e416a7a24706b802ff9da521b20000000007ac6a0065ac52ac957cf45501b9f06501000000000500ac6363ab25f1110b", "00526500536a635253", 0, 911316637, "5fa09d43c8aef6f6fa01c383a69a5a61a609cd06e37dce35a39dc9eae3ddfe6c");
        run_test_sighash("f940888f023dce6360263c850372eb145b864228fdbbb4c1186174fa83aab890ff38f8c9a90300000000ffffffff01e80ccdb081e7bbae1c776531adcbfb77f2e5a7d0e5d0d0e2e6c8758470e85f00000000020053ffffffff03b49088050000000004656a52ab428bd604000000000951630065ab63ac636a0cbacf0400000000070063ac5265ac53d6e16604", "ac63", 0, 39900215, "713ddeeefcfe04929e7b6593c792a4efbae88d2b5280d1f0835d2214eddcbad6");
        run_test_sighash("530ecd0b01ec302d97ef6f1b5a6420b9a239714013e20d39aa3789d191ef623fc215aa8b940200000005ac5351ab6a3823ab8202572eaa04000000000752ab6a51526563fd8a270100000000036a006581a798f0", "525153656a0063", 0, 1784562684, "fe42f73a8742676e640698222b1bd6b9c338ff1ccd766d3d88d7d3c6c6ac987e");
        run_test_sighash("5d781d9303acfcce964f50865ddfddab527ea971aee91234c88e184979985c00b4de15204b0100000003ab6352a009c8ab01f93c8ef2447386c434b4498538f061845862c3f9d5751ad0fce52af442b3a902000000045165ababb909c66b5a3e7c81b3c45396b944be13b8aacfc0204f3f3c105a66fa8fa6402f1b5efddb01000000096a65ac636aacab656ac3c677c402b79fa4050000000004006aab5133e35802000000000751ab635163ab0078c2e025", "6aac51636a6a005265", 0, -882306874, "551ce975d58647f10adefb3e529d9bf9cda34751627ec45e690f135ef0034b95");
        run_test_sighash("a9c57b1a018551bcbc781b256642532bbc09967f1cbe30a227d352a19365d219d3f11649a3030000000451655352b140942203182894030000000006ab00ac6aab654add350400000000003d379505000000000553abacac00e1739d36", "5363", 0, -1069721025, "6da32416deb45a0d720a1dbe6d357886eabc44029dd5db74d50feaffbe763245");
        run_test_sighash("05c4fb94040f5119dc0b10aa9df054871ed23c98c890f1e931a98ffb0683dac45e98619fdc0200000007acab6a525263513e7495651c9794c4d60da835d303eb4ee6e871f8292f6ad0b32e85ef08c9dc7aa4e03c9c010000000500ab52acacfffffffffee953259cf14ced323fe8d567e4c57ba331021a1ef5ac2fa90f7789340d7c550100000007ac6aacac6a6a53ffffffff08d9dc820d00f18998af247319f9de5c0bbd52a475ea587f16101af3afab7c210100000003535363569bca7c0468e34f00000000000863536353ac51ac6584e319010000000006650052ab6a533debea030000000003ac0053ee7070020000000006ac52005253ac00000000", "6351005253", 2, 1386916157, "76c4013c40bfa1481badd9d342b6d4b8118de5ab497995fafbf73144469e5ff0");
        run_test_sighash("ca66ae10049533c2b39f1449791bd6d3f039efe0a121ab7339d39ef05d6dcb200ec3fb2b3b020000000465006a53ffffffff534b8f97f15cc7fb4f4cea9bf798472dc93135cd5b809e4ca7fe4617a61895980100000000ddd83c1dc96f640929dd5e6f1151dab1aa669128591f153310d3993e562cc7725b6ae3d903000000046a52536582f8ccddb8086d8550f09128029e1782c3f2624419abdeaf74ecb24889cc45ac1a64492a0100000002516a4867b41502ee6ccf03000000000752acacab52ab6a4b7ba80000000000075151ab0052536300000000", "6553", 2, -62969257, "8085e904164ab9a8c20f58f0d387f6adb3df85532e11662c03b53c3df8c943cb");
        run_test_sighash("ba646d0b0453999f0c70cb0430d4cab0e2120457bb9128ed002b6e9500e9c7f8d7baa20abe0200000001652a4e42935b21db02b56bf6f08ef4be5adb13c38bc6a0c3187ed7f6197607ba6a2c47bc8a03000000040052516affffffffa55c3cbfc19b1667594ac8681ba5d159514b623d08ed4697f56ce8fcd9ca5b0b00000000096a6a5263ac655263ab66728c2720fdeabdfdf8d9fb2bfe88b295d3b87590e26a1e456bad5991964165f888c03a0200000006630051ac00acffffffff0176fafe0100000000070063acac65515200000000", "63", 1, 2002322280, "9db4e320208185ee70edb4764ee195deca00ba46412d5527d9700c1cf1c3d057");
        run_test_sighash("2ddb8f84039f983b45f64a7a79b74ff939e3b598b38f436def7edd57282d0803c7ef34968d02000000026a537eb00c4187de96e6e397c05f11915270bcc383959877868ba93bac417d9f6ed9f627a7930300000004516551abffffffffacc12f1bb67be3ae9f1d43e55fda8b885340a0df1175392a8bbd9f959ad3605003000000025163ffffffff02ff0f4700000000000070bd99040000000003ac53abf8440b42", "", 2, -393923011, "0133f1a161363b71dfb3a90065c7128c56bd0028b558b610142df79e055ab5c7");
        run_test_sighash("b21fc15403b4bdaa994204444b59323a7b8714dd471bd7f975a4e4b7b48787e720cbd1f5f00000000000ffffffff311533001cb85c98c1d58de0a5fbf27684a69af850d52e22197b0dc941bc6ca9030000000765ab6363ab5351a8ae2c2c7141ece9a4ff75c43b7ea9d94ec79b7e28f63e015ac584d984a526a73fe1e04e0100000007526352536a5365ffffffff02a0a9ea030000000002ab52cfc4f300000000000465525253e8e0f342", "000000", 1, 1305253970, "d1df1f4bba2484cff8a816012bb6ec91c693e8ca69fe85255e0031711081c46a");
        run_test_sighash("f821a042036ad43634d29913b77c0fc87b4af593ac86e9a816a9d83fd18dfcfc84e1e1d57102000000076a63ac52006351ffffffffbcdaf490fc75086109e2f832c8985716b3a624a422cf9412fe6227c10585d21203000000095252abab5352ac526affffffff2efed01a4b73ad46c7f7bc7fa3bc480f8e32d741252f389eaca889a2e9d2007e000000000353ac53ffffffff032ac8b3020000000009636300000063516300d3d9f2040000000006510065ac656aafa5de0000000000066352ab5300ac9042b57d", "525365", 1, 667065611, "0d17a92c8d5041ba09b506ddf9fd48993be389d000aad54f9cc2a44fcc70426b");
        run_test_sighash("d62f183e037e0d52dcf73f9b31f70554bce4f693d36d17552d0e217041e01f15ad3840c838000000000963acac6a6a6a63ab63ffffffffabdfb395b6b4e63e02a763830f536fc09a35ff8a0cf604021c3c751fe4c88f4d0300000006ab63ab65ac53aa4d30de95a2327bccf9039fb1ad976f84e0b4a0936d82e67eafebc108993f1e57d8ae39000000000165ffffffff04364ad30500000000036a005179fd84010000000007ab636aac6363519b9023030000000008510065006563ac6acd2a4a02000000000000000000", "52", 1, 595020383, "da8405db28726dc4e0f82b61b2bfd82b1baa436b4e59300305cc3b090b157504");
        run_test_sighash("44c200a5021238de8de7d80e7cce905606001524e21c8d8627e279335554ca886454d692e6000000000500acac52abbb8d1dc876abb1f514e96b21c6e83f429c66accd961860dc3aed5071e153e556e6cf076d02000000056553526a51870a928d0360a580040000000004516a535290e1e302000000000851ab6a00510065acdd7fc5040000000007515363ab65636abb1ec182", "6363", 0, -785766894, "ed53cc766cf7cb8071cec9752460763b504b2183442328c5a9761eb005c69501");
        run_test_sighash("d682d52d034e9b062544e5f8c60f860c18f029df8b47716cabb6c1b4a4b310a0705e754556020000000400656a0016eeb88eef6924fed207fba7ddd321ff3d84f09902ff958c815a2bf2bb692eb52032c4d803000000076365ac516a520099788831f8c8eb2552389839cfb81a9dc55ecd25367acad4e03cfbb06530f8cccf82802701000000085253655300656a53ffffffff02d543200500000000056a510052ac03978b05000000000700ac51525363acfdc4f784", "", 2, -696035135, "e1a256854099907050cfee7778f2018082e735a1f1a3d91437584850a74c87bb");
        run_test_sighash("e8c0dec5026575ddf31343c20aeeca8770afb33d4e562aa8ee52eeda6b88806fdfd4fe0a97030000000953acabab65ab516552ffffffffdde122c2c3e9708874286465f8105f43019e837746686f442666629088a970e0010000000153ffffffff01f98eee0100000000025251fe87379a", "63", 1, 633826334, "abe441209165d25bc6d8368f2e7e7dc21019056719fef1ace45542aa2ef282e2");
        run_test_sighash("b288c331011c17569293c1e6448e33a64205fc9dc6e35bc756a1ac8b97d18e912ea88dc0770200000007635300ac6aacabfc3c890903a3ccf8040000000004656500ac9c65c9040000000009ab6a6aabab65abac63ac5f7702000000000365005200000000", "526a63", 0, 1574937329, "0dd1bd5c25533bf5f268aa316ce40f97452cca2061f0b126a59094ca5b65f7a0");
        run_test_sighash("6b68ba00023bb4f446365ea04d68d48539aae66f5b04e31e6b38b594d2723ab82d44512460000000000200acffffffff5dfc6febb484fff69c9eeb7c7eb972e91b6d949295571b8235b1da8955f3137b020000000851ac6352516a535325828c8a03365da801000000000800636aabac6551ab0f594d03000000000963ac536365ac63636a45329e010000000005abac53526a00000000", "005151", 0, 1317038910, "42f5ba6f5fe1e00e652a08c46715871dc4b40d89d9799fd7c0ea758f86eab6a7");
        run_test_sighash("046ac25e030a344116489cc48025659a363da60bc36b3a8784df137a93b9afeab91a04c1ed020000000951ab0000526a65ac51ffffffff6c094a03869fde55b9a8c4942a9906683f0a96e2d3e5a03c73614ea3223b2c29020000000500ab636a6affffffff3da7aa5ecef9071600866267674b54af1740c5aeb88a290c459caa257a2683cb0000000004ab6565ab7e2a1b900301b916030000000005abac63656308f4ed03000000000852ab53ac63ac51ac73d620020000000003ab00008deb1285", "6a", 2, 1299505108, "f79e6b776e2592bad45ca328c54abf14050c241d8f822d982c36ea890fd45757");
        run_test_sighash("ff5400dd02fec5beb9a396e1cbedc82bedae09ed44bae60ba9bef2ff375a6858212478844b03000000025253ffffffff01e46c203577a79d1172db715e9cc6316b9cfc59b5e5e4d9199fef201c6f9f0f000000000900ab6552656a5165acffffffff02e8ce62040000000002515312ce3e00000000000251513f119316", "", 0, 1541581667, "1e0da47eedbbb381b0e0debbb76e128d042e02e65b11125e17fd127305fc65cd");
        run_test_sighash("b54bf5ac043b62e97817abb892892269231b9b220ba08bc8dbc570937cd1ea7cdc13d9676c010000000451ab5365a10adb7b35189e1e8c00b86250f769319668189b7993d6bdac012800f1749150415b2deb0200000003655300ffffffff60b9f4fb9a7e17069fd00416d421f804e2ef2f2c67de4ca04e0241b9f9c1cc5d0200000003ab6aacfffffffff048168461cce1d40601b42fbc5c4f904ace0d35654b7cc1937ccf53fe78505a0100000008526563525265abacffffffff01dbf4e6040000000007acac656553636500000000", "63", 2, 882302077, "f5b38b0f06e246e47ce622e5ee27d5512c509f8ac0e39651b3389815eff2ab93");
        run_test_sighash("ebf628b30360bab3fa4f47ce9e0dcbe9ceaf6675350e638baff0c2c197b2419f8e4fb17e16000000000452516365ac4d909a79be207c6e5fb44fbe348acc42fc7fe7ef1d0baa0e4771a3c4a6efdd7e2c118b0100000003acacacffffffffa6166e9101f03975721a3067f1636cc390d72617be72e5c3c4f73057004ee0ee010000000863636a6a516a5252c1b1e82102d8d54500000000000153324c900400000000015308384913", "0063516a51", 1, -1658428367, "eb2d8dea38e9175d4d33df41f4087c6fea038a71572e3bad1ea166353bf22184");
        run_test_sighash("d6a8500303f1507b1221a91adb6462fb62d741b3052e5e7684ea7cd061a5fc0b0e93549fa50100000004acab65acfffffffffdec79bf7e139c428c7cfd4b35435ae94336367c7b5e1f8e9826fcb0ebaaaea30300000000ffffffffd115fdc00713d52c35ea92805414bd57d1e59d0e6d3b79a77ee18a3228278ada020000000453005151ffffffff040231510300000000085100ac6a6a000063c6041c0400000000080000536a6563acac138a0b04000000000263abd25fbe03000000000900656a00656aac510000000000", "ac526aac6a00", 1, -2007972591, "13d12a51598b34851e7066cd93ab8c5212d60c6ed2dae09d91672c10ccd7f87c");
        run_test_sighash("e92492cc01aec4e62df67ea3bc645e2e3f603645b3c5b353e4ae967b562d23d6e043badecd0100000003acab65ffffffff02c7e5ea040000000002ab52e1e584010000000005536365515195d16047", "6551", 0, -424930556, "93c34627f526d73f4bea044392d1a99776b4409f7d3d835f23b03c358f5a61c2");
        run_test_sighash("b28d5f5e015a7f24d5f9e7b04a83cd07277d452e898f78b50aae45393dfb87f94a26ef57720200000008ababac630053ac52ffffffff046475ed040000000008ab5100526363ac65c9834a04000000000251abae26b30100000000040000ac65ceefb900000000000000000000", "ac6551ac6a536553", 0, -1756558188, "5848d93491044d7f21884eef7a244fe7d38886f8ae60df49ce0dfb2a342cd51a");
        run_test_sighash("efb8b09801f647553b91922a5874f8e4bb2ed8ddb3536ed2d2ed0698fac5e0e3a298012391030000000952ac005263ac52006affffffff04cdfa0f050000000007ac53ab51abac65b68d1b02000000000553ab65ac00d057d50000000000016a9e1fda010000000007ac63ac536552ac00000000", "6aac", 0, 1947322973, "603a9b61cd30fcea43ef0a5c18b88ca372690b971b379ee9e01909c336280511");
        run_test_sighash("67761f2a014a16f3940dcb14a22ba5dc057fcffdcd2cf6150b01d516be00ef55ef7eb07a830100000004636a6a51ffffffff01af67bd050000000008526553526300510000000000", "6a00", 0, 1570943676, "079fa62e9d9d7654da8b74b065da3154f3e63c315f25751b4d896733a1d67807");
        run_test_sighash("ec02fbee03120d02fde12574649660c441b40d330439183430c6feb404064d4f507e704f3c0100000000ffffffffe108d99c7a4e5f75cc35c05debb615d52fac6e3240a6964a29c1704d98017fb60200000002ab63fffffffff726ec890038977adfc9dadbeaf5e486d5fcb65dc23acff0dd90b61b8e2773410000000002ac65e9dace55010f881b010000000005ac00ab650000000000", "51ac525152ac6552", 2, -1564046020, "3f988922d8cd11c7adff1a83ce9499019e5ab5f424752d8d361cf1762e04269b");
        run_test_sighash("33b03bf00222c7ca35c2f8870bbdef2a543b70677e413ce50494ac9b22ea673287b6aa55c50000000005ab00006a52ee4d97b527eb0b427e4514ea4a76c81e68c34900a23838d3e57d0edb5410e62eeb8c92b6000000000553ac6aacac42e59e170326245c000000000009656553536aab516aabb1a10603000000000852ab52ab6a516500cc89c802000000000763ac6a63ac516300000000", "", 0, 557416556, "41bead1b073e1e9fee065dd612a617ca0689e8f9d3fed9d0acfa97398ebb404c");
        run_test_sighash("813eda1103ac8159850b4524ef65e4644e0fc30efe57a5db0c0365a30446d518d9b9aa8fdd0000000003656565c2f1e89448b374b8f12055557927d5b33339c52228f7108228149920e0b77ef0bcd69da60000000006abac00ab63ab82cdb7978d28630c5e1dc630f332c4245581f787936f0b1e84d38d33892141974c75b4750300000004ac53ab65ffffffff0137edfb02000000000000000000", "0063", 1, -1948560575, "71dfcd2eb7f2e6473aed47b16a6d5fcbd0af22813d892e9765023151e07771ec");
        run_test_sighash("9e45d9aa0248c16dbd7f435e8c54ae1ad086de50c7b25795a704f3d8e45e1886386c653fbf01000000025352fb4a1acefdd27747b60d1fb79b96d14fb88770c75e0da941b7803a513e6d4c908c6445c7010000000163ffffffff014069a8010000000001520a794fb3", "51ac005363", 1, -719113284, "0d31a221c69bd322ef7193dd7359ddfefec9e0a1521d4a8740326d46e44a5d6a");
        run_test_sighash("c3efabba03cb656f154d1e159aa4a1a4bf9423a50454ebcef07bc3c42a35fb8ad84014864d0000000000d1cc73d260980775650caa272e9103dc6408bdacaddada6b9c67c88ceba6abaa9caa2f7d020000000553536a5265ffffffff9f946e8176d9b11ff854b76efcca0a4c236d29b69fb645ba29d406480427438e01000000066a0065005300ffffffff040419c0010000000003ab6a63cdb5b6010000000009006300ab5352656a63f9fe5e050000000004acac5352611b980100000000086a00acac00006a512d7f0c40", "0053", 0, -59089911, "c503001c16fbff82a99a18d88fe18720af63656fccd8511bca1c3d0d69bd7fc0");
        run_test_sighash("efb55c2e04b21a0c25e0e29f6586be9ef09f2008389e5257ebf2f5251051cdc6a79fce2dac020000000351006affffffffaba73e5b6e6c62048ba5676d18c33ccbcb59866470bb7911ccafb2238cfd493802000000026563ffffffffe62d7cb8658a6eca8a8babeb0f1f4fa535b62f5fc0ec70eb0111174e72bbec5e0300000009abababac516365526affffffffbf568789e681032d3e3be761642f25e46c20322fa80346c1146cb47ac999cf1b0300000000b3dbd55902528828010000000001ab0aac7b0100000000015300000000", "acac52", 3, 1638140535, "e84444d91580da41c8a7dcf6d32229bb106f1be0c811b2292967ead5a96ce9d4");
        run_test_sighash("91d3b21903629209b877b3e1aef09cd59aca6a5a0db9b83e6b3472aceec3bc2109e64ab85a0200000003530065ffffffffca5f92de2f1b7d8478b8261eaf32e5656b9eabbc58dcb2345912e9079a33c4cd010000000700ab65ab00536ad530611da41bbd51a389788c46678a265fe85737b8d317a83a8ff7a839debd18892ae5c80300000007ab6aac65ab51008b86c501038b8a9a05000000000263525b3f7a040000000007ab535353ab00abd4e3ff04000000000665ac51ab65630b7b656f", "6551525151516a00", 2, 499657927, "ef4bd7622eb7b2bbbbdc48663c1bc90e01d5bde90ff4cb946596f781eb420a0c");
        run_test_sighash("ceecfa6c02b7e3345445b82226b15b7a097563fa7d15f3b0c979232b138124b62c0be007890200000009abac51536a63525253ffffffffbae481ccb4f15d94db5ec0d8854c24c1cc8642bd0c6300ede98a91ca13a4539a0200000001ac50b0813d023110f5020000000006acabac526563e2b0d0040000000009656aac0063516a536300000000", "0063526500", 0, -1862053821, "e1600e6df8a6160a79ac32aa40bb4644daa88b5f76c0d7d13bf003327223f70c");
        run_test_sighash("f06f64af04fdcb830464b5efdb3d5ee25869b0744005375481d7b9d7136a0eb8828ad1f0240200000003516563fffffffffd3ba192dabe9c4eb634a1e3079fca4f072ee5ceb4b57deb6ade5527053a92c5000000000165ffffffff39f43401a36ba13a5c6dd7f1190e793933ae32ee3bf3e7bfb967be51e681af760300000009650000536552636a528e34f50b21183952cad945a83d4d56294b55258183e1627d6e8fb3beb8457ec36cadb0630000000005abab530052334a7128014bbfd10100000000085352ab006a63656afc424a7c", "53650051635253ac00", 2, 313255000, "d309da5afd91b7afa257cfd62df3ca9df036b6a9f4b38f5697d1daa1f587312b");
        run_test_sighash("6dfd2f98046b08e7e2ef5fff153e00545faf7076699012993c7a30cb1a50ec528281a9022f030000000152ffffffff1f535e4851920b968e6c437d84d6ecf586984ebddb7d5db6ae035bd02ba222a8010000000651006a53ab51605072acb3e17939fa0737bc3ee43bc393b4acd58451fc4ffeeedc06df9fc649828822d5010000000253525a4955221715f27788d302382112cf60719be9ae159c51f394519bd5f7e70a4f9816c7020200000009526a6a51636aab656a36d3a5ff0445548e0100000000086a6a00516a52655167030b050000000004ac6a63525cfda8030000000000e158200000000000010000000000", "535263ac6a65515153", 3, 585774166, "72b7da10704c3ca7d1deb60c31b718ee12c70dc9dfb9ae3461edce50789fe2ba");
        run_test_sighash("2a45cd1001bf642a2315d4a427eddcc1e2b0209b1c6abd2db81a800c5f1af32812de42032702000000050051525200ffffffff032177db050000000005530051abac49186f000000000004ab6aab00645c0000000000000765655263acabac00000000", "6a65", 0, -1774715722, "6a9ac3f7da4c7735fbc91f728b52ecbd602233208f96ac5592656074a5db118a");
        run_test_sighash("ca816e7802cd43d66b9374cd9bf99a8da09402d69c688d8dcc5283ace8f147e1672b757e020200000005516aabab5240fb06c95c922342279fcd88ba6cd915933e320d7becac03192e0941e0345b79223e89570300000004005151ac353ecb5d0264dfbd010000000005ac6aacababd5d70001000000000752ac53ac6a5151ec257f71", "63ac", 1, 774695685, "cc180c4f797c16a639962e7aec58ec4b209853d842010e4d090895b22e7a7863");
        run_test_sighash("b42b955303942fedd7dc77bbd9040aa0de858afa100f399d63c7f167b7986d6c2377f66a7403000000066aac00525100ffffffff0577d04b64880425a3174055f94191031ad6b4ca6f34f6da9be7c3411d8b51fc000000000300526a6391e1cf0f22e45ef1c44298523b516b3e1249df153590f592fcb5c5fc432dc66f3b57cb03000000046a6aac65ffffffff0393a6c9000000000004516a65aca674ac0400000000046a525352c82c370000000000030053538e577f89", "", 1, -1237094944, "566953eb806d40a9fb684d46c1bf8c69dea86273424d562bd407b9461c8509af");
        run_test_sighash("92c9fe210201e781b72554a0ed5e22507fb02434ddbaa69aff6e74ea8bad656071f1923f3f02000000056a63ac6a514470cef985ba83dcb8eee2044807bedbf0d983ae21286421506ae276142359c8c6a34d68020000000863ac63525265006aa796dd0102ca3f9d05000000000800abab52ab535353cd5c83010000000007ac00525252005322ac75ee", "5165", 0, 97879971, "6e6307cef4f3a9b386f751a6f40acebab12a0e7e17171d2989293cbec7fd45c2");
        run_test_sighash("ccca1d5b01e40fe2c6b3ee24c660252134601dab785b8f55bd6201ffaf2fddc7b3e2192325030000000365535100496d4703b4b66603000000000665535253ac633013240000000000015212d2a502000000000951abac636353636a5337b82426", "0052", 0, -1691630172, "577bf2b3520b40aef44899a20d37833f1cded6b167e4d648fc5abe203e43b649");
        run_test_sighash("bc1a7a3c01691e2d0c4266136f12e391422f93655c71831d90935fbda7e840e50770c61da20000000008635253abac516353ffffffff031f32aa020000000003636563786dbc0200000000003e950f00000000000563516a655184b8a1de", "51536a", 0, -1627072905, "730bc25699b46703d7718fd5f5c34c4b5f00f594a9968ddc247fa7d5175124ed");
        run_test_sighash("076d209e02d904a6c40713c7225d23e7c25d4133c3c3477828f98c7d6dbd68744023dbb66b030000000753ab00536565acffffffff10975f1b8db8861ca94c8cc7c7cff086ddcd83e10b5fffd4fc8f2bdb03f9463c0100000000ffffffff029dff76010000000006526365530051a3be6004000000000000000000", "515253ac65acacac", 1, -1207502445, "66c488603b2bc53f0d22994a1f0f66fb2958203102eba30fe1d37b27a55de7a5");
        run_test_sighash("cac6382d0462375e83b67c7a86c922b569a7473bfced67f17afd96c3cd2d896cf113febf9e0300000003006a53ffffffffaa4913b7eae6821487dd3ca43a514e94dcbbf350f8cc4cafff9c1a88720711b800000000096a6a525300acac6353ffffffff184fc4109c34ea27014cc2c1536ef7ed1821951797a7141ddacdd6e429fae6ff01000000055251655200ffffffff9e7b79b4e6836e290d7b489ead931cba65d1030ccc06f20bd4ca46a40195b33c030000000008f6bc8304a09a2704000000000563655353511dbc73050000000000cf34c500000000000091f76e0000000000085200ab00005100abd07208cb", "0063656a", 2, -1488731031, "bf078519fa87b79f40abc38f1831731422722c59f88d86775535f209cb41b9b1");
        run_test_sighash("1711146502c1a0b82eaa7893976fefe0fb758c3f0e560447cef6e1bde11e42de91a125f71c030000000015bd8c04703b4030496c7461482481f290c623be3e76ad23d57a955807c9e851aaaa20270300000000d04abaf20326dcb7030000000001632225350400000000075263ac00520063dddad9020000000000af23d148", "52520053510063", 0, 1852122830, "e33d5ee08c0f3c130a44d7ce29606450271b676f4a80c52ab9ffab00cecf67f8");
        run_test_sighash("22d81c740469695a6a83a9a4824f77ecff8804d020df23713990afce2b72591ed7de98500502000000065352526a6a6affffffff90dc85e118379b1005d7bbc7d2b8b0bab104dad7eaa49ff5bead892f17d8c3ba010000000665656300ab51ffffffff965193879e1d5628b52005d8560a35a2ba57a7f19201a4045b7cbab85133311d0200000003ac005348af21a13f9b4e0ad90ed20bf84e4740c8a9d7129632590349afc03799414b76fd6e826200000000025353ffffffff04a0d40d04000000000060702700000000000652655151516ad31f1502000000000365ac0069a1ac0500000000095100655300ab53525100000000", "51636a52ac", 0, -1644680765, "add7f5da27262f13da6a1e2cc2feafdc809bd66a67fb8ae2a6f5e6be95373b6f");
        run_test_sighash("a27dcbc801e3475174a183586082e0914c314bc9d79d1570f29b54591e5e0dff07fbb45a7f0000000004ac53ab51ffffffff027347f5020000000005535351ab63d0e5c9030000000009ac65ab6a63515200ab7cd632ed", "ac63636553", 0, -686435306, "883a6ea3b2cc53fe8a803c229106366ca14d25ffbab9fef8367340f65b201da6");
        run_test_sighash("0728c606014c1fd6005ccf878196ba71a54e86cc8c53d6db500c3cc0ac369a26fac6fcbc210000000005ab53ac5365ba9668290182d7870100000000066a000053655100000000", "65", 0, 1789961588, "ab6baa6da3b2bc853868d166f8996ad31d63ef981179f9104f49968fd61c8427");
        run_test_sighash("ed3bb93802ddbd08cb030ef60a2247f715a0226de390c9c1a81d52e83f8674879065b5f87d0300000003ab6552ffffffff04d2c5e60a21fb6da8de20bf206db43b720e2a24ce26779bca25584c3f765d1e0200000008ab656a6aacab00ab6e946ded025a811d04000000000951abac6352ac00ab5143cfa3030000000005635200636a00000000", "5352ac650065535300", 1, -668727133, "e9995065e1fddef72a796eef5274de62012249660dc9d233a4f24e02a2979c87");
        run_test_sighash("59f4629d030fa5d115c33e8d55a79ea3cba8c209821f979ed0e285299a9c72a73c5bba00150200000002636affffffffd8aca2176df3f7a96d0dc4ee3d24e6cecde1582323eec2ebef9a11f8162f17ac0000000007ab6565acab6553ffffffffeebc10af4f99c7a21cbc1d1074bd9f0ee032482a71800f44f26ee67491208e0403000000065352ac656351ffffffff0434e955040000000004ab515152caf2b305000000000365ac007b1473030000000003ab530033da970500000000060051536a5253bb08ab51", "", 2, 396340944, "0e9c47973ef2c292b2252c623f465bbb92046fe0b893eebf4e1c9e02cb01c397");
        run_test_sighash("33a98004029d262f951881b20a8d746c8c707ea802cd2c8b02a33b7e907c58699f97e42be80100000007ac53536552abacdee04cc01d205fd8a3687fdf265b064d42ab38046d76c736aad8865ca210824b7c622ecf02000000070065006a536a6affffffff01431c5d010000000000270d48ee", "", 1, 921554116, "ff9d7394002f3f196ea25472ea6c46f753bd879a7244795157bb7235c9322902");
        run_test_sighash("aac18f2b02b144ed481557c53f2146ae523f24fcde40f3445ab0193b6b276c315dc2894d2300000000075165650000636a233526947dbffc76aec7db1e1baa6868ad4799c76e14794dcbaaec9e713a83967f6a65170200000005abac6551ab27d518be01b652a30000000000015300000000", "52ac5353", 1, 1559377136, "59fc2959bb7bb24576cc8a237961ed95bbb900679d94da6567734c4390cb6ef5");
        run_test_sighash("5ab79881033555b65fe58c928883f70ce7057426fbdd5c67d7260da0fe8b1b9e6a2674cb850300000009ac516aac6aac006a6affffffffa5be9223b43c2b1a4d120b5c5b6ec0484f637952a3252181d0f8e813e76e11580200000000e4b5ceb8118cb77215bbeedc9a076a4d087bb9cd1473ea32368b71daeeeacc451ec209010000000005acac5153aced7dc34e02bc5d11030000000005ac5363006a54185803000000000552ab00636a00000000", "5100", 1, 1927062711, "e9f53d531c12cce1c50abed4ac521a372b4449b6a12f9327c80020df6bff66c0");
        run_test_sighash("6c2c8fac0124b0b7d4b610c3c5b91dee32b7c927ac71abdf2d008990ca1ac40de0dfd530660300000006ababac5253656bd7eada01d847ec000000000004ac52006af4232ec8", "6a6a6a0051", 0, -340809707, "fb51eb9d7e47d32ff2086205214f90c7c139e08c257a64829ae4d2b301071c6a");
        run_test_sighash("0e803682024f79337b25c98f276d412bc27e56a300aa422c42994004790cee213008ff1b8303000000080051ac65ac655165f421a331892b19a44c9f88413d057fea03c3c4a6c7de4911fe6fe79cf2e9b3b10184b1910200000005525163630096cb1c670398277204000000000253acf7d5d502000000000963536a6a636a5363ab381092020000000002ac6a911ccf32", "6565", 1, -1492094009, "f0672638a0e568a919e9d8a9cbd7c0189a3e132940beeb52f111a89dcc2daa2c");
        run_test_sighash("7d71669d03022f9dd90edac323cde9e56354c6804c6b8e687e9ae699f46805aafb8bcaa636000000000253abffffffff698a5fdd3d7f2b8b000c68333e4dd58fa8045b3e2f689b889beeb3156cecdb490300000009525353abab0051acabc53f0aa821cdd69b473ec6e6cf45cf9b38996e1c8f52c27878a01ec8bb02e8cb31ad24e500000000055353ab0052ffffffff0447a23401000000000565ab53ab5133aaa0030000000006515163656563057d110300000000056a6aacac52cf13b5000000000003526a5100000000", "6a6a51", 1, -1349253507, "722efdd69a7d51d3d77bed0ac5544502da67e475ea5857cd5af6bdf640a69945");
        run_test_sighash("9ff618e60136f8e6bb7eabaaac7d6e2535f5fba95854be6d2726f986eaa9537cb283c701ff02000000026a65ffffffff012d1c0905000000000865ab00ac6a516a652f9ad240", "51515253635351ac", 0, 1571304387, "659cd3203095d4a8672646add7d77831a1926fc5b66128801979939383695a79");
        run_test_sighash("09adb2e90175ca0e816326ae2dce7750c1b27941b16f6278023dbc294632ab97977852a09d030000000465ab006affffffff027739cf0100000000075151ab63ac65ab8a5bb601000000000653ac5151520011313cdc", "ac", 0, -76831756, "478ee06501b4965b40bdba6cbaad9b779b38555a970912bb791b86b7191c54bc");
        run_test_sighash("fd22ebaa03bd588ad16795bea7d4aa7f7d48df163d75ea3afebe7017ce2f350f6a0c1cb0bb00000000086aabac5153526363ffffffff488e0bb22e26a565d77ba07178d17d8f85702630ee665ec35d152fa05af3bda10200000004515163abffffffffeb21035849e85ad84b2805e1069a91bb36c425dc9c212d9bae50a95b6bfde1200300000001ab5df262fd02b69848040000000008ab6363636a6363ace23bf2010000000007655263635253534348c1da", "006353526563516a00", 0, -1491036196, "92364ba3c7a85d4e88885b8cb9b520dd81fc29e9d2b750d0790690e9c1246673");
        run_test_sighash("b04154610363fdade55ceb6942d5e5a723323863b48a0cb04fdcf56210717955763f56b08d0300000009ac526a525151635151ffffffff93a176e76151a9eabdd7af00ef2af72f9e7af5ecb0aa4d45d00618f394cdd03c030000000074d818b332ebe05dc24c44d776cf9d275c61f471cc01efce12fd5a16464157f1842c65cb00000000066a0000ac6352d3c4134f01d8a1c0030000000005520000005200000000", "5200656a656351", 2, -9757957, "6e3e5ba77f760b6b5b5557b13043f1262418f3dd2ce7f0298b012811fc8ad5bc");
        run_test_sighash("94533db7015e70e8df715066efa69dbb9c3a42ff733367c18c22ff070392f988f3b93920820000000006535363636300ce4dac3e03169af80300000000080065ac6a53ac65ac39c050020000000006abacab6aacac708a02050000000005ac5251520000000000", "6553", 0, -360458507, "5418cf059b5f15774836edd93571e0eed3855ba67b2b08c99dccab69dc87d3e9");
        run_test_sighash("c8597ada04f59836f06c224a2640b79f3a8a7b41ef3efa2602592ddda38e7597da6c639fee0300000009005251635351acabacffffffff4c518f347ee694884b9d4072c9e916b1a1f0a7fc74a1c90c63fdf8e5a185b6ae02000000007113af55afb41af7518ea6146786c7c726641c68c8829a52925e8d4afd07d8945f68e7230300000008ab00ab65ab650063ffffffffc28e46d7598312c420e11dfaae12add68b4d85adb182ae5b28f8340185394b63000000000165ffffffff04dbabb7010000000000ee2f6000000000000852ab6500ab6a51acb62a27000000000009ac53515300ac006a6345fb7505000000000752516a0051636a00000000", "", 3, 15199787, "0d66003aff5bf78cf492ecbc8fd40c92891acd58d0a271be9062e035897f317e");
        run_test_sighash("c33028b301d5093e1e8397270d75a0b009b2a6509a01861061ab022ca122a6ba935b8513320200000000ffffffff013bcf5a0500000000015200000000", "", 0, -513413204, "6b1459536f51482f5dbf42d7e561896557461e1e3b6bf67871e2b51faae2832c");
        run_test_sighash("43b2727901a7dd06dd2abf690a1ccedc0b0739cb551200796669d9a25f24f71d8d101379f50300000000ffffffff0418e031040000000000863d770000000000085352ac526563ac5174929e040000000004ac65ac00ec31ac0100000000066a51ababab5300000000", "65", 0, -492874289, "154ff7a9f0875edcfb9f8657a0b98dd9600fabee3c43eb88af37cf99286d516c");
        run_test_sighash("fe6ddf3a02657e42a7496ef170b4a8caf245b925b91c7840fd28e4a22c03cb459cb498b8d603000000065263656a650071ce6bf8d905106f9f1faf6488164f3decac65bf3c5afe1dcee20e6bc3cb6d052561985a030000000163295b117601343dbb0000000000026563dba521df", "", 1, -1696179931, "d9684685c99ce48f398fb467a91a1a59629a850c429046fb3071f1fa9a5fe816");
        run_test_sighash("c61523ef0129bb3952533cbf22ed797fa2088f307837dd0be1849f20decf709cf98c6f032f03000000026563c0f1d378044338310400000000066363516a5165a14fcb0400000000095163536a6a00ab53657271d60200000000001d953f0500000000010000000000", "53516353005153", 0, 1141615707, "7e975a72db5adaa3c48d525d9c28ac11cf116d0f8b16ce08f735ad75a80aec66");
        run_test_sighash("ba3dac6c0182562b0a26d475fe1e36315f0913b6869bdad0ecf21f1339a5fcbccd32056c840200000000ffffffff04300351050000000000220ed405000000000851abac636565ac53dbbd19020000000007636363ac6a52acbb005a0500000000016abd0c78a8", "63006a635151005352", 0, 1359658828, "47bc8ab070273e1f4a0789c37b45569a6e16f3f3092d1ce94dddc3c34a28f9f4");
        run_test_sighash("ac27e7f5025fc877d1d99f7fc18dd4cadbafa50e34e1676748cc89c202f93abf36ed46362101000000036300abffffffff958cd5381962b765e14d87fc9524d751e4752dd66471f973ed38b9d562e525620100000003006500ffffffff02b67120050000000004ac51516adc330c0300000000015200000000", "656352", 1, 15049991, "f3374253d64ac264055bdbcc32e27426416bd595b7c7915936c70f839e504010");
        run_test_sighash("7e50207303146d1f7ad62843ae8017737a698498d4b9118c7a89bb02e8370307fa4fada41d000000000753006300005152b7afefc85674b1104ba33ef2bf37c6ed26316badbc0b4aa6cb8b00722da4f82ff3555a6c020000000900ac656363ac51ac52ffffffff93fab89973bd322c5d7ad7e2b929315453e5f7ada3072a36d8e33ca8bebee6e0020000000300acab930da52b04384b04000000000004650052ac435e380200000000076a6a515263ab6aa9494705000000000600ab6a525252af8ba90100000000096565acab526353536a279b17ad", "acac005263536aac63", 1, -34754133, "4e6357da0057fb7ff79da2cc0f20c5df27ff8b2f8af4c1709e6530459f7972b0");
        run_test_sighash("5a59e0b9040654a3596d6dab8146462363cd6549898c26e2476b1f6ae42915f73fd9aedfda00000000036363abffffffff9ac9e9ca90be0187be2214251ff08ba118e6bf5e2fd1ba55229d24e50a510d53010000000165ffffffff41d42d799ac4104644969937522873c0834cc2fcdab7cdbecd84d213c0e96fd60000000000ffffffffd838db2c1a4f30e2eaa7876ef778470f8729fcf258ad228b388df2488709f8410300000000fdf2ace002ceb6d903000000000265654c1310040000000003ac00657e91c0ec", "536a63ac", 0, 82144555, "98ccde2dc14d14f5d8b1eeea5364bd18fc84560fec2fcea8de4d88b49c00695e");
        run_test_sighash("156ebc8202065d0b114984ee98c097600c75c859bfee13af75dc93f57c313a877efb09f230010000000463536a51ffffffff81114e8a697be3ead948b43b5005770dd87ffb1d5ccd4089fa6c8b33d3029e9c03000000066a5251656351ffffffff01a87f140000000000050000ac51ac00000000", "00", 0, -362221092, "a903c84d8c5e71134d1ab6dc1e21ac307c4c1a32c90c90f556f257b8a0ec1bf5");
        run_test_sighash("6d97a9a5029220e04f4ccc342d8394c751282c328bf1c132167fc05551d4ca4da4795f6d4e02000000076a0052ab525165ffffffff9516a205e555fa2a16b73e6db6c223a9e759a7e09c9a149a8f376c0a7233fa1b0100000007acab51ab63ac6affffffff04868aed04000000000652ac65ac536a396edf01000000000044386c0000000000076aab5363655200894d48010000000001ab8ebefc23", "6351526aac51", 1, 1943666485, "f0bd4ca8e97203b9b4e86bc24bdc8a1a726db5e99b91000a14519dc83fc55c29");
        run_test_sighash("05921d7c048cf26f76c1219d0237c226454c2a713c18bf152acc83c8b0647a94b13477c07f0300000003ac526afffffffff2f494453afa0cabffd1ba0a626c56f90681087a5c1bd81d6adeb89184b27b7402000000036a6352ffffffff0ad10e2d3ce355481d1b215030820da411d3f571c3f15e8daf22fe15342fed04000000000095f29f7b93ff814a9836f54dc6852ec414e9c4e16a506636715f569151559100ccfec1d100000000055263656a53ffffffff04f4ffef010000000008ac6a6aabacabab6a0e6689040000000006ab536a5352abe364d005000000000965536363655251ab53807e00010000000004526aab63f18003e3", "6363ac51", 3, -375891099, "001b0b176f0451dfe2d9787b42097ceb62c70d324e925ead4c58b09eebdf7f67");
        run_test_sighash("b9b44d9f04b9f15e787d7704e6797d51bc46382190c36d8845ec68dfd63ee64cf7a467b21e00000000096aac00530052ab636aba1bcb110a80c5cbe073f12c739e3b20836aa217a4507648d133a8eedd3f02cb55c132b203000000076a000063526352b1c288e3a9ff1f2da603f230b32ef7c0d402bdcf652545e2322ac01d725d75f5024048ad0100000000ffffffffffd882d963be559569c94febc0ef241801d09dc69527c9490210f098ed8203c700000000056a006300ab9109298d01719d9a0300000000066a52ab006365d7894c5b", "ac6351650063636a", 3, -622355349, "ac87b1b93a6baab6b2c6624f10e8ebf6849b0378ef9660a3329073e8f5553c8d");
        run_test_sighash("ff60473b02574f46d3e49814c484081d1adb9b15367ba8487291fc6714fd6e3383d5b335f001000000026a6ae0b82da3dc77e5030db23d77b58c3c20fa0b70aa7d341a0f95f3f72912165d751afd57230300000008ac536563516a6363ffffffff04f86c0200000000000553acab636ab13111000000000003510065f0d3f305000000000951ab516a65516aabab730a3a010000000002515200000000", "ac6a", 1, 1895032314, "0767e09bba8cd66d55915677a1c781acd5054f530d5cf6de2d34320d6c467d80");
        run_test_sighash("f218026204f4f4fc3d3bd0eada07c57b88570d544a0436ae9f8b753792c0c239810bb30fbc0200000002536affffffff8a468928d6ec4cc10aa0f73047697970e99fa64ae8a3b4dca7551deb0b639149010000000851ab520052650051ffffffffa98dc5df357289c9f6873d0f5afcb5b030d629e8f23aa082cf06ec9a95f3b0cf0000000000ffffffffea2c2850c5107705fd380d6f29b03f533482fd036db88739122aac9eff04e0aa010000000365536a03bd37db034ac4c4020000000007515152655200ac33b27705000000000151efb71e0000000000007b65425b", "515151", 3, -1772252043, "de35c84a58f2458c33f564b9e58bc57c3e028d629f961ad1b3c10ee020166e5a");
        run_test_sighash("48e7d42103b260b27577b70530d1ac2fed2551e9dd607cbcf66dca34bb8c03862cf8f5fd5401000000075151526aacab00ffffffff1e3d3b841552f7c6a83ee379d9d66636836673ce0b0eda95af8f2d2523c91813030000000665acac006365ffffffff388b3c386cd8c9ef67c83f3eaddc79f1ff910342602c9152ffe8003bce51b28b0100000008636363006a636a52ffffffff04b8f67703000000000852005353ac6552520cef720200000000085151ab6352ab00ab5096d6030000000005516a005100662582020000000001ac6c137280", "6a65", 1, 1513618429, "e2fa3e1976aed82c0987ab30d4542da2cb1cffc2f73be13480132da8c8558d5c");
        run_test_sighash("91ebc4cf01bc1e068d958d72ee6e954b196f1d85b3faf75a521b88a78021c543a06e056279000000000265ab7c12df0503832121030000000000cc41a6010000000005ab5263516540a951050000000006ab63ab65acac00000000", "526a0065636a6a6aac", 0, -614046478, "7de4ba875b2e584a7b658818c112e51ee5e86226f5a80e5f6b15528c86400573");
        run_test_sighash("3cd4474201be7a6c25403bf00ca62e2aa8f8f4f700154e1bb4d18c66f7bb7f9b975649f0dc0100000006535151535153ffffffff01febbeb000000000006005151006aac00000000", "", 0, -1674687131, "6b77ca70cc452cc89acb83b69857cda98efbfc221688fe816ef4cb4faf152f86");
        run_test_sighash("92fc95f00307a6b3e2572e228011b9c9ed41e58ddbaefe3b139343dbfb3b34182e9fcdc3f50200000002acab847bf1935fde8bcfe41c7dd99683289292770e7f163ad09deff0e0665ed473cd2b56b0f40300000006516551ab6351294dab312dd87b9327ce2e95eb44b712cfae0e50fda15b07816c8282e8365b643390eaab01000000026aacffffffff016e0b6b040000000001ac00000000", "650065acac005300", 2, -1885164012, "bd7d26bb3a98fc8c90c972500618bf894cb1b4fe37bf5481ff60eef439d3b970");
        run_test_sighash("4db591ab018adcef5f4f3f2060e41f7829ce3a07ea41d681e8cb70a0e37685561e4767ac3b0000000005000052acabd280e63601ae6ef20000000000036a636326c908f7", "ac6a51526300630052", 0, 862877446, "355ccaf30697c9c5b966e619a554d3323d7494c3ea280a9b0dfb73f953f5c1cb");
        run_test_sighash("c80abebd042cfec3f5c1958ee6970d2b4586e0abec8305e1d99eb9ee69ecc6c2cbd76374380000000007ac53006300ac510acee933b44817db79320df8094af039fd82111c7726da3b33269d3820123694d849ee5001000000056a65ab526562699bea8530dc916f5d61f0babea709dac578774e8a4dcd9c640ec3aceb6cb2443f24f302000000020063ea780e9e57d1e4245c1e5df19b4582f1bf704049c5654f426d783069bcc039f2d8fa659f030000000851ab53635200006a8d00de0b03654e8500000000000463ab635178ebbb0400000000055100636aab239f1d030000000006ab006300536500000000", "6565ac515100", 3, 1460851377, "b35bb1b72d02fab866ed6bbbea9726ab32d968d33a776686df3ac16aa445871e");
        run_test_sighash("0337b2d5043eb6949a76d6632b8bb393efc7fe26130d7409ef248576708e2d7f9d0ced9d3102000000075352636a5163007034384dfa200f52160690fea6ce6c82a475c0ef1caf5c9e5a39f8f9ddc1c8297a5aa0eb02000000026a51ffffffff38e536298799631550f793357795d432fb2d4231f4effa183c4e2f61a816bcf0030000000463ac5300706f1cd3454344e521fde05b59b96e875c8295294da5d81d6cc7efcfe8128f150aa54d6503000000008f4a98c704c1561600000000000072cfa6000000000000e43def01000000000100cf31cc0500000000066365526a6500cbaa8e2e", "", 3, 2029506437, "7615b4a7b3be865633a31e346bc3db0bcc410502c8358a65b8127089d81b01f8");
        run_test_sighash("cbc79b10020b15d605680a24ee11d8098ad94ae5203cb6b0589e432832e20c27b72a926af20300000006ab65516a53acbb854f3146e55c508ece25fa3d99dbfde641a58ed88c051a8a51f3dacdffb1afb827814b02000000026352c43e6ef30302410a020000000000ff4bd90100000000065100ab63000008aa8e0400000000095265526565ac5365abc52c8a77", "53526aac0051", 0, 202662340, "984efe0d8d12e43827b9e4b27e97b3777ece930fd1f589d616c6f9b71dab710e");
        run_test_sighash("4e8594d803b1d0a26911a2bcdd46d7cbc987b7095a763885b1a97ca9cbb747d32c5ab9aa91030000000353ac53a0cc4b215e07f1d648b6eeb5cdbe9fa32b07400aa773b9696f582cebfd9930ade067b2b200000000060065abab6500fc99833216b8e27a02defd9be47fafae4e4a97f52a9d2a210d08148d2a4e5d02730bcd460100000004516351ac37ce3ae1033baa55040000000006006a636a63acc63c990400000000025265eb1919030000000005656a6a516a00000000", "", 1, -75217178, "04c5ee48514cd033b82a28e336c4d051074f477ef2675ce0ce4bafe565ee9049");
        run_test_sighash("44e1a2b4010762af23d2027864c784e34ef322b6e24c70308a28c8f2157d90d17b99cd94a401000000085163656565006300ffffffff0198233d020000000002000000000000", "52525153656365", 0, 1119696980, "d9096de94d70c6337da6202e6e588166f31bff5d51bb5adc9468594559d65695");
        run_test_sighash("44ca65b901259245abd50a745037b17eb51d9ce1f41aa7056b4888285f48c6f26cb97b7a25020000000552636363abffffffff047820350400000000040053acab14f3e603000000000652635100ab630ce66c03000000000001bdc704000000000765650065ac51ac3e886381", "51", 0, -263340864, "ed5622ac642d11f90e68c0feea6a2fe36d880ecae6b8c0d89c4ea4b3d162bd90");
        run_test_sighash("cfa147d2017fe84122122b4dda2f0d6318e59e60a7207a2d00737b5d89694d480a2c26324b0000000006006351526552ffffffff0456b5b804000000000800516aab525363ab166633000000000004655363ab254c0e02000000000952ab6a6a00ab525151097c1b020000000009656a52ac6300530065ad0d6e50", "6a535165ac6a536500", 0, -574683184, "f926d4036eac7f019a2b0b65356c4ee2fe50e089dd7a70f1843a9f7bc6997b35");
        run_test_sighash("91c5d5f6022fea6f230cc4ae446ce040d8313071c5ac1749c82982cc1988c94cb1738aa48503000000016a19e204f30cb45dd29e68ff4ae160da037e5fc93538e21a11b92d9dd51cf0b5efacba4dd70000000005656a6aac51ffffffff03db126905000000000953006a53ab6563636a36a273030000000006656a52656552b03ede00000000000352516500000000", "530052526a00", 1, 1437328441, "255c125b60ee85f4718b2972174c83588ee214958c3627f51f13b5fb56c8c317");
        run_test_sighash("03f20dc202c886907b607e278731ebc5d7373c348c8c66cac167560f19b341b782dfb634cb03000000076a51ac6aab63abea3e8de7adb9f599c9caba95aa3fa852e947fc88ed97ee50e0a0ec0d14d164f44c0115c10100000004ab5153516fdd679e0414edbd000000000005ac636a53512021f2040000000007006a0051536a52c73db2050000000005525265ac5369046e000000000003ab006a1ef7bd1e", "52656a", 0, 1360223035, "5a0a05e32ce4cd0558aabd5d79cd5fcbffa95c07137506e875a9afcba4bef5a2");
        run_test_sighash("d9611140036881b61e01627078512bc3378386e1d4761f959d480fdb9d9710bebddba2079d020000000763536aab5153ab819271b41e228f5b04daa1d4e72c8e1955230accd790640b81783cfc165116a9f535a74c000000000163ffffffffa2e7bb9a28e810624c251ff5ba6b0f07a356ac082048cf9f39ec036bba3d431a02000000076a000000ac65acffffffff01678a820000000000085363515153ac635100000000", "535353", 2, -82213851, "52b9e0778206af68998cbc4ebdaad5a9469e04d0a0a6cef251abfdbb74e2f031");
        run_test_sighash("98b3a0bf034233afdcf0df9d46ac65be84ef839e58ee9fa59f32daaa7d684b6bdac30081c60200000007636351acabababffffffffc71cf82ded4d1593e5825618dc1d5752ae30560ecfaa07f192731d68ea768d0f0100000006650052636563f3a2888deb5ddd161430177ce298242c1a86844619bc60ca2590d98243b5385bc52a5b8f00000000095365acacab520052ac50d4722801c3b8a60300000000035165517e563b65", "51", 1, -168940690, "b6b684e2d2ecec8a8dce4ed3fc1147f8b2e45732444222aa8f52d860c2a27a9d");
        run_test_sighash("97be4f7702dc20b087a1fdd533c7de762a3f2867a8f439bddf0dcec9a374dfd0276f9c55cc0300000000cdfb1dbe6582499569127bda6ca4aaff02c132dc73e15dcd91d73da77e92a32a13d1a0ba0200000002ab51ffffffff048cfbe202000000000900516351515363ac535128ce0100000000076aac5365ab6aabc84e8302000000000863536a53ab6a6552f051230500000000066aac535153510848d813", "ac51", 0, 229541474, "e5da9a416ea883be1f8b8b2d178463633f19de3fa82ae25d44ffb531e35bdbc8");
        run_test_sighash("085b6e04040b5bff81e29b646f0ed4a45e05890a8d32780c49d09643e69cdccb5bd81357670100000001abffffffffa5c981fe758307648e783217e3b4349e31a557602225e237f62b636ec26df1a80300000004650052ab4792e1da2930cc90822a8d2a0a91ea343317bce5356b6aa8aae6c3956076aa33a5351a9c0300000004abac5265e27ddbcd472a2f13325cc6be40049d53f3e266ac082172f17f6df817db1936d9ff48c02b000000000152ffffffff021aa7670500000000085353635163ab51ac14d584000000000001aca4d136cc", "6a525300536352536a", 0, -1398925877, "41ecca1e8152ec55074f4c39f8f2a7204dda48e9ec1e7f99d5e7e4044d159d43");
        run_test_sighash("eec32fff03c6a18b12cd7b60b7bdc2dd74a08977e53fdd756000af221228fe736bd9c42d870100000007005353ac515265ffffffff037929791a188e9980e8b9cc154ad1b0d05fb322932501698195ab5b219488fc02000000070063510065ab6a0bfc176aa7e84f771ea3d45a6b9c24887ceea715a0ff10ede63db8f089e97d927075b4f1000000000551abab63abffffffff02eb933c000000000000262c420000000000036563632549c2b6", "6352", 2, 1480445874, "ff8a4016dfdd918f53a45d3a1f62b12c407cd147d68ca5c92b7520e12c353ff5");
        run_test_sighash("3ab70f4604e8fc7f9de395ec3e4c3de0d560212e84a63f8d75333b604237aa52a10da17196000000000763526a6553ac63a25de6fd66563d71471716fe59087be0dde98e969e2b359282cf11f82f14b00f1c0ac70f02000000050052516aacdffed6bb6889a13e46956f4b8af20752f10185838fd4654e3191bf49579c961f5597c36c0100000005ac636363abc3a1785bae5b8a1b4be5d0cbfadc240b4f7acaa7dfed6a66e852835df5eb9ac3c553766801000000036a65630733b7530218569602000000000952006a6a6a51acab52777f06030000000007ac0063530052abc08267c9", "000000536aac0000", 1, 1919096509, "df1c87cf3ba70e754d19618a39fdbd2970def0c1bfc4576260cba5f025b87532");
        run_test_sighash("bdb6b4d704af0b7234ced671c04ba57421aba7ead0a117d925d7ebd6ca078ec6e7b93eea6600000000026565ffffffff3270f5ad8f46495d69b9d71d4ab0238cbf86cc4908927fbb70a71fa3043108e6010000000700516a65655152ffffffff6085a0fdc03ae8567d0562c584e8bfe13a1bd1094c518690ebcb2b7c6ce5f04502000000095251530052536a53aba576a37f2c516aad9911f687fe83d0ae7983686b6269b4dd54701cb5ce9ec91f0e6828390300000000ffffffff04cc76cc020000000002656a01ffb702000000000253ab534610040000000009acab006565516a00521f55f5040000000000389dfee9", "6a525165", 0, 1336204763, "71c294523c48fd7747eebefbf3ca06e25db7b36bff6d95b41c522fecb264a919");
        run_test_sighash("54258edd017d22b274fbf0317555aaf11318affef5a5f0ae45a43d9ca4aa652c6e85f8a040010000000953ac65ab5251656500ffffffff03321d450000000000085265526a51526a529ede8b030000000003635151ce6065020000000001534c56ec1b", "acac", 0, 2094130012, "110d90fea9470dfe6c5048f45c3af5e8cc0cb77dd58fd13d338268e1c24b1ccc");
        run_test_sighash("ce0d322e04f0ffc7774218b251530a7b64ebefca55c90db3d0624c0ff4b3f03f918e8cf6f60300000003656500ffffffff9cce943872da8d8af29022d0b6321af5fefc004a281d07b598b95f6dcc07b1830200000007abab515351acab8d926410e69d76b7e584aad1470a97b14b9c879c8b43f9a9238e52a2c2fefc2001c56af8010000000400ab5253cd2cd1fe192ce3a93b5478af82fa250c27064df82ba416dfb0debf4f0eb307a746b6928901000000096500abacac6a0063514214524502947efc0200000000035251652c40340100000000096a6aab52000052656a5231c54c", "51", 2, -2090320538, "0322ca570446869ec7ec6ad66d9838cff95405002d474c0d3c17708c7ee039c6");
        run_test_sighash("233cd90b043916fc41eb870c64543f0111fb31f3c486dc72457689dea58f75c16ae59e9eb2000000000500536a6a6affffffff9ae30de76be7cd57fb81220fce78d74a13b2dbcad4d023f3cadb3c9a0e45a3ce000000000965ac6353ac5165515130834512dfb293f87cb1879d8d1b20ebad9d7d3d5c3e399a291ce86a3b4d30e4e32368a9020000000453005165ffffffff26d84ae93eb58c81158c9b3c3cbc24a84614d731094f38d0eea8686dec02824d0300000005636a65abacf02c784001a0bd5d03000000000900655351ab65ac516a416ef503", "", 1, -295106477, "b79f31c289e95d9dadec48ebf88e27c1d920661e50d090e422957f90ff94cb6e");
        run_test_sighash("9200e26b03ff36bc4bf908143de5f97d4d02358db642bd5a8541e6ff709c420d1482d471b70000000008abab65536a636553ffffffff61ba6d15f5453b5079fb494af4c48de713a0c3e7f6454d7450074a2a80cb6d880300000007ac6a00ab5165515dfb7574fbce822892c2acb5d978188b1d65f969e4fe874b08db4c791d176113272a5cc10100000000ffffffff0420958d000000000009ac63516a0063516353dd885505000000000465ac00007b79e901000000000066d8bf010000000005525252006a00000000", "ac5152", 0, 2089531339, "89ec7fab7cfe7d8d7d96956613c49dc48bf295269cfb4ea44f7333d88c170e62");
        run_test_sighash("45f335ba01ce2073a8b0273884eb5b48f56df474fc3dff310d9706a8ac7202cf5ac188272103000000025363ffffffff049d859502000000000365ab6a8e98b1030000000002ac51f3a80603000000000752535151ac00000306e30300000000020051b58b2b3a", "", 0, 1899564574, "78e01310a228f645c23a2ad0acbb8d91cedff4ecdf7ca997662c6031eb702b11");
        run_test_sighash("94083c840288d40a6983faca876d452f7c52a07de9268ad892e70a81e150d602a773c175ad03000000007ec3637d7e1103e2e7e0c61896cbbf8d7e205b2ecc93dd0d6d7527d39cdbf6d335789f660300000000ffffffff019e1f7b03000000000800ac0051acac0053539cb363", "", 1, -183614058, "a17b66d6bb427f42653d08207a22b02353dd19ccf2c7de6a9a3a2bdb7c49c9e7");
        run_test_sighash("30e0d4d20493d0cd0e640b757c9c47a823120e012b3b64c9c1890f9a087ae4f2001ca22a61010000000152f8f05468303b8fcfaad1fb60534a08fe90daa79bff51675472528ebe1438b6f60e7f60c10100000009526aab6551ac510053ffffffffaaab73957ea2133e32329795221ed44548a0d3a54d1cf9c96827e7cffd1706df0200000009ab00526a005265526affffffffd19a6fe54352015bf170119742821696f64083b5f14fb5c7d1b5a721a3d7786801000000085265abababac53abffffffff020f39bd030000000004ab6aac52049f6c050000000004ab52516aba5b4c60", "6a6365516a6a655253", 0, -624256405, "8e221a6c4bf81ca0d8a0464562674dcd14a76a32a4b7baf99450dd9195d411e6");
        run_test_sighash("5c0ac112032d6885b7a9071d3c5f493aa16c610a4a57228b2491258c38de8302014276e8be030000000300ab6a17468315215262ad5c7393bb5e0c5a6429fd1911f78f6f72dafbbbb78f3149a5073e24740300000003ac5100ffffffff33c7a14a062bdea1be3c9c8e973f54ade53fe4a69dcb5ab019df5f3345050be00100000008ac63655163526aab428defc0033ec36203000000000765516365536a00ae55b2000000000002ab53f4c0080400000000095265516a536563536a00000000", "6a005151006a", 2, 272749594, "91082410630337a5d89ff19145097090f25d4a20bdd657b4b953927b2f62c73b");
        run_test_sighash("e3683329026720010b08d4bec0faa244f159ae10aa582252dd0f3f80046a4e145207d54d31000000000852acac52656aacac3aaf2a5017438ad6adfa3f9d05f53ebed9ceb1b10d809d507bcf75e0604254a8259fc29c020000000653526552ab51f926e52c04b44918030000000000f7679c0100000000090000525152005365539e3f48050000000009516500ab635363ab008396c905000000000253650591024f", "6a6365", 0, 908746924, "458aec3b5089a585b6bad9f99fd37a2b443dc5a2eefac2b7e8c5b06705efc9db");
        run_test_sighash("00b20fd104dd59705b84d67441019fa26c4c3dec5fd3b50eca1aa549e750ef9ddb774dcabe000000000651ac656aac65ffffffff52d4246f2db568fc9eea143e4d260c698a319f0d0670f84c9c83341204fde48b0200000000ffffffffb8aeabb85d3bcbc67b132f1fd815b451ea12dcf7fc169c1bc2e2cf433eb6777a03000000086a51ac6aab6563acd510d209f413da2cf036a31b0def1e4dcd8115abf2e511afbcccb5ddf41d9702f28c52900100000006ac52ab6a0065ffffffff039c8276000000000008ab53655200656a52401561010000000003acab0082b7160100000000035100ab00000000", "535265", 1, -947367579, "3212c6d6dd8d9d3b2ac959dec11f4638ccde9be6ed5d36955769294e23343da0");
        run_test_sighash("624d28cb02c8747915e9af2b13c79b417eb34d2fa2a73547897770ace08c6dd9de528848d3030000000651ab63abab533c69d3f9b75b6ef8ed2df50c2210fd0bf4e889c42477d58682f711cbaece1a626194bb85030000000765acab53ac5353ffffffff018cc280040000000009abacabac52636352ac6859409e", "ac51ac", 1, 1005144875, "919144aada50db8675b7f9a6849c9d263b86450570293a03c245bd1e3095e292");
        run_test_sighash("8f28471d02f7d41b2e70e9b4c804f2d90d23fb24d53426fa746bcdcfffea864925bdeabe3e0200000001acffffffff76d1d35d04db0e64d65810c808fe40168f8d1f2143902a1cc551034fd193be0e0000000001acffffffff048a5565000000000005005151516afafb610400000000045263ac53648bb30500000000086363516a6a5165513245de01000000000000000000", "6a0053510053", 1, -1525137460, "305fc8ff5dc04ebd9b6448b03c9a3d945a11567206c8d5214666b30ec6d0d6cc");
        run_test_sighash("10ec50d7046b8b40e4222a3c6449490ebe41513aad2eca7848284a08f3069f3352c2a9954f0000000009526aac656352acac53ffffffff0d979f236155aa972472d43ee6f8ce22a2d052c740f10b59211454ff22cb7fd00200000007acacacab63ab53ffffffffbbf97ebde8969b35725b2e240092a986a2cbfd58de48c4475fe077bdd493a20c010000000663ab5365ababffffffff4600722d33b8dba300d3ad037bcfc6038b1db8abfe8008a15a1de2da2264007302000000035351ac6dbdafaf020d0ccf04000000000663ab6a51ab6ae06e5e0200000000036aabab00000000", "", 0, -1658960232, "2420dd722e229eccafae8508e7b8d75c6920bfdb3b5bac7cb8e23419480637c2");
        run_test_sighash("43559290038f32fda86580dd8a4bc4422db88dd22a626b8bd4f10f1c9dd325c8dc49bf479f01000000026351ffffffff401339530e1ed3ffe996578a17c3ec9d6fccb0723dd63e7b3f39e2c44b976b7b0300000006ab6a65656a51ffffffff6fb9ba041c96b886482009f56c09c22e7b0d33091f2ac5418d05708951816ce7000000000551ac525100ffffffff020921e40500000000035365533986f40500000000016a00000000", "52ac51", 0, 1769771809, "02040283ef2291d8e1f79bb71bdabe7c1546c40d7ed615c375643000a8b9600d");
        run_test_sighash("35b6fc06047ebad04783a5167ab5fc9878a00c4eb5e7d70ef297c33d5abd5137a2dea9912402000000036aacacffffffff21dc291763419a584bdb3ed4f6f8c60b218aaa5b99784e4ba8acfec04993e50c03000000046a00ac6affffffff69e04d77e4b662a82db71a68dd72ef0af48ca5bebdcb40f5edf0caf591bb41020200000000b5db78a16d93f5f24d7d932f93a29bb4b784febd0cbb1943f90216dc80bba15a0567684b000000000853ab52ab5100006a1be2208a02f6bdc103000000000265ab8550ea04000000000365636a00000000", "", 0, -1114114836, "1c8655969b241e717b841526f87e6bd68b2329905ba3fc9e9f72526c0b3ea20c");
        run_test_sighash("f35befbc03faf8c25cc4bc0b92f6239f477e663b44b83065c9cb7cf231243032cf367ce3130000000005ab65526a517c4c334149a9c9edc39e29276a4b3ffbbab337de7908ea6f88af331228bd90086a6900ba020000000151279d19950d2fe81979b72ce3a33c6d82ebb92f9a2e164b6471ac857f3bbd3c0ea213b542010000000953ab51635363520065052657c20300a9ba04000000000452636a6a0516ea020000000008535253656365ababcfdd3f01000000000865ac516aac00530000000000", "", 2, -99793521, "c834a5485e68dc13edb6c79948784712122440d7fa5bbaa5cd2fc3d4dac8185d");
        run_test_sighash("d3da18520216601acf885414538ce2fb4d910997eeb91582cac42eb6982c9381589587794f0300000000fffffffff1b1c9880356852e10cf41c02e928748dd8fae2e988be4e1c4cb32d0bfaea6f7000000000465ab6aabffffffff02fb0d69050000000002ababeda8580500000000085163526565ac52522b913c95", "ac", 1, -1247973017, "99b32b5679d91e0f9cdd6737afeb07459806e5acd7630c6a3b9ab5d550d0c003");
        run_test_sighash("8218eb740229c695c252e3630fc6257c42624f974bc856b7af8208df643a6c520ef681bfd00000000002510066f30f270a09b2b420e274c14d07430008e7886ec621ba45665057120afce58befca96010300000004525153ab84c380a9015d96100000000000076a5300acac526500000000", "ac005263", 0, -1855679695, "5071f8acf96aea41c7518bd1b5b6bbe16258b529df0c03f9e374b83c66b742c6");
        run_test_sighash("1123e7010240310013c74e5def60d8e14dd67aedff5a57d07a24abc84d933483431b8cf8ea0300000003530051fc6775ff1a23c627a2e605dd2560e84e27f4208300071e90f4589e762ad9c9fe8d0da95e020000000465655200ffffffff04251598030000000004ab65ab639d28d90400000000096563636aacac525153474df801000000000851525165ac51006a75e23b040000000000e5bd3a4a", "6363636565", 0, -467124448, "9cb0dd04e9fe287b112e94a1647590d27e8b164ca13c4fe70c610fd13f82c2fd");
        run_test_sighash("3b937e05032b8895d2f4945cb7e3679be2fbd15311e2414f4184706dbfc0558cf7de7b4d000000000001638b91a12668a3c3ce349788c961c26aa893c862f1e630f18d80e7843686b6e1e6fc396310000000000852635353ab65ac51eeb09dd1c9605391258ee6f74b9ae17b5e8c2ef010dc721c5433dcdc6e93a1593e3b6d1700000000085365ac6553526351ffffffff0308b18e04000000000253acb6dd00040000000008536aac5153ac516ab0a88201000000000500ac006500804e3ff2", "", 0, 416167343, "595a3c02254564634e8085283ec4ea7c23808da97ce9c5da7aecd7b553e7fd7f");
        run_test_sighash("a48f27ca047997470da74c8ee086ddad82f36d9c22e790bd6f8603ee6e27ad4d3174ea875403000000095153ac636aab6aacabffffffffefc936294e468d2c9a99e09909ba599978a8c0891ad47dc00ba424761627cef202000000056a51630053ffffffff304cae7ed2d3dbb4f2fbd679da442aed06221ffda9aee460a28ceec5a9399f4e0200000000f5bddf82c9c25fc29c5729274c1ff0b43934303e5f595ce86316fc66ad263b96ca46ab8d0100000003536500d7cf226b0146b00c04000000000200ac5c2014ce", "515100636563", 0, 1991799059, "9c051a7092fe17fa62b1720bc2c4cb2ffc1527d9fb0b006d2e142bb8fe07bf3c");
        run_test_sighash("180cd53101c5074cf0b7f089d139e837fe49932791f73fa2342bd823c6df6a2f72fe6dba1303000000076a6a63ac53acabffffffff03853bc1020000000007ac526a6a6a6a003c4a8903000000000453515163a0fbbd030000000005ab656a5253253d64cf", "ac65", 0, -1548453970, "4d8efb3b99b9064d2f6be33b194a903ffabb9d0e7baa97a48fcec038072aac06");
        run_test_sighash("c21ec8b60376c47e057f2c71caa90269888d0ffd5c46a471649144a920d0b409e56f190b700000000008acac6a526a536365ffffffff5d315d9da8bf643a9ba11299450b1f87272e6030fdb0c8adc04e6c1bfc87de9a0000000000ea43a9a142e5830c96b0ce827663af36b23b0277244658f8f606e95384574b91750b8e940000000007516a63ac0063acffffffff023c61be0400000000055165ab5263313cc8020000000006006a53526551ed8c3d56", "6a", 1, 1160627414, "a638cc17fd91f4b1e77877e8d82448c84b2a4e100df1373f779de7ad32695112");
        run_test_sighash("b8fd394001ed255f49ad491fecc990b7f38688e9c837ccbc7714ddbbf5404f42524e68c18f0000000007ab6353535363ab081e15ee02706f7d050000000008515200535351526364c7ec040000000005636a53acac9206cbe1", "655352ac", 0, -1251578838, "8e0697d8cd8a9ccea837fd798cc6c5ed29f6fbd1892ee9bcb6c944772778af19");
        run_test_sighash("e42a76740264677829e30ed610864160c7f97232c16528fe5610fc08814b21c34eefcea69d010000000653006a6a0052ffffffff647046cf44f217d040e6a8ff3f295312ab4dd5a0df231c66968ad1c6d8f4428000000000025352ffffffff0199a7f900000000000000000000", "655263006a005163", 1, 1122505713, "7cda43f1ff9191c646c56a4e29b1a8c6cb3f7b331da6883ef2f0480a515d0861");
        run_test_sighash("a2dfa4690214c1ab25331815a5128f143219de51a47abdc7ce2d367e683eeb93960a31af9f010000000363636affffffff8be0628abb1861b078fcc19c236bc4cc726fa49068b88ad170adb2a97862e7460200000004ac655363ffffffff0441f11103000000000153dbab0c000000000009ab53ac5365526aab63abbb95050000000004ab52516a29a029040000000003ac526a00000000", "6a52ac63", 1, -1302210567, "913060c7454e6c80f5ba3835454b54db2188e37dc4ce72a16b37d11a430b3d23");
        run_test_sighash("9dbc591f04521670af83fb3bb591c5d4da99206f5d38e020289f7db95414390dddbbeb56680100000004ac5100acffffffffb6a40b5e29d5e459f8e72d39f800089529f0889006cad3d734011991da8ef09d0100000009526a5100acab536a515fc427436df97cc51dc8497642ffc868857ee245314d28b356bd70adba671bd6071301fc0000000000ffffffff487efde2f620566a9b017b2e6e6d42525e4070f73a602f85c6dfd58304518db30000000005516353006a8d8090180244904a0200000000046a65656ab1e9c203000000000451ab63aba06a5449", "", 0, -1414953913, "bae189eb3d64aedbc28a6c28f6c0ccbd58472caaf0cf45a5aabae3e031dd1fea");
        run_test_sighash("1345fb2c04bb21a35ae33a3f9f295bece34650308a9d8984a989dfe4c977790b0c21ff9a7f0000000006ac52ac6a0053ffffffff7baee9e8717d81d375a43b691e91579be53875350dfe23ba0058ea950029fcb7020000000753ab53ab63ab52ffffffff684b6b3828dfb4c8a92043b49b8cb15dd3a7c98b978da1d314dce5b9570dadd202000000086353ab6a5200ac63d1a8647bf667ceb2eae7ec75569ca249fbfd5d1b582acfbd7e1fcf5886121fca699c011d0100000003ac006affffffff049b1eb00300000000001e46dc0100000000080065ab6a6a630065ca95b40300000000030051520c8499010000000006ab6aac526a6500000000", "53526aac636300", 2, 1809978100, "cfeaa36790bc398783d4ca45e6354e1ea52ee74e005df7f9ebd10a680e9607bf");
        run_test_sighash("7d75dc8f011e5f9f7313ba6aedef8dbe10d0a471aca88bbfc0c4a448ce424a2c5580cda1560300000003ab5152ffffffff01997f8e0200000000096552ac6a65656563530d93bbcc", "00656a6563", 0, 1414485913, "ec91eda1149f75bffb97612569a78855498c5d5386d473752a2c81454f297fa7");
        run_test_sighash("1459179504b69f01c066e8ade5e124c748ae5652566b34ed673eea38568c483a5a4c4836ca0100000008ac5352006563656affffffff5d4e037880ab1975ce95ea378d2874dcd49d5e01e1cdbfae3343a01f383fa35800000000095251ac52ac6aac6500ffffffff7de3ae7d97373b7f2aeb4c55137b5e947b2d5fb325e892530cb589bc4f92abd503000000086563ac53ab520052ffffffffb4db36a32d6e543ef49f4bafde46053cb85b2a6c4f0e19fa0860d9083901a1190300000003ab51531bbcfe5504a6dbda040000000008536a5365abac6500d660c80300000000096565abab6a53536a6a54e84e010000000003acac52df2ccf0500000000025351220c857e", "", 2, 1879181631, "3aad18a209fab8db44954eb55fd3cc7689b5ec9c77373a4d5f4dae8f7ae58d14");
        run_test_sighash("cabb1b06045a895e6dcfc0c1e971e94130c46feace286759f69a16d298c8b0f6fd0afef8f20300000004ac006352ffffffffa299f5edac903072bfb7d29b663c1dd1345c2a33546a508ba5cf17aab911234602000000056a65515365ffffffff89a20dc2ee0524b361231092a070ace03343b162e7162479c96b757739c8394a0300000002abab92ec524daf73fabee63f95c1b79fa8b84e92d0e8bac57295e1d0adc55dc7af5534ebea410200000001534d70e79b04674f6f00000000000600abacab53517d60cc0200000000035265ab96c51d040000000004ac6300ac62a787050000000008006a516563ab63639e2e7ff7", "6551ac6351ac", 3, 1942663262, "d0c4a780e4e0bc22e2f231e23f01c9d536b09f6e5be51c123d218e906ec518be");
        run_test_sighash("8b96d7a30132f6005b5bd33ea82aa325e2bcb441f46f63b5fca159ac7094499f380f6b7e2e00000000076aacabac6300acffffffff0158056700000000000465005100c319e6d0", "52006a", 0, -1100733473, "fb4bd26a91b5cf225dd3f170eb09bad0eac314bc1e74503cc2a3f376833f183e");
        run_test_sighash("112191b7013cfbe18a175eaf09af7a43cbac2c396f3695bbe050e1e5f4250603056d60910e02000000001c8a5bba03738a22010000000005525352656a77a149010000000002510003b52302000000000351ac52722be8e6", "65ac6565", 0, -1847972737, "8e795aeef18f510d117dfa2b9f4a2bd2e2847a343205276cedd2ba14548fd63f");
        run_test_sighash("ce6e1a9e04b4c746318424705ea69517e5e0343357d131ad55d071562d0b6ebfedafd6cb840100000003656553ffffffff67bd2fa78e2f52d9f8900c58b84c27ef9d7679f67a0a6f78645ce61b883fb8de000000000100d699a56b9861d99be2838e8504884af4d30b909b1911639dd0c5ad47c557a0773155d4d303000000046a5151abffffffff9fdb84b77c326921a8266854f7bbd5a71305b54385e747fe41af8a397e78b7fa010000000863acac6a51ab00ac0d2e9b9d049b8173010000000007ac53526a650063ba9b7e010000000008526a00525263acac0ab3fd030000000000ea8a0303000000000200aca61a97b9", "", 1, -1276952681, "b6ed4a3721be3c3c7305a5128c9d418efa58e419580cec0d83f133a93e3a22c5");
        run_test_sighash("2f7353dd02e395b0a4d16da0f7472db618857cd3de5b9e2789232952a9b154d249102245fd030000000151617fd88f103280b85b0a198198e438e7cab1a4c92ba58409709997cc7a65a619eb9eec3c0200000003636aabffffffff0397481c0200000000045300636a0dc97803000000000009d389030000000003ac6a53134007bb", "0000536552526a", 0, -1912746174, "30c4cd4bd6b291f7e9489cc4b4440a083f93a7664ea1f93e77a9597dab8ded9c");
        run_test_sighash("89e7928c04363cb520eff4465251fd8e41550cbd0d2cdf18c456a0be3d634382abcfd4a2130200000006ac516a6a656355042a796061ed72db52ae47d1607b1ceef6ca6aea3b7eea48e7e02429f382b378c4e51901000000085351ab6352ab5252ffffffff53631cbda79b40183000d6ede011c778f70147dc6fa1aed3395d4ce9f7a8e69701000000096a6553ab52516a52abad0de418d80afe059aab5da73237e0beb60af4ac490c3394c12d66665d1bac13bdf29aa8000000000153f2b59ab6027a33eb040000000007005351ac5100ac88b941030000000003ab0052e1e8a143", "63656a", 0, 1258533326, "b575a04b0bb56e38bbf26e1a396a76b99fb09db01527651673a073a75f0a7a34");
        run_test_sighash("ca356e2004bea08ec2dd2df203dc275765dc3f6073f55c46513a588a7abcc4cbde2ff011c7020000000553525100003aefec4860ef5d6c1c6be93e13bd2d2a40c6fb7361694136a7620b020ecbaca9413bcd2a030000000965ac00536352535100ace4289e00e97caaea741f2b89c1143060011a1f93090dc230bee3f05e34fbd8d8b6c399010000000365526affffffff48fc444238bda7a757cb6a98cb89fb44338829d3e24e46a60a36d4e24ba05d9002000000026a53ffffffff03d70b440200000000056a6a526aac853c97010000000002515335552202000000000351635300000000", "0052", 3, -528192467, "fc93cc056c70d5e033933d730965f36ad81ef64f1762e57f0bc5506c5b507e24");
        run_test_sighash("82d4fa65017958d53e562fac073df233ab154bd0cf6e5a18f57f4badea8200b217975e31030200000004636aab51ac0891a204227cc9050000000006635200655365bfef8802000000000865650051635252acfc2d09050000000006ab65ac51516380195e030000000007ac52525352510063d50572", "53", 0, -713567171, "e095003ca82af89738c1863f0f5488ec56a96fb81ea7df334f9344fcb1d0cf40");
        run_test_sighash("75f6949503e0e47dd70426ef32002d6cdb564a45abedc1575425a18a8828bf385fa8e808e600000000036aabab82f9fd14e9647d7a1b5284e6c55169c8bd228a7ea335987cef0195841e83da45ec28aa2e0300000002516350dc6fe239d150efdb1b51aa288fe85f9b9f741c72956c11d9dcd176889963d699abd63f0000000001ab429a63f502777d20010000000007abac52ac516a53d081d9020000000003acac630c3cc3a8", "535152516551510000", 1, 973814968, "c6ec1b7cb5c16a1bfd8a3790db227d2acc836300534564252b57bd66acf95092");
        run_test_sighash("e86a24bc03e4fae784cdf81b24d120348cb5e52d937cd9055402fdba7e43281e482e77a1c100000000046363006affffffffa5447e9bdcdab22bd20d88b19795d4c8fb263fbbf7ce8f4f9a85f865953a6325020000000663ac53535253ffffffff9f8b693bc84e0101fc73748e0513a8cecdc264270d8a4ee1a1b6717607ee1eaa00000000026a513417bf980158d82c020000000009005253005351acac5200000000", "6353516365536a6a", 2, -563792735, "508129278ef07b43112ac32faf00170ad38a500eed97615a860fd58baaad174b");
        run_test_sighash("536bc5e60232eb60954587667d6bcdd19a49048d67a027383cc0c2a29a48b960dc38c5a0370300000005ac636300abffffffff8f1cfc102f39b1c9348a2195d496e602c77d9f57e0769dabde7eaaedf9c69e250100000006acabab6a6351ffffffff0432f56f0400000000046a5365517fd54b0400000000035265539484e4050000000003536a5376dc25020000000008ac536aab6aab536ab978e686", "ac0051006a006a006a", 0, -273074082, "f151f1ec305f698d9fdce18ea292b145a58d931f1518cf2a4c83484d9a429638");
        run_test_sighash("fab796ee03f737f07669160d1f1c8bf0800041157e3ac7961fea33a293f976d79ce49c02ab0200000003ac5252eb097ea1a6d1a7ae9dace338505ba559e579a1ee98a2e9ad96f30696d6337adcda5a85f403000000096500abab656a6a656396d5d41a9b11f571d91e4242ddc0cf2420eca796ad4882ef1251e84e42b930398ec69dd80100000005526551ac6a8e5d0de804f763bb0400000000015288271a010000000001acf2bf2905000000000300ab51c9641500000000000952655363636365ac5100000000", "00ac536552", 0, -1854521113, "f3bbab70b759fe6cfae1bf349ce10716dbc64f6e9b32916904be4386eb461f1f");
        run_test_sighash("f2b539a401e4e8402869d5e1502dbc3156dbce93583f516a4947b333260d5af1a34810c6a00200000003525363ffffffff01d305e2000000000005acab535200a265fe77", "", 0, -1435650456, "41617b27321a830c712638dbb156dae23d4ef181c7a06728ccbf3153ec53d7dd");
        run_test_sighash("9f10b1d8033aee81ac04d84ceee0c03416a784d1017a2af8f8a34d2f56b767aea28ff88c8f02000000025352ffffffff748cb29843bea8e9c44ed5ff258df1faf55fbb9146870b8d76454786c4549de100000000016a5ba089417305424d05112c0ca445bc7107339083e7da15e430050d578f034ec0c589223b0200000007abac53ac6565abffffffff025a4ecd010000000006636563ab65ab40d2700000000000056a6553526333fa296c", "", 0, -395044364, "20fd0eee5b5716d6cbc0ddf852614b686e7a1534693570809f6719b6fcb0a626");
        run_test_sighash("ff2ecc09041b4cf5abb7b760e910b775268abee2792c7f21cc5301dd3fecc1b4233ee70a2c0200000009acac5300006a51526affffffffeb39c195a5426afff38379fc85369771e4933587218ef4968f3f05c51d6b7c92000000000165453a5f039b8dbef7c1ffdc70ac383b481f72f99f52b0b3a5903c825c45cfa5d2c0642cd50200000001654b5038e6c49daea8c0a9ac8611cfe904fc206dad03a41fb4e5b1d6d85b1ecad73ecd4c0102000000096a51000053ab656565bdb5548302cc719200000000000452655265214a3603000000000300ab6a00000000", "52516a006a63", 1, -2113289251, "37ed6fae36fcb3360c69cac8b359daa62230fc1419b2cf992a32d8f3e079dcff");
        run_test_sighash("70a8577804e553e462a859375957db68cfdf724d68caeacf08995e80d7fa93db7ebc04519d02000000045352ab53619f4f2a428109c5fcf9fee634a2ab92f4a09dc01a5015e8ecb3fc0d9279c4a77fb27e900000000006ab6a51006a6affffffff3ed1a0a0d03f25c5e8d279bb5d931b7eb7e99c8203306a6c310db113419a69ad010000000565516300abffffffff6bf668d4ff5005ef73a1b0c51f32e8235e67ab31fe019bf131e1382050b39a630000000004536a6563ffffffff02faf0bb00000000000163cf2b4b05000000000752ac635363acac15ab369f", "ac", 0, -1175809030, "1c9d6816c20865849078f9777544b5ddf37c8620fe7bd1618e4b72fb72dddca1");
        run_test_sighash("a3604e5304caa5a6ba3c257c20b45dcd468f2c732a8ca59016e77b6476ac741ce8b16ca8360200000004acac6553ffffffff695e7006495517e0b79bd4770f955040610e74d35f01e41c9932ab8ccfa3b55d0300000007ac5253515365acffffffff6153120efc5d73cd959d72566fc829a4eb00b3ef1a5bd3559677fb5aae116e38000000000400abab52c29e7abd06ff98372a3a06227386609adc7665a602e511cadcb06377cc6ac0b8f63d4fdb03000000055100acabacffffffff04209073050000000009ab5163ac525253ab6514462e05000000000952abacab636300656a20672c0400000000025153b276990000000000056565ab6a5300000000", "5351", 0, 1460890590, "249c4513a49076c6618aabf736dfd5ae2172be4311844a62cf313950b4ba94be");
        run_test_sighash("c6a72ed403313b7d027f6864e705ec6b5fa52eb99169f8ea7cd884f5cdb830a150cebade870100000009ac63ab516565ab6a51ffffffff398d5838735ff43c390ca418593dbe43f3445ba69394a6d665b5dc3b4769b5d700000000075265acab515365ffffffff7ee5616a1ee105fd18189806a477300e2a9cf836bf8035464e8192a0d785eea3030000000700ac6a51516a52ffffffff018075fd0000000000015100000000", "005251acac5252", 2, -656067295, "2cc1c7514fdc512fd45ca7ba4f7be8a9fe6d3318328bc1a61ae6e7675047e654");
        run_test_sighash("93c12cc30270fc4370c960665b8f774e07942a627c83e58e860e38bd6b0aa2cb7a2c1e060901000000036300abffffffff4d9b618035f9175f564837f733a2b108c0f462f28818093372eec070d9f0a5440300000001acffffffff039c2137020000000001525500990100000000055265ab636a07980e0300000000005ba0e9d1", "656a5100", 1, 18954182, "6beca0e0388f824ca33bf3589087a3c8ad0857f9fe7b7609ae3704bef0eb83e2");
        run_test_sighash("97bddc63015f1767619d56598ad0eb5c7e9f880b24a928fea1e040e95429c930c1dc653bdb0100000008ac53acac00005152aaa94eb90235ed10040000000000287bdd0400000000016a8077673a", "acac6a536352655252", 0, -813649781, "5990b139451847343c9bb89cdba0e6daee6850b60e5b7ea505b04efba15f5d92");
        run_test_sighash("cc3c9dd303637839fb727270261d8e9ddb8a21b7f6cbdcf07015ba1e5cf01dc3c3a327745d0300000000d2d7804fe20a9fca9659a0e49f258800304580499e8753046276062f69dbbde85d17cd2201000000096352536a520000acabffffffffbc75dfa9b5f81f3552e4143e08f485dfb97ae6187330e6cd6752de6c21bdfd21030000000600ab53650063ffffffff0313d0140400000000096565515253526aacac167f0a040000000008acab00535263536a9a52f8030000000006abab5151ab63f75b66f2", "6a635353636a65ac65", 1, 377286607, "dbc7935d718328d23d73f8a6dc4f53a267b8d4d9816d0091f33823bd1f0233e9");
        run_test_sighash("236f91b702b8ffea3b890700b6f91af713480769dda5a085ae219c8737ebae90ff25915a3203000000056300ac6300811a6a10230f12c9faa28dae5be2ebe93f37c06a79e76214feba49bb017fb25305ff84eb020000000100ffffffff041e351703000000000351ac004ff53e050000000003ab53636c1460010000000000cb55f701000000000651520051ab0000000000", "acac636a6aac5300", 0, 406448919, "793a3d3c37f6494fab79ff10c16702de002f63e34be25dd8561f424b0ea938c4");
        run_test_sighash("c47d5ad60485cb2f7a825587b95ea665a593769191382852f3514a486d7a7a11d220b62c54000000000663655253acab8c3cf32b0285b040e50dcf6987ddf7c385b3665048ad2f9317b9e0c5ba0405d8fde4129b00000000095251ab00ac65635300ffffffff549fe963ee410d6435bb2ed3042a7c294d0c7382a83edefba8582a2064af3265000000000152fffffffff7737a85e0e94c2d19cd1cde47328ece04b3e33cd60f24a8a345da7f2a96a6d0000000000865ab6a0051656aab28ff30d5049613ea020000000005ac51000063f06df1050000000008ac63516aabac5153afef5901000000000700656500655253688bc00000000000086aab5352526a53521ff1d5ff", "51ac52", 2, -1296011911, "0c1fd44476ff28bf603ad4f306e8b6c7f0135a441dc3194a6f227cb54598642a");
        run_test_sighash("0b43f122032f182366541e7ee18562eb5f39bc7a8e5e0d3c398f7e306e551cdef773941918030000000863006351ac51acabffffffffae586660c8ff43355b685dfa8676a370799865fbc4b641c5a962f0849a13d8250100000005abab63acabffffffff0b2b6b800d8e77807cf130de6286b237717957658443674df047a2ab18e413860100000008ab6aac655200ab63ffffffff04f1dbca03000000000800635253ab656a52a6eefd0300000000036365655d8ca90200000000005a0d530400000000015300000000", "65ac65acac", 0, 351448685, "86f26e23822afd1bdfc9fff92840fc1e60089f12f54439e3ab9e5167d0361dcf");
        run_test_sighash("af1c4ab301ec462f76ee69ba419b1b2557b7ded639f3442a3522d4f9170b2d6859765c3df402000000016affffffff01a5ca6c000000000008ab52536aab00005300000000", "6a6351", 0, 110304602, "e88ed2eea9143f2517b15c03db00767eb01a5ce12193b99b964a35700607e5f4");
        run_test_sighash("0bfd34210451c92cdfa02125a62ba365448e11ff1db3fb8bc84f1c7e5615da40233a8cd368010000000252ac9a070cd88dec5cf9aed1eab10d19529720e12c52d3a21b92c6fdb589d056908e43ea910e0200000009ac516a52656a6a5165ffffffffc3edcca8d2f61f34a5296c405c5f6bc58276416c720c956ff277f1fb81541ddd00000000030063abffffffff811247905cdfc973d179c03014c01e37d44e78f087233444dfdce1d1389d97c302000000065163000063ab1724a26e02ca37c902000000000851ab53525352ac529012a90100000000085200525253535353fa32575b", "5352ac6351", 1, -1087700448, "b8f1e1f35e3e1368bd17008c756e59cced216b3c699bcd7bebdb5b6c8eec4697");
        run_test_sighash("467a3e7602e6d1a7a531106791845ec3908a29b833598e41f610ef83d02a7da3a1900bf2960000000005ab6a636353ffffffff031db6dac6f0bafafe723b9199420217ad2c94221b6880654f2b35114f44b1df010000000965ab52636a63ac6352ffffffff02b3b95c0100000000026300703216030000000001ab3261c0aa", "6a", 0, 2110869267, "3078b1d1a7713c6d101c64afe35adfae0977a5ab4c7e07a0b170b041258adbf2");
        run_test_sighash("8713bc4f01b411149d575ebae575f5dd7e456198d61d238695df459dd9b86c4e3b2734b62e0300000004abac6363ffffffff03b58049050000000002ac653c714c04000000000953656a005151526a527b5a9e03000000000652ac5100525300000000", "52", 0, -647281251, "0e0bed1bf2ff255aef6e5c587f879ae0be6222ab33bd75ee365ec6fbb8acbe38");
        run_test_sighash("b5a7df6102107beded33ae7f1dec0531d4829dff7477260925aa2cba54119b7a07d92d5a1d02000000046a516a52803b625c334c1d2107a326538a3db92c6c6ae3f7c3516cd90a09b619ec6f58d10e77bd6703000000056563006a63ffffffff0117484b03000000000853acab52526a65abc1b548a1", "ac006a525100", 0, 2074359913, "680336db57347d8183b8898cd27a83f1ba5884155aeae5ce20b4840b75e12871");
        run_test_sighash("49eb2178020a04fca08612c34959fd41447319c190fb7ffed9f71c235aa77bec28703aa1820200000003ac6353abaff326071f07ec6b77fb651af06e8e8bd171068ec96b52ed584de1d71437fed186aecf0300000001acffffffff03da3dbe02000000000652ac63ac6aab8f3b680400000000096a536a65636a53516a5175470100000000016500000000", "6a536365", 0, 1283691249, "c670219a93234929f662ecb9aa148a85a2d281e83f4e53d10509461cdea47979");
        run_test_sighash("0f96cea9019b4b3233c0485d5b1bad770c246fe8d4a58fb24c3b7dfdb3b0fd90ea4e8e947f0300000006006a5163515303571e1e01906956030000000005ab635353abadc0fbbe", "acac", 0, -1491469027, "716a8180e417228f769dcb49e0491e3fda63badf3d5ea0ceeac7970d483dd7e2");
        run_test_sighash("148e68480196eb52529af8e83e14127cbfdbd4a174e60a86ac2d86eac9665f46f4447cf7aa01000000045200ac538f8f871401cf240c0300000000065252ab52656a5266cf61", "", 0, -344314825, "eacc47c5a53734d6ae3aedbc6a7c0a75a1565310851b29ef0342dc4745ceb607");
        run_test_sighash("6c7913f902aa3f5f939dd1615114ce961beda7c1e0dd195be36a2f0d9d047c28ac62738c3a020000000453abac00ffffffff477bf2c5b5c6733881447ac1ecaff3a6f80d7016eee3513f382ad7f554015b970100000007ab6563acab5152ffffffff04e58fe1040000000009ab00526aabab526553e59790010000000002ab525a834b03000000000035fdaf0200000000086551ac65515200ab00000000", "63ac53", 1, 1285478169, "1536da582a0b6de017862445e91ba14181bd6bf953f4de2f46b040d351a747c9");
        run_test_sighash("3320f6730132f830c4681d0cae542188e4177cad5d526fae84565c60ceb5c0118e844f90bd030000000163ffffffff0257ec5a040000000005525251ac6538344d000000000002515200000000", "5352656a53ac516a65", 0, 788050308, "3afacaca0ef6be9d39e71d7b1b118994f99e4ea5973c9107ca687d28d8eba485");
        run_test_sighash("c13aa4b702eedd7cde09d0416e649a890d40e675aa9b5b6d6912686e20e9b9e10dbd40abb1000000000863ab6353515351ac11d24dc4cc22ded7cdbc13edd3f87bd4b226eda3e4408853a57bcd1becf2df2a1671fd1600000000045165516affffffff01baea300100000000076aab52ab53005300000000", "0065", 0, -1195908377, "241a23e7b1982d5f78917ed97a8678087acbbffe7f624b81df78a5fe5e41e754");
        run_test_sighash("a2692fff03b2387f5bacd5640c86ba7df574a0ee9ed7f66f22c73cccaef3907eae791cbd230200000004536363abffffffff4d9fe7e5b375de88ba48925d9b2005447a69ea2e00495a96eafb2f144ad475b40000000008000053000052636537259bee3cedd3dcc07c8f423739690c590dc195274a7d398fa196af37f3e9b4a1413f810000000006ac63acac52abffffffff04c65fe60200000000075151536365ab657236fc020000000009005263ab00656a6a5195b8b6030000000007ac5165636aac6a7d7b66010000000002acab00000000", "51", 2, -826546582, "925037c7dc7625f3f12dc83904755a37016560de8e1cdd153c88270a7201cf15");
        run_test_sighash("2c5b003201b88654ac2d02ff6762446cb5a4af77586f05e65ee5d54680cea13291efcf930d0100000005ab536a006a37423d2504100367000000000004536a515335149800000000000152166aeb03000000000452510063226c8e03000000000000000000", "635251", 0, 1060344799, "7e058ca5dd07640e4aae7dea731cfb7d7fef1bfd0d6d7b6ce109d041f4ca2a31");
        run_test_sighash("f981b9e104acb93b9a7e2375080f3ea0e7a94ce54cd8fb25c57992fa8042bdf4378572859f0100000002630008604febba7e4837da77084d5d1b81965e0ea0deb6d61278b6be8627b0d9a2ecd7aeb06a0300000005ac5353536a42af3ef15ce7a2cd60482fc0d191c4236e66b4b48c9018d7dbe4db820f5925aad0e8b52a0300000008ab0063510052516301863715efc8608bf69c0343f18fb81a8b0c720898a3563eca8fe630736c0440a179129d03000000086aac6a52ac6a63ac44fec4c00408320a03000000000062c21c030000000007ac6a655263006553835f0100000000015303cd60000000000005535263536558b596e0", "00", 0, -2140385880, "49870a961263354c9baf108c6979b28261f99b374e97605baa532d9fa3848797");
        run_test_sighash("c4b702e502f1a54f235224f0e6de961d2e53b506ab45b9a40805d1dacd35148f0acf24ca5e00000000085200ac65ac53acabf34ba6099135658460de9d9b433b84a8562032723635baf21ca1db561dce1c13a06f4407000000000851ac006a63516aabffffffff02a853a603000000000163d17a67030000000005ab63006a5200000000", "ac5363515153", 1, 480734903, "5c46f7ac3d6460af0da28468fcc5b3c87f2b9093d0f837954b7c8174b4d7b6e7");
        run_test_sighash("9b83f78704f492b9b353a3faad8d93f688e885030c274856e4037818848b99e490afef27770200000000ffffffff36b60675a5888c0ef4d9e11744ecd90d9fe9e6d8abb4cff5666c898fdce98d9e00000000056aab656352596370fca7a7c139752971e169a1af3e67d7656fc4fc7fd3b98408e607c2f2c836c9f27c030000000653ac51ab6300a0761de7e158947f401b3595b7dc0fe7b75fa9c833d13f1af57b9206e4012de0c41b8124030000000953656a53ab53510052242e5f5601bf83b301000000000465516a6300000000", "63515200ac656365", 3, -150879312, "9cf05990421ea853782e4a2c67118e03434629e7d52ab3f1d55c37cf7d72cdc4");
        run_test_sighash("f492a9da04f80b679708c01224f68203d5ea2668b1f442ebba16b1aa4301d2fe5b4e2568f3010000000953005351525263ab65ffffffff93b34c3f37d4a66df255b514419105b56d7d60c24bf395415eda3d3d8aa5cd0101000000020065ffffffff9dba34dabdc4f1643b372b6b77fdf2b482b33ed425914bb4b1a61e4fad33cf390000000002ab52ffffffffbbf3dc82f397ef3ee902c5146c8a80d9a1344fa6e38b7abce0f157be7adaefae0000000009515351005365006a51ffffffff021359ba010000000000403fea0200000000095200ac6353abac635300000000", "00ac51acacac", 0, -2115078404, "fd44fc98639ca32c927929196fc3f3594578f4c4bd248156a25c04a65bf3a9f3");
        run_test_sighash("2f73e0b304f154d3a00fde2fdd40e791295e28d6cb76af9c0fd8547acf3771a02e3a92ba37030000000852ac6351ab6565639aa95467b065cec61b6e7dc4d6192b5536a7c569315fb43f470078b31ed22a55dab8265f02000000080065636a6aab6a53ffffffff9e3addbff52b2aaf9fe49c67017395198a9b71f0aa668c5cb354d06c295a691a0100000000ffffffff45c2b4019abaf05c5e484df982a4a07459204d1343a6ee5badade358141f8f990300000007ac516a6aacac6308655cd601f3bc2f0000000000015200000000", "", 0, -2082053939, "9a95e692e1f78efd3e46bb98f178a1e3a0ef60bd0301d9f064c0e5703dc879c2");
        run_test_sighash("5a60b9b503553f3c099f775db56af3456330f1e44e67355c4ab290d22764b9144a7b5f959003000000030052acbd63e0564decc8659aa53868be48c1bfcda0a8c9857b0db32a217bc8b46d9e7323fe9649020000000553ac6551abd0ecf806211db989bead96c09c7f3ec5f73c1411d3329d47d12f9e46678f09bac0dc383e0200000000ffffffff01494bb202000000000500516551ac00000000", "ac", 0, 1169947809, "62a36c6e8da037202fa8aeae03e533665376d5a4e0a854fc4624a75ec52e4eb1");
        run_test_sighash("e3649aa40405e6ffe377dbb1bbbb672a40d8424c430fa6512c6165273a2b9b6afa9949ec430200000007630052ab655153a365f62f2792fa90c784efe3f0981134d72aac0b1e1578097132c7f0406671457c332b84020000000353ab6ad780f40cf51be22bb4ff755434779c7f1def4999e4f289d2bd23d142f36b66fbe5cfbb4b01000000076a5252abac52ab1430ffdc67127c9c0fc97dcd4b578dab64f4fb9550d2b59d599773962077a563e8b6732c02000000016affffffff04cb2687000000000002ab636e320904000000000252acf70e9401000000000100dc3393050000000006ab0063536aacbc231765", "65520053", 3, -2016196547, "f64f805f0ff7f237359fa6b0e58085f3c766d1859003332223444fd29144112a");
        run_test_sighash("4c4be7540344050e3044f0f1d628039a334a7c1f7b4573469cfea46101d6888bb6161fe9710200000000ffffffffac85a4fdad641d8e28523f78cf5b0f4dc74e6c5d903c10b358dd13a5a1fd8a06000000000163e0ae75d05616b72467b691dc207fe2e65ea35e2eadb7e06ea442b2adb9715f212c0924f10200000000ffffffff0194ddfe02000000000265ac00000000", "00006500", 1, -479922562, "d66924d49f03a6960d3ca479f3415d638c45889ce9ab05e25b65ac260b51d634");
        run_test_sighash("202c18eb012bc0a987e69e205aea63f0f0c089f96dd8f0e9fcde199f2f37892b1d4e6da90302000000055352ac6565ffffffff0257e5450100000000025300ad257203000000000000000000", "520052ac6a005265", 0, 168054797, "502967a6f999f7ee25610a443caf8653dda288e6d644a77537bcc115a8a29894");
        run_test_sighash("32fa0b0804e6ea101e137665a041cc2350b794e59bf42d9b09088b01cde806ec1bbea077df0200000008515153650000006506a11c55904258fa418e57b88b12724b81153260d3f4c9f080439789a391ab147aabb0fa0000000007000052ac51ab510986f2a15c0d5e05d20dc876dd2dafa435276d53da7b47c393f20900e55f163b97ce0b800000000008ab526a520065636a8087df7d4d9c985fb42308fb09dce704650719140aa6050e8955fa5d2ea46b464a333f870000000009636300636a6565006affffffff01994a0d040000000002536500000000", "516563530065", 2, -163068286, "f58637277d2bc42e18358dc55f7e87e7043f5e33f4ce1fc974e715ef0d3d1c2a");
        run_test_sighash("ae23424d040cd884ebfb9a815d8f17176980ab8015285e03fdde899449f4ae71e04275e9a80100000007ab006553530053ffffffff018e06db6af519dadc5280c07791c0fd33251500955e43fe4ac747a4df5c54df020000000251ac330e977c0fec6149a1768e0d312fdb53ed9953a3737d7b5d06aad4d86e9970346a4feeb5030000000951ab51ac6563ab526a67cabc431ee3d8111224d5ecdbb7d717aa8fe82ce4a63842c9bd1aa848f111910e5ae1eb0100000004ac515300bfb7e0d7048acddc030000000009636a5253636a655363a3428e040000000001525b99c6050000000004655265ab717e6e020000000000d99011eb", "ac6a6a516565", 1, -716251549, "b098eb9aff1bbd375c70a0cbb9497882ab51f3abfebbf4e1f8d74c0739dc7717");
        run_test_sighash("030f44fc01b4a9267335a95677bd190c1c12655e64df74addc53b753641259af1a54146baa020000000152e004b56c04ba11780300000000026a53f125f001000000000251acd2cc7c03000000000763536563655363c9b9e50500000000015200000000", "ac", 0, -1351818298, "19dd32190ed2a37be22f0224a9b55b91e37290577c6c346d36d32774db0219a3");
        run_test_sighash("c05f448f02817740b30652c5681a3b128322f9dc97d166bd4402d39c37c0b14506d8adb5890300000003536353ffffffffa188b430357055ba291c648f951cd2f9b28a2e76353bef391b71a889ba68d5fc02000000056565526a6affffffff02745f73010000000001ab3ec34c0400000000036aac5200000000", "516551510053", 0, -267877178, "3a1c6742d4c374f061b1ebe330b1e169a113a19792a1fdde979b53e094cc4a3c");
        run_test_sighash("163ba45703dd8c2c5a1c1f8b806afdc710a2a8fc40c0138e2d83e329e0e02a9b6c837ff6b8000000000700655151ab6a522b48b8f134eb1a7e6f5a6fa319ce9d11b36327ba427b7d65ead3b4a6a69f85cda8bbcd22030000000563656552acffffffffdbcf4955232bd11eef0cc6954f3f6279675b2956b9bcc24f08c360894027a60201000000066500006500abffffffff04d0ce9d0200000000008380650000000000015233f360040000000003006aabedcf0801000000000000000000", "000065006500ac", 0, 216965323, "9afe3f4978df6a86e9a8ebd62ef6a9d48a2203f02629349f1864ef2b8b92fd55");
        run_test_sighash("fe647f950311bf8f3a4d90afd7517df306e04a344d2b2a2fea368935faf11fa6882505890d0000000005ab5100516affffffff43c140947d9778718919c49c0535667fc6cc727f5876851cb8f7b6460710c7f60100000000ffffffffce4aa5d90d7ab93cbec2e9626a435afcf2a68dd693c15b0e1ece81a9fcbe025e0300000000ffffffff02f34806020000000002515262e54403000000000965635151ac655363636de5ce24", "6a005100ac516351", 2, 989643518, "818a7ceaf963f52b5c48a7f01681ac6653c26b63a9f491856f090d9d60f2ffe3");
        run_test_sighash("cef7316804c3e77fe67fc6207a1ea6ae6eb06b3bf1b3a4010a45ae5c7ad677bb8a4ebd16d90200000009ac536a5152ac5263005301ab8a0da2b3e0654d31a30264f9356ba1851c820a403be2948d35cafc7f9fe67a06960300000006526a63636a53ffffffffbada0d85465199fa4232c6e4222df790470c5b7afd54704595a48eedd7a4916b030000000865ab63ac006a006ab28dba4ad55e58b5375053f78b8cdf4879f723ea4068aed3dd4138766cb4d80aab0aff3d0300000003ac6a00ffffffff010f5dd6010000000006ab006aab51ab00000000", "", 1, 889284257, "d0f32a6db43378af84b063a6706d614e2d647031cf066997c48c04de3b493a94");
        run_test_sighash("7b3ff28004ba3c7590ed6e36f45453ebb3f16636fe716acb2418bb2963df596a50ed954d2e03000000065251515265abffffffff706ee16e32e22179400c9841013971645dabf63a3a6d2d5feb42f83aa468983e030000000653ac51ac5152ffffffffa03a16e5e5de65dfa848b9a64ee8bf8656cc1f96b06a15d35bd5f3d32629876e020000000043c1a3965448b3b46f0f0689f1368f3b2981208a368ec5c30defb35595ef9cf95ffd10e902000000036aac65253a5bbe042e907204000000000800006565656352634203b4020000000002656336b3b7010000000001ab7a063f0100000000026500a233cb76", "006551636a53ac5251", 1, -1144216171, "68c7bd717b399b1ee33a6562a916825a2fed3019cdf4920418bb72ffd7403c8c");
        run_test_sighash("1be8ee5604a9937ebecffc832155d9ba7860d0ca451eaced58ca3688945a31d93420c27c460100000006abac5300535288b65458af2f17cbbf7c5fbcdcfb334ffd84c1510d5500dc7d25a43c36679b702e850f7c0200000003005300ffffffff7c237281cb859653eb5bb0a66dbb7aeb2ac11d99ba9ed0f12c766a8ae2a2157203000000086aabac526365acabfffffffff09d3d6639849f442a6a52ad10a5d0e4cb1f4a6b22a98a8f442f60280c9e5be80200000007ab00ab6565ab52ffffffff0398fe83030000000005526aababacbdd6ec010000000005535252ab6a82c1e6040000000001652b71c40c", "6563526353656351", 2, -853634888, "0d936cceda2f56c7bb87d90a7b508f6208577014ff280910a710580357df25f3");
        run_test_sighash("9e0f99c504fbca858c209c6d9371ddd78985be1ab52845db0720af9ae5e2664d352f5037d4010000000552ac53636affffffff0e0ce866bc3f5b0a49748f597c18fa47a2483b8a94cef1d7295d9a5d36d31ae7030000000663515263ac635bb5d1698325164cdd3f7f3f7831635a3588f26d47cc30bf0fefd56cd87dc4e84f162ab702000000036a6365ffffffff85c2b1a61de4bcbd1d5332d5f59f338dd5e8accbc466fd860f96eef1f54c28ec030000000165ffffffff04f5cabd010000000007000052ac526563c18f1502000000000465510051dc9157050000000008655363ac525253ac506bb600000000000865656a53ab63006a00000000", "006a6a0052", 0, 1186324483, "2f9b7348600336512686e7271c53015d1cb096ab1a5e0bce49acd35bceb42bc8");
        run_test_sighash("11ce51f90164b4b54b9278f0337d95c50d16f6828fcb641df9c7a041a2b274aa70b1250f2b0000000008ab6a6a65006551524c9fe7f604af44be050000000005525365006521f79a0300000000015306bb4e04000000000265ac99611a05000000000765acab656500006dc866d0", "", 0, -1710478768, "cfa4b7573559b3b199478880c8013fa713ca81ca8754a3fd68a6d7ee6147dc5a");
        run_test_sighash("86bc233e02ba3c647e356558e7252481a7769491fb46e883dd547a4ce9898fc9a1ca1b77790000000006ab5351abab51f0c1d09c37696d5c7c257788f5dff5583f4700687bcb7d4acfb48521dc953659e325fa390300000003acac5280f29523027225af03000000000963abac0065ab65acab7e59d90400000000016549dac846", "53006aac52acac", 0, 711159875, "880330ccde00991503ea598a6dfd81135c6cda9d317820352781417f89134d85");
        run_test_sighash("beac155d03a853bf18cd5c490bb2a245b3b2a501a3ce5967945b0bf388fec2ba9f04c03d68030000000012fe96283aec4d3aafed8f888b0f1534bd903f9cd1af86a7e64006a2fa0d2d30711af770010000000163ffffffffd963a19d19a292104b9021c535d3e302925543fb3b5ed39fb2124ee23a9db00302000000056500ac63acffffffff01ad67f503000000000300ac5189f78db2", "53536a636500", 2, 748992863, "bde3dd0575164d7ece3b5783ce0783ffddb7df98f178fe6468683230314f285a");
        run_test_sighash("489ebbf10478e260ba88c0168bd7509a651b36aaee983e400c7063da39c93bf28100011f280100000004abab63ab2fc856f05f59b257a4445253e0d91b6dffe32302d520ac8e7f6f2467f7f6b4b65f2f59e903000000096353abacab6351656affffffff0122d9480db6c45a2c6fd68b7bc57246edffbf6330c39ccd36aa3aa45ec108fc030000000265ab9a7e78a69aadd6b030b12602dff0739bbc346b466c7c0129b34f50ae1f61e634e11e9f3d0000000006516a53525100ffffffff011271070000000000086563ab6353536352c4dd0e2c", "", 0, -293358504, "4eba3055bc2b58765593ec6e11775cea4b6493d8f785e28d01e2d5470ea71575");
        run_test_sighash("6911195d04f449e8eade3bc49fd09b6fb4b7b7ec86529918b8593a9f6c34c2f2d301ec378b000000000263ab49162266af054643505b572c24ff6f8e4c920e601b23b3c42095881857d00caf56b28acd030000000565525200ac3ac4d24cb59ee8cfec0950312dcdcc14d1b360ab343e834004a5628d629642422f3c5acc02000000035100accf99b663e3c74787aba1272129a34130668a877cc6516bfb7574af9fa6d07f9b4197303400000000085351ab5152635252ffffffff042b3c95000000000000ff92330200000000046a5252ab884a2402000000000853530065520063000d78be03000000000953abab52ab53ac65aba72cb34b", "6a", 2, -637739405, "6b80d74eb0e7ee59d14f06f30ba7d72a48d3a8ff2d68d3b99e770dec23e9284f");
        run_test_sighash("746347cf03faa548f4c0b9d2bd96504d2e780292730f690bf0475b188493fb67ca58dcca4f0000000002005336e3521bfb94c254058e852a32fc4cf50d99f9cc7215f7c632b251922104f638aa0b9d080100000008656aac5351635251ffffffff4da22a678bb5bb3ad1a29f97f6f7e5b5de11bb80bcf2f7bb96b67b9f1ac44d09030000000365ababffffffff036f02b30000000000076353ab6aac63ac50b72a050000000002acaba8abf804000000000663006a6a6353797eb999", "acac5100", 1, -1484493812, "164c32a263f357e385bd744619b91c3f9e3ce6c256d6a827d6defcbdff38fa75");
        run_test_sighash("e17149010239dd33f847bf1f57896db60e955117d8cf013e7553fae6baa9acd3d0f1412ad90200000006516500516500cb7b32a8a67d58dddfb6ceb5897e75ef1c1ff812d8cd73875856487826dec4a4e2d2422a0100000004ac525365196dbb69039229270400000000070000535351636a8b7596020000000006ab51ac52655131e99d040000000003516551ee437f5c", "ac656a53", 1, 1102662601, "8858bb47a042243f369f27d9ab4a9cd6216adeac1c1ac413ed0890e46f23d3f3");
        run_test_sighash("144971940223597a2d1dec49c7d4ec557e4f4bd207428618bafa3c96c411752d494249e1fb0100000004526a5151ffffffff340a545b1080d4f7e2225ff1c9831f283a7d4ca4d3d0a29d12e07d86d6826f7f0200000003006553ffffffff03c36965000000000000dfa9af00000000000451636aac7f7d140300000000016300000000", "", 1, -108117779, "c84fcaf9d779df736a26cc3cabd04d0e61150d4d5472dd5358d6626e610be57f");
        run_test_sighash("2aee6b9a02172a8288e02fac654520c9dd9ab93cf514d73163701f4788b4caeeb9297d2e250300000004ab6363008fb36695528d7482710ea2926412f877a3b20acae31e9d3091406bfa6b62ebf9d9d2a6470100000009535165536a63520065ffffffff03f7b560050000000003acab6a9a8338050000000000206ce90000000000056552516a5100000000", "5252", 1, -1102319963, "fa4676c374ae3a417124b4c970d1ed3319dc3ac91fb36efca1aa9ed981a8aa1b");
        run_test_sighash("9554595203ad5d687f34474685425c1919e3d2cd05cf2dac89d5f33cd3963e5bb43f8706480100000000ffffffff9de2539c2fe3000d59afbd376cb46cefa8bd01dbc43938ff6089b63d68acdc2b02000000096553655251536a6500fffffffff9695e4016cd4dfeb5f7dadf00968e6a409ef048f81922cec231efed4ac78f5d010000000763abab6a5365006caaf0070162cc640200000000045163ab5100000000", "", 0, -1105256289, "e8e10ed162b1a43bfd23bd06b74a6c2f138b8dc1ab094ffb2fa11d5b22869bee");
        run_test_sighash("04f51f2a0484cba53d63de1cb0efdcb222999cdf2dd9d19b3542a896ca96e23a643dfc45f00200000007acac53510063002b091fd0bfc0cfb386edf7b9e694f1927d7a3cf4e1d2ce937c1e01610313729ef6419ae7030000000165a3372a913c59b8b3da458335dc1714805c0db98992fd0d93f16a7f28c55dc747fe66a5b503000000095351ab65ab52536351ffffffff5650b318b3e236802a4e41ed9bc0a19c32b7aa3f9b2cda1178f84499963a0cde000000000165ffffffff0383954f04000000000553ac536363a8fc90030000000000a2e315000000000005acab00ab5100000000", "0053", 2, -1424653648, "a5bc0356f56b2b41a2314ec05bee7b91ef57f1074bcd2efc4da442222269d1a3");
        run_test_sighash("53dc1a88046531c7b57a35f4d9adf101d068bf8d63fbbedaf4741dba8bc5e92c8725def571030000000453655251fcdf116a226b3ec240739c4c7493800e4edfe67275234e371a227721eac43d3d9ecaf1b50300000003ac0052ffffffff2c9279ffeea4718d167e9499bd067600715c14484e373ef93ae4a31d2f5671ab0000000009516553ac636a6a65001977752eeba95a8f16b88c571a459c2f2a204e23d48cc7090e4f4cc35846ca7fc0a455ce00000000055165ac0063188143f80205972902000000000765ac63ac516353c7b6a50000000000036a510000000000", "655351536a", 0, 103806788, "b276584d3514e5b4e058167c41dc02915b9d97f6795936a51f40e894ed8508bc");
        run_test_sighash("5a06cb4602dcfc85f49b8d14513f33c48f67146f2ee44959bbca092788e6823b2719f3160b0200000001ab3c013f2518035b9ea635f9a1c74ec1a3fb7496a160f46aae2e09bfc5cd5111a0f20969e003000000015158c89ab7049f20d6010000000008ac6a52abac53515349765e00000000000300ab638292630100000000045351ab0086da09010000000006656a6365525300000000", "526a63", 1, 1502936586, "bdfaff8a4e775379c5dc26e024968efa805f923de53fa8272dd53ec582afa0c5");
        run_test_sighash("ca9d84fa0129011e1bf27d7cb71819650b59fb292b053d625c6f02b0339249b498ff7fd4b601000000025352ffffffff032173a0040000000008525253abab5152639473bb030000000009005153526a53535151d085bd0000000000086a5365ab5165655300000000", "005152ac51", 0, 580353445, "c629d93b02037f40aa110e46d903edb34107f64806aa0c418d435926feef68b8");
        run_test_sighash("e3cdbfb4014d90ae6a4401e85f7ac717adc2c035858bf6ff48979dd399d155bce1f150daea0300000002ac51a67a0d39017f6c71040000000005535200535200000000", "", 0, -1899950911, "c1c7df8206e661d593f6455db1d61a364a249407f88e99ecad05346e495b38d7");
        run_test_sighash("b2b6b9ab0283d9d73eeae3d847f41439cd88279c166aa805e44f8243adeb3b09e584efb1df00000000026300ffffffff7dfe653bd67ca094f8dab51007c6adaced09de2af745e175b9714ca1f5c68d050000000003ac6500aa8e596903fd3f3204000000000553ac6a6a533a2e210500000000075253acabab526392d0ee020000000008520065635200ab5200000000", "65acacac65005365", 0, 28298553, "39c2aaa2496212b3ab120ab7d7f37c5e852bfe38d20f5226413a2268663eeae8");
        run_test_sighash("4314339e01de40faabcb1b970245a7f19eedbc17c507dac86cf986c2973715035cf95736ae0200000007abababababab65bde67b900151510b04000000000853ac00655200535300000000", "52", 0, 399070095, "47585dc25469d04ff3a60939d0a03779e3e81a411bf0ca18b91bb925ebd30718");
        run_test_sighash("f063171b03e1830fdc1d685a30a377537363ccafdc68b42bf2e3acb908dac61ee24b37595c020000000765ac5100ab6aacf447bc8e037b89d6cadd62d960cc442d5ced901d188867b5122b42a862929ce45e7b628d010000000253aba009a1ba42b00f1490b0b857052820976c675f335491cda838fb7934d5eea0257684a2a202000000001e83cf2401a7f777030000000008ab6553526a53526a00000000", "", 2, 1984790332, "c19caada8e71535e29a86fa29cfd9b74a0c7412003fc722a121005e461e01636");
        run_test_sighash("cf7bdc250249e22cbe23baf6b648328d31773ea0e771b3b76a48b4748d7fbd390e88a004d30000000003ac536a4ab8cce0e097136c90b2037f231b7fde2063017facd40ed4e5896da7ad00e9c71dd70ae600000000096a0063516352525365ffffffff01b71e3e00000000000300536a00000000", "", 1, 546970113, "6a815ba155270af102322c882f26d22da11c5330a751f520807936b320b9af5d");
        run_test_sighash("ac7a125a0269d35f5dbdab9948c48674616e7507413cd10e1acebeaf85b369cd8c88301b7c030000000963656aac6a530053abffffffffed94c39a582e1a46ce4c6bffda2ccdb16cda485f3a0d94b06206066da12aecfe010000000752abab63536363ef71dcfb02ee07fa0400000000016a6908c802000000000751656a6551abac688c2c2d", "6a6351526551", 0, 858400684, "552ff97d7924f51cda6d1b94be53483153ef725cc0a3a107adbef220c753f9a6");
        run_test_sighash("3a1f454a03a4591e46cf1f7605a3a130b631bf4dfd81bd2443dc4fac1e0a224e74112884fe0000000005516aac6a53a87e78b55548601ffc941f91d75eab263aa79cd498c88c37fdf275a64feff89fc1710efe03000000016a39d7ef6f2a52c00378b4f8f8301853b61c54792c0f1c4e2cd18a08cb97a7668caa008d970200000002656affffffff017642b20100000000096a63535253abac6a6528271998", "51", 2, 1459585400, "e9a7f21fc2d38be7be47095fbc8f1bf8923660aa4d71df6d797ae0ba5ca4d5b0");
        run_test_sighash("6269e0fa0173e76e89657ca495913f1b86af5b8f1c1586bcd6c960aede9bc759718dfd5044000000000352ac530e2c7bd90219849b000000000007ab00ab6a53006319f281000000000007ab00515165ac5200000000", "6a", 0, -2039568300, "62094f98234a05bf1b9c7078c5275ed085656856fb5bdfd1b48090e86b53dd85");
        run_test_sighash("eb2bc00604815b9ced1c604960d54beea4a3a74b5c0035d4a8b6bfec5d0c9108f143c0e99a0000000000ffffffff22645b6e8da5f11d90e5130fd0a0df8cf79829b2647957471d881c2372c527d8010000000263acffffffff1179dbaf17404109f706ae27ad7ba61e860346f63f0c81cb235d2b05d14f2c1003000000025300264cb23aaffdc4d6fa8ec0bb94eff3a2e50a83418a8e9473a16aaa4ef8b855625ed77ef40100000003ac51acf8414ad404dd328901000000000652526500006ab6261c000000000002526a72a4c9020000000006ac526500656586d2e7000000000006656aac00ac5279cd8908", "51", 1, -399279379, "d37532e7b2b8e7db5c7c534197600397ebcc15a750e3af07a3e2d2e4f84b024f");
        run_test_sighash("eba8b0de04ac276293c272d0d3636e81400b1aaa60db5f11561480592f99e6f6fa13ad387002000000070053acab536563bebb23d66fd17d98271b182019864a90e60a54f5a615e40b643a54f8408fa8512cfac927030000000963ac6a6aabac65ababffffffff890a72192bc01255058314f376bab1dc72b5fea104c154a15d6faee75dfa5dba020000000100592b3559b0085387ac7575c05b29b1f35d9a2c26a0c27903cc0f43e7e6e37d5a60d8305a030000000252abffffffff0126518f05000000000000000000", "005300635252635351", 1, 664344756, "26dc2cba4bd5334e5c0b3a520b44cc1640c6b923d10e576062f1197171724097");
        run_test_sighash("185cda1a01ecf7a8a8c28466725b60431545fc7a3367ab68e34d486e8ea85ee3128e0d8384000000000465ac63abec88b7bb031c56eb04000000000965636a51005252006a7c78d5040000000007acac63abac51ac3024a40500000000086300526a51abac51464c0e8c", "0065535265515352", 0, 1594558917, "b5280b9610c0625a65b36a8c2402a95019a7bbb9dd3de77f7c3cb1d82c3263ba");
        run_test_sighash("92c9fb780138abc472e589d5b59489303f234acc838ca66ffcdf0164517a8679bb622a4267020000000153468e373d04de03fa020000000009ac006a5265ab5163006af649050000000007515153006a00658ceb59030000000001ac36afa0020000000009ab53006351ab51000000000000", "6a", 0, 2059357502, "e2358dfb51831ee81d7b0bc602a65287d6cd2dbfacf55106e2bf597e22a4b573");
        run_test_sighash("6f62138301436f33a00b84a26a0457ccbfc0f82403288b9cbae39986b34357cb2ff9b889b302000000045253655335a7ff6701bac9960400000000086552ab656352635200000000", "6aac51", 0, 1444414211, "502a2435fd02898d2ff3ab08a3c19078414b32ec9b73d64a944834efc9dae10c");
        run_test_sighash("9981143a040a88c2484ac3abe053849e72d04862120f424f373753161997dd40505dcb4783030000000700536365536565a2e10da3f4b1c1ad049d97b33f0ae0ea48c5d7c30cc8810e144ad93be97789706a5ead180100000003636a00ffffffffbdcbac84c4bcc87f03d0ad83fbe13b369d7e42ddb3aecf40870a37e814ad8bb5010000000963536a5100636a53abffffffff883609905a80e34202101544f69b58a0b4576fb7391e12a769f890eef90ffb72020000000651656352526affffffff04243660000000000004ab5352534a9ce001000000000863656363ab6a53652df19d030000000003ac65acedc51700000000000000000000", "ac6300acac", 2, 293672388, "7ba99b289c04718a7283f150d831175ed6303081e191a0608ea81f78926c5bdf");
        run_test_sighash("49f7d0b6037bba276e910ad3cd74966c7b3bc197ffbcfefd6108d6587006947e97789835ea0300000008526a52006a650053ffffffff8d7b6c07cd10f4c4010eac7946f61aff7fb5f3920bdf3467e939e58a1d4100ab03000000076aac63ac535351ffffffff8f48c3ba2d52ad67fbcdc90d8778f3c8a3894e3c35b9730562d7176b81af23c80100000003ab5265ffffffff0301e3ef0300000000046a525353e899ac0500000000075153ab6a65abac259bea0400000000007b739972", "53516aacac6aac", 1, 955403557, "5d366a7f4346ae18aeb7c9fc4dab5af71173184aa20ed22fcb4ea8511ad25449");
        run_test_sighash("58a4fed801fbd8d92db9dfcb2e26b6ff10b120204243fee954d7dcb3b4b9b53380e7bb8fb60100000003006351ffffffff02a0795b050000000006536351ac6aac2718d00200000000075151acabac515354d21ba1", "005363515351", 0, -1322430665, "bbee941bbad950424bf40e3623457db47f60ed29deaa43c99dec702317cb3326");
        run_test_sighash("17fad0d303da0d764fedf9f2887a91ea625331b28704940f41e39adf3903d8e75683ef6d46020000000151ffffffffff376eea4e880bcf0f03d33999104aafed2b3daf4907950bb06496af6b51720a020000000900636a63525253525196521684f3b08497bad2c660b00b43a6a517edc58217876eb5e478aa3b5fda0f29ee1bea00000000046aacab6affffffff03dde8e2050000000007ac5365ac51516a14772e000000000005630000abacbbb360010000000006ab5251ab656a50f180f0", "0053", 0, -1043701251, "a3bdf8771c8990971bff9b4e7d59b7829b067ed0b8d3ac1ec203429811384668");
        run_test_sighash("5a2257df03554550b774e677f348939b37f8e765a212e566ce6b60b4ea8fed4c9504b7f7d1000000000653655265ab5258b67bb931df15b041177cf9599b0604160b79e30f3d7a594e7826bae2c29700f6d8f8f40300000005515300ac6a159cf8808a41f504eb5c2e0e8a9279f3801a5b5d7bc6a70515fbf1c5edc875bb4c9ffac500000000050063510052ffffffff0422a90105000000000965006a650000516a006417d2020000000006526363ab00524d969d0100000000035153acc4f077040000000005ac5200636500000000", "6a52", 1, -1482463464, "37b794b05d0687c9b93d5917ab068f6b2f0e38406ff04e7154d104fc1fb14cdc");
        run_test_sighash("e0032ad601269154b3fa72d3888a3151da0aed32fb2e1a15b3ae7bee57c3ddcffff76a1321010000000100110d93ae03f5bd080100000000075263516a6551002871e60100000000046a005252eaa753040000000004ab6aab526e325c71", "630052", 0, -1857873018, "ea117348e94de86381bb8ad1c7f93b8c623f0272104341701bb54e6cb433596c");
        run_test_sighash("014b2a5304d46764817aca180dca50f5ab25f2e0d5749f21bb74a2f8bf6b8b7b3fa8189cb7030000000965ac5165ab6a51ac6360ecd91e8abc7e700a4c36c1a708a494c94bb20cbe695c408543146566ab22be43beae9103000000045163ab00ffffffffffa48066012829629a9ec06ccd4905a05df0e2b745b966f6a269c9c8e13451fc00000000026565ffffffffc40ccadc21e65fe8a4b1e072f4994738ccaf4881ae6fede2a2844d7da4d199ab02000000065152ab536aabffffffff01b6e054030000000004515352ab3e063432", "", 0, 1056459916, "a7aff48f3b8aeb7a4bfe2e6017c80a84168487a69b69e46681e0d0d8e63a84b6");
        run_test_sighash("1201ab5d04f89f07c0077abd009762e59db4bb0d86048383ba9e1dad2c9c2ad96ef660e6d00200000007ab6a65ac5200652466fa5143ab13d55886b6cdc3d0f226f47ec1c3020c1c6e32602cd3428aceab544ef43e00000000086a6a6a526a6a5263ffffffffd5be0b0be13ab75001243749c839d779716f46687e2e9978bd6c9e2fe457ee48020000000365abab1e1bac0f72005cf638f71a3df2e3bbc0fa35bf00f32d9c7dc9c39a5e8909f7d53170c8ae0200000008ab6a51516363516affffffff02f0a6210500000000036300ac867356010000000009acab65ac6353536a659356d367", "ac53535252", 0, 917543338, "418acc156c2bc76a5d7baa58db29f1b4cf6c266c9222ed167ef5b4d47f0e0f41");
        run_test_sighash("344fa11e01c19c4dd232c77742f0dd0aeb3695f18f76da627628741d0ee362b0ea1fb3a2180200000007635151005100529bab25af01937c1f0500000000055153ab53656e7630af", "6351005163ac51", 0, -629732125, "228ca52a0a376fe0527a61cfa8da6d7baf87486bba92d49dfd3899cac8a1034f");
        run_test_sighash("b2fda1950191358a2b855f5626a0ebc830ab625bea7480f09f9cd3b388102e35c0f303124c030000000565ac65ab53ffffffff03f9c5ec04000000000765ab51516551650e2b9f0500000000045365525284e8f6040000000001ac00000000", "ac51655253", 0, 1433027632, "d2fa7e13c34cecda5105156bd2424c9b84ee0a07162642b0706f83243ff811a8");
        run_test_sighash("a4a6bbd201aa5d882957ac94f2c74d4747ae32d69fdc765add4acc2b68abd1bdb8ee333d6e0300000008516a6552515152abffffffff02c353cb040000000007ac6351ab51536588bd320500000000066552525253ac00000000", "", 0, 1702060459, "499da7d74032388f820645191ac3c8d20f9dba8e8ded7fa3a5401ea2942392a1");
        run_test_sighash("83a583d204d926f2ee587a83dd526cf1e25a44bb668e45370798f91a2907d184f7cddcbbc7030000000700ab6565536a539f71d3776300dffdfa0cdd1c3784c9a1f773e34041ca400193612341a9c42df64e3f550e01000000050052515251ffffffff52dab2034ab0648553a1bb8fc4e924b2c89ed97c18dfc8a63e248b454035564b01000000015139ab54708c7d4d2c2886290f08a5221cf69592a810fd1979d7b63d35c271961e710424fd0300000005ac65ac5251ffffffff01168f7c030000000000a85e5fb0", "6a536353656a00", 0, 179595345, "5350a31ac954a0b49931239d0ecafbf34d035a537fd0c545816b8fdc355e9961");
        run_test_sighash("ffd35d51042f290108fcb6ea49a560ba0a6560f9181da7453a55dfdbdfe672dc800b39e7320200000006630065516a65f2166db2e3827f44457e86dddfd27a8af3a19074e216348daa0204717d61825f198ec0030100000006ab51abab00abffffffffdf41807adb7dff7db9f14d95fd6dc4e65f8402c002d009a3f1ddedf6f4895fc8030000000500ab006a65a5a848345052f860620abd5fcd074195548ce3bd0839fa9ad8642ed80627bf43a0d47dbd010000000765ab006a656a53b38cdd6502a186da05000000000765ab00ab006a53527c0e0100000000085365ab51acacac52534bd1b1", "6a635253ac0000", 0, 1095082149, "3c05473a816621a3613f0e903faa1a1e44891dd40862b029e41fc520776350fa");
        run_test_sighash("6c9a4b98013c8f1cae1b1df9f0f2de518d0c50206a0ab871603ac682155504c0e0ce946f460100000000ffffffff04e9266305000000000753535100ac6aacded39e04000000000365ac6ab93ccd010000000002515397bf3d050000000003ab636300000000", "63520052ac656353", 0, -352633155, "936eff8cdfd771be24124da87c7b24feb48da7cbc2c25fb5ba13d1a23255d902");
        run_test_sighash("c4b80f850323022205b3e1582f1ed097911a81be593471a8dce93d5c3a7bded92ef6c7c1260100000002006affffffff70294d62f37c3da7c5eae5d67dce6e1b28fedd7316d03f4f48e1829f78a88ae801000000096a5200530000516351f6b7b544f7c39189d3a2106ca58ce4130605328ce7795204be592a90acd81bef517d6f170200000000ffffffff012ab8080000000000075100006365006335454c1e", "53ac6a536aacac", 0, -1124103895, "06277201504e6bf8b8c94136fad81b6e3dadacb9d4a2c21a8e10017bfa929e0e");
        run_test_sighash("8ab69ed50351b47b6e04ac05e12320984a63801716739ed7a940b3429c9c9fed44d3398ad40300000006536a516a52638171ef3a46a2adb8025a4884b453889bc457d63499971307a7e834b0e76eec69c943038a0300000000ffffffff566bb96f94904ed8d43d9d44a4a6301073cef2c011bf5a12a89bedbaa03e4724030000000265acb606affd01edea38050000000008515252516aacac6300000000", "65000000006365ac53", 0, -1338942849, "7912573937824058103cb921a59a7f910a854bf2682f4116a393a2045045a8c3");
        run_test_sighash("2484991e047f1cf3cfe38eab071f915fe86ebd45d111463b315217bf9481daf0e0d10902a402000000006e71a424eb1347ffa638363604c0d5eccbc90447ff371e000bf52fc743ec832851bb564a0100000001abffffffffef7d014fad3ae7927948edbbb3afe247c1bcbe7c4c8f5d6cf97c799696412612020000000851536a5353006a001dfee0d7a0dd46ada63b925709e141863f7338f34f7aebde85d39268ae21b77c3068c01d0000000008535151ab00636563ffffffff018478070200000000095200635365ac52ab5341b08cd3", "", 3, 265623923, "24cb420a53b4f8bb477f7cbb293caabfd2fc47cc400ce37dbbab07f92d3a9575");
        run_test_sighash("54839ef9026f65db30fc9cfcb71f5f84d7bb3c48731ab9d63351a1b3c7bc1e7da22bbd508e0300000000442ad138f170e446d427d1f64040016032f36d8325c3b2f7a4078766bdd8fb106e52e8d20000000003656500ffffffff02219aa101000000000851ababac52ab00659646bd02000000000552acacabac24c394a5", "ac", 0, 906807497, "69264faadcd1a581f7000570a239a0a26b82f2ad40374c5b9c1f58730514de96");
        run_test_sighash("5036d7080434eb4eef93efda86b9131b0b4c6a0c421e1e5feb099a28ff9dd8477728639f77030000000951516aab535152ab5391429be9cce85d9f3d358c5605cf8c3666f034af42740e94d495e28b9aaa1001ba0c87580300000008006552ab00ab006affffffffd838978e10c0c78f1cd0a0830d6815f38cdcc631408649c32a25170099669daa0000000002acab8984227e804ad268b5b367285edcdf102d382d027789250a2c0641892b480c21bf84e3fb0100000000b518041e023d8653010000000001004040fb0100000000080051ac5200636a6300000000", "52ac", 0, 366357656, "bd0e88829afa6bdc1e192bb8b2d9d14db69298a4d81d464cbd34df0302c634c6");
        run_test_sighash("9ad5ccf503fa4facf6a27b538bc910cce83c118d6dfd82f3fb1b8ae364a1aff4dcefabd38f03000000096365655263ac655300807c48130c5937190a996105a69a8eba585e0bd32fadfc57d24029cbed6446d30ebc1f100100000004000053650f0ccfca1356768df7f9210cbf078a53c72e0712736d9a7a238e0115faac0ca383f219d0010000000600ab536552002799982b0221b8280000000000000c41320000000000086552ac6365636a6595f233a3", "6a5152", 2, 553208588, "f99c29a79f1d73d2a69c59abbb5798e987639e36d4c44125d8dc78a94ddcfb13");
        run_test_sighash("669538a204047214ce058aed6a07ca5ad4866c821c41ac1642c7d63ed0054f84677077a84f030000000853abacab6a655353ffffffff70c2a071c115282924e3cb678b13800c1d29b6a028b3c989a598c491bc7c76c5030000000752ac52ac5163ac80420e8a6e43d39af0163271580df6b936237f15de998e9589ec39fe717553d415ac02a4030000000463635153184ad8a5a4e69a8969f71288c331aff3c2b7d1b677d2ebafad47234840454b624bf7ac1d03000000056a63abab63df38c24a02fbc63a040000000002ab535ec3dc050000000002536500000000", "635153", 3, -190399351, "9615541884dfb1feeb08073a6a6aa73ef694bc5076e52187fdf4138a369f94d9");
        run_test_sighash("972128b904e7b673517e96e98d80c0c8ceceae76e2f5c126d63da77ffd7893fb53308bb2da0300000006ac6552ab52acffffffff4cac767c797d297c079a93d06dc8569f016b4bf7a7d79b605c526e1d36a40e2202000000095365ab636aac6a6a6a69928d2eddc836133a690cfb72ec2d3115bf50fb3b0d10708fa5d2ebb09b4810c426a1db01000000060052526300001e8e89585da7e77b2dd2e30625887f0660accdf29e53a614d23cf698e6fc8ab03310e87700000000076a520051acac6555231ddb0330ec2d03000000000200abfaf457040000000004ab6a6352bdc42400000000000153d6dd2f04", "", 0, 209234698, "4a92fec1eb03f5bd754ee9bfd70707dc4420cc13737374f4675f48529be518e4");
        run_test_sighash("5374f0c603d727f63006078bd6c3dce48bd5d0a4b6ea00a47e5832292d86af258ea0825c260000000009655353636352526a6af2221067297d42a9f8933dfe07f61a574048ff9d3a44a3535cd8eb7de79fb7c45b6f47320200000003ac006affffffff153d917c447d367e75693c5591e0abf4c94bbdd88a98ab8ad7f75bfe69a08c470200000005ac65516365ffffffff037b5b7b000000000001515dc4d904000000000004bb26010000000004536a6aac00000000", "516552516352ac", 2, 328538756, "8bb7a0129eaf4b8fc23e911c531b9b7637a21ab11a246352c6c053ff6e93fcb6");
        run_test_sighash("c441132102cc82101b6f31c1025066ab089f28108c95f18fa67db179610247086350c163bd010000000651525263ab00ffffffff9b8d56b1f16746f075249b215bdb3516cbbe190fef6292c75b1ad8a8988897c3000000000751ab6553abab00ffffffff02f9078b000000000009ab0053ac51ac00ab51c0422105000000000651006563525200000000", "ac51", 0, -197051790, "55acd8293ed0be6792150a3d7ced6c5ccd153ca7daf09cee035c1b0dac92bb96");
        run_test_sighash("8bff9d170419fa6d556c65fa227a185fe066efc1decf8a1c490bc5cbb9f742d68da2ab7f320100000007ab000053525365a7a43a80ab9593b9e8b6130a7849603b14b5c9397a190008d89d362250c3a2257504eb810200000007acabacac00ab51ee141be418f003e75b127fd3883dbf4e8c3f6cd05ca4afcaac52edd25dd3027ae70a62a00000000008ac52526a5200536affffffffb8058f4e1d7f220a1d1fa17e96d81dfb9a304a2de4e004250c9a576963a586ae0300000005abacac5363b9bc856c039c01d804000000000951656aac53005365acb0724e00000000000565abab63acea7c7a0000000000036a00ac00000000", "6565", 1, -1349282084, "2b822737c2affeefae13451d7c9db22ff98e06490005aba57013f6b9bbc97250");
        run_test_sighash("0e1633b4041c50f656e882a53fde964e7f0c853b0ada0964fc89ae124a2b7ffc5bc97ea6230100000006ac6aacacabacffffffff2e35f4dfcad2d53ea1c8ada8041d13ea6c65880860d96a14835b025f76b1fbd9000000000351515121270867ef6bf63a91adbaf790a43465c61a096acc5a776b8e5215d4e5cd1492e611f761000000000600ac6aab5265ffffffff63b5fc39bcac83ca80ac36124abafc5caee608f9f63a12479b68473bd4bae769000000000965ac52acac5263acabffffffff0163153e020000000008ab005165ab65515300000000", "6a6aac00", 0, -968477862, "20732d5073805419f275c53784e78db45e53332ee618a9fcf60a3417a6e2ca69");
        run_test_sighash("2b052c24022369e956a8d318e38780ef73b487ba6a8f674a56bdb80a9a63634c6110fb5154010000000251acffffffff48fe138fb7fdaa014d67044bc05940f4127e70c113c6744fbd13f8d51d45143e01000000005710db3804e01aa9030000000008acac6a516a5152abfd55aa01000000000751ab510000ac636d6026010000000000b97da9000000000000fddf3b53", "006552", 0, 595461670, "685d67d84755906d67a007a7d4fa311519467b9bdc6a351913246a41e082a29f");
        run_test_sighash("7888b71403f6d522e414d4ca2e12786247acf3e78f1918f6d727d081a79813d129ee8befce0100000009ab516a6353ab6365abffffffff4a882791bf6400fda7a8209fb2c83c6eef51831bdf0f5dacde648859090797ec030000000153ffffffffbb08957d59fa15303b681bad19ccf670d7d913697a2f4f51584bf85fcf91f1f30200000008526565ac52ac63acffffffff0227c0e8050000000001ac361dc801000000000800515165ab00ab0000000000", "656a", 2, 1869281295, "f43378a0b7822ad672773944884e866d7a46579ee34f9afc17b20afc1f6cf197");
        run_test_sighash("cc4dda57047bd0ca6806243a6a4b108f7ced43d8042a1acaa28083c9160911cf47eab910c40200000007526a0000ab6a63e4154e581fcf52567836c9a455e8b41b162a78c85906ccc1c2b2b300b4c69caaaa2ba0230300000008ab5152ac5100ab65ffffffff69696b523ed4bd41ecd4d65b4af73c9cf77edf0e066138712a8e60a04614ea1c0300000004ab6a000016c9045c7df7836e05ac4b2e397e2dd72a5708f4a8bf6d2bc36adc5af3cacefcf074b8b403000000065352ac5252acffffffff01d7e380050000000000cf4e699a", "525163656351", 1, -776533694, "ff18c5bffd086e00917c2234f880034d24e7ea2d1e1933a28973d134ca9e35d2");
        run_test_sighash("b7877f82019c832707a60cf14fba44cfa254d787501fdd676bd58c744f6e951dbba0b3b77f0200000009ac515263ac53525300a5a36e500148f89c0500000000085265ac6a6a65acab00000000", "6563", 0, -1785108415, "cb6e4322955af12eb29613c70e1a00ddbb559c887ba844df0bcdebed736dffbd");
        run_test_sighash("aeb14046045a28cc59f244c2347134d3434faaf980961019a084f7547218785a2bd03916f3000000000165f852e6104304955bda5fa0b75826ee176211acc4a78209816bbb4419feff984377b2352200000000003a94a5032df1e0d60390715b4b188c330e4bb7b995f07cdef11ced9d17ee0f60bb7ffc8e0100000002516513e343a5c1dc1c80cd4561e9dddad22391a2dbf9c8d2b6048e519343ca1925a9c6f0800a020000000665516365ac513180144a0290db27000000000006ab655151ab5138b187010000000007ab5363abac516a9e5cd98a", "53ac", 0, 478591320, "e8d89a302ae626898d4775d103867a8d9e81f4fd387af07212adab99946311ef");
        run_test_sighash("57a5a04c0278c8c8e243d2df4bb716f81d41ac41e2df153e7096f5682380c4f441888d9d260300000004ab63ab6afdbe4203525dff42a7b1e628fe22bccaa5edbb34d8ab02faff198e085580ea5fcdb0c61b0000000002ac6affffffff03375e6c05000000000663ab516a6a513cb6260400000000007ca328020000000006516a636a52ab94701cc7", "0053ac5152", 0, -550925626, "b7ca991ab2e20d0158168df2d3dd842a57ab4a3b67cca8f45b07c4b7d1d11126");
        run_test_sighash("7e27c42d0279c1a05eeb9b9faedcc9be0cab6303bde351a19e5cbb26dd0d594b9d74f40d2b020000000200518c8689a08a01e862d5c4dcb294a2331912ff11c13785be7dce3092f154a005624970f84e0200000000500cf5a601e74c1f0000000000076aab52636a6a5200000000", "6500006a5351", 0, 449533391, "535ba819d74770d4d613ee19369001576f98837e18e1777b8246238ff2381dd0");
        run_test_sighash("2b3470dd028083910117f86614cdcfb459ee56d876572510be4df24c72e8f58c70d5f5948b03000000066aab65635265da2c3aac9d42c9baafd4b655c2f3efc181784d8cba5418e053482132ee798408ba43ccf90300000000ffffffff047dda4703000000000765516a52ac53009384a603000000000651636a63ab6a8cf57a03000000000352ab6a8cf6a405000000000952636a6a6565525100661e09cb", "ac520063ac6a6a52", 1, 1405647183, "9b360c3310d55c845ef537125662b9fe56840c72136891274e9fedfef56f9bb5");
        run_test_sighash("3a5644a9010f199f253f858d65782d3caec0ac64c3262b56893022b9796086275c9d4d097b02000000009d168f7603a67b30050000000007ac51536a0053acd9d88a050000000007655363535263ab3cf1f403000000000352ac6a00000000", "005363536565acac6a", 0, -1383947195, "6390ab0963cf611e0cea35a71dc958b494b084e6fd71d22217fdc5524787ade6");
        run_test_sighash("bda1ff6804a3c228b7a12799a4c20917301dd501c67847d35da497533a606701ad31bf9d5e0300000001ac16a6c5d03cf516cd7364e4cbbf5aeccd62f8fd03cb6675883a0636a7daeb650423cb1291010000000500656553ac4a63c30b6a835606909c9efbae1b2597e9db020c5ecfc0642da6dc583fba4e84167539a8020000000865525353515200acffffffff990807720a5803c305b7da08a9f24b92abe343c42ac9e917a84e1f335aad785d00000000026a52ffffffff04981f20030000000001ab8c762200000000000253ab690b9605000000000151ce88b301000000000753526a6a51006500000000", "000052ac52530000", 1, -1809193140, "5299b0fb7fc16f40a5d6b337e71fcd1eb04d2600aefd22c06fe9c71fe0b0ba54");
        run_test_sighash("db4904e6026b6dd8d898f278c6428a176410d1ffbde75a4fa37cda12263108ccd4ca6137440100000007656a0000515263ffffffff1db7d5005c1c40da0ed17b74cf6b2a6ee2c33c9e0bacda76c0da2017dcac2fc70200000004abab6a53ffffffff0454cf2103000000000153463aef000000000009ab6a630065ab52636387e0ed050000000000e8d16f05000000000352ac63e4521b22", "", 1, 1027042424, "48315a95e49277ab6a2d561ee4626820b7bab919eea372b6bf4e9931ab221d04");
        run_test_sighash("dca31ad10461ead74751e83d9a81dcee08db778d3d79ad9a6d079cfdb93919ac1b0b61871102000000086500525365ab51ac7f7e9aed78e1ef8d213d40a1c50145403d196019985c837ffe83836222fe3e5955e177e70100000006525152525300ffffffff5e98482883cc08a6fe946f674cca479822f0576a43bf4113de9cbf414ca628060100000006ac53516a5253ffffffff07490b0b898198ec16c23b75d606e14fa16aa3107ef9818594f72d5776805ec502000000036a0052ffffffff01932a2803000000000865ab6551ac6a516a2687aa06", "635300ac", 2, -1880362326, "74d6a2fa7866fd8b74b2e34693e2d6fd690410384b7afdcd6461b1ae71d265ce");
        run_test_sighash("e14e1a9f0442ab44dfc5f6d945ad1ff8a376bc966aad5515421e96ddbe49e529614995cafc03000000055165515165fffffffff97582b8290e5a5cfeb2b0f018882dbe1b43f60b7f45e4dd21dbd3a8b0cfca3b0200000000daa267726fe075db282d694b9fee7d6216d17a8c1f00b2229085495c5dc5b260c8f8cd5d000000000363ac6affffffffaab083d22d0465471c896a438c6ac3abf4d383ae79420617a8e0ba8b9baa872b010000000963526563ac5363ababd948b5ce022113440200000000076a636552006a53229017040000000000e6f62ac8", "526353636a65", 3, -485265025, "1bc8ad76f9b7c366c5d052dc479d6a8a2015566d3a42e93ab12f727692c89d65");
        run_test_sighash("69df842a04c1410bfca10896467ce664cfa31c681a5dac10106b34d4b9d4d6d0dc1eac01c1000000000551536a5165269835ca4ad7268667b16d0a2df154ec81e304290d5ed69e0069b43f8c89e673328005e200000000076a5153006aacabffffffffc9314bd80b176488f3d634360fcba90c3a659e74a52e100ac91d3897072e3509010000000765abac51636363ffffffff0e0768b13f10f0fbd2fa3f68e4b4841809b3b5ba0e53987c3aaffcf09eee12bf0300000008ac535263526a53ac514f4c2402da8fab0400000000001ef15201000000000451526a52d0ec9aca", "525365ac52", 1, 313967049, "a72a760b361af41832d2c667c7488dc9702091918d11e344afc234a4aea3ec44");
        run_test_sighash("e4fec9f10378a95199c1dd23c6228732c9de0d7997bf1c83918a5cfd36012476c0c3cba24002000000085165536500ac0000ad08ab93fb49d77d12a7ccdbb596bc5110876451b53a79fdce43104ff1c316ad63501de801000000046a6352ab76af9908463444aeecd32516a04dd5803e02680ed7f16307242a794024d93287595250f4000000000089807279041a82e603000000000200521429100200000000055253636a63f20b940400000000004049ed04000000000500ab5265ab43dfaf7d", "6563526aac", 2, -1923470368, "32f3c012eca9a823bebb9b282240aec40ca65df9f38da43b1dcfa0cac0c0df7e");
        run_test_sighash("4000d3600100b7a3ff5b41ec8d6ccdc8b2775ad034765bad505192f05d1f55d2bc39d0cbe10100000007ab5165ac6a5163ffffffff034949150100000000026a6a92c9f6000000000008ab6553ab6aab635200e697040000000007636a5353525365237ae7d2", "52000063", 0, -880046683, "c76146f68f43037289aaeb2bacf47408cddc0fb326b350eb4f5ef6f0f8564793");
        run_test_sighash("eabc0aa701fe489c0e4e6222d72b52f083166b49d63ad1410fb98caed027b6a71c02ab830c03000000075253ab63530065ffffffff01a5dc0b05000000000253533e820177", "", 0, 954499283, "1d849b92eedb9bf26bd4ced52ce9cb0595164295b0526842ab1096001fcd31b1");
        run_test_sighash("d48d55d304aad0139783b44789a771539d052db565379f668def5084daba0dfd348f7dcf6b00000000006826f59e5ffba0dd0ccbac89c1e2d69a346531d7f995dea2ca6d7e6d9225d81aec257c6003000000096a655200ac656552acffffffffa188ffbd5365cae844c8e0dea6213c4d1b2407274ae287b769ab0bf293e049eb0300000005ac6a6aab51ad1c407c5b116ca8f65ed496b476183f85f072c5f8a0193a4273e2015b1cc288bf03e9e2030000000252abffffffff04076f44040000000006655353abab53be6500050000000003ac65ac3c15040500000000095100ab536353516a52ed3aba04000000000900ac53ab53636aabac00000000", "5253526563acac", 2, -1506108646, "bbee17c8582514744bab5df50012c94b0db4aff5984d2e13a8d09421674404e2");
        run_test_sighash("9746f45b039bfe723258fdb6be77eb85917af808211eb9d43b15475ee0b01253d33fc3bfc502000000065163006a655312b12562dc9c54e11299210266428632a7d0ee31d04dfc7375dcad2da6e9c11947ced0e000000000009074095a5ac4df057554566dd04740c61490e1d3826000ad9d8f777a93373c8dddc4918a00000000025351ffffffff01287564030000000004636a00ab00000000", "52", 2, -1380411075, "84af1623366c4db68d81f452b86346832344734492b9c23fbb89015e516c60b2");
        run_test_sighash("df0a32ae01c4672fd1abd0b2623aae0a1a8256028df57e532f9a472d1a9ceb194267b6ee190200000009536a6a51516a525251b545f9e803469a2302000000000465526500810631040000000000441f5b050000000006530051006aaceb183c76", "536a635252ac6a", 0, 1601138113, "9a0435996cc58bdba09643927fe48c1fc908d491a050abbef8daec87f323c58f");
        run_test_sighash("b1c0b71804dff30812b92eefb533ac77c4b9fdb9ab2f77120a76128d7da43ad70c20bbfb990200000002536392693e6001bc59411aebf15a3dc62a6566ec71a302141b0c730a3ecc8de5d76538b30f55010000000665535252ac514b740c6271fb9fe69fdf82bf98b459a7faa8a3b62f3af34943ad55df4881e0d93d3ce0ac0200000000c4158866eb9fb73da252102d1e64a3ce611b52e873533be43e6883137d0aaa0f63966f060000000001abffffffff04a605b604000000000851006a656a630052f49a0300000000000252515a94e1050000000009abac65ab0052abab00fd8dd002000000000651535163526a2566852d", "ac5363", 0, -1718831517, "b0dc030661783dd9939e4bf1a6dfcba809da2017e1b315a6312e5942d714cf05");
        run_test_sighash("3657e4260304ccdc19936e47bdf058d36167ee3d4eb145c52b224eff04c9eb5d1b4e434dfc0000000001ab58aefe57707c66328d3cceef2e6f56ab6b7465e587410c5f73555a513ace2b232793a74400000000036a006522e69d3a785b61ad41a635d59b3a06b2780a92173f85f8ed428491d0aaa436619baa9c4501000000046351abab2609629902eb7793050000000000a1b967040000000003525353a34d6192", "516a", 0, -1761874713, "0a2ff41f6d155d8d0e37cd9438f3b270df9f9214cda8e95c76d5a239ca189df2");
        run_test_sighash("a0eb6dc402994e493c787b45d1f946d267b09c596c5edde043e620ce3d59e95b2b5b93d43002000000096a5252526aac63ab6555694287a279e29ee491c177a801cd685b8744a2eab83824255a3bcd08fc0e3ea13fb8820000000009abab6365ab52ab0063ffffffff029e424a040000000008acab53ab516a636a23830f0400000000016adf49c1f9", "ac0065ac6500005252", 1, 669294500, "e05e3d383631a7ed1b78210c13c2eb26564e5577db7ddfcea2583c7c014091d4");
        run_test_sighash("6e67c0d3027701ef71082204c85ed63c700ef1400c65efb62ce3580d187fb348376a23e9710200000001655b91369d3155ba916a0bc6fe4f5d94cad461d899bb8aaac3699a755838bfc229d6828920010000000765536353526a52ffffffff04c0c792000000000005650052535372f79e000000000001527fc0ee010000000005ac5300ab65d1b3e902000000000251aba942b278", "6a5151", 0, 1741407676, "e657e2c8ec4ebc769ddd3198a83267b47d4f2a419fc737e813812acefad92ff7");
        run_test_sighash("8f53639901f1d643e01fc631f632b7a16e831d846a0184cdcda289b8fa7767f0c292eb221a00000000046a53abacffffffff037a2daa01000000000553ac6a6a51eac349020000000005ac526552638421b3040000000007006a005100ac63048a1492", "ac65", 0, 1033685559, "da86c260d42a692358f46893d6f91563985d86eeb9ea9e21cd38c2d8ffcfcc4d");
        run_test_sighash("b3cad3a7041c2c17d90a2cd994f6c37307753fa3635e9ef05ab8b1ff121ca11239a0902e700300000009ab635300006aac5163ffffffffcec91722c7468156dce4664f3c783afef147f0e6f80739c83b5f09d5a09a57040200000004516a6552ffffffff969d1c6daf8ef53a70b7cdf1b4102fb3240055a8eaeaed2489617cd84cfd56cf020000000352ab53ffffffff46598b6579494a77b593681c33422a99559b9993d77ca2fa97833508b0c169f80200000009655300655365516351ffffffff04d7ddf800000000000853536a65ac6351ab09f3420300000000056aab65abac33589d04000000000952656a65655151acac944d6f0400000000006a8004ba", "005165", 1, 1035865506, "fe1dc9e8554deecf8f50c417c670b839cc9d650722ebaaf36572418756075d58");
        run_test_sighash("cf781855040a755f5ba85eef93837236b34a5d3daeb2dbbdcf58bb811828d806ed05754ab8010000000351ac53ffffffffda1e264727cf55c67f06ebcc56dfe7fa12ac2a994fecd0180ce09ee15c480f7d00000000096351516a51acac00ab53dd49ff9f334befd6d6f87f1a832cddfd826a90b78fd8cf19a52cb8287788af94e939d6020000000700525251ac526310d54a7e8900ed633f0f6f0841145aae7ee0cbbb1e2a0cae724ee4558dbabfdc58ba6855010000000552536a53abfd1b101102c51f910500000000096300656a525252656a300bee010000000009ac52005263635151abe19235c9", "53005365", 2, 1422854188, "d5981bd4467817c1330da72ddb8760d6c2556cd809264b2d85e6d274609fc3a3");
    }

    #[test]
    #[cfg(feature="bitcoinconsensus")]
    fn test_transaction_verify () {
        use hex::decode as hex_decode;
        use std::collections::HashMap;
        use blockdata::script;
        // a random recent segwit transaction from blockchain using both old and segwit inputs
        let mut spending: Transaction = deserialize(hex_decode("020000000001031cfbc8f54fbfa4a33a30068841371f80dbfe166211242213188428f437445c91000000006a47304402206fbcec8d2d2e740d824d3d36cc345b37d9f65d665a99f5bd5c9e8d42270a03a8022013959632492332200c2908459547bf8dbf97c65ab1a28dec377d6f1d41d3d63e012103d7279dfb90ce17fe139ba60a7c41ddf605b25e1c07a4ddcb9dfef4e7d6710f48feffffff476222484f5e35b3f0e43f65fc76e21d8be7818dd6a989c160b1e5039b7835fc00000000171600140914414d3c94af70ac7e25407b0689e0baa10c77feffffffa83d954a62568bbc99cc644c62eb7383d7c2a2563041a0aeb891a6a4055895570000000017160014795d04cc2d4f31480d9a3710993fbd80d04301dffeffffff06fef72f000000000017a91476fd7035cd26f1a32a5ab979e056713aac25796887a5000f00000000001976a914b8332d502a529571c6af4be66399cd33379071c588ac3fda0500000000001976a914fc1d692f8de10ae33295f090bea5fe49527d975c88ac522e1b00000000001976a914808406b54d1044c429ac54c0e189b0d8061667e088ac6eb68501000000001976a914dfab6085f3a8fb3e6710206a5a959313c5618f4d88acbba20000000000001976a914eb3026552d7e3f3073457d0bee5d4757de48160d88ac0002483045022100bee24b63212939d33d513e767bc79300051f7a0d433c3fcf1e0e3bf03b9eb1d70220588dc45a9ce3a939103b4459ce47500b64e23ab118dfc03c9caa7d6bfc32b9c601210354fd80328da0f9ae6eef2b3a81f74f9a6f66761fadf96f1d1d22b1fd6845876402483045022100e29c7e3a5efc10da6269e5fc20b6a1cb8beb92130cc52c67e46ef40aaa5cac5f0220644dd1b049727d991aece98a105563416e10a5ac4221abac7d16931842d5c322012103960b87412d6e169f30e12106bdf70122aabb9eb61f455518322a18b920a4dfa887d30700")
            .unwrap().as_slice()).unwrap();
        let spent1: Transaction = deserialize(hex_decode("020000000001040aacd2c49f5f3c0968cfa8caf9d5761436d95385252e3abb4de8f5dcf8a582f20000000017160014bcadb2baea98af0d9a902e53a7e9adff43b191e9feffffff96cd3c93cac3db114aafe753122bd7d1afa5aa4155ae04b3256344ecca69d72001000000171600141d9984579ceb5c67ebfbfb47124f056662fe7adbfeffffffc878dd74d3a44072eae6178bb94b9253177db1a5aaa6d068eb0e4db7631762e20000000017160014df2a48cdc53dae1aba7aa71cb1f9de089d75aac3feffffffe49f99275bc8363f5f593f4eec371c51f62c34ff11cc6d8d778787d340d6896c0100000017160014229b3b297a0587e03375ab4174ef56eeb0968735feffffff03360d0f00000000001976a9149f44b06f6ee92ddbc4686f71afe528c09727a5c788ac24281b00000000001976a9140277b4f68ff20307a2a9f9b4487a38b501eb955888ac227c0000000000001976a9148020cd422f55eef8747a9d418f5441030f7c9c7788ac0247304402204aa3bd9682f9a8e101505f6358aacd1749ecf53a62b8370b97d59243b3d6984f02200384ad449870b0e6e89c92505880411285ecd41cf11e7439b973f13bad97e53901210205b392ffcb83124b1c7ce6dd594688198ef600d34500a7f3552d67947bbe392802473044022033dfd8d190a4ae36b9f60999b217c775b96eb10dee3a1ff50fb6a75325719106022005872e4e36d194e49ced2ebcf8bb9d843d842e7b7e0eb042f4028396088d292f012103c9d7cbf369410b090480de2aa15c6c73d91b9ffa7d88b90724614b70be41e98e0247304402207d952de9e59e4684efed069797e3e2d993e9f98ec8a9ccd599de43005fe3f713022076d190cc93d9513fc061b1ba565afac574e02027c9efbfa1d7b71ab8dbb21e0501210313ad44bc030cc6cb111798c2bf3d2139418d751c1e79ec4e837ce360cc03b97a024730440220029e75edb5e9413eb98d684d62a077b17fa5b7cc19349c1e8cc6c4733b7b7452022048d4b9cae594f03741029ff841e35996ef233701c1ea9aa55c301362ea2e2f68012103590657108a72feb8dc1dec022cf6a230bb23dc7aaa52f4032384853b9f8388baf9d20700")
            .unwrap().as_slice()).unwrap();
        let spent2: Transaction = deserialize(hex_decode("0200000000010166c3d39490dc827a2594c7b17b7d37445e1f4b372179649cd2ce4475e3641bbb0100000017160014e69aa750e9bff1aca1e32e57328b641b611fc817fdffffff01e87c5d010000000017a914f3890da1b99e44cd3d52f7bcea6a1351658ea7be87024830450221009eb97597953dc288de30060ba02d4e91b2bde1af2ecf679c7f5ab5989549aa8002202a98f8c3bd1a5a31c0d72950dd6e2e3870c6c5819a6c3db740e91ebbbc5ef4800121023f3d3b8e74b807e32217dea2c75c8d0bd46b8665b3a2d9b3cb310959de52a09bc9d20700")
            .unwrap().as_slice()).unwrap();
        let spent3: Transaction = deserialize(hex_decode("01000000027a1120a30cef95422638e8dab9dedf720ec614b1b21e451a4957a5969afb869d000000006a47304402200ecc318a829a6cad4aa9db152adbf09b0cd2de36f47b53f5dade3bc7ef086ca702205722cda7404edd6012eedd79b2d6f24c0a0c657df1a442d0a2166614fb164a4701210372f4b97b34e9c408741cd1fc97bcc7ffdda6941213ccfde1cb4075c0f17aab06ffffffffc23b43e5a18e5a66087c0d5e64d58e8e21fcf83ce3f5e4f7ecb902b0e80a7fb6010000006b483045022100f10076a0ea4b4cf8816ed27a1065883efca230933bf2ff81d5db6258691ff75202206b001ef87624e76244377f57f0c84bc5127d0dd3f6e0ef28b276f176badb223a01210309a3a61776afd39de4ed29b622cd399d99ecd942909c36a8696cfd22fc5b5a1affffffff0200127a000000000017a914f895e1dd9b29cb228e9b06a15204e3b57feaf7cc8769311d09000000001976a9144d00da12aaa51849d2583ae64525d4a06cd70fde88ac00000000")
            .unwrap().as_slice()).unwrap();

        let mut spent = HashMap::new();
        spent.insert(spent1.txid(), spent1);
        spent.insert(spent2.txid(), spent2);
        spent.insert(spent3.txid(), spent3);
        let mut spent2 = spent.clone();
        let mut spent3 = spent.clone();

        spending.verify(|point: &OutPoint| {
            if let Some(tx) = spent.remove(&point.txid) {
                return tx.output.get(point.vout as usize).cloned();
            }
            None
        }).unwrap();

        // test that we fail with repeated use of same input
        let mut double_spending = spending.clone();
        let re_use = double_spending.input[0].clone();
        double_spending.input.push (re_use);

        assert!(double_spending.verify(|point: &OutPoint| {
            if let Some(tx) = spent2.remove(&point.txid) {
                return tx.output.get(point.vout as usize).cloned();
            }
            None
        }).is_err());

        // test that we get a failure if we corrupt a signature
        spending.input[1].witness[0][10] = 42;
        match spending.verify(|point: &OutPoint| {
            if let Some(tx) = spent3.remove(&point.txid) {
                return tx.output.get(point.vout as usize).cloned();
            }
            None
        }).err().unwrap() {
            script::Error::BitcoinConsensus(_) => {},
            _ => panic!("Wrong error type"),
        }
    }
}

