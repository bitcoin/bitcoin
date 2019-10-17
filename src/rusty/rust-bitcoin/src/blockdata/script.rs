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

//! Script
//!
//! Scripts define Bitcoin's digital signature scheme: a signature is formed
//! from a script (the second half of which is defined by a coin to be spent,
//! and the first half provided by the spending transaction), and is valid
//! iff the script leaves `TRUE` on the stack after being evaluated.
//! Bitcoin's script is a stack-based assembly language similar in spirit to
//! Forth.
//!
//! This module provides the structures and functions needed to support scripts.
//!

use std::default::Default;
use std::{error, fmt, io};

#[cfg(feature = "serde")] use serde;

use blockdata::opcodes;
use consensus::{encode, Decodable, Encodable};
use hashes::{hash160, sha256, Hash};
#[cfg(feature="bitcoinconsensus")] use bitcoinconsensus;
#[cfg(feature="bitcoinconsensus")] use std::convert;
#[cfg(feature="bitcoinconsensus")] use OutPoint;

use util::key::PublicKey;

#[derive(Clone, Default, PartialOrd, Ord, PartialEq, Eq, Hash)]
/// A Bitcoin script
pub struct Script(Box<[u8]>);

impl fmt::Debug for Script {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str("Script(")?;
        self.fmt_asm(f)?;
        f.write_str(")")
    }
}

impl fmt::Display for Script {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self, f)
    }
}

impl fmt::LowerHex for Script {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for &ch in self.0.iter() {
            write!(f, "{:02x}", ch)?;
        }
        Ok(())
    }
}

impl fmt::UpperHex for Script {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for &ch in self.0.iter() {
            write!(f, "{:02X}", ch)?;
        }
        Ok(())
    }
}

#[derive(PartialEq, Eq, Debug, Clone)]
/// An object which can be used to construct a script piece by piece
pub struct Builder(Vec<u8>, Option<opcodes::All>);
display_from_debug!(Builder);

/// Ways that a script might fail. Not everything is split up as
/// much as it could be; patches welcome if more detailed errors
/// would help you.
#[derive(PartialEq, Eq, Debug, Clone)]
pub enum Error {
    /// Something did a non-minimal push; for more information see
    /// `https://github.com/bitcoin/bips/blob/master/bip-0062.mediawiki#Push_operators`
    NonMinimalPush,
    /// Some opcode expected a parameter, but it was missing or truncated
    EarlyEndOfScript,
    /// Tried to read an array off the stack as a number when it was more than 4 bytes
    NumericOverflow,
    #[cfg(feature="bitcoinconsensus")]
    /// Error validating the script with bitcoinconsensus library
    BitcoinConsensus(bitcoinconsensus::Error),
    #[cfg(feature="bitcoinconsensus")]
    /// Can not find the spent output
    UnknownSpentOutput(OutPoint),
    #[cfg(feature="bitcoinconsensus")]
    /// Can not serialize the spending transaction
    SerializationError
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str(error::Error::description(self))
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&error::Error> { None }

    fn description(&self) -> &'static str {
        match *self {
            Error::NonMinimalPush => "non-minimal datapush",
            Error::EarlyEndOfScript => "unexpected end of script",
            Error::NumericOverflow => "numeric overflow (number on stack larger than 4 bytes)",
            #[cfg(feature="bitcoinconsensus")]
            Error::BitcoinConsensus(ref _n) => "bitcoinconsensus verification failed",
            #[cfg(feature="bitcoinconsensus")]
            Error::UnknownSpentOutput(ref _point) => "unknown spent output Transaction::verify()",
            #[cfg(feature="bitcoinconsensus")]
            Error::SerializationError => "can not serialize the spending transaction in Transaction::verify()",
        }
    }
}

#[cfg(feature="bitcoinconsensus")]
#[doc(hidden)]
impl convert::From<bitcoinconsensus::Error> for Error {
    fn from(err: bitcoinconsensus::Error) -> Error {
        match err {
            _ => Error::BitcoinConsensus(err)
        }
    }
}
/// Helper to encode an integer in script format
fn build_scriptint(n: i64) -> Vec<u8> {
    if n == 0 { return vec![] }

    let neg = n < 0;

    let mut abs = if neg { -n } else { n } as usize;
    let mut v = vec![];
    while abs > 0xFF {
        v.push((abs & 0xFF) as u8);
        abs >>= 8;
    }
    // If the number's value causes the sign bit to be set, we need an extra
    // byte to get the correct value and correct sign bit
    if abs & 0x80 != 0 {
        v.push(abs as u8);
        v.push(if neg { 0x80u8 } else { 0u8 });
    }
    // Otherwise we just set the sign bit ourselves
    else {
        abs |= if neg { 0x80 } else { 0 };
        v.push(abs as u8);
    }
    v
}

/// Helper to decode an integer in script format
/// Notice that this fails on overflow: the result is the same as in
/// bitcoind, that only 4-byte signed-magnitude values may be read as
/// numbers. They can be added or subtracted (and a long time ago,
/// multiplied and divided), and this may result in numbers which
/// can't be written out in 4 bytes or less. This is ok! The number
/// just can't be read as a number again.
/// This is a bit crazy and subtle, but it makes sense: you can load
/// 32-bit numbers and do anything with them, which back when mult/div
/// was allowed, could result in up to a 64-bit number. We don't want
/// overflow since that's surprising --- and we don't want numbers that
/// don't fit in 64 bits (for efficiency on modern processors) so we
/// simply say, anything in excess of 32 bits is no longer a number.
/// This is basically a ranged type implementation.
pub fn read_scriptint(v: &[u8]) -> Result<i64, Error> {
    let len = v.len();
    if len == 0 { return Ok(0); }
    if len > 4 { return Err(Error::NumericOverflow); }

    let (mut ret, sh) = v.iter()
                         .fold((0, 0), |(acc, sh), n| (acc + ((*n as i64) << sh), sh + 8));
    if v[len - 1] & 0x80 != 0 {
        ret &= (1 << (sh - 1)) - 1;
        ret = -ret;
    }
    Ok(ret)
}

