# Contributing to PIP-2030 Quantum Shield

## Overview
This contribution adds a post-quantum cryptography (PQC) toolset to Bitcoin Core's `contrib/` directory. It implements CRYSTALS-Dilithium Mode3 for quantum-safe signing and verification.

## Directory Structure
- `pqc/`: Core cryptographic implementation (Dilithium Mode3 wrapper).
- `cmd/pq-shield/`: CLI tool for key generation, signing, and verification.
- `cmd/audit/`: Forensic audit tool for validating key/signature formats and permissions.

## Build Instructions
To build the tools:
```bash
cd contrib/pq-shield
go build ./cmd/pq-shield
go build ./cmd/audit
```

## Testing
Run the unit tests and fuzz tests:
```bash
cd contrib/pq-shield/pqc
go test -v -fuzz=Fuzz -fuzztime=10s
```

## Usage
### Generate Keys
```bash
./pq-shield generate -pub public.key -priv private.key
```

### Sign Message
```bash
echo "Satoshi Nakamoto" > message.txt
./pq-shield sign -priv private.key -msg message.txt -sig signature.sig
```

### Verify Signature
```bash
./pq-shield verify -pub public.key -msg message.txt -sig signature.sig
```

## Audit
```bash
./audit public.key private.key signature.sig
```

## License
MIT License (same as Bitcoin Core).
