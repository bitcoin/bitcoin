// This file is used to check if the typescript typings are working
import createBlsSignaturesModule from '..';
import {deepStrictEqual, ok, strictEqual} from 'assert';
import {createHash} from 'crypto';

createBlsSignaturesModule().then((blsSignatures) => {
    const {
        AugSchemeMPL,
        PrivateKey,
        G1Element,
        G2Element
    } = blsSignatures;

    function makehash(msg : Uint8Array) : Uint8Array {
        return createHash('sha256').update(msg).digest();
    }

    function getSkSeed(): Uint8Array {
        return Uint8Array.from([
            0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6, 220,
            18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22
         ]);
    }

    function getSkBytes(): Uint8Array {
        return Uint8Array.from([
            55, 112, 145, 240, 231, 40,  70,  59,
            194, 218, 125,  84, 108, 83, 185, 246,
            184,  29, 244, 161, 204, 26, 181, 191,
            41, 197, 144, 139, 113, 81, 163,  45
        ]);
    }

    function getPkBytes(): Uint8Array {
        return Uint8Array.from([
            134,  36,  50, 144, 187, 203, 253, 154, 231,
            91, 222, 206, 121, 129, 150,  83,  80,  32,
            142, 181, 233, 155,   4, 213, 205,  36, 233,
            85, 173, 169,  97, 248, 192, 161,  98, 222,
            231,  64, 190, 123, 220, 108,  60,   6,  19,
            186,  46, 177
        ]);
    }

    function getMessageBytes(): Uint8Array {
        return Uint8Array.from([1, 2, 3]);
    }

    function getMessageHash(): Uint8Array {
        return Uint8Array.from(createHash('sha256').update(getMessageBytes()).digest());
    }

    function getSignatureBytes(): Uint8Array {
        return Uint8Array.from([
            150,  18, 129, 243, 101, 246,  44,  55,  26, 188,  24,  50,
            34, 223, 147,  98, 184, 115, 124,  54, 133,  86, 100, 135,
            205, 155, 191, 212,  36,  49,   2, 244, 241, 202, 143, 223,
            89,  34,   7, 115,  96, 209,  22,  24, 100,  22,  55, 175,
            2, 199, 236, 131, 152, 217,  76, 146,  32,  13, 235,  31,
            129,  47,  84, 187, 229, 161,  27,  89, 214,  21,   7, 240,
            1, 130, 214,  13, 134, 189, 139, 112,  40,  94, 221,  76,
            249, 168, 114, 254,  55,  45,  42,  30,  14,  22, 107,  10
        ]);
    }

    function getChainCodeBytes(): Uint8Array {
        return Uint8Array.from([137, 75, 79, 148, 193, 235, 158, 172, 163, 41, 102, 134, 72, 161, 187, 104, 97, 202, 38, 27, 206, 125, 64, 60, 149, 248, 29, 53, 180, 23, 253, 255]);
    }

    describe('typings', () => {
        it('PrivateKey', () => {
            strictEqual(PrivateKey.PRIVATE_KEY_SIZE, 32);
            const sk = AugSchemeMPL.key_gen(getSkSeed());
            const aggSk = PrivateKey.aggregate([sk]);
            const pk = sk.get_g1();
            const bytes: Uint8Array = sk.serialize();
            const sig = AugSchemeMPL.sign(sk, getMessageBytes());
            ok(AugSchemeMPL.verify(pk, getMessageBytes(), sig));
            aggSk.delete();
            pk.delete();
            sig.delete();
        });

        it('G1Element', () => {
            strictEqual(G1Element.SIZE, 48);
            const pk = G1Element.from_bytes(getPkBytes());
            const aggPk = pk.add(pk);
            const fingerprint: number = pk.get_fingerprint();
            const bytes: Uint8Array = pk.serialize();
            pk.delete();
            aggPk.delete();
        });

        it('G2Element', () => {
            strictEqual(G2Element.SIZE, 96);
            const pk = G1Element.from_bytes(getPkBytes());
            const sig = G2Element.from_bytes(getSignatureBytes());
            const aggSig = AugSchemeMPL.aggregate([sig]);
            const sig2 = G2Element.from_bytes(getSignatureBytes());
            const isValid: boolean =
              AugSchemeMPL.verify(pk, getMessageBytes(), sig);
            const serialized: Uint8Array = sig.serialize();
            ok(isValid);
            sig.delete();
            aggSig.delete();
            sig2.delete();
        });
    });
});
