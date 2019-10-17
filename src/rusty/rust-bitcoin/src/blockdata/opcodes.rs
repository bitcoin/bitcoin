// Rust Bitcoin Library
// Written in 2014 by
//   Andrew Poelstra <apoelstra@wpsoftware.net>
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

//! Opcodes
//!
//! Bitcoin's script uses a stack-based assembly language. This module defines
//! all of the opcodes
//!

#![allow(non_camel_case_types)]

#[cfg(feature = "serde")] use serde;

use std::fmt;

// Note: I am deliberately not implementing PartialOrd or Ord on the
//       opcode enum. If you want to check ranges of opcodes, etc.,
//       write an #[inline] helper function which casts to u8s.

/// A script Opcode
#[derive(Copy, Clone, PartialEq, Eq)]
pub struct All {
    code: u8,
}

pub mod all {
    //! Constants associated with All type
    use super::All;

    /// Push an empty array onto the stack
    pub const OP_PUSHBYTES_0: All = All {code: 0x00};
    /// Push the next byte as an array onto the stack
    pub const OP_PUSHBYTES_1: All = All {code: 0x01};
    /// Push the next 2 bytes as an array onto the stack
    pub const OP_PUSHBYTES_2: All = All {code: 0x02};
    /// Push the next 2 bytes as an array onto the stack
    pub const OP_PUSHBYTES_3: All = All {code: 0x03};
    /// Push the next 4 bytes as an array onto the stack
    pub const OP_PUSHBYTES_4: All = All {code: 0x04};
    /// Push the next 5 bytes as an array onto the stack
    pub const OP_PUSHBYTES_5: All = All {code: 0x05};
    /// Push the next 6 bytes as an array onto the stack
    pub const OP_PUSHBYTES_6: All = All {code: 0x06};
    /// Push the next 7 bytes as an array onto the stack
    pub const OP_PUSHBYTES_7: All = All {code: 0x07};
    /// Push the next 8 bytes as an array onto the stack
    pub const OP_PUSHBYTES_8: All = All {code: 0x08};
    /// Push the next 9 bytes as an array onto the stack
    pub const OP_PUSHBYTES_9: All = All {code: 0x09};
    /// Push the next 10 bytes as an array onto the stack
    pub const OP_PUSHBYTES_10: All = All {code: 0x0a};
    /// Push the next 11 bytes as an array onto the stack
    pub const OP_PUSHBYTES_11: All = All {code: 0x0b};
    /// Push the next 12 bytes as an array onto the stack
    pub const OP_PUSHBYTES_12: All = All {code: 0x0c};
    /// Push the next 13 bytes as an array onto the stack
    pub const OP_PUSHBYTES_13: All = All {code: 0x0d};
    /// Push the next 14 bytes as an array onto the stack
    pub const OP_PUSHBYTES_14: All = All {code: 0x0e};
    /// Push the next 15 bytes as an array onto the stack
    pub const OP_PUSHBYTES_15: All = All {code: 0x0f};
    /// Push the next 16 bytes as an array onto the stack
    pub const OP_PUSHBYTES_16: All = All {code: 0x10};
    /// Push the next 17 bytes as an array onto the stack
    pub const OP_PUSHBYTES_17: All = All {code: 0x11};
    /// Push the next 18 bytes as an array onto the stack
    pub const OP_PUSHBYTES_18: All = All {code: 0x12};
    /// Push the next 19 bytes as an array onto the stack
    pub const OP_PUSHBYTES_19: All = All {code: 0x13};
    /// Push the next 20 bytes as an array onto the stack
    pub const OP_PUSHBYTES_20: All = All {code: 0x14};
    /// Push the next 21 bytes as an array onto the stack
    pub const OP_PUSHBYTES_21: All = All {code: 0x15};
    /// Push the next 22 bytes as an array onto the stack
    pub const OP_PUSHBYTES_22: All = All {code: 0x16};
    /// Push the next 23 bytes as an array onto the stack
    pub const OP_PUSHBYTES_23: All = All {code: 0x17};
    /// Push the next 24 bytes as an array onto the stack
    pub const OP_PUSHBYTES_24: All = All {code: 0x18};
    /// Push the next 25 bytes as an array onto the stack
    pub const OP_PUSHBYTES_25: All = All {code: 0x19};
    /// Push the next 26 bytes as an array onto the stack
    pub const OP_PUSHBYTES_26: All = All {code: 0x1a};
    /// Push the next 27 bytes as an array onto the stack
    pub const OP_PUSHBYTES_27: All = All {code: 0x1b};
    /// Push the next 28 bytes as an array onto the stack
    pub const OP_PUSHBYTES_28: All = All {code: 0x1c};
    /// Push the next 29 bytes as an array onto the stack
    pub const OP_PUSHBYTES_29: All = All {code: 0x1d};
    /// Push the next 30 bytes as an array onto the stack
    pub const OP_PUSHBYTES_30: All = All {code: 0x1e};
    /// Push the next 31 bytes as an array onto the stack
    pub const OP_PUSHBYTES_31: All = All {code: 0x1f};
    /// Push the next 32 bytes as an array onto the stack
    pub const OP_PUSHBYTES_32: All = All {code: 0x20};
    /// Push the next 33 bytes as an array onto the stack
    pub const OP_PUSHBYTES_33: All = All {code: 0x21};
    /// Push the next 34 bytes as an array onto the stack
    pub const OP_PUSHBYTES_34: All = All {code: 0x22};
    /// Push the next 35 bytes as an array onto the stack
    pub const OP_PUSHBYTES_35: All = All {code: 0x23};
    /// Push the next 36 bytes as an array onto the stack
    pub const OP_PUSHBYTES_36: All = All {code: 0x24};
    /// Push the next 37 bytes as an array onto the stack
    pub const OP_PUSHBYTES_37: All = All {code: 0x25};
    /// Push the next 38 bytes as an array onto the stack
    pub const OP_PUSHBYTES_38: All = All {code: 0x26};
    /// Push the next 39 bytes as an array onto the stack
    pub const OP_PUSHBYTES_39: All = All {code: 0x27};
    /// Push the next 40 bytes as an array onto the stack
    pub const OP_PUSHBYTES_40: All = All {code: 0x28};
    /// Push the next 41 bytes as an array onto the stack
    pub const OP_PUSHBYTES_41: All = All {code: 0x29};
    /// Push the next 42 bytes as an array onto the stack
    pub const OP_PUSHBYTES_42: All = All {code: 0x2a};
    /// Push the next 43 bytes as an array onto the stack
    pub const OP_PUSHBYTES_43: All = All {code: 0x2b};
    /// Push the next 44 bytes as an array onto the stack
    pub const OP_PUSHBYTES_44: All = All {code: 0x2c};
    /// Push the next 45 bytes as an array onto the stack
    pub const OP_PUSHBYTES_45: All = All {code: 0x2d};
    /// Push the next 46 bytes as an array onto the stack
    pub const OP_PUSHBYTES_46: All = All {code: 0x2e};
    /// Push the next 47 bytes as an array onto the stack
    pub const OP_PUSHBYTES_47: All = All {code: 0x2f};
    /// Push the next 48 bytes as an array onto the stack
    pub const OP_PUSHBYTES_48: All = All {code: 0x30};
    /// Push the next 49 bytes as an array onto the stack
    pub const OP_PUSHBYTES_49: All = All {code: 0x31};
    /// Push the next 50 bytes as an array onto the stack
    pub const OP_PUSHBYTES_50: All = All {code: 0x32};
    /// Push the next 51 bytes as an array onto the stack
    pub const OP_PUSHBYTES_51: All = All {code: 0x33};
    /// Push the next 52 bytes as an array onto the stack
    pub const OP_PUSHBYTES_52: All = All {code: 0x34};
    /// Push the next 53 bytes as an array onto the stack
    pub const OP_PUSHBYTES_53: All = All {code: 0x35};
    /// Push the next 54 bytes as an array onto the stack
    pub const OP_PUSHBYTES_54: All = All {code: 0x36};
    /// Push the next 55 bytes as an array onto the stack
    pub const OP_PUSHBYTES_55: All = All {code: 0x37};
    /// Push the next 56 bytes as an array onto the stack
    pub const OP_PUSHBYTES_56: All = All {code: 0x38};
    /// Push the next 57 bytes as an array onto the stack
    pub const OP_PUSHBYTES_57: All = All {code: 0x39};
    /// Push the next 58 bytes as an array onto the stack
    pub const OP_PUSHBYTES_58: All = All {code: 0x3a};
    /// Push the next 59 bytes as an array onto the stack
    pub const OP_PUSHBYTES_59: All = All {code: 0x3b};
    /// Push the next 60 bytes as an array onto the stack
    pub const OP_PUSHBYTES_60: All = All {code: 0x3c};
    /// Push the next 61 bytes as an array onto the stack
    pub const OP_PUSHBYTES_61: All = All {code: 0x3d};
    /// Push the next 62 bytes as an array onto the stack
    pub const OP_PUSHBYTES_62: All = All {code: 0x3e};
    /// Push the next 63 bytes as an array onto the stack
    pub const OP_PUSHBYTES_63: All = All {code: 0x3f};
    /// Push the next 64 bytes as an array onto the stack
    pub const OP_PUSHBYTES_64: All = All {code: 0x40};
    /// Push the next 65 bytes as an array onto the stack
    pub const OP_PUSHBYTES_65: All = All {code: 0x41};
    /// Push the next 66 bytes as an array onto the stack
    pub const OP_PUSHBYTES_66: All = All {code: 0x42};
    /// Push the next 67 bytes as an array onto the stack
    pub const OP_PUSHBYTES_67: All = All {code: 0x43};
    /// Push the next 68 bytes as an array onto the stack
    pub const OP_PUSHBYTES_68: All = All {code: 0x44};
    /// Push the next 69 bytes as an array onto the stack
    pub const OP_PUSHBYTES_69: All = All {code: 0x45};
    /// Push the next 70 bytes as an array onto the stack
    pub const OP_PUSHBYTES_70: All = All {code: 0x46};
    /// Push the next 71 bytes as an array onto the stack
    pub const OP_PUSHBYTES_71: All = All {code: 0x47};
    /// Push the next 72 bytes as an array onto the stack
    pub const OP_PUSHBYTES_72: All = All {code: 0x48};
    /// Push the next 73 bytes as an array onto the stack
    pub const OP_PUSHBYTES_73: All = All {code: 0x49};
    /// Push the next 74 bytes as an array onto the stack
    pub const OP_PUSHBYTES_74: All = All {code: 0x4a};
    /// Push the next 75 bytes as an array onto the stack
    pub const OP_PUSHBYTES_75: All = All {code: 0x4b};
    /// Read the next byte as N; push the next N bytes as an array onto the stack
    pub const OP_PUSHDATA1: All = All {code: 0x4c};
    /// Read the next 2 bytes as N; push the next N bytes as an array onto the stack
    pub const OP_PUSHDATA2: All = All {code: 0x4d};
    /// Read the next 4 bytes as N; push the next N bytes as an array onto the stack
    pub const OP_PUSHDATA4: All = All {code: 0x4e};
    /// Push the array [0x81] onto the stack
    pub const OP_PUSHNUM_NEG1: All = All {code: 0x4f};
    /// Synonym for OP_RETURN
    pub const OP_RESERVED: All = All {code: 0x50};
    /// Push the array [0x01] onto the stack
    pub const OP_PUSHNUM_1: All = All {code: 0x51};
    /// Push the array [0x02] onto the stack
    pub const OP_PUSHNUM_2: All = All {code: 0x52};
    /// Push the array [0x03] onto the stack
    pub const OP_PUSHNUM_3: All = All {code: 0x53};
    /// Push the array [0x04] onto the stack
    pub const OP_PUSHNUM_4: All = All {code: 0x54};
    /// Push the array [0x05] onto the stack
    pub const OP_PUSHNUM_5: All = All {code: 0x55};
    /// Push the array [0x06] onto the stack
    pub const OP_PUSHNUM_6: All = All {code: 0x56};
    /// Push the array [0x07] onto the stack
    pub const OP_PUSHNUM_7: All = All {code: 0x57};
    /// Push the array [0x08] onto the stack
    pub const OP_PUSHNUM_8: All = All {code: 0x58};
    /// Push the array [0x09] onto the stack
    pub const OP_PUSHNUM_9: All = All {code: 0x59};
    /// Push the array [0x0a] onto the stack
    pub const OP_PUSHNUM_10: All = All {code: 0x5a};
    /// Push the array [0x0b] onto the stack
    pub const OP_PUSHNUM_11: All = All {code: 0x5b};
    /// Push the array [0x0c] onto the stack
    pub const OP_PUSHNUM_12: All = All {code: 0x5c};
    /// Push the array [0x0d] onto the stack
    pub const OP_PUSHNUM_13: All = All {code: 0x5d};
    /// Push the array [0x0e] onto the stack
    pub const OP_PUSHNUM_14: All = All {code: 0x5e};
    /// Push the array [0x0f] onto the stack
    pub const OP_PUSHNUM_15: All = All {code: 0x5f};
    /// Push the array [0x10] onto the stack
    pub const OP_PUSHNUM_16: All = All {code: 0x60};
    /// Does nothing
    pub const OP_NOP: All = All {code: 0x61};
    /// Synonym for OP_RETURN
    pub const OP_VER: All = All {code: 0x62};
    /// Pop and execute the next statements if a nonzero element was popped
    pub const OP_IF: All = All {code: 0x63};
    /// Pop and execute the next statements if a zero element was popped
    pub const OP_NOTIF: All = All {code: 0x64};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_VERIF: All = All {code: 0x65};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_VERNOTIF: All = All {code: 0x66};
    /// Execute statements if those after the previous OP_IF were not, and vice-versa.
    /// If there is no previous OP_IF, this acts as a RETURN.
    pub const OP_ELSE: All = All {code: 0x67};
    /// Pop and execute the next statements if a zero element was popped
    pub const OP_ENDIF: All = All {code: 0x68};
    /// If the top value is zero or the stack is empty, fail; otherwise, pop the stack
    pub const OP_VERIFY: All = All {code: 0x69};
    /// Fail the script immediately. (Must be executed.)
    pub const OP_RETURN: All = All {code: 0x6a};
    /// Pop one element from the main stack onto the alt stack
    pub const OP_TOALTSTACK: All = All {code: 0x6b};
    /// Pop one element from the alt stack onto the main stack
    pub const OP_FROMALTSTACK: All = All {code: 0x6c};
    /// Drops the top two stack items
    pub const OP_2DROP: All = All {code: 0x6d};
    /// Duplicates the top two stack items as AB -> ABAB
    pub const OP_2DUP: All = All {code: 0x6e};
    /// Duplicates the two three stack items as ABC -> ABCABC
    pub const OP_3DUP: All = All {code: 0x6f};
    /// Copies the two stack items of items two spaces back to
    /// the front, as xxAB -> ABxxAB
    pub const OP_2OVER: All = All {code: 0x70};
    /// Moves the two stack items four spaces back to the front,
    /// as xxxxAB -> ABxxxx
    pub const OP_2ROT: All = All {code: 0x71};
    /// Swaps the top two pairs, as ABCD -> CDAB
    pub const OP_2SWAP: All = All {code: 0x72};
    /// Duplicate the top stack element unless it is zero
    pub const OP_IFDUP: All = All {code: 0x73};
    /// Push the current number of stack items onto the stack
    pub const OP_DEPTH: All = All {code: 0x74};
    /// Drops the top stack item
    pub const OP_DROP: All = All {code: 0x75};
    /// Duplicates the top stack item
    pub const OP_DUP: All = All {code: 0x76};
    /// Drops the second-to-top stack item
    pub const OP_NIP: All = All {code: 0x77};
    /// Copies the second-to-top stack item, as xA -> AxA
    pub const OP_OVER: All = All {code: 0x78};
    /// Pop the top stack element as N. Copy the Nth stack element to the top
    pub const OP_PICK: All = All {code: 0x79};
    /// Pop the top stack element as N. Move the Nth stack element to the top
    pub const OP_ROLL: All = All {code: 0x7a};
    /// Rotate the top three stack items, as [top next1 next2] -> [next2 top next1]
    pub const OP_ROT: All = All {code: 0x7b};
    /// Swap the top two stack items
    pub const OP_SWAP: All = All {code: 0x7c};
    /// Copy the top stack item to before the second item, as [top next] -> [top next top]
    pub const OP_TUCK: All = All {code: 0x7d};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_CAT: All = All {code: 0x7e};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_SUBSTR: All = All {code: 0x7f};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_LEFT: All = All {code: 0x80};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_RIGHT: All = All {code: 0x81};
    /// Pushes the length of the top stack item onto the stack
    pub const OP_SIZE: All = All {code: 0x82};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_INVERT: All = All {code: 0x83};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_AND: All = All {code: 0x84};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_OR: All = All {code: 0x85};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_XOR: All = All {code: 0x86};
    /// Pushes 1 if the inputs are exactly equal, 0 otherwise
    pub const OP_EQUAL: All = All {code: 0x87};
    /// Returns success if the inputs are exactly equal, failure otherwise
    pub const OP_EQUALVERIFY: All = All {code: 0x88};
    /// Synonym for OP_RETURN
    pub const OP_RESERVED1: All = All {code: 0x89};
    /// Synonym for OP_RETURN
    pub const OP_RESERVED2: All = All {code: 0x8a};
    /// Increment the top stack element in place
    pub const OP_1ADD: All = All {code: 0x8b};
    /// Decrement the top stack element in place
    pub const OP_1SUB: All = All {code: 0x8c};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_2MUL: All = All {code: 0x8d};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_2DIV: All = All {code: 0x8e};
    /// Multiply the top stack item by -1 in place
    pub const OP_NEGATE: All = All {code: 0x8f};
    /// Absolute value the top stack item in place
    pub const OP_ABS: All = All {code: 0x90};
    /// Map 0 to 1 and everything else to 0, in place
    pub const OP_NOT: All = All {code: 0x91};
    /// Map 0 to 0 and everything else to 1, in place
    pub const OP_0NOTEQUAL: All = All {code: 0x92};
    /// Pop two stack items and push their sum
    pub const OP_ADD: All = All {code: 0x93};
    /// Pop two stack items and push the second minus the top
    pub const OP_SUB: All = All {code: 0x94};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_MUL: All = All {code: 0x95};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_DIV: All = All {code: 0x96};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_MOD: All = All {code: 0x97};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_LSHIFT: All = All {code: 0x98};
    /// Fail the script unconditionally, does not even need to be executed
    pub const OP_RSHIFT: All = All {code: 0x99};
    /// Pop the top two stack items and push 1 if both are nonzero, else push 0
    pub const OP_BOOLAND: All = All {code: 0x9a};
    /// Pop the top two stack items and push 1 if either is nonzero, else push 0
    pub const OP_BOOLOR: All = All {code: 0x9b};
    /// Pop the top two stack items and push 1 if both are numerically equal, else push 0
    pub const OP_NUMEQUAL: All = All {code: 0x9c};
    /// Pop the top two stack items and return success if both are numerically equal, else return failure
    pub const OP_NUMEQUALVERIFY: All = All {code: 0x9d};
    /// Pop the top two stack items and push 0 if both are numerically equal, else push 1
    pub const OP_NUMNOTEQUAL: All = All {code: 0x9e};
    /// Pop the top two items; push 1 if the second is less than the top, 0 otherwise
    pub const OP_LESSTHAN : All = All {code: 0x9f};
    /// Pop the top two items; push 1 if the second is greater than the top, 0 otherwise
    pub const OP_GREATERTHAN : All = All {code: 0xa0};
    /// Pop the top two items; push 1 if the second is <= the top, 0 otherwise
    pub const OP_LESSTHANOREQUAL : All = All {code: 0xa1};
    /// Pop the top two items; push 1 if the second is >= the top, 0 otherwise
    pub const OP_GREATERTHANOREQUAL : All = All {code: 0xa2};
    /// Pop the top two items; push the smaller
    pub const OP_MIN: All = All {code: 0xa3};
    /// Pop the top two items; push the larger
    pub const OP_MAX: All = All {code: 0xa4};
    /// Pop the top three items; if the top is >= the second and < the third, push 1, otherwise push 0
    pub const OP_WITHIN: All = All {code: 0xa5};
    /// Pop the top stack item and push its RIPEMD160 hash
    pub const OP_RIPEMD160: All = All {code: 0xa6};
    /// Pop the top stack item and push its SHA1 hash
    pub const OP_SHA1: All = All {code: 0xa7};
    /// Pop the top stack item and push its SHA256 hash
    pub const OP_SHA256: All = All {code: 0xa8};
    /// Pop the top stack item and push its RIPEMD(SHA256) hash
    pub const OP_HASH160: All = All {code: 0xa9};
    /// Pop the top stack item and push its SHA256(SHA256) hash
    pub const OP_HASH256: All = All {code: 0xaa};
    /// Ignore this and everything preceding when deciding what to sign when signature-checking
    pub const OP_CODESEPARATOR: All = All {code: 0xab};
    /// https://en.bitcoin.it/wiki/OP_CHECKSIG pushing 1/0 for success/failure
    pub const OP_CHECKSIG: All = All {code: 0xac};
    /// https://en.bitcoin.it/wiki/OP_CHECKSIG returning success/failure
    pub const OP_CHECKSIGVERIFY: All = All {code: 0xad};
    /// Pop N, N pubkeys, M, M signatures, a dummy (due to bug in reference code), and verify that all M signatures are valid.
    /// Push 1 for "all valid", 0 otherwise
    pub const OP_CHECKMULTISIG: All = All {code: 0xae};
    /// Like the above but return success/failure
    pub const OP_CHECKMULTISIGVERIFY: All = All {code: 0xaf};
    /// Does nothing
    pub const OP_NOP1: All = All {code: 0xb0};
    /// https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki
    pub const OP_CLTV: All = All {code: 0xb1};
    /// https://github.com/bitcoin/bips/blob/master/bip-0112.mediawiki
    pub const OP_CSV: All = All {code: 0xb2};
    /// Does nothing
    pub const OP_NOP4: All = All {code: 0xb3};
    /// Does nothing
    pub const OP_NOP5: All = All {code: 0xb4};
    /// Does nothing
    pub const OP_NOP6: All = All {code: 0xb5};
    /// Does nothing
    pub const OP_NOP7: All = All {code: 0xb6};
    /// Does nothing
    pub const OP_NOP8: All = All {code: 0xb7};
    /// Does nothing
    pub const OP_NOP9: All = All {code: 0xb8};
    /// Does nothing
    pub const OP_NOP10: All = All {code: 0xb9};
    // Every other opcode acts as OP_RETURN
    /// Synonym for OP_RETURN
    pub const OP_RETURN_186: All = All {code: 0xba};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_187: All = All {code: 0xbb};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_188: All = All {code: 0xbc};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_189: All = All {code: 0xbd};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_190: All = All {code: 0xbe};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_191: All = All {code: 0xbf};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_192: All = All {code: 0xc0};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_193: All = All {code: 0xc1};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_194: All = All {code: 0xc2};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_195: All = All {code: 0xc3};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_196: All = All {code: 0xc4};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_197: All = All {code: 0xc5};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_198: All = All {code: 0xc6};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_199: All = All {code: 0xc7};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_200: All = All {code: 0xc8};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_201: All = All {code: 0xc9};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_202: All = All {code: 0xca};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_203: All = All {code: 0xcb};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_204: All = All {code: 0xcc};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_205: All = All {code: 0xcd};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_206: All = All {code: 0xce};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_207: All = All {code: 0xcf};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_208: All = All {code: 0xd0};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_209: All = All {code: 0xd1};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_210: All = All {code: 0xd2};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_211: All = All {code: 0xd3};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_212: All = All {code: 0xd4};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_213: All = All {code: 0xd5};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_214: All = All {code: 0xd6};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_215: All = All {code: 0xd7};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_216: All = All {code: 0xd8};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_217: All = All {code: 0xd9};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_218: All = All {code: 0xda};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_219: All = All {code: 0xdb};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_220: All = All {code: 0xdc};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_221: All = All {code: 0xdd};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_222: All = All {code: 0xde};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_223: All = All {code: 0xdf};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_224: All = All {code: 0xe0};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_225: All = All {code: 0xe1};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_226: All = All {code: 0xe2};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_227: All = All {code: 0xe3};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_228: All = All {code: 0xe4};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_229: All = All {code: 0xe5};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_230: All = All {code: 0xe6};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_231: All = All {code: 0xe7};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_232: All = All {code: 0xe8};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_233: All = All {code: 0xe9};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_234: All = All {code: 0xea};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_235: All = All {code: 0xeb};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_236: All = All {code: 0xec};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_237: All = All {code: 0xed};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_238: All = All {code: 0xee};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_239: All = All {code: 0xef};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_240: All = All {code: 0xf0};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_241: All = All {code: 0xf1};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_242: All = All {code: 0xf2};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_243: All = All {code: 0xf3};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_244: All = All {code: 0xf4};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_245: All = All {code: 0xf5};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_246: All = All {code: 0xf6};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_247: All = All {code: 0xf7};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_248: All = All {code: 0xf8};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_249: All = All {code: 0xf9};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_250: All = All {code: 0xfa};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_251: All = All {code: 0xfb};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_252: All = All {code: 0xfc};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_253: All = All {code: 0xfd};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_254: All = All {code: 0xfe};
    /// Synonym for OP_RETURN
    pub const OP_RETURN_255: All = All {code: 0xff};
}