/// This is like "`read_scriptint` then map 0 to false and everything
/// else as true", except that the overflow rules don't apply.
#[inline]
pub fn read_scriptbool(v: &[u8]) -> bool {
    !(v.is_empty() ||
        ((v[v.len() - 1] == 0 || v[v.len() - 1] == 0x80) &&
         v.iter().rev().skip(1).all(|&w| w == 0)))
}

/// Read a script-encoded unsigned integer
pub fn read_uint(data: &[u8], size: usize) -> Result<usize, Error> {
    if data.len() < size {
        Err(Error::EarlyEndOfScript)
    } else {
        let mut ret = 0;
        for (i, item) in data.iter().take(size).enumerate() {
            ret += (*item as usize) << (i * 8);
        }
        Ok(ret)
    }
}

impl Script {
    /// Creates a new empty script
    pub fn new() -> Script { Script(vec![].into_boxed_slice()) }

    /// The length in bytes of the script
    pub fn len(&self) -> usize { self.0.len() }

    /// Whether the script is the empty script
    pub fn is_empty(&self) -> bool { self.0.is_empty() }

    /// Returns the script data
    pub fn as_bytes(&self) -> &[u8] { &*self.0 }

    /// Returns a copy of the script data
    pub fn to_bytes(&self) -> Vec<u8> { self.0.clone().into_vec() }

    /// Convert the script into a byte vector
    pub fn into_bytes(self) -> Vec<u8> { self.0.into_vec() }

    /// Compute the P2SH output corresponding to this redeem script
    pub fn to_p2sh(&self) -> Script {
        Builder::new().push_opcode(opcodes::all::OP_HASH160)
                      .push_slice(&hash160::Hash::hash(&self.0)[..])
                      .push_opcode(opcodes::all::OP_EQUAL)
                      .into_script()
    }

    /// Compute the P2WSH output corresponding to this witnessScript (aka the "witness redeem
    /// script")
    pub fn to_v0_p2wsh(&self) -> Script {
        Builder::new().push_int(0)
                      .push_slice(&sha256::Hash::hash(&self.0)[..])
                      .into_script()
    }

    /// Checks whether a script pubkey is a p2sh output
    #[inline]
    pub fn is_p2sh(&self) -> bool {
        self.0.len() == 23 &&
        self.0[0] == opcodes::all::OP_HASH160.into_u8() &&
        self.0[1] == opcodes::all::OP_PUSHBYTES_20.into_u8() &&
        self.0[22] == opcodes::all::OP_EQUAL.into_u8()
    }

    /// Checks whether a script pubkey is a p2pkh output
    #[inline]
    pub fn is_p2pkh(&self) -> bool {
        self.0.len() == 25 &&
        self.0[0] == opcodes::all::OP_DUP.into_u8() &&
        self.0[1] == opcodes::all::OP_HASH160.into_u8() &&
        self.0[2] == opcodes::all::OP_PUSHBYTES_20.into_u8() &&
        self.0[23] == opcodes::all::OP_EQUALVERIFY.into_u8() &&
        self.0[24] == opcodes::all::OP_CHECKSIG.into_u8()
    }

    /// Checks whether a script pubkey is a p2pk output
    #[inline]
    pub fn is_p2pk(&self) -> bool {
        (self.0.len() == 67 &&
            self.0[0] == opcodes::all::OP_PUSHBYTES_65.into_u8() &&
            self.0[66] == opcodes::all::OP_CHECKSIG.into_u8())
     || (self.0.len() == 35 &&
            self.0[0] == opcodes::all::OP_PUSHBYTES_33.into_u8() &&
            self.0[34] == opcodes::all::OP_CHECKSIG.into_u8())
    }

    /// Checks whether a script pubkey is a Segregated Witness (segwit) program.
    #[inline]
    pub fn is_witness_program(&self) -> bool {
        // A scriptPubKey (or redeemScript as defined in BIP16/P2SH) that consists of a 1-byte
        // push opcode (for 0 to 16) followed by a data push between 2 and 40 bytes gets a new
        // special meaning. The value of the first push is called the "version byte". The following
        // byte vector pushed is called the "witness program".
        let min_vernum: u8 = opcodes::all::OP_PUSHNUM_1.into_u8();
        let max_vernum: u8 = opcodes::all::OP_PUSHNUM_16.into_u8();
        self.0.len() >= 4
            && self.0.len() <= 42
            // Version 0 or PUSHNUM_1-PUSHNUM_16
            && (self.0[0] == 0 || self.0[0] >= min_vernum && self.0[0] <= max_vernum)
            // Second byte push opcode 2-40 bytes
            && self.0[1] >= opcodes::all::OP_PUSHBYTES_2.into_u8()
            && self.0[1] <= opcodes::all::OP_PUSHBYTES_40.into_u8()
            // Check that the rest of the script has the correct size
            && self.0.len() - 2 == self.0[1] as usize
    }

