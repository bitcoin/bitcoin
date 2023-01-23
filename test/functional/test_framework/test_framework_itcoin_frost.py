from test_framework.test_node import TestNode
from test_framework.address import key_to_p2pkh, byte_to_base58
from test_framework.conftest import SIGNET
from test_framework.frost import FROST
from test_framework.key import TaggedHash, SECP256K1_ORDER, ECKey
from test_framework.script import SIGHASH_DEFAULT, TaprootSignatureHash, CScriptOp
from test_framework.test_framework_itcoin import BaseItcoinTest
from test_framework.util import assert_equal
from test_framework.messages import ser_string_vector
import time

# import itcoin's miner
from test_framework.itcoin_abs_import import import_miner

miner = import_miner()


class BaseFrostTest(BaseItcoinTest):
    TAPTWEAK_CONTEXT = 'TapTweak'
    participants = []

    def set_test_params(self, set_signet_challenge_as_extra_arg=True, tweak_public_key=True) -> None:
        """Configure the test."""
        self.signet_num_signers: int
        self.signet_num_signatures: int

        # Validate parameters
        if not (0 < self.signet_num_signatures <= self.signet_num_signers):
            raise ValueError(f"expected 0 < K <= N, got K={self.signet_num_signatures} and N={self.signet_num_signers}, please configure signet_num_signers and signet_num_signatures")

        # parent class attributes
        self.chain = SIGNET
        self.requires_wallet = True
        self.setup_clean_chain = True

        # Define FROST participants and perform key generation
        self.participants = []
        for i in range(self.signet_num_signers):
            p = FROST.Participant(index=i+1, threshold=self.signet_num_signatures, participants=self.signet_num_signers)
            p.init_keygen()
            p.generate_shares()
            self.participants.append(p)

        for p in self.participants:
            p.aggregate_shares([participant.shares[p.index-1] for participant in self.participants if participant.index != p.index])

        # We need to set the key pairs
        keypairs = []
        for p in self.participants:
            aggregate_share_bytes = p.aggregate_share.to_bytes(32, byteorder='big')
            priv_key = ECKey()
            priv_key.set(aggregate_share_bytes, True)
            pub_key = priv_key.get_pubkey()
            # Note that private keys for compressed and uncompressed bitcoin
            # public keys use the same version byte (239).
            # The reason for the compressed form starting with a different
            # character ('c' instead of '9') is because a 0x01 byte is appended
            # to the private key before base58 encoding.
            priv_key_b58 = byte_to_base58(bytes(priv_key.get_bytes() + b"\x01"), 239)
            address = key_to_p2pkh(pub_key.get_bytes().hex())
            keypairs.append(TestNode.AddressKeyPair(address, priv_key_b58))
        TestNode.PRIV_KEYS[:self.signet_num_signers] = keypairs
        self._key_pairs = keypairs

        if (tweak_public_key):
            # 1. Derive pubkey without tweak, Y = ∏ 𝜙_j_0, 1 ≤ j ≤ n
            public_key = FROST.Point()
            for p in self.participants:
                public_key = public_key + p.coefficient_commitments[0]

            # 2. Tweak the public key
            # As per BIP-341 If the spending conditions do not require a script
            # path, the output key should commit to an unspendable script path
            # instead of having no script path. This can be achieved by
            # computing the output key point as
            #     Q = Y + int(hash_{TapTweak}(bytes(Y)))G
            # where
            #     hash_{TapTweak}(bytes(Y)) = sha256(sha256("TapTweak") + sha256("TapTweak") + data)
            tweak_bytes = TaggedHash(self.TAPTWEAK_CONTEXT, public_key.x.to_bytes(32, byteorder='big'))
            for participant in self.participants:
                public_key = participant.derive_public_key([p.coefficient_commitments[0] for p in self.participants if p.index != participant.index], tweak_bytes)
        else:
            for participant in self.participants:
                public_key = participant.derive_public_key([p.coefficient_commitments[0] for p in self.participants if p.index != participant.index])

        # 3. Derive Witness Program
        witness_program = public_key.sec_serialize()[1:].hex()

        # 4. Derive Signet Challenge
        self.signet_challenge = '5120' + witness_program
        if set_signet_challenge_as_extra_arg:
            arg_signetchallenge = f"-signetchallenge={self.signet_challenge}"
            self.extra_args = [[arg_signetchallenge]] * self.num_nodes

        return

    def setup_nodes(self):
        """Set up nodes."""
        super().setup_nodes()
        # Set an address corresponding to the (tweaked or not) public key to
        # nodes (to mine blocks)
        address = key_to_p2pkh(self.participants[0].public_key.sec_serialize().hex())
        for i in range(self.signet_num_signers):
            self.node(i).args.address = address

    def run_test(self) -> None:
        """
        Run the test.

        This method must be overridden and implemented by subclasses.
        """
        raise NotImplementedError

    def mine_block(self):
        # Generate block + coinbase tx
        args0 = self.node(0).args
        block = miner.do_generate_next_block(args0)[0]

        # Creating the signet transactions
        signet_spk = self.signet_challenge
        signet_spk_bin = bytes.fromhex(signet_spk)
        signme, spendme = miner.signet_txs(block, signet_spk_bin)

        # Sign sign_me tx
        signme_signed = self.schnorr_sign_tx(signme, spendme, [1, 2, 3])

        # Derive the signet solution
        signet_solution = b"\x00" + ser_string_vector([signme_signed.vin[0].scriptSig])

        # Append the signet solution
        #
        # - we remove the last 5 bytes, in order to remove the previously
        #   appended SIGNET_HEADER;
        # - we append SIGNET_HEADER and signet_solution again as a single
        #   pushdata operation.
        block.vtx[0].vout[-1].scriptPubKey = block.vtx[0].vout[-1].scriptPubKey[:-5] + CScriptOp.encode_op_pushdata(miner.SIGNET_HEADER + signet_solution)
        block.vtx[0].rehash()

        # Propagate signed block
        miner.do_propagate_block(args0, block)
        time.sleep(1)

        # Check 1st block mined correctly on node 0
        self.assert_blockchaininfo_property(0, "blocks", 1)

        # Check that 1st block propagates correctly to node 1
        assert_equal(self.nodes[1].getblockchaininfo()["blocks"], 1)

    def schnorr_sign_tx(self, sign_tx, spend_tx, participant_indexes):
        sighash = TaprootSignatureHash(sign_tx, spend_tx.vout, SIGHASH_DEFAULT, input_index=0)
        nonce_commitment, s = self.frost_signature(sighash, participant_indexes)
        # As per BIP-340
        # sig = bytes(R) || bytes((k + ed) mod n).
        # with a total size of 64-bytes (32B Point x-coord only encoding + 32B int)
        signature = nonce_commitment.x.to_bytes(32, byteorder='big') + (s % SECP256K1_ORDER).to_bytes(32, byteorder='big')
        sign_tx.vin[0].scriptSig = signature
        return sign_tx

    def frost_signature(self, msg, participant_indexes):
        pk = self.participants[0].public_key  # it's the same for all the participants
        # NonceGen
        for participant in self.participants:
            participant.generate_nonces(1)

        # Build nonce commitment pairs list
        nonce_commitment_pairs = [self.participants[i-1].nonce_commitment_pairs for i in participant_indexes]
        agg = FROST.Aggregator(pk, msg, nonce_commitment_pairs, participant_indexes)
        message, nonce_commitment_pairs_list = agg.signing_inputs()

        # Compute shares z_i for each signer
        shares = [self.participants[i-1].sign(message, nonce_commitment_pairs_list, participant_indexes) for i in participant_indexes]

        # σ = (R, z)
        nonce_commitment, s = agg.signature(shares)
        nc_negated = False

        # Verify signature
        G = FROST.secp256k1.G()
        # c = H_2(R, Y, m)
        challenge_hash = FROST.Aggregator.challenge_hash(nonce_commitment, pk, msg)
        tweak_secret = 0
        participants_secret = 0
        for i in participant_indexes:
            participants_secret += self.participants[i-1].lagrange_coefficient(participant_indexes) * self.participants[i-1].aggregate_share

        if (self.participants[0].tweak_bytes is not None):
            """
            The equation R ≟ g^z * Y^-c must still be valid, however since we
            tweaked the public key multiplying it by tweak_point, we now have
            Y = public_key * tweak_point
            We multiplied the right side of the equation by tweak^(-c) so we
            need to do the same on the other side
            R*(tweak_point)^(-c) = g^z * (public_key * tweak_point)^(-c)
            R' = g^z * Y^(-c)
            where R' = R*(tweak_point)^(-c)
            """
            tweak_secret = int.from_bytes(self.participants[0].tweak_bytes,"big")
            tweak_point = tweak_secret * G
            nonce_commitment = nonce_commitment + (SECP256K1_ORDER - challenge_hash) * tweak_point
            """
            A validator receiving σ = (R', z) would compute c' = H_2(R',Y,m)
            and would verify R' =? g^z * Y^(-c') which would be false as c != c'
            So now we wondering
            ∃ z' : R' = g^(z') * Y^(-c') ?

            Let's define:
            - ps = participants_secret = ∑(λ_i * s_i)
            - ts = tweak_secret

            R' = g^z * (public_key * tweak_point)^(-c)
               = g^z * g^( (ps + ts)*(-c) )
               = g^z * g^( (ps + ts)*(-c + c' - c') )
               = g^( z + (ps + ts)*(c' - c) ) * g^( (ps+ts) * (-c') )
               = g^(z') * Y^(-c')

            where z' = z + (ps+ts)*(c'-c)
            and R' = R*(tweak_point)^(-c)
            """
            challenge_hash_new = FROST.Aggregator.challenge_hash(nonce_commitment, pk, msg)
            s = (s + (participants_secret + tweak_secret) * (challenge_hash_new - challenge_hash)) % SECP256K1_ORDER

            challenge_hash = challenge_hash_new

        assert_equal((participants_secret + tweak_secret) * G, pk)
        """
        No matter if we tweaked the public key or not, the nonce commitment
        could potentially have an odd y-coordinate which is not acceptable,
        since as per BIP-340 the Y coordinate of P (public key) and R (nonce
        commitment) are implicitly chosen to be even.
        Hence if nonce_commitment y-coordinate is odd we need to negate it
        """
        if (nonce_commitment.y % 2 != 0):
            nonce_commitment = nonce_commitment.__neg__()
            nc_negated = True
        """
        The initial equation R = g^(z) * Y^(-c) or the tweaked version
        R' = g^(z') * Y^(-c') must still be respected.

        Now we have 4 cases:
        1. R (or R') has been negated while pk not
        2. R (or R') hasn't been negated while pk yes
        3. Both R(or R') and pk have been negated
        4. Neither R(or R') and pk have been negated

        The fact that pk has been negated has no influence, since when we
        negated pk we also negated the aggregate share (and the tweak), i.e.
        the secret, so the sign share z_i have been already computed
        accordingly.

        Hence we can reduce the number of cases to 2:
        1. R (or R') has been negated
        2. R (or R') hasn't been negated

        Case 1
        R' = g^(z') * g^((ps + ts)*(-c'))
        what happens if I need (R')^(-1) ? c' won't change because it depends
        only on the x-coord.

        (R')^(-1) = (g^(z'))-1 * g^((ps+ts)*c')
                  = g^((-z') + 2*(ps+ts)*c') * g^(-(ps+ts)*c')
                  = g^(z'') * (Y*tweak)^(-c')
        with z'' = -z' + 2*(ps+ts)*c'
        Similarly it can be shown that if Y hasn't been tweaked
        z'' = -z + 2*ps*c

        Case 2
        Nothing to do
        """
        if nc_negated:
            s = (-s + 2*(participants_secret + tweak_secret)*challenge_hash) % SECP256K1_ORDER

        # R ≟ g^z * Y^-c
        assert_equal(nonce_commitment, (s * G) + (SECP256K1_ORDER - challenge_hash) * pk)

        return nonce_commitment, s
