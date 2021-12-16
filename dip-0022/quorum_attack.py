#!/usr/bin/env python3
# This script was written by UdjinM6 based on dip-0008/quorum_attack.py

from math import comb, floor, log


###This function takes inputs and outputs the probability
#of success in one trial
#pcalc is short for probability calculation
def pcalc(attack_threshold, byz_nodes):
    pctemp = 0
    for x in range(attack_threshold, quorum_size + 1):
        pctemp = pctemp + comb(byz_nodes, x) * comb(masternodes - byz_nodes, quorum_size - x)
    #at this juncture the answer is pctemp/sample_space
    #but that will produce an overflow error.  We use logarithms to
    #calculate this value
    return 10 ** (log(pctemp, 10) - log(sample_space, 10))


##We evaluate the function pcalc(10,5,3,4)
##print(pcalc(10,5,3,4))
##as a test vector
##The answer would be [comb(3,4)*comb(2,6)+(comb(4,4)*comb(1,6)]/comb(10,5)
##[4*15+1*6]/252 = 66/252
##print(float(66)/252)

##Number of masternodes
masternodes = 5000
##Quorum size for ChainLocks
quorum_size = 400
##Sample space
sample_space = comb(masternodes,quorum_size)

##Number of Byzantine masternodes
bfts = [500, 1000, 1500]

##Thresholds
thresholds = [240]
##Number of compromised/total quorums
quorums = [[1,4], [2,4], [3,4]]

##Quorum overlap ratio
overlap = 0.1

for i in range(len(quorums)):
    q_c = quorums[i][0]  # compromised
    q_t = quorums[i][1]  # total
    assert q_c < q_t
    print("%4d masternodes, at least %d of %d %d node quorums are compromised, %.0f%% quorum overlap:" % (masternodes, q_c, q_t, quorum_size, overlap * 100))
    for threshold in thresholds:
        for bft in bfts:
            bft_left = bft
            chance = 1
            for k in range(0, q_c):
                try:
                    chance *= pcalc(threshold, bft_left)
                except Exception:
                    chance = 0.0
                    break
                bft_left -= floor(threshold * (1 - overlap))
            print("%8s Byzantine total, the chance of having at least %3d Byzantine nodes (%4d nodes left) in each compromised quorum is %s" % (bft, threshold, bft_left, chance))