    /// Checks whether a script pubkey is a p2wsh output
    #[inline]
    pub fn is_v0_p2wsh(&self) -> bool {
        self.0.len() == 34 &&
        self.0[0] == opcodes::all::OP_PUSHBYTES_0.into_u8() &&
        self.0[1] == opcodes::all::OP_PUSHBYTES_32.into_u8()
    }

    /// Checks whether a script pubkey is a p2wpkh output
    #[inline]
    pub fn is_v0_p2wpkh(&self) -> bool {
        self.0.len() == 22 &&
            self.0[0] == opcodes::all::OP_PUSHBYTES_0.into_u8() &&
            self.0[1] == opcodes::all::OP_PUSHBYTES_20.into_u8()
    }

    /// Check if this is an OP_RETURN output
    pub fn is_op_return (&self) -> bool {
        !self.0.is_empty() && (opcodes::All::from(self.0[0]) == opcodes::all::OP_RETURN)
    }

    /// Whether a script can be proven to have no satisfying input
    pub fn is_provably_unspendable(&self) -> bool {
        !self.0.is_empty() && (opcodes::All::from(self.0[0]).classify() == opcodes::Class::ReturnOp ||
                               opcodes::All::from(self.0[0]).classify() == opcodes::Class::IllegalOp)
    }

    /// Iterate over the script in the form of `Instruction`s, which are an enum covering
    /// opcodes, datapushes and errors. At most one error will be returned and then the
    /// iterator will end. To instead iterate over the script as sequence of bytes, treat
    /// it as a slice using `script[..]` or convert it to a vector using `into_bytes()`.
    pub fn iter(&self, enforce_minimal: bool) -> Instructions {
        Instructions {
            data: &self.0[..],
            enforce_minimal: enforce_minimal,
        }
    }

    #[cfg(feature="bitcoinconsensus")]
    /// verify spend of an input script
    /// # Parameters
    ///  * index - the input index in spending which is spending this transaction
    ///  * amount - the amount this script guards
    ///  * spending - the transaction that attempts to spend the output holding this script
    pub fn verify (&self, index: usize, amount: u64, spending: &[u8]) -> Result<(), Error> {
        Ok(bitcoinconsensus::verify (&self.0[..], amount, spending, index)?)
    }

    /// Write the assembly decoding of the script to the formatter.
    pub fn fmt_asm(&self, f: &mut fmt::Write) -> fmt::Result {
        let mut index = 0;
        while index < self.0.len() {
            let opcode = opcodes::All::from(self.0[index]);
            index += 1;

            let data_len = if let opcodes::Class::PushBytes(n) = opcode.classify() {
                n as usize
            } else {
                match opcode {
                    opcodes::all::OP_PUSHDATA1 => {
                        if self.0.len() < index + 1 {
                            f.write_str("<unexpected end>")?;
                            break;
                        }
                        match read_uint(&self.0[index..], 1) {
                            Ok(n) => { index += 1; n as usize }
                            Err(_) => { f.write_str("<bad length>")?; break; }
                        }
                    }
                    opcodes::all::OP_PUSHDATA2 => {
                        if self.0.len() < index + 2 {
                            f.write_str("<unexpected end>")?;
                            break;
                        }
                        match read_uint(&self.0[index..], 2) {
                            Ok(n) => { index += 2; n as usize }
                            Err(_) => { f.write_str("<bad length>")?; break; }
                        }
                    }
                    opcodes::all::OP_PUSHDATA4 => {
                        if self.0.len() < index + 4 {
                            f.write_str("<unexpected end>")?;
                            break;
                        }
                        match read_uint(&self.0[index..], 4) {
                            Ok(n) => { index += 4; n as usize }
                            Err(_) => { f.write_str("<bad length>")?; break; }
                        }
                    }
                    _ => 0
                }
            };

            if index > 1 { f.write_str(" ")?; }
            // Write the opcode
            if opcode == opcodes::all::OP_PUSHBYTES_0 {
                f.write_str("OP_0")?;
            } else {
                write!(f, "{:?}", opcode)?;
            }
            // Write any pushdata
            if data_len > 0 {
                f.write_str(" ")?;
                if index + data_len <= self.0.len() {
                    for ch in &self.0[index..index + data_len] {
                            write!(f, "{:02x}", ch)?;
                    }
                    index += data_len;
                } else {
                    f.write_str("<push past end>")?;
                    break;
                }
            }
        }
        Ok(())
    }

    /// Get the assembly decoding of the script.
    pub fn asm(&self) -> String {
        let mut buf = String::new();
        self.fmt_asm(&mut buf).unwrap();
        buf
    }
}

/// Creates a new script from an existing vector
impl From<Vec<u8>> for Script {
    fn from(v: Vec<u8>) -> Script { Script(v.into_boxed_slice()) }
}

impl_index_newtype!(Script, u8);

/// A "parsed opcode" which allows iterating over a Script in a more sensible way
#[derive(Debug, PartialEq, Eq, Clone)]
pub enum Instruction<'a> {
    /// Push a bunch of data
    PushBytes(&'a [u8]),
    /// Some non-push opcode
    Op(opcodes::All),
    /// An opcode we were unable to parse
    Error(Error)
}

/// Iterator over a script returning parsed opcodes
pub struct Instructions<'a> {
    data: &'a [u8],
    enforce_minimal: bool,
}

impl<'a> Iterator for Instructions<'a> {
    type Item = Instruction<'a>;

    fn next(&mut self) -> Option<Instruction<'a>> {
        if self.data.is_empty() {
            return None;
        }