impl fmt::Debug for All {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.write_str("OP_")?;
        match *self {
            All {code: x} if x <= 75 => write!(f, "PUSHBYTES_{}", self.code),
            all::OP_PUSHDATA1 => write!(f, "PUSHDATA1"),
            all::OP_PUSHDATA2 => write!(f, "PUSHDATA2"),
            all::OP_PUSHDATA4 => write!(f, "PUSHDATA4"),
            all::OP_PUSHNUM_NEG1 => write!(f, "PUSHNUM_NEG1"),
            all::OP_RESERVED => write!(f, "RESERVED"),
            All {code: x} if x >= all::OP_PUSHNUM_1.code && x <= all::OP_PUSHNUM_16.code => write!(f, "PUSHNUM_{}", x - all::OP_PUSHNUM_1.code + 1),
            all::OP_NOP => write!(f, "NOP"),
            all::OP_VER => write!(f, "VER"),
            all::OP_IF => write!(f, "IF"),
            all::OP_NOTIF => write!(f, "NOTIF"),
            all::OP_VERIF => write!(f, "VERIF"),
            all::OP_VERNOTIF => write!(f, "VERNOTIF"),
            all::OP_ELSE => write!(f, "ELSE"),
            all::OP_ENDIF => write!(f, "ENDIF"),
            all::OP_VERIFY => write!(f, "VERIFY"),
            all::OP_RETURN => write!(f, "RETURN"),
            all::OP_TOALTSTACK => write!(f, "TOALTSTACK"),
            all::OP_FROMALTSTACK => write!(f, "FROMALTSTACK"),
            all::OP_2DROP => write!(f, "2DROP"),
            all::OP_2DUP => write!(f, "2DUP"),
            all::OP_3DUP => write!(f, "3DUP"),
            all::OP_2OVER => write!(f, "2OVER"),
            all::OP_2ROT => write!(f, "2ROT"),
            all::OP_2SWAP => write!(f, "2SWAP"),
            all::OP_IFDUP => write!(f, "IFDUP"),
            all::OP_DEPTH => write!(f, "DEPTH"),
            all::OP_DROP => write!(f, "DROP"),
            all::OP_DUP => write!(f, "DUP"),
            all::OP_NIP => write!(f, "NIP"),
            all::OP_OVER => write!(f, "OVER"),
            all::OP_PICK => write!(f, "PICK"),
            all::OP_ROLL => write!(f, "ROLL"),
            all::OP_ROT => write!(f, "ROT"),
            all::OP_SWAP => write!(f, "SWAP"),
            all::OP_TUCK => write!(f, "TUCK"),
            all::OP_CAT => write!(f, "CAT"),
            all::OP_SUBSTR => write!(f, "SUBSTR"),
            all::OP_LEFT => write!(f, "LEFT"),
            all::OP_RIGHT => write!(f, "RIGHT"),
            all::OP_SIZE => write!(f, "SIZE"),
            all::OP_INVERT => write!(f, "INVERT"),
            all::OP_AND => write!(f, "AND"),
            all::OP_OR => write!(f, "OR"),
            all::OP_XOR => write!(f, "XOR"),
            all::OP_EQUAL => write!(f, "EQUAL"),
            all::OP_EQUALVERIFY => write!(f, "EQUALVERIFY"),
            all::OP_RESERVED1 => write!(f, "RESERVED1"),
            all::OP_RESERVED2 => write!(f, "RESERVED2"),
            all::OP_1ADD => write!(f, "1ADD"),
            all::OP_1SUB => write!(f, "1SUB"),
            all::OP_2MUL => write!(f, "2MUL"),
            all::OP_2DIV => write!(f, "2DIV"),
            all::OP_NEGATE => write!(f, "NEGATE"),
            all::OP_ABS => write!(f, "ABS"),
            all::OP_NOT => write!(f, "NOT"),
            all::OP_0NOTEQUAL => write!(f, "0NOTEQUAL"),
            all::OP_ADD => write!(f, "ADD"),
            all::OP_SUB => write!(f, "SUB"),
            all::OP_MUL => write!(f, "MUL"),
            all::OP_DIV => write!(f, "DIV"),
            all::OP_MOD => write!(f, "MOD"),
            all::OP_LSHIFT => write!(f, "LSHIFT"),
            all::OP_RSHIFT => write!(f, "RSHIFT"),
            all::OP_BOOLAND => write!(f, "BOOLAND"),
            all::OP_BOOLOR => write!(f, "BOOLOR"),
            all::OP_NUMEQUAL => write!(f, "NUMEQUAL"),
            all::OP_NUMEQUALVERIFY => write!(f, "NUMEQUALVERIFY"),
            all::OP_NUMNOTEQUAL => write!(f, "NUMNOTEQUAL"),
            all::OP_LESSTHAN  => write!(f, "LESSTHAN"),
            all::OP_GREATERTHAN  => write!(f, "GREATERTHAN"),
            all::OP_LESSTHANOREQUAL  => write!(f, "LESSTHANOREQUAL"),
            all::OP_GREATERTHANOREQUAL  => write!(f, "GREATERTHANOREQUAL"),
            all::OP_MIN => write!(f, "MIN"),
            all::OP_MAX => write!(f, "MAX"),
            all::OP_WITHIN => write!(f, "WITHIN"),
            all::OP_RIPEMD160 => write!(f, "RIPEMD160"),
            all::OP_SHA1 => write!(f, "SHA1"),
            all::OP_SHA256 => write!(f, "SHA256"),
            all::OP_HASH160 => write!(f, "HASH160"),
            all::OP_HASH256 => write!(f, "HASH256"),
            all::OP_CODESEPARATOR => write!(f, "CODESEPARATOR"),
            all::OP_CHECKSIG => write!(f, "CHECKSIG"),
            all::OP_CHECKSIGVERIFY => write!(f, "CHECKSIGVERIFY"),
            all::OP_CHECKMULTISIG => write!(f, "CHECKMULTISIG"),
            all::OP_CHECKMULTISIGVERIFY => write!(f, "CHECKMULTISIGVERIFY"),
            all::OP_CLTV => write!(f, "CLTV"),
            all::OP_CSV => write!(f, "CSV"),
            All {code: x} if x >= all::OP_NOP1.code && x <= all::OP_NOP10.code => write!(f, "NOP{}", x - all::OP_NOP1.code + 1),
            All {code: x} => write!(f, "RETURN_{}", x),
        }
    }
}

