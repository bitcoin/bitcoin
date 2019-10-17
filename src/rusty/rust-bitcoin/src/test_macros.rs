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

//! Macros
//!
//! Internal macros used for unit tests

#[cfg(feature = "serde")]
macro_rules! serde_round_trip (
    ($var:expr) => ({
        use serde_json;

        let encoded = serde_json::to_value(&$var).unwrap();
        let decoded = serde_json::from_value(encoded).unwrap();
        assert_eq!($var, decoded);
    })
);

