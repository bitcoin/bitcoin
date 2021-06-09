const blsSignatures = require('..')();
const assert = require('assert');

function getSeed() {
    return Uint8Array.from([1, 50, 6, 244, 24, 199, 1, 25]);
}

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('ExtendedPrivateKey', () => {
    it('Should derive correctly', () => {
        const {ExtendedPrivateKey} = blsSignatures;

        const seed = getSeed();
        const esk = ExtendedPrivateKey.fromSeed(seed);
        assert.strictEqual(esk.getPublicKey().getFingerprint(), 0xa4700b27);

        let chainCode = esk.getChainCode().serialize();
        assert.strictEqual(Buffer.from(chainCode).toString('hex'), 'd8b12555b4cc5578951e4a7c80031e22019cc0dce168b3ed88115311b8feb1e3');

        const esk77 = esk.privateChild(2147483725);
        chainCode = esk77.getChainCode().serialize();
        assert.strictEqual(Buffer.from(chainCode).toString('hex'), 'f2c8e4269bb3e54f8179a5c6976d92ca14c3260dd729981e9d15f53049fd698b');
        assert.strictEqual(esk77.getPrivateKey().getPublicKey().getFingerprint(), 0xa8063dcf);

        assert.strictEqual(esk.privateChild(3)
            .privateChild(17)
            .getPublicKey()
            .getFingerprint(), 0xff26a31f);
        assert.strictEqual(esk.getExtendedPublicKey()
            .publicChild(3)
            .publicChild(17)
            .getPublicKey()
            .getFingerprint(), 0xff26a31f);
    });
});