impl All {
    /// Classifies an Opcode into a broad class
    #[inline]
    pub fn classify(&self) -> Class {
        // 17 opcodes
        if *self == all::OP_VERIF || *self == all::OP_VERNOTIF ||
           *self == all::OP_CAT || *self == all::OP_SUBSTR ||
           *self == all::OP_LEFT || *self == all::OP_RIGHT ||
           *self == all::OP_INVERT || *self == all::OP_AND ||
           *self == all::OP_OR || *self == all::OP_XOR ||
           *self == all::OP_2MUL || *self == all::OP_2DIV ||
           *self == all::OP_MUL || *self == all::OP_DIV || *self == all::OP_MOD ||
           *self == all::OP_LSHIFT || *self == all::OP_RSHIFT {
            Class::IllegalOp
        // 11 opcodes
        } else if *self == all::OP_NOP ||
                  (all::OP_NOP1.code <= self.code &&
                   self.code <= all::OP_NOP10.code) {
            Class::NoOp
        // 75 opcodes
        } else if *self == all::OP_RESERVED || *self == all::OP_VER || *self == all::OP_RETURN ||
                  *self == all::OP_RESERVED1 || *self == all::OP_RESERVED2 ||
                  self.code >= all::OP_RETURN_186.code {
            Class::ReturnOp
        // 1 opcode
        } else if *self == all::OP_PUSHNUM_NEG1 {
            Class::PushNum(-1)
        // 16 opcodes
        } else if all::OP_PUSHNUM_1.code <= self.code &&
                  self.code <= all::OP_PUSHNUM_16.code {
            Class::PushNum(1 + self.code as i32 - all::OP_PUSHNUM_1.code as i32)
        // 76 opcodes
        } else if self.code <= all::OP_PUSHBYTES_75.code {
            Class::PushBytes(self.code as u32)
        // 60 opcodes
        } else {
            Class::Ordinary(Ordinary::try_from_all(*self).unwrap())
        }
    }