        match opcodes::All::from(self.data[0]).classify() {
            opcodes::Class::PushBytes(n) => {
                let n = n as usize;
                if self.data.len() < n + 1 {
                    self.data = &[];  // Kill iterator so that it does not return an infinite stream of errors
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                if self.enforce_minimal {
                    if n == 1 && (self.data[1] == 0x81 || (self.data[1] > 0 && self.data[1] <= 16)) {
                        self.data = &[];
                        return Some(Instruction::Error(Error::NonMinimalPush));
                    }
                }
                let ret = Some(Instruction::PushBytes(&self.data[1..n+1]));
                self.data = &self.data[n + 1..];
                ret
            }
            opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA1) => {
                if self.data.len() < 2 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                let n = match read_uint(&self.data[1..], 1) {
                    Ok(n) => n,
                    Err(e) => {
                        self.data = &[];
                        return Some(Instruction::Error(e));
                    }
                };
                if self.data.len() < n + 2 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                if self.enforce_minimal && n < 76 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::NonMinimalPush));
                }
                let ret = Some(Instruction::PushBytes(&self.data[2..n+2]));
                self.data = &self.data[n + 2..];
                ret
            }
            opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA2) => {
                if self.data.len() < 3 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                let n = match read_uint(&self.data[1..], 2) {
                    Ok(n) => n,
                    Err(e) => {
                        self.data = &[];
                        return Some(Instruction::Error(e));
                    }
                };
                if self.enforce_minimal && n < 0x100 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::NonMinimalPush));
                }
                if self.data.len() < n + 3 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                let ret = Some(Instruction::PushBytes(&self.data[3..n + 3]));
                self.data = &self.data[n + 3..];
                ret
            }
            opcodes::Class::Ordinary(opcodes::Ordinary::OP_PUSHDATA4) => {
                if self.data.len() < 5 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                let n = match read_uint(&self.data[1..], 4) {
                    Ok(n) => n,
                    Err(e) => {
                        self.data = &[];
                        return Some(Instruction::Error(e));
                    }
                };
                if self.enforce_minimal && n < 0x10000 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::NonMinimalPush));
                }
                if self.data.len() < n + 5 {
                    self.data = &[];
                    return Some(Instruction::Error(Error::EarlyEndOfScript));
                }
                let ret = Some(Instruction::PushBytes(&self.data[5..n + 5]));
                self.data = &self.data[n + 5..];
                ret
            }
            // Everything else we can push right through
            _ => {
                let ret = Some(Instruction::Op(opcodes::All::from(self.data[0])));
                self.data = &self.data[1..];
                ret
            }
        }
    }
}

impl Builder {
    /// Creates a new empty script
    pub fn new() -> Builder {
        Builder(vec![], None)
    }

    /// The length in bytes of the script
    pub fn len(&self) -> usize { self.0.len() }

    /// Whether the script is the empty script
    pub fn is_empty(&self) -> bool { self.0.is_empty() }

    /// Adds instructions to push an integer onto the stack. Integers are
    /// encoded as little-endian signed-magnitude numbers, but there are
    /// dedicated opcodes to push some small integers.
    pub fn push_int(self, data: i64) -> Builder {
        // We can special-case -1, 1-16
        if data == -1 || (data >= 1 && data <= 16) {
            let opcode = opcodes::All::from(
                (data - 1 + opcodes::OP_TRUE.into_u8() as i64) as u8
            );
            self.push_opcode(opcode)
        }
        // We can also special-case zero
        else if data == 0 {
            self.push_opcode(opcodes::OP_FALSE)
        }
        // Otherwise encode it as data
        else { self.push_scriptint(data) }
    }

    /// Adds instructions to push an integer onto the stack, using the explicit
    /// encoding regardless of the availability of dedicated opcodes.
    pub fn push_scriptint(self, data: i64) -> Builder {
        self.push_slice(&build_scriptint(data))
    }

    /// Adds instructions to push some arbitrary data onto the stack
    pub fn push_slice(mut self, data: &[u8]) -> Builder {
        // Start with a PUSH opcode
        match data.len() as u64 {
            n if n < opcodes::Ordinary::OP_PUSHDATA1 as u64 => { self.0.push(n as u8); },
            n if n < 0x100 => {
                self.0.push(opcodes::Ordinary::OP_PUSHDATA1.into_u8());
                self.0.push(n as u8);
            },
            n if n < 0x10000 => {
                self.0.push(opcodes::Ordinary::OP_PUSHDATA2.into_u8());
                self.0.push((n % 0x100) as u8);
                self.0.push((n / 0x100) as u8);
            },
            n if n < 0x100000000 => {
                self.0.push(opcodes::Ordinary::OP_PUSHDATA4.into_u8());
                self.0.push((n % 0x100) as u8);
                self.0.push(((n / 0x100) % 0x100) as u8);
                self.0.push(((n / 0x10000) % 0x100) as u8);
                self.0.push((n / 0x1000000) as u8);
            }
            _ => panic!("tried to put a 4bn+ sized object into a script!")
        }
        // Then push the raw bytes
        self.0.extend(data.iter().cloned());
        self.1 = None;
        self
    }

    /// Pushes a public key
    pub fn push_key(self, key: &PublicKey) -> Builder {
        if key.compressed {
            self.push_slice(&key.key.serialize()[..])
        } else {
            self.push_slice(&key.key.serialize_uncompressed()[..])
        }
    }

    /// Adds a single opcode to the script
    pub fn push_opcode(mut self, data: opcodes::All) -> Builder {
        self.0.push(data.into_u8());
        self.1 = Some(data);
        self
    }

