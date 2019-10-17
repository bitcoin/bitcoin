// Bitcoin secp256k1 bindings
// Written in 2014 by
//   Dawid Ciężarkiewicz
//   Andrew Poelstra
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

// This is a macro that routinely comes in handy
macro_rules! impl_array_newtype {
    ($thing:ident, $ty:ty, $len:expr) => {
        impl Copy for $thing {}

        impl $thing {
            #[inline]
            /// Converts the object to a raw pointer for FFI interfacing
            pub fn as_ptr(&self) -> *const $ty {
                let &$thing(ref dat) = self;
                dat.as_ptr()
            }

            #[inline]
            /// Converts the object to a mutable raw pointer for FFI interfacing
            pub fn as_mut_ptr(&mut self) -> *mut $ty {
                let &mut $thing(ref mut dat) = self;
                dat.as_mut_ptr()
            }

            #[inline]
            /// Returns the length of the object as an array
            pub fn len(&self) -> usize { $len }

            #[inline]
            /// Returns whether the object as an array is empty
            pub fn is_empty(&self) -> bool { false }
        }

        impl PartialEq for $thing {
            #[inline]
            fn eq(&self, other: &$thing) -> bool {
                &self[..] == &other[..]
            }
        }

        impl Eq for $thing {}

        impl PartialOrd for $thing {
            #[inline]
            fn partial_cmp(&self, other: &$thing) -> Option<::core::cmp::Ordering> {
                self[..].partial_cmp(&other[..])
            }
        }

        impl Ord for $thing {
            #[inline]
            fn cmp(&self, other: &$thing) -> ::core::cmp::Ordering {
                self[..].cmp(&other[..])
            }            
        }

        impl Clone for $thing {
            #[inline]
            fn clone(&self) -> $thing {
                let &$thing(ref dat) = self;
                $thing(dat.clone())
            }
        }

        impl ::core::ops::Index<usize> for $thing {
            type Output = $ty;

            #[inline]
            fn index(&self, index: usize) -> &$ty {
                let &$thing(ref dat) = self;
                &dat[index]
            }
        }

        impl ::core::ops::Index<::core::ops::Range<usize>> for $thing {
            type Output = [$ty];

            #[inline]
            fn index(&self, index: ::core::ops::Range<usize>) -> &[$ty] {
                let &$thing(ref dat) = self;
                &dat[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeTo<usize>> for $thing {
            type Output = [$ty];

            #[inline]
            fn index(&self, index: ::core::ops::RangeTo<usize>) -> &[$ty] {
                let &$thing(ref dat) = self;
                &dat[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeFrom<usize>> for $thing {
            type Output = [$ty];

            #[inline]
            fn index(&self, index: ::core::ops::RangeFrom<usize>) -> &[$ty] {
                let &$thing(ref dat) = self;
                &dat[index]
            }
        }

        impl ::core::ops::Index<::core::ops::RangeFull> for $thing {
            type Output = [$ty];

            #[inline]
            fn index(&self, _: ::core::ops::RangeFull) -> &[$ty] {
                let &$thing(ref dat) = self;
                &dat[..]
            }
        }
        impl ::ffi::CPtr for $thing {
            type Target = $ty;
            fn as_c_ptr(&self) -> *const Self::Target {
                if self.is_empty() {
                    ::core::ptr::null()
                } else {
                    self.as_ptr()
                }
            }

            fn as_mut_c_ptr(&mut self) -> *mut Self::Target {
                if self.is_empty() {
                    ::core::ptr::null::<Self::Target>() as *mut _
                } else {
                    self.as_mut_ptr()
                }
            }
        }
    }
}

macro_rules! impl_pretty_debug {
    ($thing:ident) => {
        impl ::core::fmt::Debug for $thing {
            fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
                try!(write!(f, "{}(", stringify!($thing)));
                for i in self[..].iter().cloned() {
                    try!(write!(f, "{:02x}", i));
                }
                write!(f, ")")
            }
        }
     }
}

macro_rules! impl_raw_debug {
    ($thing:ident) => {
        impl ::core::fmt::Debug for $thing {
            fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
                for i in self[..].iter().cloned() {
                    try!(write!(f, "{:02x}", i));
                }
                Ok(())
            }
        }
     }
}

#[cfg(feature="serde")]
/// Implements `Serialize` and `Deserialize` for a type `$t` which represents
/// a newtype over a byte-slice over length `$len`. Type `$t` must implement
/// the `FromStr` and `Display` trait.
macro_rules! serde_impl(
    ($t:ident, $len:expr) => (
        impl ::serde::Serialize for $t {
            fn serialize<S: ::serde::Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
                if s.is_human_readable() {
                    s.collect_str(self)
                } else {
                    s.serialize_bytes(&self[..])
                }
            }
        }

        impl<'de> ::serde::Deserialize<'de> for $t {
            fn deserialize<D: ::serde::Deserializer<'de>>(d: D) -> Result<$t, D::Error> {
                use ::serde::de::Error;
                use core::str::FromStr;

                if d.is_human_readable() {
                    let sl: &str = ::serde::Deserialize::deserialize(d)?;
                    SecretKey::from_str(sl).map_err(D::Error::custom)
                } else {
                    let sl: &[u8] = ::serde::Deserialize::deserialize(d)?;
                    if sl.len() != $len {
                        Err(D::Error::invalid_length(sl.len(), &stringify!($len)))
                    } else {
                        let mut ret = [0; $len];
                        ret.copy_from_slice(sl);
                        Ok($t(ret))
                    }
                }
            }
        }
    )
);

#[cfg(not(feature="serde"))]
macro_rules! serde_impl(
    ($t:ident, $len:expr) => ()
);
