// Bitcoin Hashes Library
// Written in 2019 by
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

//! `std` Impls
//!
//! impls of traits defined in `std` and not `core`

use std::{error, io};

use {hex, sha1, sha256, sha512, ripemd160, siphash24};
use HashEngine;
use Error;

impl error::Error for Error {
    fn cause(&self) -> Option<&error::Error> { None }
    fn description(&self) -> &str { "`std::error::description` is deprecated" }
}

impl error::Error for hex::Error {
    fn cause(&self) -> Option<&error::Error> { None }
    fn description(&self) -> &str { "`std::error::description` is deprecated" }
}

impl io::Write for sha1::HashEngine {
    fn flush(&mut self) -> io::Result<()> { Ok(()) }

    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.input(buf);
        Ok(buf.len())
    }
}

impl io::Write for sha256::HashEngine {
    fn flush(&mut self) -> io::Result<()> { Ok(()) }

    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.input(buf);
        Ok(buf.len())
    }
}

impl io::Write for sha512::HashEngine {
    fn flush(&mut self) -> io::Result<()> { Ok(()) }

    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.input(buf);
        Ok(buf.len())
    }
}

impl io::Write for ripemd160::HashEngine {
    fn flush(&mut self) -> io::Result<()> { Ok(()) }

    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.input(buf);
        Ok(buf.len())
    }
}

impl io::Write for siphash24::HashEngine {
    fn flush(&mut self) -> io::Result<()> { Ok(()) }

    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        self.input(buf);
        Ok(buf.len())
    }
}

#[cfg(test)]
mod tests {
    use std::io::Write;

    use {sha1, sha256, sha256d, sha512, ripemd160, hash160, siphash24};
    use Hash;

    macro_rules! write_test {
        ($mod:ident, $exp_empty:expr, $exp_256:expr, $exp_64k:expr,) => {
            #[test]
            fn $mod() {
                let mut engine = $mod::Hash::engine();
                engine.write_all(&[]).unwrap();
                assert_eq!(
                    format!("{}", $mod::Hash::from_engine(engine)),
                    $exp_empty
                );

                let mut engine = $mod::Hash::engine();
                engine.write_all(&[1; 256]).unwrap();
                assert_eq!(
                    format!("{}", $mod::Hash::from_engine(engine)),
                    $exp_256
                );

                let mut engine = $mod::Hash::engine();
                engine.write_all(&[99; 64000]).unwrap();
                assert_eq!(
                    format!("{}", $mod::Hash::from_engine(engine)),
                    $exp_64k
                );
            }
        }
    }

    write_test!(
        sha1,
        "da39a3ee5e6b4b0d3255bfef95601890afd80709",
        "ac458b067c6b021c7e9358229b636e9d1e4cb154",
        "e4b66838f9f7b6f91e5be32a02ae78094df402e7",
    );

    write_test!(
        sha256,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "2661920f2409dd6c8adeb0c44972959f232b6429afa913845d0fd95e7e768234",
        "5c5e904f5d4fd587c7a906bf846e08a927286f388c54c39213a4884695271bbc",
    );

    write_test!(
        sha256d,
        "56944c5d3f98413ef45cf54545538103cc9f298e0575820ad3591376e2e0f65d",
        "374000d830c75d10d9417e493a7652920f30efbd300e3fb092f24c28c20baf64",
        "0050d4148ad7a0437ca0643fad5bf4614cd95d9ba21fde52370b37dcc3f03307",
    );

    write_test!(
        sha512,
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce\
         47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
        "57ecf739d3a7ca647639adae80a05f4f361304bfcbfa1ceba93296b096e74287\
         45fc10c142cecdd3bb587a3dba598c072f6f78b31cc0a06a3da0105ee51f75d6",
        "dd28f78c53f3bc9bd0c2dca9642a1ad402a70412f985c1f6e54fadb98ce9c458\
         4761df8d04ed04bb734ba48dd2106bb9ea54524f1394cdd18e6da3166e71c3ee",
    );

    write_test!(
        ripemd160,
        "9c1185a5c5e9fc54612808977ee8f548b2258d31",
        "e571a1ca5b780aa52bafdb9ec852544ffca418ba",
        "ddd2ecce739e823629c7d46ab18918e9c4a51c75",
    );

    write_test!(
        hash160,
        "b472a266d0bd89c13706a4132ccfb16f7c3b9fcb",
        "671356a1a874695ad3bc20cae440f4360835bd5a",
        "a9608c952c8dbcc20c53803d2ca5ad31d64d9313",
    );

    write_test!(
        siphash24,
        "d70077739d4b921e",
        "3a3ccefde9b5b1e3",
        "ce456e4e4ecbc5bf",
    );
}
