const blsSignatures = require('..')();
const assert = require('assert');
const crypto = require('crypto');

function getSeed() {
    return Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]);
}

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('AggregationInfo', () => {
    it('Should be able to serialize and deserialize data correctly', () => {
        const {AggregationInfo, PrivateKey} = blsSignatures;

        const privateKey = PrivateKey.fromSeed(getSeed());
        const message = Uint8Array.from(Buffer.from('Hello world', 'utf8'));
        const sig = privateKey.sign(message);
        assert(sig.verify());

        const info = sig.getAggregationInfo();
        const pubKeyFromInfo = info.getPublicKeys()[0];

        const serializedKeyFromInfo = pubKeyFromInfo.serialize();
        const serializedKeyFromPrivateKey = privateKey.getPublicKey().serialize();

        const messageHash = Uint8Array.from(crypto
            .createHash('sha256')
            .update(message)
            .digest());
        const messageHashFromInfo = info.getMessageHashes()[0];

        assert.deepStrictEqual(serializedKeyFromInfo, serializedKeyFromPrivateKey);
        assert.deepStrictEqual(messageHash, messageHashFromInfo);

        const restoredInfo = AggregationInfo.fromBuffers(
            info.getPublicKeys(),
            info.getMessageHashes(),
            info.getExponents()
        );

        assert.deepStrictEqual(restoredInfo.getPublicKeys()[0].serialize().toString('hex'), info.getPublicKeys()[0].serialize().toString('hex'));
        assert.deepStrictEqual(restoredInfo.getExponents(), info.getExponents());
        assert.deepStrictEqual(restoredInfo.getMessageHashes()[0].toString('hex'), info.getMessageHashes()[0].toString('hex'));
    });
});
