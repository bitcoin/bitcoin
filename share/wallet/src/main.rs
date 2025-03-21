extern crate bip39;

use std::io;
use std::process;

use bitcoin::bip32::{Xpriv};
use bip39::{Mnemonic, Language};

use bitcoin::network::NetworkKind;
use miniscript::{Descriptor};

fn main() {
    // Parse command

    if std::env::args().len() < 2 {
        println!("See README.md for commands");
        process::exit(1);
    }
    let command = std::env::args().nth(1).expect("Command missing");
    if command != "import" {
        println!("Uknown command: {command}");
        process::exit(1);
    }
    let import_type = std::env::args().nth(2).expect("Specify import type (bip39)");
    if import_type != "bip39" {
        println!("Uknown import type: {import_type} (only bip39 is supported)");
        process::exit(1);
    }

    // command: import bip39
    let mut network_input=String::new();
    let mut mnemonic_input=String::new();
    let mut passphrase_input=String::new();
    let stdin = io::stdin();

    println!("Specify network: 'main' (default) or 'test' (testnet and signet)");
    match stdin.read_line(&mut network_input) {
        Ok(len) => len,
        Err(_) => panic!("Failed to parse input"),
    };
    let network = match network_input.trim_end() {
        "" => NetworkKind::Main,
        "main" => NetworkKind::Main,
        "test" => NetworkKind::Test,
        _ => panic!("Unknown network: {}", &network_input),
    };

    println!("Enter bip39 mnemonic:");
    let len = match stdin.read_line(&mut mnemonic_input) {
        Ok(len) => len,
        Err(_) => panic!("Failed to parse input"),
    };
    // Check for empty input
    if len == 1 {
        process::exit(0);
    }
    println!("Enter passphrase (optional):");

    match stdin.read_line(&mut passphrase_input) {
        Ok(len) => len,
        Err(_) => panic!("Failed to parse input"),
    };

    let mnemonic = match Mnemonic::parse_in_normalized(Language::English, &mnemonic_input) {
        Ok(mnemonic) => mnemonic,
        Err(error) => panic!("Failed to parse mnemonic: {error:?}"),
    };
    let word_count = mnemonic.word_count();
    println!("Parsed {word_count} word mnemonic.");

    // Trim \n from passphrase
    let passphrase = passphrase_input.trim_end();

    let seed = mnemonic.to_seed(passphrase);
    let root_xprv = Xpriv::new_master(network, &seed).unwrap();
    let network_der = if network == NetworkKind::Main { "0h" } else { "1h" };

    let bip44_desc = import_desc(Descriptor::new_pkh(format!(
                                 "{root_xprv}/44h/{network_der}/0h/<0;1>/*")).expect("valid pkh() descriptor"));
    let bip49_desc = import_desc(Descriptor::new_sh_wpkh(format!(
                                 "{root_xprv}/49h/{network_der}/0h/<0;1>/*")).expect("valid sh(wpkh()) descriptor"));
    let bip84_desc = import_desc(Descriptor::new_wpkh(format!(
                                 "{root_xprv}/84h/{network_der}/0h/<0;1>/*")).expect("valid wpkh() descriptor"));
    let bip86_desc = import_desc(Descriptor::new_tr(format!(
                                 "{root_xprv}/86h/{network_der}/0h/<0;1>/*"), None).expect("valid tr() descriptor"));

    println!("Import into your wallet as follows:\n");
    println!("importdescriptors '[{bip44_desc}, {bip49_desc}, {bip84_desc}, {bip86_desc}]'\n");
}

fn import_desc(descriptor: Descriptor<String>) -> String {
    let desc = descriptor.to_string(); // includes a checksum
    return format!("{{\"desc\": \"{desc}\", \"timestamp\": \"now\", \"active\": true}}");
}
