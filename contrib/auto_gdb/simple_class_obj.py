#!/usr/bin/env python3
#

import sys
import os
import common_helpers
sys.path.append(os.getcwd())


simple_types = ["CMasternode", "CMasternodeVerification",
                "CMasternodeBroadcast", "CMasternodePing",
                "CMasternodeMan", "CDarksendQueue", "CDarkSendEntry",
                "CTransaction", "CMutableTransaction", "CCoinJoinBaseSession",
                "CCoinJoinBaseManager", "CCoinJoinClientSession",
                "CCoinJoinClientManager", "CCoinJoinServer", "CMasternodePayments",
                "CMasternodePaymentVote", "CMasternodeBlockPayees",
                "CMasternodePayee", "CInstantSend", "CTxLockRequest",
                "CTxLockVote", "CTxLockCandidate", "COutPoint",
                "COutPointLock", "CSporkManager", "CMasternodeSync",
                "CGovernanceManager", "CRateCheckBuffer", "CGovernanceObject",
                "CGovernanceVote", "CGovernanceObjectVoteFile"]

simple_templates = ["CacheMultiMap", "CacheMap"]


class SimpleClassObj:

    def __init__ (self, gobj):
        self.obj = gobj

    @classmethod
    def is_this_type(cls, obj_type):
        str_type = str(obj_type)
        if str_type in simple_types:
            return True
        for templ in simple_templates:
            if str_type.find(templ + "<") == 0:
                return True
        return False

    def get_used_size(self):
        size = 0
        fields = self.obj.type.fields()
        for f in fields:
            # check if it is static field
            if not hasattr(f, "bitpos"):
                continue
            # process base class size
            if f.is_base_class:
                size += common_helpers.get_instance_size(self.obj.cast(f.type.strip_typedefs()))
                continue
            # process simple field
            size += common_helpers.get_instance_size(self.obj[f.name])
        return size
