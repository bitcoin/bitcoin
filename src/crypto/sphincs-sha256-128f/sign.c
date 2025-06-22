#include <string.h>
#include <stdint.h>

#include "api.h"
#include "params.h"
#include "wots.h"
#include "fors.h"
#include "hash.h"
#include "hash_address.h"
#include "rng.h"
#include "utils.h"

/**
 * Computes the leaf at a given address. First generates the WOTS key pair,
 * then computes leaf by hashing horizontally.
 */
static void wots_gen_leaf(unsigned char *leaf, const unsigned char *sk_seed,
                          const unsigned char *pub_seed,
                          uint32_t addr_idx, const uint32_t tree_addr[8])
{
    unsigned char pk[SPX_WOTS_BYTES];
    uint32_t wots_addr[8] = {0};
    uint32_t wots_pk_addr[8] = {0};

    set_type(wots_addr, SPX_ADDR_TYPE_WOTS);
    set_type(wots_pk_addr, SPX_ADDR_TYPE_WOTSPK);

    copy_subtree_addr(wots_addr, tree_addr);
    set_keypair_addr(wots_addr, addr_idx);
    wots_gen_pk(pk, sk_seed, pub_seed, wots_addr);

    copy_keypair_addr(wots_pk_addr, wots_addr);
    thash(leaf, pk, SPX_WOTS_LEN, pub_seed, wots_pk_addr);
}

/*
 * Generates an SPX key pair.
 * Format sk: [SK_SEED || SK_PRF || PUB_SEED || root]
 * Format pk: [PUB_SEED || root]
 */
int crypto_sign_keypair(unsigned char *pk, unsigned char *sk)
{
    /* We do not need the auth path in key generation, but it simplifies the
       code to have just one treehash routine that computes both root and path
       in one function. */
    unsigned char auth_path[SPX_TREE_HEIGHT * SPX_N];
    uint32_t top_tree_addr[8] = {0};

    set_layer_addr(top_tree_addr, SPX_D - 1);
    set_type(top_tree_addr, SPX_ADDR_TYPE_HASHTREE);

    /* Initialize SK_SEED, SK_PRF and PUB_SEED. */
    randombytes(sk, 3 * SPX_N);

    memcpy(pk, sk + 2*SPX_N, SPX_N);

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    initialize_hash_function(pk, sk);

    /* Compute root node of the top-most subtree. */
    treehash(sk + 3*SPX_N, auth_path, sk, sk + 2*SPX_N, 0, 0, SPX_TREE_HEIGHT,
             wots_gen_leaf, top_tree_addr);

    memcpy(pk + SPX_N, sk + 3*SPX_N, SPX_N);

    return 0;
}

/**
 * Returns an array containing the signature followed by the message.
 */
int crypto_sign(unsigned char *sm, unsigned long long *smlen,
                const unsigned char *m, unsigned long long mlen,
                const unsigned char *sk)
{
    const unsigned char *sk_seed = sk;
    const unsigned char *sk_prf = sk + SPX_N;
    const unsigned char *pk = sk + 2*SPX_N;
    const unsigned char *pub_seed = pk;

    unsigned char optrand[SPX_N];
    unsigned char mhash[SPX_FORS_MSG_BYTES];
    unsigned char root[SPX_N];
    unsigned long long i;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t wots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    initialize_hash_function(pub_seed, sk_seed);

    set_type(wots_addr, SPX_ADDR_TYPE_WOTS);
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);

    /* Already put the message in the right place, to make it easier to prepend
     * things when computing the hash over the message. */
    /* We need to do this from back to front, so that it works when sm = m */
    for (i = mlen; i > 0; i--) {
        sm[SPX_BYTES + i - 1] = m[i - 1];
    }
    *smlen = SPX_BYTES + mlen;

    /* Optionally, signing can be made non-deterministic using optrand.
       This can help counter side-channel attacks that would benefit from
       getting a large number of traces when the signer uses the same nodes. */
    randombytes(optrand, SPX_N);
    /* Compute the digest randomization value. */
    gen_message_random(sm, sk_prf, optrand, sm + SPX_BYTES, mlen);

    /* Derive the message digest and leaf index from R, PK and M. */
    hash_message(mhash, &tree, &idx_leaf, sm, pk, sm + SPX_BYTES, mlen);
    sm += SPX_N;

    set_tree_addr(wots_addr, tree);
    set_keypair_addr(wots_addr, idx_leaf);

    /* Sign the message hash using FORS. */
    fors_sign(sm, root, mhash, sk_seed, pub_seed, wots_addr);
    sm += SPX_FORS_BYTES;

    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_subtree_addr(wots_addr, tree_addr);
        set_keypair_addr(wots_addr, idx_leaf);

        /* Compute a WOTS signature. */
        wots_sign(sm, root, sk_seed, pub_seed, wots_addr);
        sm += SPX_WOTS_BYTES;

        /* Compute the authentication path for the used WOTS leaf. */
        treehash(root, sm, sk_seed, pub_seed, idx_leaf, 0,
                 SPX_TREE_HEIGHT, wots_gen_leaf, tree_addr);
        sm += SPX_TREE_HEIGHT * SPX_N;

        /* Update the indices for the next layer. */
        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    return 0;
}