    /// Encode as a byte
    #[inline]
    pub fn into_u8(&self) -> u8 {
        self.code
    }
}

impl From<u8> for All {
    #[inline]
    fn from(b: u8) -> All {
        All {code: b}
    }
}


display_from_debug!(All);

#[cfg(feature = "serde")]
impl serde::Serialize for All {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_string())
    }
}

/// Empty stack is also FALSE
pub static OP_FALSE: All = all::OP_PUSHBYTES_0;
/// Number 1 is also TRUE
pub static OP_TRUE: All = all::OP_PUSHNUM_1;
/// previously called OP_NOP2
pub static OP_NOP2: All = all::OP_CLTV;
/// previously called OP_NOP3
pub static OP_NOP3: All = all::OP_CSV;

/// Broad categories of opcodes with similar behavior
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Class {
    /// Pushes the given number onto the stack
    PushNum(i32),
    /// Pushes the given number of bytes onto the stack
    PushBytes(u32),
    /// Fails the script if executed
    ReturnOp,
    /// Fails the script even if not executed
    IllegalOp,
    /// Does nothing
    NoOp,
    /// Any opcode not covered above
    Ordinary(Ordinary)
}

display_from_debug!(Class);

#[cfg(feature = "serde")]
impl serde::Serialize for Class {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_str(&self.to_string())
    }
}

