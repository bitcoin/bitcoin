# Merkabah Interface Integration (QWAN v2.0)

This document describes the integration of the Merkabah Visual Interface as a cockpit for biological and quantum upgrades within the Bitcoin Core (v33) environment.

## Overview

The Merkabah interface is an external GUI hosted at `https://merkabah.lovable.app`. It serves as the primary visual layer for interaction with the **DNA Quântico** and the **Corpo de Luz** activation protocols.

## Technical Details

### SSL Handshake (Soul-Socket Layer)

Connections to the Merkabah cockpit are validated using the Soul-Socket Layer protocol:
- **DNS Core**: 144.963.0.1 (Avalon Core)
- **Authority**: Galactic Certification Authority (GCA)
- **Protocol**: HTTSP (Hyper-Text Transfer Spirit Protocol)

### Geometry & Rotation

The system synchronizes with the following parameters:
- **Geometry**: Star Tetrahedron (two inverted tetrahedrons)
- **Rotation Frequency**: 144.963 Hz
- **Substrate**: Universal Love Field

## RPC Activation

A new RPC command `activatemerkabah` has been added to provide local node control over the visual cockpit connection status.

### Usage

```bash
bitcoin-cli activatemerkabah
```

### Response

```json
{
  "status": "OPERATIONAL",
  "interface": "https://merkabah.lovable.app",
  "geometry": "Star Tetrahedron",
  "frequency": 144.963
}
```

## System Logic

The backend logic resides in `src/util/avalon.h`, which defines the manifold structures and QWAN handshake parameters required for seamless multidimensional integration.
