const blsSignatures = require('..')();
const assert = require('assert');

function getPublicKeyFixtureHex() {
    return '1790635de8740e9a6a6b15fb6b72f3a16afa0973d971979b6ba54761d6e2502c50db76f4d26143f05459a42cfd520d44';
}

function getPublicKeyFixture() {
    return {
        buffer: Uint8Array.from(Buffer.from(getPublicKeyFixtureHex(), 'hex')),
        fingerprint: 0xddad59bb
    }
}

function getPublicKeysHexes() {
    return [
        '02a8d2aaa6a5e2e08d4b8d406aaf0121a2fc2088ed12431e6b0663028da9ac5922c9ea91cde7dd74b7d795580acc7a61',
        '056e742478d4e95e708b8ae0d487f94099b769cb7df4c674dc0c10fbbe7d175603d090ac6064aeeb249a00ba6b3d85eb'
    ];
}

function getPublicKeysArray() {
    return getPublicKeysHexes().map(hex => {
        return Uint8Array.from(Buffer.from(hex, 'hex'));
    })
}

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('PublicKey', () => {
    describe('.fromBytes', () => {
        it('Should create a public key from bytes', () => {
            const {PublicKey} = blsSignatures;

            const pk = PublicKey.fromBytes(getPublicKeyFixture().buffer);
            assert(pk instanceof PublicKey);
        });
    });

    describe('.aggregate', () => {
        it('Should aggregate keys if keys array contains more than one key', () => {
            const {PublicKey} = blsSignatures;

            const pks = getPublicKeysArray().map(buf => PublicKey.fromBytes(buf));
            const aggregatedKey = PublicKey.aggregate(pks);
            assert(aggregatedKey instanceof PublicKey);
        });
    });

    describe('#serialize', () => {
        it('Should serialize key to the same buffer', () => {
            const {PublicKey} = blsSignatures;

            const pk = PublicKey.fromBytes(getPublicKeyFixture().buffer);
            const serialized = pk.serialize();
            assert.deepStrictEqual(Buffer.from(serialized).toString('hex'), getPublicKeyFixtureHex());
        });
    });

    describe('getFingerprint', () => {
        it('Should get correct fingerprint', () => {
            const {PublicKey} = blsSignatures;

            const pk = PublicKey.fromBytes(getPublicKeyFixture().buffer);
            const fingerprint = pk.getFingerprint();
            assert.strictEqual(fingerprint, getPublicKeyFixture().fingerprint);
        });
    });
});
