# pq11... Post-Quantum Hybrid Address

## Overview
`pq11...` is a Bech32m address format for the **Schnorr + Falcon-1024 hybrid 
post-quantum signature scheme**. Addresses start with `pq11` and encode a 
composite public key: 32-byte Schnorr (BIP 340, secp256k1) + 1793-byte Falcon-1024 
(NIST FIPS 204 Level 5).

## Address Format
- **HRP:** `pq1`
- **Witness version:** 2
- **Program size:** 1825 bytes (32 Schnorr + 1793 Falcon-1024)
- **Encoding:** Bech32m

## Security
- **Classical:** 128-bit (secp256k1 Schnorr)
- **Post-Quantum:** 230-bit (Falcon-1024, NIST Level 5)
- **Composite security:** Both must be broken to forge

## Usage
    # Generate pq1 address
    bitcoin-cli getnewaddress "" "pq1"
    
    # pq11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq...

## Dependencies
- liboqs 0.15.0+ (Falcon-1024)
- secp256k1 with Schnorr module (BIP 340)

## References
- NIST FIPS 204: Module-Lattice-Based Digital Signature Standard
- BIP 340: Schnorr Signatures for secp256k1
- RFC 8235: Schnorr Non-interactive Zero-Knowledge Proof
- BIP 350: Bech32m format for v1+ witness addresses