macro_rules! ordinary_opcode {
    ($($op:ident),*) => (
        #[repr(u8)]
        #[doc(hidden)]
        #[derive(Copy, Clone, PartialEq, Eq, Debug)]
        pub enum Ordinary {
            $( $op = all::$op.code ),*
        }

        impl Ordinary {
            /// Try to create from an All
            pub fn try_from_all(b: All) -> Option<Self> {
                match b {
                    $( all::$op => { Some(Ordinary::$op) } ),*
                    _ => None,
                }
            }
        }
    );
}

// "Ordinary" opcodes -- should be 60 of these
ordinary_opcode! {
    // pushdata
    OP_PUSHDATA1, OP_PUSHDATA2, OP_PUSHDATA4,
    // control flow
    OP_IF, OP_NOTIF, OP_ELSE, OP_ENDIF, OP_VERIFY,
    // stack
    OP_TOALTSTACK, OP_FROMALTSTACK,
    OP_2DROP, OP_2DUP, OP_3DUP, OP_2OVER, OP_2ROT, OP_2SWAP,
    OP_DROP, OP_DUP, OP_NIP, OP_OVER, OP_PICK, OP_ROLL, OP_ROT, OP_SWAP, OP_TUCK,
    OP_IFDUP, OP_DEPTH, OP_SIZE,
    // equality
    OP_EQUAL, OP_EQUALVERIFY,
    // arithmetic
    OP_1ADD, OP_1SUB, OP_NEGATE, OP_ABS, OP_NOT, OP_0NOTEQUAL,
    OP_ADD, OP_SUB, OP_BOOLAND, OP_BOOLOR,
    OP_NUMEQUAL, OP_NUMEQUALVERIFY, OP_NUMNOTEQUAL, OP_LESSTHAN,
    OP_GREATERTHAN, OP_LESSTHANOREQUAL, OP_GREATERTHANOREQUAL,
    OP_MIN, OP_MAX, OP_WITHIN,
    // crypto
    OP_RIPEMD160, OP_SHA1, OP_SHA256, OP_HASH160, OP_HASH256,
    OP_CODESEPARATOR, OP_CHECKSIG, OP_CHECKSIGVERIFY,
    OP_CHECKMULTISIG, OP_CHECKMULTISIGVERIFY
}

