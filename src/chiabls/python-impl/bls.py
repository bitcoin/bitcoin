from ec import (generator_Fq, default_ec,
                AffinePoint, JacobianPoint,
                hash_to_point_prehashed_Fq2)
from util import hash_pks
from fields import Fq, Fq2, Fq12
from pairing import ate_pairing_multi
from keys import PrivateKey, PublicKey
from signature import Signature
from aggregation_info import AggregationInfo


class BLS:
    @staticmethod
    def aggregate_sigs_simple(signatures):
        """
        Aggregate signatures by multiplying them together. This is NOT secure
        against rogue public key attacks, so do not use this for signatures
        on the same message.
        """
        q = default_ec.q
        agg_sig = (AffinePoint(Fq2.zero(q), Fq2.zero(q), True, default_ec)
                   .to_jacobian())

        for sig in signatures:
            agg_sig += sig.value

        return Signature.from_g2(agg_sig)

    @staticmethod
    def aggregate_sigs_secure(signatures, public_keys, message_hashes):
        """
        Aggregate signatures using the secure method, which calculates
        exponents based on public keys, and raises each signature to an
        exponent before multiplying them together. This is secure against
        rogue public key attack, but is slower than simple aggregation.
        """
        if (len(signatures) != len(public_keys) or
                len(public_keys) != len(message_hashes)):
            raise Exception("Invalid number of keys")
        mh_pub_sigs = [(message_hashes[i], public_keys[i], signatures[i])
                       for i in range(len(signatures))]

        # Sort by message hash + pk
        mh_pub_sigs.sort()

        computed_Ts = hash_pks(len(public_keys), public_keys)

        # Raise each sig to a power of each t,
        # and multiply all together into agg_sig
        ec = public_keys[0].ec
        agg_sig = JacobianPoint(Fq2.one(ec.q), Fq2.one(ec.q),
                                Fq2.zero(ec.q), True, ec)

        for i, (_, _, signature) in enumerate(mh_pub_sigs):
            agg_sig += signature * computed_Ts[i]

        return Signature.from_g2(agg_sig)

    @staticmethod
    def aggregate_sigs(signatures):
        """
        Aggregates many (aggregate) signatures, using a combination of simple
        and secure aggregation. Signatures are grouped based on which ones
        share common messages, and these are all merged securely.
        """
        public_keys = []  # List of lists
        message_hashes = []  # List of lists

        for signature in signatures:
            if signature.aggregation_info.empty():
                raise Exception("Each signature must have a valid aggregation "
                                + "info")
            public_keys.append(signature.aggregation_info.public_keys)
            message_hashes.append(signature.aggregation_info.message_hashes)

        # Find colliding vectors, save colliding messages
        messages_set = set()
        colliding_messages_set = set()

        for msg_vector in message_hashes:
            messages_set_local = set()
            for msg in msg_vector:
                if msg in messages_set and msg not in messages_set_local:
                    colliding_messages_set.add(msg)
                messages_set.add(msg)
                messages_set_local.add(msg)

        if len(colliding_messages_set) == 0:
            # There are no colliding messages between the groups, so we
            # will just aggregate them all simply. Note that we assume
            # that every group is a valid aggregate signature. If an invalid
            # or insecure signature is given, and invalid signature will
            # be created. We don't verify for performance reasons.
            final_sig = BLS.aggregate_sigs_simple(signatures)
            aggregation_infos = [sig.aggregation_info for sig in signatures]
            final_agg_info = AggregationInfo.merge_infos(aggregation_infos)
            final_sig.set_aggregation_info(final_agg_info)
            return final_sig

        # There are groups that share messages, therefore we need
        # to use a secure form of aggregation. First we find which
        # groups collide, and securely aggregate these. Then, we
        # use simple aggregation at the end.
        colliding_sigs = []
        non_colliding_sigs = []
        colliding_message_hashes = []  # List of lists
        colliding_public_keys = []  # List of lists

        for i in range(len(signatures)):
            group_collides = False
            for msg in message_hashes[i]:
                if msg in colliding_messages_set:
                    group_collides = True
                    colliding_sigs.append(signatures[i])
                    colliding_message_hashes.append(message_hashes[i])
                    colliding_public_keys.append(public_keys[i])
                    break
            if not group_collides:
                non_colliding_sigs.append(signatures[i])

        # Arrange all signatures, sorted by their aggregation info
        colliding_sigs.sort(key=lambda s: s.aggregation_info)

        # Arrange all public keys in sorted order, by (m, pk)
        sort_keys_sorted = []
        for i in range(len(colliding_public_keys)):
            for j in range(len(colliding_public_keys[i])):
                sort_keys_sorted.append((colliding_message_hashes[i][j],
                                         colliding_public_keys[i][j]))
        sort_keys_sorted.sort()
        sorted_public_keys = [pk for (mh, pk) in sort_keys_sorted]

        computed_Ts = hash_pks(len(colliding_sigs), sorted_public_keys)

        # Raise each sig to a power of each t,
        # and multiply all together into agg_sig
        ec = sorted_public_keys[0].value.ec
        agg_sig = JacobianPoint(Fq2.one(ec.q), Fq2.one(ec.q),
                                Fq2.zero(ec.q), True, ec)

        for i, signature in enumerate(colliding_sigs):
            agg_sig += signature.value * computed_Ts[i]

        for signature in non_colliding_sigs:
            agg_sig += signature.value

        final_sig = Signature.from_g2(agg_sig)
        aggregation_infos = [sig.aggregation_info for sig in signatures]
        final_agg_info = AggregationInfo.merge_infos(aggregation_infos)
        final_sig.set_aggregation_info(final_agg_info)

        return final_sig

    @staticmethod
    def verify(signature):
        """
        This implementation of verify has several steps. First, it
        reorganizes the pubkeys and messages into groups, where
        each group corresponds to a message. Then, it checks if the
        siganture has info on how it was aggregated. If so, we
        exponentiate each pk based on the exponent in the AggregationInfo.
        If not, we find public keys that share messages with others,
        and aggregate all of these securely (with exponents.).
        Finally, since each public key now corresponds to a unique
        message (since we grouped them), we can verify using the
        distinct verification procedure.
        """
        message_hashes = signature.aggregation_info.message_hashes
        public_keys = signature.aggregation_info.public_keys

        hash_to_public_keys = {}
        for i in range(len(message_hashes)):
            if message_hashes[i] in hash_to_public_keys:
                hash_to_public_keys[message_hashes[i]].append(public_keys[i])
            else:
                hash_to_public_keys[message_hashes[i]] = [public_keys[i]]

        final_message_hashes = []
        final_public_keys = []
        ec = public_keys[0].value.ec
        for message_hash, mapped_keys in hash_to_public_keys.items():
            dedup = list(set(mapped_keys))
            public_key_sum = JacobianPoint(Fq.one(ec.q), Fq.one(ec.q),
                                           Fq.zero(ec.q), True, ec)
            for public_key in dedup:
                try:
                    exponent = signature.aggregation_info.tree[(message_hash,
                                                                public_key)]
                    public_key_sum += (public_key.value * exponent)
                except KeyError:
                    return False
            final_message_hashes.append(message_hash)
            final_public_keys.append(public_key_sum.to_affine())

        mapped_hashes = [hash_to_point_prehashed_Fq2(mh)
                         for mh in final_message_hashes]

        g1 = Fq(default_ec.n, -1) * generator_Fq()
        Ps = [g1] + final_public_keys
        Qs = [signature.value.to_affine()] + mapped_hashes
        res = ate_pairing_multi(Ps, Qs, default_ec)
        return res == Fq12.one(default_ec.q)

    @staticmethod
    def aggregate_pub_keys(public_keys, secure):
        """
        Aggregates public keys together
        """
        if len(public_keys) < 1:
            raise Exception("Invalid number of keys")
        public_keys.sort()

        computed_Ts = hash_pks(len(public_keys), public_keys)

        ec = public_keys[0].value.ec
        sum_keys = JacobianPoint(Fq.one(ec.q), Fq.one(ec.q),
                                 Fq.zero(ec.q), True, ec)
        for i in range(len(public_keys)):
            addend = public_keys[i].value
            if secure:
                addend *= computed_Ts[i]
            sum_keys += addend

        return PublicKey.from_g1(sum_keys)

    @staticmethod
    def aggregate_priv_keys(private_keys, public_keys, secure):
        """
        Aggregates private keys together
        """
        if secure and len(private_keys) != len(public_keys):
            raise Exception("Invalid number of keys")

        priv_pub_keys = [(public_keys[i], private_keys[i])
                         for i in range(len(private_keys))]
        # Sort by public keys
        priv_pub_keys.sort()

        computed_Ts = hash_pks(len(private_keys), public_keys)

        n = public_keys[0].value.ec.n
        sum_keys = 0
        for i in range(len(priv_pub_keys)):
            addend = priv_pub_keys[i][1].value
            if (secure):
                addend *= computed_Ts[i]
            sum_keys = (sum_keys + addend) % n

        return PrivateKey.from_bytes(sum_keys.to_bytes(32, "big"))


"""
Copyright 2018 Chia Network Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
