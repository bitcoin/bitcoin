const blsSignaturesModule = require('..')();
const assert = require('assert');
const {Buffer} = require('buffer');

// Value from test.js, ported from test.py
// Form of serialized public key requires first two bits 10 unless infinite,
// in which case 11 is required.
// (elements.cpp:49)
function getPublicKeyFixtureHex() {
    return '9790635de8740e9a6a6b15fb6b72f3a16afa0973d971979b6ba54761d6e2502c50db76f4d26143f05459a42cfd520d44';
}

function getPublicKeyFixture() {
    return {
        buffer: Uint8Array.from(Buffer.from(getPublicKeyFixtureHex(), 'hex')),
        fingerprint: 0xa14c4f99
    };
}

// These values were adapted to fit the constraints in elements.cpp, namely to set
// the high bit of the first byte.
function getPublicKeysHexes() {
    return [
        '82a8d2aaa6a5e2e08d4b8d406aaf0121a2fc2088ed12431e6b0663028da9ac5922c9ea91cde7dd74b7d795580acc7a61',
        '856e742478d4e95e708b8ae0d487f94099b769cb7df4c674dc0c10fbbe7d175603d090ac6064aeeb249a00ba6b3d85eb'
    ];
}

function getPublicKeysArray() {
    return getPublicKeysHexes().map(hex => {
        return Uint8Array.from(Buffer.from(hex, 'hex'));
    });
}

let blsSignatures = null;
before((done) => {
    blsSignaturesModule.then((mod) => {
        blsSignatures = mod;
        done();
    });
});

describe('G1Element', () => {
    describe('.from_bytes', () => {
        it('Should create a public key from bytes', () => {
            const {G1Element, Util} = blsSignatures;

            const pk = G1Element.from_bytes(getPublicKeyFixture().buffer);
            assert(pk instanceof G1Element);
        });
    });

    describe('.aggregate', () => {
        it('Should aggregate keys if keys array contains more than one key', () => {
            const {G1Element} = blsSignatures;

            const pks = getPublicKeysArray().map(buf => G1Element.from_bytes(buf));
            let first_pk = pks[0];
            for (var i = 1; i < pks.length; i++) {
                first_pk = first_pk.add(pks[i]);
            }
            assert(first_pk instanceof G1Element);
        });
    });

    describe('#serialize', () => {
        it('Should serialize key to the same buffer', () => {
            const {G1Element} = blsSignatures;

            const pk = G1Element.from_bytes(getPublicKeyFixture().buffer);
            const serialized = pk.serialize();
            assert.deepStrictEqual(Buffer.from(serialized).toString('hex'), getPublicKeyFixtureHex());
        });
    });

    describe('getFingerprint', () => {
        it('Should get correct fingerprint', () => {
            const {G1Element} = blsSignatures;

            const pk = G1Element.from_bytes(getPublicKeyFixture().buffer);
            const fingerprint = pk.get_fingerprint();
            assert.strictEqual(fingerprint, getPublicKeyFixture().fingerprint);
        });
    });
});