/**
 * Verifies a given signature-message pair under a given public key.
 */
int crypto_sign_open(unsigned char *m, unsigned long long *mlen,
                     const unsigned char *sm, unsigned long long smlen,
                     const unsigned char *pk)
{
    const unsigned char *pub_seed = pk;
    const unsigned char *pub_root = pk + SPX_N;
    unsigned char mhash[SPX_FORS_MSG_BYTES];
    unsigned char wots_pk[SPX_WOTS_BYTES];
    unsigned char root[SPX_N];
    unsigned char leaf[SPX_N];
    unsigned int i;
    uint64_t tree;
    uint32_t idx_leaf;
    uint32_t wots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};
    uint32_t wots_pk_addr[8] = {0};

    /* This hook allows the hash function instantiation to do whatever
       preparation or computation it needs, based on the public seed. */
    initialize_hash_function(pub_seed, NULL);

    set_type(wots_addr, SPX_ADDR_TYPE_WOTS);
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
    set_type(wots_pk_addr, SPX_ADDR_TYPE_WOTSPK);

    *mlen = smlen - SPX_BYTES;

    /* Put the message all the way at the end of the m buffer, so that we can
     * prepend the required other inputs for the hash function. */
    memcpy(m + SPX_BYTES, sm + SPX_BYTES, *mlen);

    /* Derive the message digest and leaf index from R || PK || M. */
    /* The additional SPX_N is a result of the hash domain separator. */
    hash_message(mhash, &tree, &idx_leaf, sm, pk, m + SPX_BYTES, *mlen);
    sm += SPX_N;

    /* Layer correctly defaults to 0, so no need to set_layer_addr */
    set_tree_addr(wots_addr, tree);
    set_keypair_addr(wots_addr, idx_leaf);

    fors_pk_from_sig(root, sm, mhash, pub_seed, wots_addr);
    sm += SPX_FORS_BYTES;

    /* For each subtree.. */
    for (i = 0; i < SPX_D; i++) {
        set_layer_addr(tree_addr, i);
        set_tree_addr(tree_addr, tree);

        copy_subtree_addr(wots_addr, tree_addr);
        set_keypair_addr(wots_addr, idx_leaf);

        copy_keypair_addr(wots_pk_addr, wots_addr);

        /* The WOTS public key is only correct if the signature was correct. */
        /* Initially, root is the FORS pk, but on subsequent iterations it is
           the root of the subtree below the currently processed subtree. */
        wots_pk_from_sig(wots_pk, sm, root, pub_seed, wots_addr);
        sm += SPX_WOTS_BYTES;

        /* Compute the leaf node using the WOTS public key. */
        thash(leaf, wots_pk, SPX_WOTS_LEN, pub_seed, wots_pk_addr);

        /* Compute the root node of this subtree. */
        compute_root(root, leaf, idx_leaf, 0, sm, SPX_TREE_HEIGHT,
                     pub_seed, tree_addr);
        sm += SPX_TREE_HEIGHT * SPX_N;

        /* Update the indices for the next layer. */
        idx_leaf = (tree & ((1 << SPX_TREE_HEIGHT)-1));
        tree = tree >> SPX_TREE_HEIGHT;
    }

    /* Check if the root node equals the root node in the public key. */
    if (memcmp(root, pub_root, SPX_N)) {
        /* If not, zero the message */
        memset(m, 0, *mlen);
        *mlen = 0;
        return -1;
    }

    /* If verification was successful, copy the message from the signature. */
    memcpy(m, sm, *mlen);

    return 0;
}