impl Ordinary {
    /// Encode as a byte
    #[inline]
    pub fn into_u8(&self) -> u8 {
      *self as u8
  }
}

#[cfg(test)]
mod tests {
    use std::collections::HashSet;

    use super::*;

    macro_rules! roundtrip {
        ($unique:expr, $op:ident) => {
            assert_eq!(all::$op, All::from(all::$op.into_u8()));

            let s1 = format!("{}", all::$op);
            let s2 = format!("{:?}", all::$op);
            assert_eq!(s1, s2);
            assert_eq!(s1, stringify!($op));
            assert!($unique.insert(s1));
        }
    }

    #[test]
    fn str_roundtrip() {
        let mut unique = HashSet::new();
        roundtrip!(unique, OP_PUSHBYTES_0);
        roundtrip!(unique, OP_PUSHBYTES_1);
        roundtrip!(unique, OP_PUSHBYTES_2);
        roundtrip!(unique, OP_PUSHBYTES_3);
        roundtrip!(unique, OP_PUSHBYTES_4);
        roundtrip!(unique, OP_PUSHBYTES_5);
        roundtrip!(unique, OP_PUSHBYTES_6);
        roundtrip!(unique, OP_PUSHBYTES_7);
        roundtrip!(unique, OP_PUSHBYTES_8);
        roundtrip!(unique, OP_PUSHBYTES_9);
        roundtrip!(unique, OP_PUSHBYTES_10);
        roundtrip!(unique, OP_PUSHBYTES_11);
        roundtrip!(unique, OP_PUSHBYTES_12);
        roundtrip!(unique, OP_PUSHBYTES_13);
        roundtrip!(unique, OP_PUSHBYTES_14);
        roundtrip!(unique, OP_PUSHBYTES_15);
        roundtrip!(unique, OP_PUSHBYTES_16);
        roundtrip!(unique, OP_PUSHBYTES_17);
        roundtrip!(unique, OP_PUSHBYTES_18);
        roundtrip!(unique, OP_PUSHBYTES_19);
        roundtrip!(unique, OP_PUSHBYTES_20);
        roundtrip!(unique, OP_PUSHBYTES_21);
        roundtrip!(unique, OP_PUSHBYTES_22);
        roundtrip!(unique, OP_PUSHBYTES_23);
        roundtrip!(unique, OP_PUSHBYTES_24);
        roundtrip!(unique, OP_PUSHBYTES_25);
        roundtrip!(unique, OP_PUSHBYTES_26);
        roundtrip!(unique, OP_PUSHBYTES_27);
        roundtrip!(unique, OP_PUSHBYTES_28);
        roundtrip!(unique, OP_PUSHBYTES_29);
        roundtrip!(unique, OP_PUSHBYTES_30);
        roundtrip!(unique, OP_PUSHBYTES_31);
        roundtrip!(unique, OP_PUSHBYTES_32);
        roundtrip!(unique, OP_PUSHBYTES_33);
        roundtrip!(unique, OP_PUSHBYTES_34);
        roundtrip!(unique, OP_PUSHBYTES_35);
        roundtrip!(unique, OP_PUSHBYTES_36);
        roundtrip!(unique, OP_PUSHBYTES_37);
        roundtrip!(unique, OP_PUSHBYTES_38);
        roundtrip!(unique, OP_PUSHBYTES_39);
        roundtrip!(unique, OP_PUSHBYTES_40);
        roundtrip!(unique, OP_PUSHBYTES_41);
        roundtrip!(unique, OP_PUSHBYTES_42);
        roundtrip!(unique, OP_PUSHBYTES_43);
        roundtrip!(unique, OP_PUSHBYTES_44);
        roundtrip!(unique, OP_PUSHBYTES_45);
        roundtrip!(unique, OP_PUSHBYTES_46);
        roundtrip!(unique, OP_PUSHBYTES_47);
        roundtrip!(unique, OP_PUSHBYTES_48);
        roundtrip!(unique, OP_PUSHBYTES_49);
        roundtrip!(unique, OP_PUSHBYTES_50);
        roundtrip!(unique, OP_PUSHBYTES_51);
        roundtrip!(unique, OP_PUSHBYTES_52);
        roundtrip!(unique, OP_PUSHBYTES_53);
        roundtrip!(unique, OP_PUSHBYTES_54);
        roundtrip!(unique, OP_PUSHBYTES_55);
        roundtrip!(unique, OP_PUSHBYTES_56);
        roundtrip!(unique, OP_PUSHBYTES_57);
        roundtrip!(unique, OP_PUSHBYTES_58);
        roundtrip!(unique, OP_PUSHBYTES_59);
        roundtrip!(unique, OP_PUSHBYTES_60);
        roundtrip!(unique, OP_PUSHBYTES_61);
        roundtrip!(unique, OP_PUSHBYTES_62);
        roundtrip!(unique, OP_PUSHBYTES_63);
        roundtrip!(unique, OP_PUSHBYTES_64);
        roundtrip!(unique, OP_PUSHBYTES_65);
        roundtrip!(unique, OP_PUSHBYTES_66);
        roundtrip!(unique, OP_PUSHBYTES_67);
        roundtrip!(unique, OP_PUSHBYTES_68);
        roundtrip!(unique, OP_PUSHBYTES_69);
        roundtrip!(unique, OP_PUSHBYTES_70);
        roundtrip!(unique, OP_PUSHBYTES_71);
        roundtrip!(unique, OP_PUSHBYTES_72);
        roundtrip!(unique, OP_PUSHBYTES_73);
        roundtrip!(unique, OP_PUSHBYTES_74);
        roundtrip!(unique, OP_PUSHBYTES_75);
        roundtrip!(unique, OP_PUSHDATA1);
        roundtrip!(unique, OP_PUSHDATA2);
        roundtrip!(unique, OP_PUSHDATA4);
        roundtrip!(unique, OP_PUSHNUM_NEG1);
        roundtrip!(unique, OP_RESERVED);
        roundtrip!(unique, OP_PUSHNUM_1);
        roundtrip!(unique, OP_PUSHNUM_2);
        roundtrip!(unique, OP_PUSHNUM_3);
        roundtrip!(unique, OP_PUSHNUM_4);
        roundtrip!(unique, OP_PUSHNUM_5);
        roundtrip!(unique, OP_PUSHNUM_6);
        roundtrip!(unique, OP_PUSHNUM_7);
        roundtrip!(unique, OP_PUSHNUM_8);
        roundtrip!(unique, OP_PUSHNUM_9);
        roundtrip!(unique, OP_PUSHNUM_10);
        roundtrip!(unique, OP_PUSHNUM_11);
        roundtrip!(unique, OP_PUSHNUM_12);
        roundtrip!(unique, OP_PUSHNUM_13);
        roundtrip!(unique, OP_PUSHNUM_14);
        roundtrip!(unique, OP_PUSHNUM_15);
        roundtrip!(unique, OP_PUSHNUM_16);
        roundtrip!(unique, OP_NOP);
        roundtrip!(unique, OP_VER);
        roundtrip!(unique, OP_IF);
        roundtrip!(unique, OP_NOTIF);
        roundtrip!(unique, OP_VERIF);
        roundtrip!(unique, OP_VERNOTIF);
        roundtrip!(unique, OP_ELSE);
        roundtrip!(unique, OP_ENDIF);
        roundtrip!(unique, OP_VERIFY);
        roundtrip!(unique, OP_RETURN);
        roundtrip!(unique, OP_TOALTSTACK);
        roundtrip!(unique, OP_FROMALTSTACK);
        roundtrip!(unique, OP_2DROP);
        roundtrip!(unique, OP_2DUP);
        roundtrip!(unique, OP_3DUP);
        roundtrip!(unique, OP_2OVER);
        roundtrip!(unique, OP_2ROT);
        roundtrip!(unique, OP_2SWAP);
        roundtrip!(unique, OP_IFDUP);
        roundtrip!(unique, OP_DEPTH);
        roundtrip!(unique, OP_DROP);
        roundtrip!(unique, OP_DUP);
        roundtrip!(unique, OP_NIP);
        roundtrip!(unique, OP_OVER);
        roundtrip!(unique, OP_PICK);
        roundtrip!(unique, OP_ROLL);
        roundtrip!(unique, OP_ROT);
        roundtrip!(unique, OP_SWAP);
        roundtrip!(unique, OP_TUCK);
        roundtrip!(unique, OP_CAT);
        roundtrip!(unique, OP_SUBSTR);
        roundtrip!(unique, OP_LEFT);
        roundtrip!(unique, OP_RIGHT);
        roundtrip!(unique, OP_SIZE);
        roundtrip!(unique, OP_INVERT);
        roundtrip!(unique, OP_AND);
        roundtrip!(unique, OP_OR);
        roundtrip!(unique, OP_XOR);
        roundtrip!(unique, OP_EQUAL);
        roundtrip!(unique, OP_EQUALVERIFY);
        roundtrip!(unique, OP_RESERVED1);
        roundtrip!(unique, OP_RESERVED2);
        roundtrip!(unique, OP_1ADD);
        roundtrip!(unique, OP_1SUB);
        roundtrip!(unique, OP_2MUL);
        roundtrip!(unique, OP_2DIV);
        roundtrip!(unique, OP_NEGATE);
        roundtrip!(unique, OP_ABS);
        roundtrip!(unique, OP_NOT);
        roundtrip!(unique, OP_0NOTEQUAL);
        roundtrip!(unique, OP_ADD);
        roundtrip!(unique, OP_SUB);
        roundtrip!(unique, OP_MUL);
        roundtrip!(unique, OP_DIV);
        roundtrip!(unique, OP_MOD);
        roundtrip!(unique, OP_LSHIFT);
        roundtrip!(unique, OP_RSHIFT);
        roundtrip!(unique, OP_BOOLAND);
        roundtrip!(unique, OP_BOOLOR);
        roundtrip!(unique, OP_NUMEQUAL);
        roundtrip!(unique, OP_NUMEQUALVERIFY);
        roundtrip!(unique, OP_NUMNOTEQUAL);
        roundtrip!(unique, OP_LESSTHAN );
        roundtrip!(unique, OP_GREATERTHAN );
        roundtrip!(unique, OP_LESSTHANOREQUAL );
        roundtrip!(unique, OP_GREATERTHANOREQUAL );
        roundtrip!(unique, OP_MIN);
        roundtrip!(unique, OP_MAX);
        roundtrip!(unique, OP_WITHIN);
        roundtrip!(unique, OP_RIPEMD160);
        roundtrip!(unique, OP_SHA1);
        roundtrip!(unique, OP_SHA256);
        roundtrip!(unique, OP_HASH160);
        roundtrip!(unique, OP_HASH256);
        roundtrip!(unique, OP_CODESEPARATOR);
        roundtrip!(unique, OP_CHECKSIG);
        roundtrip!(unique, OP_CHECKSIGVERIFY);
        roundtrip!(unique, OP_CHECKMULTISIG);
        roundtrip!(unique, OP_CHECKMULTISIGVERIFY);
        roundtrip!(unique, OP_NOP1);
        roundtrip!(unique, OP_CLTV);
        roundtrip!(unique, OP_CSV);
        roundtrip!(unique, OP_NOP4);
        roundtrip!(unique, OP_NOP5);
        roundtrip!(unique, OP_NOP6);
        roundtrip!(unique, OP_NOP7);
        roundtrip!(unique, OP_NOP8);
        roundtrip!(unique, OP_NOP9);
        roundtrip!(unique, OP_NOP10);
        roundtrip!(unique, OP_RETURN_186);
        roundtrip!(unique, OP_RETURN_187);
        roundtrip!(unique, OP_RETURN_188);
        roundtrip!(unique, OP_RETURN_189);
        roundtrip!(unique, OP_RETURN_190);
        roundtrip!(unique, OP_RETURN_191);
        roundtrip!(unique, OP_RETURN_192);
        roundtrip!(unique, OP_RETURN_193);
        roundtrip!(unique, OP_RETURN_194);
        roundtrip!(unique, OP_RETURN_195);
        roundtrip!(unique, OP_RETURN_196);
        roundtrip!(unique, OP_RETURN_197);
        roundtrip!(unique, OP_RETURN_198);
        roundtrip!(unique, OP_RETURN_199);
        roundtrip!(unique, OP_RETURN_200);
        roundtrip!(unique, OP_RETURN_201);
        roundtrip!(unique, OP_RETURN_202);
        roundtrip!(unique, OP_RETURN_203);
        roundtrip!(unique, OP_RETURN_204);
        roundtrip!(unique, OP_RETURN_205);
        roundtrip!(unique, OP_RETURN_206);
        roundtrip!(unique, OP_RETURN_207);
        roundtrip!(unique, OP_RETURN_208);
        roundtrip!(unique, OP_RETURN_209);
        roundtrip!(unique, OP_RETURN_210);
        roundtrip!(unique, OP_RETURN_211);
        roundtrip!(unique, OP_RETURN_212);
        roundtrip!(unique, OP_RETURN_213);
        roundtrip!(unique, OP_RETURN_214);
        roundtrip!(unique, OP_RETURN_215);
        roundtrip!(unique, OP_RETURN_216);
        roundtrip!(unique, OP_RETURN_217);
        roundtrip!(unique, OP_RETURN_218);
        roundtrip!(unique, OP_RETURN_219);
        roundtrip!(unique, OP_RETURN_220);
        roundtrip!(unique, OP_RETURN_221);
        roundtrip!(unique, OP_RETURN_222);
        roundtrip!(unique, OP_RETURN_223);
        roundtrip!(unique, OP_RETURN_224);
        roundtrip!(unique, OP_RETURN_225);
        roundtrip!(unique, OP_RETURN_226);
        roundtrip!(unique, OP_RETURN_227);
        roundtrip!(unique, OP_RETURN_228);
        roundtrip!(unique, OP_RETURN_229);
        roundtrip!(unique, OP_RETURN_230);
        roundtrip!(unique, OP_RETURN_231);
        roundtrip!(unique, OP_RETURN_232);
        roundtrip!(unique, OP_RETURN_233);
        roundtrip!(unique, OP_RETURN_234);
        roundtrip!(unique, OP_RETURN_235);
        roundtrip!(unique, OP_RETURN_236);
        roundtrip!(unique, OP_RETURN_237);
        roundtrip!(unique, OP_RETURN_238);
        roundtrip!(unique, OP_RETURN_239);
        roundtrip!(unique, OP_RETURN_240);
        roundtrip!(unique, OP_RETURN_241);
        roundtrip!(unique, OP_RETURN_242);
        roundtrip!(unique, OP_RETURN_243);
        roundtrip!(unique, OP_RETURN_244);
        roundtrip!(unique, OP_RETURN_245);
        roundtrip!(unique, OP_RETURN_246);
        roundtrip!(unique, OP_RETURN_247);
        roundtrip!(unique, OP_RETURN_248);
        roundtrip!(unique, OP_RETURN_249);
        roundtrip!(unique, OP_RETURN_250);
        roundtrip!(unique, OP_RETURN_251);
        roundtrip!(unique, OP_RETURN_252);
        roundtrip!(unique, OP_RETURN_253);
        roundtrip!(unique, OP_RETURN_254);
        roundtrip!(unique, OP_RETURN_255);
        assert_eq!(unique.len(), 256);
    }
}

