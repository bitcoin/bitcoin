from util import hash256, hash_pks
from copy import deepcopy


class AggregationInfo:
    """
    AggregationInfo represents information of how a tree of aggregate
    signatures was created. Different tress will result in different
    signatures, due to exponentiations required for security.

    An AggregationInfo is represented as a map from (message_hash, pk)
    to exponents. When verifying, a verifier will take the signature,
    along with this map, and raise each public key to the correct
    exponent, and multiply the pks together, for identical messages.
    """
    def __init__(self, tree, message_hashes, public_keys):
        self.tree = tree
        self.message_hashes = message_hashes
        self.public_keys = public_keys

    def empty(self):
        return not self.tree

    def __eq__(self, other):
        return not self.__lt__(other) and not other.__lt__(self)

    def __lt__(self, other):
        """
        Compares two AggregationInfo objects, this is necessary for sorting
        them. Comparison is done by comparing (message hash, pk, exponent)
        """
        combined = [(self.message_hashes[i], self.public_keys[i],
                     self.tree[(self.message_hashes[i], self.public_keys[i])])
                    for i in range(len(self.public_keys))]
        combined_other = [(other.message_hashes[i], other.public_keys[i],
                           other.tree[(other.message_hashes[i],
                                       other.public_keys[i])])
                          for i in range(len(other.public_keys))]

        for i in range(max(len(combined), len(combined_other))):
            if i == len(combined):
                return True
            if i == len(combined_other):
                return False
            if combined[i] < combined_other[i]:
                return True
            if combined_other[i] < combined[i]:
                return False
        return False

    def __str__(self):
        ret = ""
        for key, value in self.tree.items():
            ret += ("(" + key[0].hex() + "," + key[1].serialize().hex()
                    + "):\n" + hex(value) + "\n")
        return ret

    def __deepcopy__(self, memo):
        new_tree = deepcopy(self.tree, memo)
        new_mh = deepcopy(self.message_hashes, memo)
        new_pubkeys = deepcopy(self.public_keys, memo)
        return AggregationInfo(new_tree, new_mh, new_pubkeys)

    @staticmethod
    def from_msg_hash(public_key, message_hash):
        tree = {}
        tree[(message_hash, public_key)] = 1
        return AggregationInfo(tree, [message_hash], [public_key])

    @staticmethod
    def from_msg(pk, message):
        return AggregationInfo.from_msg_hash(pk, hash256(message))

    @staticmethod
    def simple_merge_infos(aggregation_infos):
        """
        Infos are just merged together with no addition of exponents,
        since they are disjoint
        """
        new_tree = {}
        for info in aggregation_infos:
            new_tree.update(info.tree)
        mh_pubkeys = [k for k, v in new_tree.items()]

        mh_pubkeys.sort()

        message_hashes = [message_hash for (message_hash, public_key)
                          in mh_pubkeys]
        public_keys = [public_key for (message_hash, public_key)
                       in mh_pubkeys]

        return AggregationInfo(new_tree, message_hashes, public_keys)

    @staticmethod
    def secure_merge_infos(colliding_infos):
        """
        Infos are merged together with combination of exponents
        """

        # Groups are sorted by message then pk then exponent
        # Each info object (and all of it's exponents) will be
        # exponentiated by one of the Ts
        colliding_infos.sort()

        sorted_keys = []
        for info in colliding_infos:
            for key, value in info.tree.items():
                sorted_keys.append(key)
        sorted_keys.sort()
        sorted_pks = [public_key for (message_hash, public_key)
                      in sorted_keys]
        computed_Ts = hash_pks(len(colliding_infos), sorted_pks)

        # Group order, exponents can be reduced mod the order
        order = sorted_pks[0].value.ec.n

        new_tree = {}
        for i in range(len(colliding_infos)):
            for key, value in colliding_infos[i].tree.items():
                if key not in new_tree:
                    # This message & pk have not been included yet
                    new_tree[key] = (value * computed_Ts[i]) % order
                else:
                    # This message and pk are already included, so multiply
                    addend = value * computed_Ts[i]
                    new_tree[key] = (new_tree[key] + addend) % order
        mh_pubkeys = [k for k, v in new_tree.items()]
        mh_pubkeys.sort()
        message_hashes = [message_hash for (message_hash, public_key)
                          in mh_pubkeys]
        public_keys = [public_key for (message_hash, public_key)
                       in mh_pubkeys]
        return AggregationInfo(new_tree, message_hashes, public_keys)

    @staticmethod
    def merge_infos(aggregation_infos):
        messages = set()
        colliding_messages = set()
        for info in aggregation_infos:
            messages_local = set()
            for key, value in info.tree.items():
                if key[0] in messages and key[0] not in messages_local:
                    colliding_messages.add(key[0])
                messages.add(key[0])
                messages_local.add(key[0])

        if len(colliding_messages) == 0:
            return AggregationInfo.simple_merge_infos(aggregation_infos)

        colliding_infos = []
        non_colliding_infos = []
        for info in aggregation_infos:
            info_collides = False
            for key, value in info.tree.items():
                if key[0] in colliding_messages:
                    info_collides = True
                    colliding_infos.append(info)
                    break
            if not info_collides:
                non_colliding_infos.append(info)

        combined = AggregationInfo.secure_merge_infos(colliding_infos)
        non_colliding_infos.append(combined)
        return AggregationInfo.simple_merge_infos(non_colliding_infos)


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
