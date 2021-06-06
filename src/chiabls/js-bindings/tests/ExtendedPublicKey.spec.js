const blsSignatures = require('..')();
const assert = require('assert');

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('ExtendedPublicKey', () => {
    it('Should return correct data structures', () => {
        const {ExtendedPrivateKey} = blsSignatures;

        const seed = Uint8Array.from([1, 50, 6, 244, 24, 199, 1, 0, 0, 0]);
        const esk = ExtendedPrivateKey.fromSeed(seed);
        const epk = esk.getExtendedPublicKey();

        assert.strictEqual(Buffer.from(epk.getPublicKey().serialize()).toString('hex'), '13cfff346826510d1b60a5754c4e906d0696b21a43492b6681fb625ffa77e49c10fc06d7588cfc986431ba05bfece592');
        assert.strictEqual(Buffer.from(epk.getChainCode().serialize()).toString('hex'), '9728accd39f28f05077f224440e7f1781684e22855de41170ef2af0b75022083');

        const sig1 = esk.getPrivateKey().sign(seed);
        assert.strictEqual(Buffer.from(sig1.serialize()).toString('hex'), '916c053e48c7b8946b429175c53651e2532da6fa7d3d417667a7798322d8f32d6df85a428d0cb6e312fecd83386478a515bb3e6f94189c4447a428182e7daa8991b83c7fd4fcc18494d64da660d81cb639002413fb639517be689dea5bb31bde');
    });
});
