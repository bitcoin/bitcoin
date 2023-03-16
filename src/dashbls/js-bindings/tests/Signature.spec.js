const blsSignaturesModule = require('..')();
const assert = require('assert');
const crypto = require('crypto');
const {Buffer} = require('buffer');

// Current format of signature taken from test.js, ported from test.py
function getSignatureHex() {
    return '900d5223412ee471b42dbb8a6706de5f5eba4ca7e1a0fb6b3fa431f510b3e6d73c440748693d787e1a25c8bc4596f66b1130634f4b32e8e09f52f2c6a843b700cbb5aeadb8ab3456d002c143be68998573166e5979e6a48fcbb67ac8fd981f73';
}

function getSignatureBytes() {
    return Uint8Array.from(Buffer.from(getSignatureHex(), 'hex'));
}

function makehash(msg) {
    return crypto
        .createHash('sha256')
        .update(msg)
        .digest();
}

var blsSignatures;
before((done) => {
    blsSignaturesModule.then((mod) => {
        blsSignatures = mod;
        done();
    });
});

describe('Signature', () => {
    describe('Integration', () => {
        it('Should verify signatures', function () {
            const {BasicSchemeMPL, G2Element, G1Element, PrivateKey} = blsSignatures;

            this.timeout(10000);
            const message = Uint8Array.from([100, 2, 254, 88, 90, 45, 23]);
            const seed1 = makehash(Uint8Array.from([1, 2, 3, 4, 5]));
            const seed2 = makehash(Uint8Array.from([3, 4, 5, 6, 7]));
            const seed3 = makehash(Uint8Array.from([4, 5, 6, 7, 8]));

            const privateKey1 = BasicSchemeMPL.keyGen(seed1);
            const privateKey2 = BasicSchemeMPL.keyGen(seed2);
            const privateKey3 = BasicSchemeMPL.keyGen(seed3);

            const publicKey1 = BasicSchemeMPL.skToG1(privateKey1);
            const publicKey2 = BasicSchemeMPL.skToG1(privateKey2);
            const publicKey3 = BasicSchemeMPL.skToG1(privateKey3);

            const sig1 = BasicSchemeMPL.sign(privateKey1, message);
            const sig2 = BasicSchemeMPL.sign(privateKey2, message);
            const sig3 = BasicSchemeMPL.sign(privateKey3, message);

            assert(BasicSchemeMPL.verify(publicKey1, message, sig1), 'Signature 1 is not verifiable');
            assert(BasicSchemeMPL.verify(publicKey2, message, sig2), 'Signature 2 is not verifiable');
            assert(BasicSchemeMPL.verify(publicKey3, message, sig3), 'Signature 3 is not verifiable');

            const aggregatedSignature = BasicSchemeMPL.aggregate([sig1, sig2, sig3]);
            // PublicKey aggregate was replaced with G1Element add.
            const aggregatedPubKey = publicKey1.add(publicKey2).add(publicKey3);

            assert(BasicSchemeMPL.verify(aggregatedPubKey, message, aggregatedSignature));

            privateKey1.delete();
            privateKey2.delete();
            privateKey3.delete();
            sig1.delete();
            sig2.delete();
            sig3.delete();
            aggregatedSignature.delete();
            aggregatedPubKey.delete();
        });
    });
    describe('.fromBytes', () => {
        it('Should create verifiable signature from bytes', () => {
            const {AugSchemeMPL, G2Element, Util} = blsSignatures;

            const sig = G2Element.fromBytes(getSignatureBytes());

            assert.strictEqual(Buffer.from(sig.serialize()).toString('hex'), getSignatureHex());
            // Since there is no aggregation info, it's impossible to verify sig
            // This iteration of the library differs in that Signature objects
            // aren't stateful and therefore can't be self-verified without a
            // message and public key.

            sig.delete();
        });
    });
    describe('.aggregateSigs', () => {
        it('Should aggregate signature', () => {
            const {AugSchemeMPL, G2Element, PrivateKey} = blsSignatures;

            const sk = AugSchemeMPL.keyGen(makehash(Uint8Array.from([1, 2, 3])));
            const pk = AugSchemeMPL.skToG1(sk);
            const msg1 = Uint8Array.from([3, 4, 5]);
            const msg2 = Uint8Array.from([6, 7, 8]);
            const sig1 = AugSchemeMPL.sign(sk, msg1);
            const sig2 = AugSchemeMPL.sign(sk, msg2);
            const aggregatedSig = G2Element.aggregateSigs([sig1, sig2]);
            assert.strictEqual(AugSchemeMPL.aggregateVerify([pk, pk], [msg1, msg2], aggregatedSig), true);

            sk.delete();
            pk.delete();
            sig1.delete();
            sig2.delete();
            aggregatedSig.delete();
        });
    });
    describe('#serialize', () => {
        it('Should serialize signature to Buffer', () => {
            const {AugSchemeMPL, G2Element, PrivateKey} = blsSignatures;

            const sk = AugSchemeMPL.keyGen(makehash(Uint8Array.from([1, 2, 3, 4, 5])));
            const sig = AugSchemeMPL.sign(sk, Uint8Array.from([100, 2, 254, 88, 90, 45, 23]));
            assert(sig instanceof G2Element);
            assert.deepStrictEqual(Buffer.from(sig.serialize()).toString('hex'), getSignatureHex());

            sk.delete();
            sig.delete();
        });
    });
    describe('#verify', () => {
        it('Should return true if signature can be verified', () => {
            const {AugSchemeMPL, G2Element, G1Element} = blsSignatures;

            const message = Uint8Array.from(Buffer.from('Message'));
            const seed1 = makehash(Buffer.from([1, 2, 3, 4, 5]));
            const seed2 = makehash(Buffer.from([1, 2, 3, 4, 6]));
            const sk1 = AugSchemeMPL.keyGen(seed1);
            const sk2 = AugSchemeMPL.keyGen(seed2);
            const pk1 = AugSchemeMPL.skToG1(sk1);
            const pk2 = AugSchemeMPL.skToG1(sk2);
            const sig1 = AugSchemeMPL.sign(sk1, message);
            const sig2 = AugSchemeMPL.sign(sk2, message);
            const sig = AugSchemeMPL.aggregate([sig1, sig2]);

            assert(AugSchemeMPL.aggregateVerify([pk1, pk2], [message, message], sig));

            sk1.delete();
            sk2.delete();
            pk1.delete();
            pk2.delete();
            sig1.delete();
            sig2.delete();
            sig.delete();
        });
        it("Should return false if signature can't be verified", () => {
            const {AugSchemeMPL, G1Element, PrivateKey} = blsSignatures;

            const message1 = Uint8Array.from(Buffer.from('Message'));
            const message2 = Uint8Array.from(Buffer.from('Nessage'));
            const seed = makehash(Buffer.from([1, 2, 3, 4, 5]));
            const sk = AugSchemeMPL.keyGen(seed);
            const pk = AugSchemeMPL.skToG1(sk);
            const sig = AugSchemeMPL.sign(sk, message1);
            assert.strictEqual(AugSchemeMPL.verify(pk, message2, sig), false);

            sk.delete();
            pk.delete();
            sig.delete();
        });
    });
});
