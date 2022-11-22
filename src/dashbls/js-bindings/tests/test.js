const assert = require('assert');
const binascii = require('binascii');
const blsjs = require('../blsjs');

function bytes(x) {
    return Buffer.from(binascii.unhexlify(x), 'binary');
}

function range(n) {
    const arr = [];
    for (var i = 0; i < n; i++) {
        arr.push(i);
    }
    return arr;
}

function repeat(n,v) {
    return range(32).map(() => v);
}

blsjs().then((blsjs) => {
    const modules = [
        'AugSchemeMPL',
        'BasicSchemeMPL',
        "Bignum",
        'G1Element',
        'G2Element',
        'PopSchemeMPL',
        'PrivateKey',
        'Util'
    ];

    // ensure all present
    for (var i = 0; i < modules.length; i++) {
        const m = modules[i];
        if (blsjs[m] === undefined) {
            console.log(`undefined required module ${m}`);
            process.exit(1);
        }
    }

    const {
        AugSchemeMPL,
        BasicSchemeMPL,
        Bignum,
        G1Element,
        G2Element,
        PopSchemeMPL,
        PrivateKey,
        Util
    } = blsjs;

    function test_schemes() {
        var seedArray = [
            0, 50, 6, 244, 24, 199, 1, 25, 52, 88, 192, 19, 18, 12, 89, 6,
            220, 18, 102, 58, 209, 82, 12, 62, 89, 110, 182, 9, 44, 20, 254, 22
        ];
        var seed = Buffer.from(seedArray);

        const msg = Buffer.from([100, 2, 254, 88, 90, 45, 23]);
        const msg2 = Buffer.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
        const sk = BasicSchemeMPL.key_gen(seed);
        const pk = sk.get_g1();

        //assert(sk == PrivateKey.fromBytes(sk.serialize(), false));
        //assert(pk == G1Element.fromBytes(pk.serialize()));

        [BasicSchemeMPL, AugSchemeMPL, PopSchemeMPL].map((Scheme) => {
            const sig = Scheme.sign(sk, msg);
            //assert(sig == G2Element.fromBytes(Buffer.from(sig)));
            assert(Scheme.verify(pk, msg, sig));
        });

        var seed = Buffer.concat([Buffer.from([1]), seed.slice(1)]);
        const sk1 = BasicSchemeMPL.key_gen(seed);
        const pk1 = sk1.get_g1();
        var seed = Buffer.concat([Buffer.from([2]), seed.slice(1)]);
        const sk2 = BasicSchemeMPL.key_gen(seed);
        const pk2 = sk2.get_g1();

        [BasicSchemeMPL, AugSchemeMPL, PopSchemeMPL].map((Scheme) => {
            // Aggregate same message
            const agg_pk = pk1.add(pk2);
            var sig1, sig2;
            if (Scheme === AugSchemeMPL) {
                sig1 = Scheme.sign_prepend(sk1, msg, agg_pk);
                sig2 = Scheme.sign_prepend(sk2, msg, agg_pk);
            } else {
                sig1 = Scheme.sign(sk1, msg);
                sig2 = Scheme.sign(sk2, msg);
            }

            var agg_sig = Scheme.aggregate([sig1, sig2]);
            assert(Scheme.verify(agg_pk, msg, agg_sig));

            // Aggregate different message
            sig1 = Scheme.sign(sk1, msg)
            sig2 = Scheme.sign(sk2, msg2)
            agg_sig = Scheme.aggregate([sig1, sig2])
            assert(Scheme.aggregate_verify([pk1, pk2], [msg, msg2], agg_sig));

            // HD keys
            const child = Scheme.derive_child_sk(sk1, 123);
            const childU = Scheme.derive_child_sk_unhardened(sk1, 123);
            const childUPk = Scheme.derive_child_pk_unhardened(pk1, 123);

            const sig_child = Scheme.sign(child, msg);
            assert(Scheme.verify(child.get_g1(), msg, sig_child));

            const sigU_child = Scheme.sign(childU, msg);
            assert(Scheme.verify(childUPk, msg, sigU_child));
        });
    }

    function test_vectors_invalid() {
        // Invalid inputs from https://github.com/algorand/bls_sigs_ref/blob/master/python-impl/serdesZ.py
        const invalid_inputs_1 = [
            // infinity points: too short
            "c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            // infinity points: not all zeros
            "c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000",
            // bad tags
            "3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
            "7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
            "fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
            // wrong length for compresed point
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaa",
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaaaa",
            // invalid x-coord
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa",
            // invalid elm of Fp --- equal to p (must be strictly less)
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
        ]
        const invalid_inputs_2 = [
            // infinity points: too short
            "c000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            // infinity points: not all zeros
            "c00000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "c00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000",
            // bad tags
            "3a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "7a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "fa0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            // wrong length for compressed point
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            // invalid x-coord
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaa7",
            // invalid elm of Fp --- equal to p (must be strictly less)
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
            "9a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaaa1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab",
        ];

        let g1_exn_count = 0;
        let g2_exn_count = 0;

        invalid_inputs_1.map((s) => {
            const bytes_ = binascii.unhexlify(s);
            try {
                const g1 = G1Element(bytes_);
                console.log(`Failed to disallow creation of G1 element for string ${s}`);
                assert(false);
            } catch(e) {
                g1_exn_count++;
            }
        });

        invalid_inputs_2.map((s) => {
            const bytes_ = binascii.unhexlify(s);
            try {
                const g2 = G2Element(bytes_);
                console.log(`Failed to disallow creation of G2 element for string ${s}`);
                assert(false);
            } catch(e) {
                g2_exn_count++;
            }
        });

        assert(g1_exn_count == invalid_inputs_1.length);
        assert(g2_exn_count == invalid_inputs_2.length);
    }

    function test_vectors_valid() {
        // The following code was used to generate these vectors
        //
        //   from py_ecc.bls import (
        //        G2Basic,
        //        G2MessageAugmentation as G2MA,
        //        G2ProofOfPossession as G2Pop,
        //    )
        //
        //    secret1 = bytes([1] * 32)
        //    secret2 = bytes([x * 314159 % 256 for x in range(32)])
        //    sk1 = int.from_bytes(secret1, 'big')
        //    sk2 = int.from_bytes(secret2, 'big')
        //    msg = bytes([3, 1, 4, 1, 5, 9])
        //    pk1 = G2Basic.SkToPk(sk1)
        //    pk2 = G2Basic.SkToPk(sk2)
        //
        //    for Scheme in (G2Basic, G2MA, G2Pop):
        //        sig1 = Scheme.Sign(sk1, msg)
        //        sig2 = Scheme.Sign(sk2, msg)
        //        sig_agg = Scheme.Aggregate([sig1, sig2])
        //        print(sig1)
        //        print(sig2)
        //        print(sig_agg)
        //
        // Javascript version converts these strings to binascii

        const ref_sig1Basic = bytes('96ba34fac33c7f129d602a0bc8a3d43f9abc014eceaab7359146b4b150e57b808645738f35671e9e10e0d862a30cab70074eb5831d13e6a5b162d01eebe687d0164adbd0a864370a7c222a2768d7704da254f1bf1823665bc2361f9dd8c00e99');
        const ref_sig2Basic = bytes('a402790932130f766af11ba716536683d8c4cfa51947e4f9081fedd692d6dc0cac5b904bee5ea6e25569e36d7be4ca59069a96e34b7f700758b716f9494aaa59a96e74d14a3b552a9a6bc129e717195b9d6006fd6d5cef4768c022e0f7316abf');
        const ref_sigABasic = bytes('987cfd3bcd62280287027483f29c55245ed831f51dd6bd999a6ff1a1f1f1f0b647778b0167359c71505558a76e158e66181ee5125905a642246b01e7fa5ee53d68a4fe9bfb29a8e26601f0b9ad577ddd18876a73317c216ea61f430414ec51c5');
        const ref_sig1Aug = bytes('8180f02ccb72e922b152fcedbe0e1d195210354f70703658e8e08cbebf11d4970eab6ac3ccf715f3fb876df9a9797abd0c1af61aaeadc92c2cfe5c0a56c146cc8c3f7151a073cf5f16df38246724c4aed73ff30ef5daa6aacaed1a26ecaa336b');
        const ref_sig2Aug = bytes('99111eeafb412da61e4c37d3e806c6fd6ac9f3870e54da9222ba4e494822c5b7656731fa7a645934d04b559e9261b86201bbee57055250a459a2da10e51f9c1a6941297ffc5d970a557236d0bdeb7cf8ff18800b08633871a0f0a7ea42f47480');
        const ref_sigAAug = bytes('8c5d03f9dae77e19a5945a06a214836edb8e03b851525d84b9de6440e68fc0ca7303eeed390d863c9b55a8cf6d59140a01b58847881eb5af67734d44b2555646c6616c39ab88d253299acc1eb1b19ddb9bfcbe76e28addf671d116c052bb1847');
        const ref_sig1Pop = bytes('9550fb4e7f7e8cc4a90be8560ab5a798b0b23000b6a54a2117520210f986f3f281b376f259c0b78062d1eb3192b3d9bb049f59ecc1b03a7049eb665e0df36494ae4cb5f1136ccaeefc9958cb30c3333d3d43f07148c386299a7b1bfc0dc5cf7c');
        const ref_sig2Pop = bytes('a69036bc11ae5efcbf6180afe39addde7e27731ec40257bfdc3c37f17b8df68306a34ebd10e9e32a35253750df5c87c2142f8207e8d5654712b4e554f585fb6846ff3804e429a9f8a1b4c56b75d0869ed67580d789870babe2c7c8a9d51e7b2a');
        const ref_sigAPop = bytes('a4ea742bcdc1553e9ca4e560be7e5e6c6efa6a64dddf9ca3bb2854233d85a6aac1b76ec7d103db4e33148b82af9923db05934a6ece9a7101cd8a9d47ce27978056b0f5900021818c45698afdd6cf8a6b6f7fee1f0b43716f55e413d4b87a6039');

        const secret1 = Buffer.from(repeat(32,1));
        const secret2 = Buffer.from(range(32).map((x) => x * 314159 % 256));
        const sk1 = PrivateKey.fromBytes(secret1, false);
        const sk2 = PrivateKey.fromBytes(secret2, false);

        const msg = Buffer.from([3, 1, 4, 1, 5, 9]);
        const sig1Basic = BasicSchemeMPL.sign(sk1, msg)
        const sig2Basic = BasicSchemeMPL.sign(sk2, msg)
        const sigABasic = BasicSchemeMPL.aggregate([sig1Basic, sig2Basic])
        const sig1Aug = AugSchemeMPL.sign(sk1, msg)
        const sig2Aug = AugSchemeMPL.sign(sk2, msg)
        const sigAAug = AugSchemeMPL.aggregate([sig1Aug, sig2Aug])
        const sig1Pop = PopSchemeMPL.sign(sk1, msg)
        const sig2Pop = PopSchemeMPL.sign(sk2, msg)
        const sigAPop = PopSchemeMPL.aggregate([sig1Pop, sig2Pop])

        assert(Buffer.from(sig1Basic.serialize()).equals(ref_sig1Basic));
        assert(Buffer.from(sig2Basic.serialize()).equals(ref_sig2Basic));
        assert(Buffer.from(sigABasic.serialize()).equals(ref_sigABasic));
        assert(Buffer.from(sig1Aug.serialize()).equals(ref_sig1Aug));
        assert(Buffer.from(sig2Aug.serialize()).equals(ref_sig2Aug));
        assert(Buffer.from(sigAAug.serialize()).equals(ref_sigAAug));
        assert(Buffer.from(sig1Pop.serialize()).equals(ref_sig1Pop));
        assert(Buffer.from(sig2Pop.serialize()).equals(ref_sig2Pop));
        assert(Buffer.from(sigAPop.serialize()).equals(ref_sigAPop));
    }

    function test_readme() {
        let seed = Buffer.from(
            [
                0,
                50,
                6,
                244,
                24,
                199,
                1,
                25,
                52,
                88,
                192,
                19,
                18,
                12,
                89,
                6,
                220,
                18,
                102,
                58,
                209,
                82,
                12,
                62,
                89,
                110,
                182,
                9,
                44,
                20,
                254,
                22,
            ]
        );
        let sk = AugSchemeMPL.key_gen(seed);
        let pk = sk.get_g1();

        const message = Buffer.from([1, 2, 3, 4, 5]);
        let signature = AugSchemeMPL.sign(sk, message);

        let ok = AugSchemeMPL.verify(pk, message, signature);
        assert(ok);

        const sk_bytes = sk.serialize();  // 32 bytes
        const pk_bytes = pk.serialize();  // 48 bytes
        const signature_bytes = signature.serialize();  // 96 bytes

        console.log(Buffer.from(sk_bytes).toString('hex'), Buffer.from(pk_bytes).toString('hex'), Buffer.from(signature_bytes).toString('hex'));

        sk = PrivateKey.fromBytes(sk_bytes, false);
        pk = G1Element.fromBytes(pk_bytes);
        signature = G2Element.fromBytes(signature_bytes);

        seed = Buffer.concat([Buffer.from([1]), seed.slice(1)]);
        const sk1 = AugSchemeMPL.key_gen(seed);
        seed = Buffer.concat([Buffer.from([2]), seed.slice(1)]);
        const sk2 = AugSchemeMPL.key_gen(seed);
        const message2 = Buffer.from([1, 2, 3, 4, 5, 6, 7]);

        const pk1 = sk1.get_g1();
        const sig1 = AugSchemeMPL.sign(sk1, message);

        const pk2 = sk2.get_g1();
        const sig2 = AugSchemeMPL.sign(sk2, message2);

        const agg_sig = AugSchemeMPL.aggregate([sig1, sig2]);

        ok = AugSchemeMPL.aggregate_verify([pk1, pk2], [message, message2], agg_sig);
        assert(ok);

        seed = Buffer.concat([Buffer.from([3]), seed.slice(1)]);
        const sk3 = AugSchemeMPL.key_gen(seed);
        const pk3 = sk3.get_g1();
        const message3 = Buffer.from([100, 2, 254, 88, 90, 45, 23]);
        const sig3 = AugSchemeMPL.sign(sk3, message3);

        const agg_sig_final = AugSchemeMPL.aggregate([agg_sig, sig3]);
        ok = AugSchemeMPL.aggregate_verify(
            [pk1, pk2, pk3], [message, message2, message3], agg_sig_final
        );
        assert(ok);

        const pop_sig1 = PopSchemeMPL.sign(sk1, message);
        const pop_sig2 = PopSchemeMPL.sign(sk2, message);
        const pop_sig3 = PopSchemeMPL.sign(sk3, message);
        const pop1 = PopSchemeMPL.pop_prove(sk1);
        const pop2 = PopSchemeMPL.pop_prove(sk2);
        const pop3 = PopSchemeMPL.pop_prove(sk3);

        ok = PopSchemeMPL.pop_verify(pk1, pop1);
        assert(ok);
        ok = PopSchemeMPL.pop_verify(pk2, pop2);
        assert(ok);
        ok = PopSchemeMPL.pop_verify(pk3, pop3);
        assert(ok);

        const pop_sig_agg = PopSchemeMPL.aggregate([pop_sig1, pop_sig2, pop_sig3]);

        ok = PopSchemeMPL.fast_aggregate_verify([pk1, pk2, pk3], message, pop_sig_agg);
        assert(ok);

        const pop_agg_pk = pk1.add(pk2).add(pk3);
        assert(Buffer.from(pop_agg_pk.serialize()).toString('hex') == '8ffb2a3c4a6ce516febdf1f2a5140c6974cce2530ebc1b56629f078c25d538aa3d8f0cf694516b4bfc8d344bbea829d3');
        const t_pop_agg_pk_1 = pk1.add(pk2.add(pk3));
        assert(Buffer.from(pop_agg_pk.serialize()).equals(Buffer.from(t_pop_agg_pk_1.serialize())));

        assert(Buffer.from(pop_sig_agg.serialize()).toString('hex') == 'b3115327c21286b043cf972db56cefe0a745a8091482c9eec47fe5c0dbdb2ffc4d05ae54e7ee45e209135e6c81e8af2212465abffe8ab3166d3f4d5ed1f4d98cb65323d8d1fc210311c7e46a5f637ef7d90b7f3206708dc4bb32a2e1549a3060');
        ok = PopSchemeMPL.verify(pop_agg_pk, message, pop_sig_agg);
        assert(ok);

        const pop_agg_sk = PrivateKey.aggregate([sk1, sk2, sk3]);
        ok = Buffer.from(PopSchemeMPL.sign(pop_agg_sk, message).serialize()).equals(Buffer.from(pop_sig_agg.serialize()));
        assert(ok);

        const master_sk = AugSchemeMPL.key_gen(seed);
        const child = AugSchemeMPL.derive_child_sk(master_sk, 152);
        const grandchild = AugSchemeMPL.derive_child_sk(child, 952);

        const master_pk = master_sk.get_g1();
        const child_u = AugSchemeMPL.derive_child_sk_unhardened(master_sk, 22);
        const grandchild_u = AugSchemeMPL.derive_child_sk_unhardened(child_u, 0);

        const child_u_pk = AugSchemeMPL.derive_child_pk_unhardened(master_pk, 22);
        const grandchild_u_pk = AugSchemeMPL.derive_child_pk_unhardened(child_u_pk, 0);

        ok = Buffer.from(grandchild_u_pk.serialize()).equals(Buffer.from(grandchild_u.get_g1().serialize()));
        assert(ok);
    }

    function test_aggregate_verify_zero_items() {
        assert(AugSchemeMPL.aggregate_verify([], [], new G2Element()));
    }

    function test_bignum() {
        const mersenne = Bignum.fromString('162259276829213363391578010288127', 10);
        assert(mersenne.toString(16).toLowerCase() == '7ffffffffffffffffffffffffff');
    }

    test_schemes();
    test_vectors_invalid();
    test_vectors_valid();
    test_readme();
    test_aggregate_verify_zero_items();
    test_bignum();
}).then(function() {
    console.log("\nAll tests passed.");
});

const copyright = [
    'Copyright 2020 Chia Network Inc',
    'Licensed under the Apache License, Version 2.0 (the "License");',
    'you may not use this file except in compliance with the License.',
    'You may obtain a copy of the License at',
    'http://www.apache.org/licenses/LICENSE-2.0',
    'Unless required by applicable law or agreed to in writing, software',
    'distributed under the License is distributed on an "AS IS" BASIS,',
    'WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.',
    'See the License for the specific language governing permissions and',
    'limitations under the License.'
];
