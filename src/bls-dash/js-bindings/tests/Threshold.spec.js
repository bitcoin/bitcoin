const blsSignatures = require('..')();
const assert = require('assert');
const crypto = require('crypto');

before((done) => {
    blsSignatures.then(() => {
        done();
    });
});

describe('Threshold', () => {
    it('Should be able to verify secret fragment', function () {
        const {Threshold, PublicKey, PrivateKey, InsecureSignature} = blsSignatures;

        this.timeout(10000);
        // To initialize a T of N threshold key under a
        // Joint-Feldman scheme:
        const T = 2;
        const N = 3;

        // 1. Each player calls Threshold.create.
        // They send everyone commitment to the polynomial,
        // and send secret share fragments frags[j-1] to
        // the j-th player (All players have index >= 1).

        // PublicKey commits[N][T]
        // PrivateKey frags[N][N]
        const commitments = [
            [
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('93075db5c398bd2682dfab816a920023e8c0337a42fafc93c0bfab937400b6e7dafa5e456f2fa127979ff9c9a140127b', 'hex'))),
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('1304180c71f137a1dc4c39c6e997285f55d9dd11f53c52f8fc7702a5b08f529190ffc01b8b6b28b64ee9704f26ca02c3', 'hex'))),
            ],
            [
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('05888f29bcad893fd7997b8b5b49fcfa276df701ded1452ab6b5d40c101333af1348a041878a970c40643ba63d7e8468', 'hex'))),
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('8af360512dfa9d18eb6cd375e3e87ec713b8ebc68853a41b6be31c3d19b6373b9f5e80845214cdd244b0a6ecd8d4bf51', 'hex'))),
            ],
            [
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('83fb5434aef48a2478f535fb69043c6653a9cf51a33d6c956ba75c0e1da749fba2652a933cb0e091ceef7ab8a7b69488', 'hex'))),
                PublicKey.fromBytes(Uint8Array.from(Buffer.from('93adf1615e23b04e16d5cae180d46e613c7f17c6173cd73079c8c0f851e688f2597b158599bb77e1b1467c43bd6c74ac', 'hex'))),
            ],
        ];
        const fragments = [
            [
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('0b5205ed2c9aa86391dc8c7de15efa868073d83fb782f92de81e3d21916e296e', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('56720e3ca7a2d6856fa16ef94eb2c2957066b849cea8a4da7fde0613efbc5401', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('49d151bb34da1f1957077e231e7cdf564f418b74cb9b3b1c751714cb649631c3', 'hex')), false),
            ],
            [
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('2d88c55ae942997b8c2e8259cd4910bf686137377f2e9679db93c69251cf7593', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('1ac23ad6960e2dc6f693b0907d810c80821848d6c880137a5eec1ea6db165135', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('621c2977b244fcae5f7fc24e94afad8e693b45c517fc5a06f17fff6d55167ff2', 'hex')), false),
            ],
            [
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('4fbf84c8a5ea8a9386807835b93326f8504e962f46da33c5cf0950031230c1b8', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('53000ec3ae170250b0bfca2fb5f12e70e7877d66c255de193dfa3738c6704e6a', 'hex')), false),
                PrivateKey.fromBytes(Uint8Array.from(Buffer.from('067959e106125cfb34be2e720140a3c12f775c12645f1cf26de8ea104596ce20', 'hex')), false),
            ]
        ];

        const sk1 = Threshold.create(commitments[0], fragments[0], T, N);
        const sk2 = Threshold.create(commitments[1], fragments[1], T, N);
        const sk3 = Threshold.create(commitments[2], fragments[2], T, N);

        // 2. Each player calls Threshold::VerifySecretFragment
        // on all secret fragments they receive.  If any verify
        // false, they complain to abort the scheme.  (Note that
        // repeatedly aborting, or 'speaking' last, can bias the
        // master public key.)

        for (let target = 0; target < N; target++) {
            for (let source = 0; source < N; source++) {
                assert(Threshold.verifySecretFragment(target + 1, fragments[source][target], commitments[source], T));
            }
        }

        // 3. Each player computes the shared, master public key:
        // masterPubkey = PublicKey::AggregateInsecure(...)
        // They also create their secret share from all secret
        // fragments received (now verified):
        // secretShare = PrivateKey::AggregateInsecure(...)

        const masterPubkey = PublicKey.aggregateInsecure([
            commitments[0][0], commitments[1][0], commitments[2][0]
        ]);

        // receivedSecretFragments[j][i] = frags[i][j]
        const receivedSecretFragments = [];
        for (let i = 0; i < N; ++i) {
            receivedSecretFragments.push([]);
            for (let j = 0; j < N; ++j) {
                receivedSecretFragments[i].push(fragments[j][i]);
            }
        }

        const secretShare1 = PrivateKey.aggregateInsecure(receivedSecretFragments[0]);
        const secretShare2 = PrivateKey.aggregateInsecure(receivedSecretFragments[1]);
        const secretShare3 = PrivateKey.aggregateInsecure(receivedSecretFragments[2]);

        // 4a. Player P creates a pre-multiplied signature share wrt T players:
        // sigShare = Threshold::SignWithCoefficient(...)
        // These signature shares can be combined to sign the msg:
        // signature = InsecureSignature::Aggregate(...)
        // The advantage of this approach is that forming the final signature
        // no longer requires information about the players.

        const msg = Uint8Array.from([100, 2, 254, 88, 90, 45, 23]);
        const hash = crypto
            .createHash('sha256')
            .update(msg)
            .digest();

        const players = [1, 3];
        // For example, players 1 and 3 sign.
        // As we have verified the coefficients through the commitments given,
        // using InsecureSignature is okay.
        const sigShareC1 = Threshold.signWithCoefficient(secretShare1, msg, 1, players);
        const sigShareC3 = Threshold.signWithCoefficient(secretShare3, msg, 3, players);

        const signature = InsecureSignature.aggregate([sigShareC1, sigShareC3]);

        assert(signature.verify([hash], [masterPubkey]));

        // 4b. Alternatively, players may sign the message blindly, creating
        // a unit signature share: sigShare = secretShare.SignInsecure(...)
        // These signatures may be combined with lagrange coefficients to
        // sign the message: signature = Threshold::AggregateUnitSigs(...)
        // The advantage to this approach is that each player does not need
        // to know the final list of signatories.

        // For example, players 1 and 3 sign.
        const sigShareU1 = secretShare1.signInsecure(msg);
        const sigShareU3 = secretShare3.signInsecure(msg);
        const signature2 = Threshold.aggregateUnitSigs(
            [sigShareU1, sigShareU3], msg, players
        );

        assert(signature2.verify([hash], [masterPubkey]));
    });
});