    /// Adds an `OP_VERIFY` to the script, unless the most-recently-added
    /// opcode has an alternate `VERIFY` form, in which case that opcode
    /// is replaced. e.g. `OP_CHECKSIG` will become `OP_CHECKSIGVERIFY`.
    pub fn push_verify(mut self) -> Builder {
        match self.1 {
            Some(opcodes::all::OP_EQUAL) => {
                self.0.pop();
                self.push_opcode(opcodes::all::OP_EQUALVERIFY)
            },
            Some(opcodes::all::OP_NUMEQUAL) => {
                self.0.pop();
                self.push_opcode(opcodes::all::OP_NUMEQUALVERIFY)
            },
            Some(opcodes::all::OP_CHECKSIG) => {
                self.0.pop();
                self.push_opcode(opcodes::all::OP_CHECKSIGVERIFY)
            },
            Some(opcodes::all::OP_CHECKMULTISIG) => {
                self.0.pop();
                self.push_opcode(opcodes::all::OP_CHECKMULTISIGVERIFY)
            },
            _ => self.push_opcode(opcodes::all::OP_VERIFY),
        }
    }

    /// Converts the `Builder` into an unmodifiable `Script`
    pub fn into_script(self) -> Script {
        Script(self.0.into_boxed_slice())
    }
}

/// Adds an individual opcode to the script
impl Default for Builder {
    fn default() -> Builder { Builder::new() }
}

/// Creates a new script from an existing vector
impl From<Vec<u8>> for Builder {
    fn from(v: Vec<u8>) -> Builder {
        let script = Script(v.into_boxed_slice());
        let last_op = match script.iter(false).last() {
            Some(Instruction::Op(op)) => Some(op),
            _ => None,
        };
        Builder(script.into_bytes(), last_op)
    }
}

impl_index_newtype!(Builder, u8);

#[cfg(feature = "serde")]
impl<'de> serde::Deserialize<'de> for Script {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        use std::fmt::Formatter;

        struct Visitor;
        impl<'de> serde::de::Visitor<'de> for Visitor {
            type Value = Script;

            fn expecting(&self, formatter: &mut Formatter) -> fmt::Result {
                formatter.write_str("a script")
            }

            fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                let v: Vec<u8> = ::hex::decode(v).map_err(E::custom)?;
                Ok(Script::from(v))
            }

            fn visit_borrowed_str<E>(self, v: &'de str) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                self.visit_str(v)
            }

            fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                self.visit_str(&v)
            }
        }

        deserializer.deserialize_str(Visitor)
    }
}

#[cfg(feature = "serde")]
impl serde::Serialize for Script {
    /// User-facing serialization for `Script`.
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&format!("{:x}", self))
    }
}

// Network serialization
impl Encodable for Script {
    #[inline]
    fn consensus_encode<S: io::Write>(
        &self,
        s: S,
    ) -> Result<usize, encode::Error> {
        self.0.consensus_encode(s)
    }
}

impl Decodable for Script {
    #[inline]
    fn consensus_decode<D: io::Read>(d: D) -> Result<Self, encode::Error> {
        Ok(Script(Decodable::consensus_decode(d)?))
    }
}

#[cfg(test)]
mod test {
    use std::str::FromStr;
    use hex::decode as hex_decode;

    use super::*;
    use super::build_scriptint;

    use consensus::encode::{deserialize, serialize};
    use blockdata::opcodes;
    use util::key::PublicKey;

    #[test]
    fn script() {
        let mut comp = vec![];
        let mut script = Builder::new();
        assert_eq!(&script[..], &comp[..]);

        // small ints
        script = script.push_int(1);  comp.push(81u8); assert_eq!(&script[..], &comp[..]);
        script = script.push_int(0);  comp.push(0u8);  assert_eq!(&script[..], &comp[..]);
        script = script.push_int(4);  comp.push(84u8); assert_eq!(&script[..], &comp[..]);
        script = script.push_int(-1); comp.push(79u8); assert_eq!(&script[..], &comp[..]);
        // forced scriptint
        script = script.push_scriptint(4); comp.extend([1u8, 4].iter().cloned()); assert_eq!(&script[..], &comp[..]);
        // big ints
        script = script.push_int(17); comp.extend([1u8, 17].iter().cloned()); assert_eq!(&script[..], &comp[..]);
        script = script.push_int(10000); comp.extend([2u8, 16, 39].iter().cloned()); assert_eq!(&script[..], &comp[..]);
        // notice the sign bit set here, hence the extra zero/128 at the end
        script = script.push_int(10000000); comp.extend([4u8, 128, 150, 152, 0].iter().cloned()); assert_eq!(&script[..], &comp[..]);
        script = script.push_int(-10000000); comp.extend([4u8, 128, 150, 152, 128].iter().cloned()); assert_eq!(&script[..], &comp[..]);

        // data
        script = script.push_slice("NRA4VR".as_bytes()); comp.extend([6u8, 78, 82, 65, 52, 86, 82].iter().cloned()); assert_eq!(&script[..], &comp[..]);

        // keys
        let keystr = "21032e58afe51f9ed8ad3cc7897f634d881fdbe49a81564629ded8156bebd2ffd1af";
        let key = PublicKey::from_str(&keystr[2..]).unwrap();
        script = script.push_key(&key); comp.extend(hex_decode(keystr).unwrap().iter().cloned()); assert_eq!(&script[..], &comp[..]);
        let keystr = "41042e58afe51f9ed8ad3cc7897f634d881fdbe49a81564629ded8156bebd2ffd1af191923a2964c177f5b5923ae500fca49e99492d534aa3759d6b25a8bc971b133";
        let key = PublicKey::from_str(&keystr[2..]).unwrap();
        script = script.push_key(&key); comp.extend(hex_decode(keystr).unwrap().iter().cloned()); assert_eq!(&script[..], &comp[..]);

        // opcodes
        script = script.push_opcode(opcodes::all::OP_CHECKSIG); comp.push(0xACu8); assert_eq!(&script[..], &comp[..]);
        script = script.push_opcode(opcodes::all::OP_CHECKSIG); comp.push(0xACu8); assert_eq!(&script[..], &comp[..]);
    }

