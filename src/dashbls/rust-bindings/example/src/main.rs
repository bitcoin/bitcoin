use bls_signatures::{bip32::ExtendedPrivateKey, LegacySchemeMPL, Scheme};

const SEED: &'static [u8] = b"seedweedseedweedseedweedseedweed";

fn check_bip32_hd_keys() {
    println!("Check BIP32 hierarchical deterministic keys work");

    let private_key =
        ExtendedPrivateKey::from_seed(SEED).expect("cannot generate extended private key");
    let public_key = private_key
        .extended_public_key()
        .expect("cannot get extended public key");

    let private_child = private_key.private_child(1337);
    let private_grandchild = private_child.private_child(420);

    let public_child = public_key.public_child(1337);
    let public_grandchild = public_child.public_child(420);

    assert_eq!(
        public_grandchild,
        private_grandchild
            .extended_public_key()
            .expect("cannot get extended public key")
    );
}

fn check_bip32_legacy_scheme() {
    println!("Check BIP32 signing works");

    let private_key =
        ExtendedPrivateKey::from_seed(SEED).expect("cannot generate extended private key");
    let public_key = private_key
        .extended_public_key()
        .expect("cannot get extended public key");

    let scheme = LegacySchemeMPL::new();
    let message = b"dash is og";
    let signature = scheme.sign(&private_key.private_key(), message);

    assert!(scheme.verify(&public_key.public_key(), message, &signature));
}

fn main() {
    println!("Run some checks to see if everything is linked up to original BLS library:");

    check_bip32_hd_keys();
    check_bip32_legacy_scheme();

    println!("All checks passed!");
}
