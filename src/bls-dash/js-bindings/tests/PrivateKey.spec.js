const blsSignatures = require('..')();
const assert = require('assert');
const crypto = require('crypto');

function getSeedAndFinferprint() {
    return {
        seed: Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]),
        fingerprint: 0xddad59bb
    };
}

function getPkSeed() {
    return Uint8Array.from([
        0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6, 220,
        18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22
    ]);
}

function getPkBuffer() {
    return Uint8Array.from([
        84, 61, 124, 70, 203, 191, 91, 170, 188, 74, 176, 22, 97, 250, 169, 26,
        105, 59, 39, 4, 246, 103, 227, 53, 133, 228, 214, 59, 165, 6, 158, 39
    ]);
}

function getPkUint8Array() {
    return new Uint8Array(getPkBuffer());
}

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('PrivateKey', () => {
    it('Should sign and verify', () => {
        const {PrivateKey, AggregationInfo} = blsSignatures;

        const message1 = Uint8Array.from([1, 65, 254, 88, 90, 45, 22]);

        const seed = Uint8Array.from([28, 20, 102, 229, 1, 157]);
        const sk1 = PrivateKey.fromSeed(seed);
        const pk1 = sk1.getPublicKey();
        const sig1 = sk1.sign(message1);

        sig1.setAggregationInfo(AggregationInfo.fromMsg(pk1, message1));
        assert(sig1.verify());

        const hash = crypto
            .createHash('sha256')
            .update(message1)
            .digest();
        const sig2 = sk1.signPrehashed(hash);
        sig2.setAggregationInfo(AggregationInfo.fromMsgHash(pk1, hash));
        assert.deepStrictEqual(sig1.serialize(), sig2.serialize());
        assert(sig2.verify());
    });

    describe('.fromSeed', () => {
        it('Should create a private key from a seed', () => {
            const {PrivateKey} = blsSignatures;

            const pk = PrivateKey.fromSeed(getPkSeed());
            assert(pk instanceof PrivateKey);
            assert.deepStrictEqual(pk.serialize(), getPkBuffer());
        });
    });
    describe('.fromBytes', () => {
        it('Should create a private key from a Buffer', () => {
            const {PrivateKey} = blsSignatures;

            const pk = PrivateKey.fromBytes(getPkBuffer(), false);
            assert(pk instanceof PrivateKey);
            assert.deepStrictEqual(pk.serialize(), getPkBuffer());
        });
        it('Should create a private key from a Uint8Array', () => {
            const {PrivateKey} = blsSignatures;

            const pk = PrivateKey.fromBytes(getPkUint8Array(), false);
            assert(pk instanceof PrivateKey);
            assert.deepStrictEqual(pk.serialize(), getPkBuffer());
        });
    });

    describe('.aggregate', () => {
        it('Should aggregate private keys', () => {
            const {PrivateKey} = blsSignatures;

            const privateKeys = [
                PrivateKey.fromSeed(Buffer.from([1, 2, 3])),
                PrivateKey.fromSeed(Buffer.from([3, 4, 5]))
            ];
            const publicKeys = [
                privateKeys[0].getPublicKey(),
                privateKeys[1].getPublicKey()
            ];
            const aggregatedKey = PrivateKey.aggregate(privateKeys, publicKeys);
            assert(aggregatedKey instanceof PrivateKey);
        });
    });

    describe('#serialize', () => {
        it('Should serialize key to a Buffer', () => {
            const {PrivateKey} = blsSignatures;

            const pk = PrivateKey.fromSeed(getPkSeed());
            const serialized = pk.serialize();
            assert(serialized instanceof Uint8Array);
            assert.deepStrictEqual(serialized, getPkBuffer());
        });
    });

    describe('#sign', () => {
        it('Should return a verifiable signature', () => {
            const {PrivateKey, Signature} = blsSignatures;

            const pk = PrivateKey.fromBytes(getPkBuffer(), false);
            const message = 'Hello world';
            const signature = pk.sign(Uint8Array.from(Buffer.from(message, 'utf8')));
            assert(signature instanceof Signature);
            assert(signature.verify());
        });
    });

    describe('#signPrehashed', () => {
        it('Should sign a hash and return a signature', () => {
            const {PrivateKey, Signature} = blsSignatures;

            const pk = PrivateKey.fromSeed(Uint8Array.from([1, 2, 3, 4, 5]));
            const messageHash = Uint8Array.from(crypto
                .createHash('sha256')
                .update('Hello world')
                .digest());
            const sig = pk.signPrehashed(messageHash);
            const info = sig.getAggregationInfo();
            assert(sig instanceof Signature);
            assert(sig.verify());
            assert.deepStrictEqual(info.getMessageHashes()[0], messageHash);
        });
    });

    describe('#getPublicKey', () => {
        it('Should return a public key with a verifiable fingerprint', () => {
            const {PrivateKey, PublicKey} = blsSignatures;

            const pk = PrivateKey.fromSeed(getSeedAndFinferprint().seed);
            const publicKey = pk.getPublicKey();
            assert(publicKey instanceof PublicKey);
            assert.strictEqual(publicKey.getFingerprint(), getSeedAndFinferprint().fingerprint);
        });
    });
});
