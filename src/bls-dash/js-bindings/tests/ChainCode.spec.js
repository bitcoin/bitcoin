const blsSignatures = require('..')();
const assert = require('assert');

function getChainCodeHex() {
    return '9728accd39f28f05077f224440e7f1781684e22855de41170ef2af0b75022083';
}

function getChainCodeBytes() {
    return Uint8Array.from(Buffer.from(getChainCodeHex(), 'hex'));
}

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('ChainCode', () => {
    it('Should create chain code from a buffer and serialize to the same buffer', () => {
        const {ChainCode} = blsSignatures;

        const chainCode = ChainCode.fromBytes(getChainCodeBytes());
        assert(chainCode instanceof ChainCode);
        const serialized = chainCode.serialize();
        assert.strictEqual(Buffer.from(serialized).toString('hex'), getChainCodeHex());
    });
});