    #[test]
    fn script_builder() {
        // from txid 3bb5e6434c11fb93f64574af5d116736510717f2c595eb45b52c28e31622dfff which was in my mempool when I wrote the test
        let script = Builder::new().push_opcode(opcodes::all::OP_DUP)
                                   .push_opcode(opcodes::all::OP_HASH160)
                                   .push_slice(&hex_decode("16e1ae70ff0fa102905d4af297f6912bda6cce19").unwrap())
                                   .push_opcode(opcodes::all::OP_EQUALVERIFY)
                                   .push_opcode(opcodes::all::OP_CHECKSIG)
                                   .into_script();
        assert_eq!(&format!("{:x}", script), "76a91416e1ae70ff0fa102905d4af297f6912bda6cce1988ac");
    }

    #[test]
    fn script_builder_verify() {
        let simple = Builder::new()
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", simple), "69");
        let simple2 = Builder::from(vec![])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", simple2), "69");

        let nonverify = Builder::new()
            .push_verify()
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", nonverify), "6969");
        let nonverify2 = Builder::from(vec![0x69])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", nonverify2), "6969");

        let equal = Builder::new()
            .push_opcode(opcodes::all::OP_EQUAL)
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", equal), "88");
        let equal2 = Builder::from(vec![0x87])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", equal2), "88");

        let numequal = Builder::new()
            .push_opcode(opcodes::all::OP_NUMEQUAL)
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", numequal), "9d");
        let numequal2 = Builder::from(vec![0x9c])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", numequal2), "9d");

        let checksig = Builder::new()
            .push_opcode(opcodes::all::OP_CHECKSIG)
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", checksig), "ad");
        let checksig2 = Builder::from(vec![0xac])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", checksig2), "ad");

        let checkmultisig = Builder::new()
            .push_opcode(opcodes::all::OP_CHECKMULTISIG)
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", checkmultisig), "af");
        let checkmultisig2 = Builder::from(vec![0xae])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", checkmultisig2), "af");

        let trick_slice = Builder::new()
            .push_slice(&[0xae]) // OP_CHECKMULTISIG
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", trick_slice), "01ae69");
        let trick_slice2 = Builder::from(vec![0x01, 0xae])
            .push_verify()
            .into_script();
        assert_eq!(format!("{:x}", trick_slice2), "01ae69");
   }

    #[test]
    fn script_serialize() {
        let hex_script = hex_decode("6c493046022100f93bb0e7d8db7bd46e40132d1f8242026e045f03a0efe71bbb8e3f475e970d790221009337cd7f1f929f00cc6ff01f03729b069a7c21b59b1736ddfee5db5946c5da8c0121033b9b137ee87d5a812d6f506efdd37f0affa7ffc310711c06c7f3e097c9447c52").unwrap();
        let script: Result<Script, _> = deserialize(&hex_script);
        assert!(script.is_ok());
        assert_eq!(serialize(&script.unwrap()), hex_script);
    }

    #[test]
    fn scriptint_round_trip() {
        assert_eq!(build_scriptint(-1), vec![0x81]);
        assert_eq!(build_scriptint(255), vec![255, 0]);
        assert_eq!(build_scriptint(256), vec![0, 1]);
        assert_eq!(build_scriptint(257), vec![1, 1]);
        assert_eq!(build_scriptint(511), vec![255, 1]);
        for &i in [10, 100, 255, 256, 1000, 10000, 25000, 200000, 5000000, 1000000000,
                             (1 << 31) - 1, -((1 << 31) - 1)].iter() {
            assert_eq!(Ok(i), read_scriptint(&build_scriptint(i)));
            assert_eq!(Ok(-i), read_scriptint(&build_scriptint(-i)));
        }
        assert!(read_scriptint(&build_scriptint(1 << 31)).is_err());
        assert!(read_scriptint(&build_scriptint(-(1 << 31))).is_err());
    }

    #[test]
    fn provably_unspendable_test() {
        // p2pk
        assert_eq!(hex_script!("410446ef0102d1ec5240f0d061a4246c1bdef63fc3dbab7733052fbbf0ecd8f41fc26bf049ebb4f9527f374280259e7cfa99c48b0e3f39c51347a19a5819651503a5ac").is_provably_unspendable(), false);
        assert_eq!(hex_script!("4104ea1feff861b51fe3f5f8a3b12d0f4712db80e919548a80839fc47c6a21e66d957e9c5d8cd108c7a2d2324bad71f9904ac0ae7336507d785b17a2c115e427a32fac").is_provably_unspendable(), false);
        // p2pkhash
        assert_eq!(hex_script!("76a914ee61d57ab51b9d212335b1dba62794ac20d2bcf988ac").is_provably_unspendable(), false);
        assert_eq!(hex_script!("6aa9149eb21980dc9d413d8eac27314938b9da920ee53e87").is_provably_unspendable(), true);
    }

    #[test]
    fn op_return_test() {
        assert_eq!(hex_script!("6aa9149eb21980dc9d413d8eac27314938b9da920ee53e87").is_op_return(), true);
        assert_eq!(hex_script!("76a914ee61d57ab51b9d212335b1dba62794ac20d2bcf988ac").is_op_return(), false);
        assert_eq!(hex_script!("").is_op_return(), false);
    }

    #[test]
    #[cfg(feature = "serde")]
    fn script_json_serialize() {
        use serde_json;

        let original = hex_script!("827651a0698faaa9a8a7a687");
        let json = serde_json::to_value(&original).unwrap();
        assert_eq!(json, serde_json::Value::String("827651a0698faaa9a8a7a687".to_owned()));
        let des = serde_json::from_value(json).unwrap();
        assert_eq!(original, des);
    }

    #[test]
    fn script_asm() {
        assert_eq!(hex_script!("6363636363686868686800").asm(),
                   "OP_IF OP_IF OP_IF OP_IF OP_IF OP_ENDIF OP_ENDIF OP_ENDIF OP_ENDIF OP_ENDIF OP_0");
        assert_eq!(hex_script!("6363636363686868686800").asm(),
                   "OP_IF OP_IF OP_IF OP_IF OP_IF OP_ENDIF OP_ENDIF OP_ENDIF OP_ENDIF OP_ENDIF OP_0");
        assert_eq!(hex_script!("2102715e91d37d239dea832f1460e91e368115d8ca6cc23a7da966795abad9e3b699ac").asm(),
                   "OP_PUSHBYTES_33 02715e91d37d239dea832f1460e91e368115d8ca6cc23a7da966795abad9e3b699 OP_CHECKSIG");
        // Elements Alpha peg-out transaction with some signatures removed for brevity. Mainly to test PUSHDATA1
        assert_eq!(hex_script!("0047304402202457e78cc1b7f50d0543863c27de75d07982bde8359b9e3316adec0aec165f2f02200203fd331c4e4a4a02f48cf1c291e2c0d6b2f7078a784b5b3649fca41f8794d401004cf1552103244e602b46755f24327142a0517288cebd159eccb6ccf41ea6edf1f601e9af952103bbbacc302d19d29dbfa62d23f37944ae19853cf260c745c2bea739c95328fcb721039227e83246bd51140fe93538b2301c9048be82ef2fb3c7fc5d78426ed6f609ad210229bf310c379b90033e2ecb07f77ecf9b8d59acb623ab7be25a0caed539e2e6472103703e2ed676936f10b3ce9149fa2d4a32060fb86fa9a70a4efe3f21d7ab90611921031e9b7c6022400a6bb0424bbcde14cff6c016b91ee3803926f3440abf5c146d05210334667f975f55a8455d515a2ef1c94fdfa3315f12319a14515d2a13d82831f62f57ae").asm(),
                   "OP_0 OP_PUSHBYTES_71 304402202457e78cc1b7f50d0543863c27de75d07982bde8359b9e3316adec0aec165f2f02200203fd331c4e4a4a02f48cf1c291e2c0d6b2f7078a784b5b3649fca41f8794d401 OP_0 OP_PUSHDATA1 552103244e602b46755f24327142a0517288cebd159eccb6ccf41ea6edf1f601e9af952103bbbacc302d19d29dbfa62d23f37944ae19853cf260c745c2bea739c95328fcb721039227e83246bd51140fe93538b2301c9048be82ef2fb3c7fc5d78426ed6f609ad210229bf310c379b90033e2ecb07f77ecf9b8d59acb623ab7be25a0caed539e2e6472103703e2ed676936f10b3ce9149fa2d4a32060fb86fa9a70a4efe3f21d7ab90611921031e9b7c6022400a6bb0424bbcde14cff6c016b91ee3803926f3440abf5c146d05210334667f975f55a8455d515a2ef1c94fdfa3315f12319a14515d2a13d82831f62f57ae");
    }

    #[test]
    fn script_p2sh_p2p2k_template() {
        // random outputs I picked out of the mempool
        assert!(hex_script!("76a91402306a7c23f3e8010de41e9e591348bb83f11daa88ac").is_p2pkh());
        assert!(!hex_script!("76a91402306a7c23f3e8010de41e9e591348bb83f11daa88ac").is_p2sh());
        assert!(!hex_script!("76a91402306a7c23f3e8010de41e9e591348bb83f11daa88ad").is_p2pkh());
        assert!(!hex_script!("").is_p2pkh());
        assert!(hex_script!("a914acc91e6fef5c7f24e5c8b3f11a664aa8f1352ffd87").is_p2sh());
        assert!(!hex_script!("a914acc91e6fef5c7f24e5c8b3f11a664aa8f1352ffd87").is_p2pkh());
        assert!(!hex_script!("a314acc91e6fef5c7f24e5c8b3f11a664aa8f1352ffd87").is_p2sh());
    }

    #[test]
    fn script_p2pk() {
        assert!(hex_script!("21021aeaf2f8638a129a3156fbe7e5ef635226b0bafd495ff03afe2c843d7e3a4b51ac").is_p2pk());
        assert!(hex_script!("410496b538e853519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a604f8141781e62294721166bf621e73a82cbf2342c858eeac").is_p2pk());
    }

    #[test]
    fn p2sh_p2wsh_conversion() {
        // Test vectors taken from Core tests/data/script_tests.json
        // bare p2wsh
        let redeem_script = hex_script!("410479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8ac");
        let expected_witout = hex_script!("0020b95237b48faaa69eb078e1170be3b5cbb3fddf16d0a991e14ad274f7b33a4f64");
        assert!(redeem_script.to_v0_p2wsh().is_v0_p2wsh());
        assert_eq!(redeem_script.to_v0_p2wsh(), expected_witout);

        // p2sh
        let redeem_script = hex_script!("0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8");
        let expected_p2shout = hex_script!("a91491b24bf9f5288532960ac687abb035127b1d28a587");
        assert!(redeem_script.to_p2sh().is_p2sh());
        assert_eq!(redeem_script.to_p2sh(), expected_p2shout);

        // p2sh-p2wsh
        let redeem_script = hex_script!("410479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8ac");
        let expected_witout = hex_script!("0020b95237b48faaa69eb078e1170be3b5cbb3fddf16d0a991e14ad274f7b33a4f64");
        let expected_out = hex_script!("a914f386c2ba255cc56d20cfa6ea8b062f8b5994551887");
        assert!(redeem_script.to_p2sh().is_p2sh());
        assert!(redeem_script.to_p2sh().to_v0_p2wsh().is_v0_p2wsh());
        assert_eq!(redeem_script.to_v0_p2wsh(), expected_witout);
        assert_eq!(redeem_script.to_v0_p2wsh().to_p2sh(), expected_out);
    }

    #[test]
    fn test_iterator() {
        let zero = hex_script!("00");
        let zeropush = hex_script!("0100");

        let nonminimal = hex_script!("4c0169b2");      // PUSHDATA1 for no reason
        let minimal = hex_script!("0169b2");           // minimal
        let nonminimal_alt = hex_script!("026900b2");  // non-minimal number but minimal push (should be OK)

        let v_zero: Vec<Instruction> = zero.iter(true).collect();
        let v_zeropush: Vec<Instruction> = zeropush.iter(true).collect();

        let v_min: Vec<Instruction> = minimal.iter(true).collect();
        let v_nonmin: Vec<Instruction> = nonminimal.iter(true).collect();
        let v_nonmin_alt: Vec<Instruction> = nonminimal_alt.iter(true).collect();
        let slop_v_min: Vec<Instruction> = minimal.iter(false).collect();
        let slop_v_nonmin: Vec<Instruction> = nonminimal.iter(false).collect();
        let slop_v_nonmin_alt: Vec<Instruction> = nonminimal_alt.iter(false).collect();

        assert_eq!(
            v_zero,
            vec![
                Instruction::PushBytes(&[]),
            ]
        );
        assert_eq!(
            v_zeropush,
            vec![
                Instruction::PushBytes(&[0]),
            ]
        );

        assert_eq!(
            v_min,
            vec![
                Instruction::PushBytes(&[105]),
                Instruction::Op(opcodes::OP_NOP3),
            ]
        );

        assert_eq!(
            v_nonmin,
            vec![
                Instruction::Error(Error::NonMinimalPush),
            ]
        );

        assert_eq!(
            v_nonmin_alt,
            vec![
                Instruction::PushBytes(&[105, 0]),
                Instruction::Op(opcodes::OP_NOP3),
            ]
        );

        assert_eq!(v_min, slop_v_min);
        assert_eq!(v_min, slop_v_nonmin);
        assert_eq!(v_nonmin_alt, slop_v_nonmin_alt);
    }

	#[test]
    fn script_ord() {
        let script_1 = Builder::new().push_slice(&[1,2,3,4]).into_script();
        let script_2 = Builder::new().push_int(10).into_script();
        let script_3 = Builder::new().push_int(15).into_script();
        let script_4 = Builder::new().push_opcode(opcodes::all::OP_RETURN).into_script();

        assert!(script_1 < script_2);
        assert!(script_2 < script_3);
        assert!(script_3 < script_4);

        assert!(script_1 <= script_1);
        assert!(script_1 >= script_1);

        assert!(script_4 > script_3);
        assert!(script_3 > script_2);
        assert!(script_2 > script_1);
    }

	#[test]
	#[cfg(feature="bitcoinconsensus")]
	fn test_bitcoinconsensus () {
		// a random segwit transaction from the blockchain using native segwit
		let spent = Builder::from(hex_decode("0020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d").unwrap()).into_script();
		let spending = hex_decode("010000000001011f97548fbbe7a0db7588a66e18d803d0089315aa7d4cc28360b6ec50ef36718a0100000000ffffffff02df1776000000000017a9146c002a686959067f4866b8fb493ad7970290ab728757d29f0000000000220020701a8d401c84fb13e6baf169d59684e17abd9fa216c8cc5b9fc63d622ff8c58d04004730440220565d170eed95ff95027a69b313758450ba84a01224e1f7f130dda46e94d13f8602207bdd20e307f062594022f12ed5017bbf4a055a06aea91c10110a0e3bb23117fc014730440220647d2dc5b15f60bc37dc42618a370b2a1490293f9e5c8464f53ec4fe1dfe067302203598773895b4b16d37485cbe21b337f4e4b650739880098c592553add7dd4355016952210375e00eb72e29da82b89367947f29ef34afb75e8654f6ea368e0acdfd92976b7c2103a1b26313f430c4b15bb1fdce663207659d8cac749a0e53d70eff01874496feff2103c96d495bfdd5ba4145e3e046fee45e84a8a48ad05bd8dbb395c011a32cf9f88053ae00000000").unwrap();
		spent.verify(0, 18393430, spending.as_slice()).unwrap();
	}
}

