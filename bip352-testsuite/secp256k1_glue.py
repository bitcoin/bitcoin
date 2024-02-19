from ctypes import *
import os

secplib = CDLL('../.libs/libsecp256k1.so')
secplib.secp256k1_context_create.restype = c_voidp
secp_context = secplib.secp256k1_context_create(1)

##################################
### API calls from secp256k1.h ###
##################################
SECP256K1_PUBKEY_STRUCT_SIZE = 64
"""
int secp256k1_ec_pubkey_create(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *seckey
);
"""
secplib.secp256k1_ec_pubkey_create.argtypes = [c_voidp, c_voidp, c_voidp]
secplib.secp256k1_ec_pubkey_create.restype = c_int
def pubkey_create(privkey_bytes):
    pk = create_string_buffer(SECP256K1_PUBKEY_STRUCT_SIZE)
    retval = secplib.secp256k1_ec_pubkey_create(secp_context, pk, privkey_bytes)
    assert retval == 1
    return pk

"""
int secp256k1_ec_pubkey_parse(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *input,
    size_t inputlen
);
"""
secplib.secp256k1_ec_pubkey_parse.argtypes = [c_voidp, c_voidp, c_voidp, c_size_t]
secplib.secp256k1_ec_pubkey_parse.restype = c_int
def pubkey_parse(pubkey_bytes):
    pk = create_string_buffer(SECP256K1_PUBKEY_STRUCT_SIZE)
    retval = secplib.secp256k1_ec_pubkey_parse(secp_context, pk, pubkey_bytes, 33)
    assert retval == 1
    return pk

"""
int secp256k1_ec_pubkey_serialize(
    const secp256k1_context *ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_pubkey *pubkey,
    unsigned int flags
);
"""
secplib.secp256k1_ec_pubkey_serialize.argtypes = [c_voidp, c_voidp, c_voidp, c_voidp, c_uint]
secplib.secp256k1_ec_pubkey_serialize.restype = c_int
def pubkey_serialize(pubkey_obj):
    ser_pubkey = create_string_buffer(33)
    ser_pubkey_size = c_size_t(33)
    retval = secplib.secp256k1_ec_pubkey_serialize(secp_context,
        ser_pubkey, pointer(ser_pubkey_size), pubkey_obj, (1 << 1) | (1 << 8))
    assert retval == 1
    return bytes(ser_pubkey)

"""
int secp256k1_ec_seckey_tweak_add(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
);
"""
secplib.secp256k1_ec_seckey_tweak_add.argtypes = [c_voidp, c_voidp]
secplib.secp256k1_ec_seckey_tweak_add.restype = c_int
def seckey_add(seckey_lhs, seckey_rhs):
    seckey_result = create_string_buffer(seckey_lhs, 32)
    retval = secplib.secp256k1_ec_seckey_tweak_add(secp_context, seckey_result, seckey_rhs)
    assert retval == 1
    return bytes(seckey_result)

############################################
### API calls from secp256k1_extrakeys.h ###
############################################
SECP256K1_XONLY_PUBKEY_STRUCT_SIZE = 64
SECP256K1_KEYPAIR_STRUCT_SIZE = 96
"""
int secp256k1_xonly_pubkey_parse(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *pubkey,
    const unsigned char *input32
);
"""
secplib.secp256k1_xonly_pubkey_parse.argtypes = [c_voidp, c_voidp, c_voidp]
secplib.secp256k1_xonly_pubkey_parse.restype = c_int
def xonly_pubkey_parse(xonly_pubkey_bytes):
    xpk = create_string_buffer(SECP256K1_XONLY_PUBKEY_STRUCT_SIZE)
    retval = secplib.secp256k1_xonly_pubkey_parse(secp_context, xpk, xonly_pubkey_bytes)
    assert retval == 1
    return xpk

"""
int secp256k1_xonly_pubkey_serialize(
    const secp256k1_context *ctx,
    unsigned char *output32,
    const secp256k1_xonly_pubkey *pubkey
);
"""
secplib.secp256k1_xonly_pubkey_serialize.argtypes = [c_voidp, c_voidp, c_voidp]
secplib.secp256k1_xonly_pubkey_serialize.restype = c_int
def xonly_pubkey_serialize(xpubkey_obj):
    ser_xpubkey = create_string_buffer(32)
    retval = secplib.secp256k1_xonly_pubkey_serialize(secp_context, ser_xpubkey, xpubkey_obj)
    assert retval == 1
    return bytes(ser_xpubkey)

"""
int secp256k1_keypair_create(
    const secp256k1_context *ctx,
    secp256k1_keypair *keypair,
    const unsigned char *seckey
);
"""
secplib.secp256k1_keypair_create.argtypes = [c_voidp, c_voidp, c_voidp]
secplib.secp256k1_keypair_create.restype = c_int
def keypair_create(seckey):
    result_keypair = create_string_buffer(SECP256K1_KEYPAIR_STRUCT_SIZE)
    retval = secplib.secp256k1_keypair_create(secp_context, result_keypair, seckey)
    assert retval == 1
    return result_keypair

##########################################
### API calls from secp256k1_schnorr.h ###
##########################################
"""
int secp256k1_schnorrsig_sign32(
    const secp256k1_context *ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_keypair *keypair,
    const unsigned char *aux_rand32
);
"""
secplib.secp256k1_schnorrsig_sign32.argtypes = [c_voidp, c_voidp, c_voidp, c_voidp, c_voidp]
secplib.secp256k1_schnorrsig_sign32.restype = c_int
def schnorrsig_sign32(msg32, privkey, aux32):
    keypair_obj = keypair_create(privkey)
    result_sig = create_string_buffer(64)
    retval = secplib.secp256k1_schnorrsig_sign32(
        secp_context, result_sig, msg32, keypair_obj, aux32)
    assert retval == 1
    return bytes(result_sig)

#################################################
### API calls from secp256k1_silentpayments.h ###
#################################################
"""
int secp256k1_silentpayments_create_private_tweak_data(
    const secp256k1_context *ctx,
    unsigned char *a_sum,
    unsigned char *input_hash,
    const unsigned char * const *plain_seckeys,
    size_t n_plain_seckeys,
    const unsigned char * const *taproot_seckeys,
    size_t n_taproot_seckeys,
    const unsigned char *outpoint_smallest36
);
"""
secplib.secp256k1_silentpayments_create_private_tweak_data.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_size_t, c_voidp, c_size_t, c_voidp
]
secplib.secp256k1_silentpayments_create_private_tweak_data.restype = c_int
def silentpayments_create_private_tweak_data(plain_seckeys, xonly_seckeys, outpoint_lowest):
    array_plain_seckeys = (c_char_p * len(plain_seckeys))()
    array_xonly_seckeys = (c_char_p * len(xonly_seckeys))()
    for i in range(len(plain_seckeys)): array_plain_seckeys[i] = plain_seckeys[i]
    if len(array_plain_seckeys) == 0: array_plain_seckeys = None
    for i in range(len(xonly_seckeys)): array_xonly_seckeys[i] = xonly_seckeys[i]
    if len(array_xonly_seckeys) == 0: array_xonly_seckeys = None

    result_a_sum = create_string_buffer(32)
    result_input_hash = create_string_buffer(32)
    retval = secplib.secp256k1_silentpayments_create_private_tweak_data(
        secp_context, result_a_sum, result_input_hash, array_plain_seckeys, len(plain_seckeys),
        array_xonly_seckeys, len(xonly_seckeys), outpoint_lowest)
    assert retval == 1

    return bytes(result_a_sum), bytes(result_input_hash)

"""
int secp256k1_silentpayments_create_public_tweak_data(
    const secp256k1_context *ctx,
    secp256k1_pubkey *A_sum,
    unsigned char *input_hash,
    const secp256k1_pubkey * const *plain_pubkeys,
    size_t n_plain_pubkeys,
    const secp256k1_xonly_pubkey * const *xonly_pubkeys,
    size_t n_xonly_pubkeys,
    const unsigned char *outpoint_smallest36
);
"""
secplib.secp256k1_silentpayments_create_public_tweak_data.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_size_t, c_voidp, c_size_t, c_voidp
]
secplib.secp256k1_silentpayments_create_public_tweak_data.restype = c_int
def silentpayments_create_tweak_data(plain_pubkeys, xonly_pubkeys, outpoint_lowest):
    # convert raw plain pubkeys to secp256k1 pubkeys
    plain_pubkeys_objs = [pubkey_parse(p) for p in plain_pubkeys]
    xonly_pubkeys_objs = [xonly_pubkey_parse(p) for p in xonly_pubkeys]
    array_plain_pubkeys = (c_voidp * len(plain_pubkeys))()
    for i in range(len(plain_pubkeys)): array_plain_pubkeys[i] = cast(plain_pubkeys_objs[i], c_voidp)
    array_xonly_pubkeys = (c_voidp * len(xonly_pubkeys))()
    for i in range(len(xonly_pubkeys)): array_xonly_pubkeys[i] = cast(xonly_pubkeys_objs[i], c_voidp)
    if len(array_plain_pubkeys) == 0: array_plain_pubkeys = None
    if len(array_xonly_pubkeys) == 0: array_xonly_pubkeys = None

    result_A_sum = create_string_buffer(SECP256K1_PUBKEY_STRUCT_SIZE)
    result_input_hash = create_string_buffer(32)
    retval = secplib.secp256k1_silentpayments_create_public_tweak_data(
        secp_context, result_A_sum, result_input_hash, array_plain_pubkeys, len(plain_pubkeys),
        array_xonly_pubkeys, len(xonly_pubkeys), outpoint_lowest)
    assert retval == 1

    return bytes(result_A_sum), bytes(result_input_hash)

"""
int secp256k1_silentpayments_create_tweaked_pubkey(
    const secp256k1_context *ctx,
    secp256k1_pubkey *A_tweaked,
    const secp256k1_pubkey *A_sum,
    const unsigned char *input_hash
);
"""
secplib.secp256k1_silentpayments_create_tweaked_pubkey.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp
]
secplib.secp256k1_silentpayments_create_tweaked_pubkey.restype = c_int
def silentpayments_create_tweaked_pubkey(A_sum, input_hash):
    result_tweaked_pubkey = create_string_buffer(SECP256K1_PUBKEY_STRUCT_SIZE)
    retval = secplib.secp256k1_silentpayments_create_tweaked_pubkey(
        secp_context, result_tweaked_pubkey, A_sum, input_hash)
    assert retval == 1
    return bytes(result_tweaked_pubkey)

"""
int secp256k1_silentpayments_create_shared_secret(
    const secp256k1_context *ctx,
    unsigned char *shared_secret33,
    const secp256k1_pubkey *public_component,
    const unsigned char *secret_component,
    const unsigned char *input_hash
);
"""
secplib.secp256k1_silentpayments_create_shared_secret.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_voidp
]
secplib.secp256k1_silentpayments_create_shared_secret.restype = c_int
def silentpayments_create_shared_secret(public_component, secret_component, input_hash):
    result_shared_secret = create_string_buffer(33)
    retval = secplib.secp256k1_silentpayments_create_shared_secret(
        secp_context, result_shared_secret, public_component, secret_component, input_hash)
    assert retval == 1
    return bytes(result_shared_secret)

"""
int secp256k1_silentpayments_create_label_tweak(
    const secp256k1_context *ctx,
    secp256k1_pubkey *label,
    unsigned char *label_tweak32,
    const unsigned char *receiver_scan_seckey,
    unsigned int m
);
"""
secplib.secp256k1_silentpayments_create_label_tweak.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_uint
]
secplib.secp256k1_silentpayments_create_label_tweak.restype = c_int
def silentpayments_create_label_tweak(receiver_scan_seckey, m):
    result_label_tweak = create_string_buffer(32)
    result_label_obj = create_string_buffer(SECP256K1_PUBKEY_STRUCT_SIZE)
    retval = secplib.secp256k1_silentpayments_create_label_tweak(
        secp_context, result_label_obj, result_label_tweak, receiver_scan_seckey, m)
    assert retval == 1
    return pubkey_serialize(result_label_obj), bytes(result_label_tweak)

"""
int secp256k1_silentpayments_create_address_spend_pubkey(
    const secp256k1_context *ctx,
    unsigned char *l_addr_spend_pubkey33,
    const secp256k1_pubkey *receiver_spend_pubkey,
    const secp256k1_pubkey *label
"""
secplib.secp256k1_silentpayments_create_address_spend_pubkey.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp
]
secplib.secp256k1_silentpayments_create_address_spend_pubkey.restype = c_int
def silentpayments_create_address_spend_pubkey(receiver_spend_pubkey, label):
    result_address_spend_pubkey = create_string_buffer(33)
    # convert receiver pubkey to secp256k1 object
    receiver_spend_pubkey_obj = pubkey_parse(receiver_spend_pubkey)
    label_obj = pubkey_parse(label)

    retval = secplib.secp256k1_silentpayments_create_address_spend_pubkey(
        secp_context, result_address_spend_pubkey, receiver_spend_pubkey_obj, label_obj)
    assert retval == 1
    return bytes(result_address_spend_pubkey)

"""
int secp256k1_silentpayments_sender_create_output_pubkey(
    const secp256k1_context *ctx,
    secp256k1_xonly_pubkey *P_output_xonly,
    const unsigned char *shared_secret33,
    const secp256k1_pubkey *receiver_spend_pubkey,
    unsigned int k
);
"""
secplib.secp256k1_silentpayments_sender_create_output_pubkey.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_uint
]
secplib.secp256k1_silentpayments_sender_create_output_pubkey.restype = c_int
def silentpayments_sender_create_output_pubkey(shared_secret, receiver_spend_pubkey, k):
    result_xonly_pubkey = create_string_buffer(SECP256K1_XONLY_PUBKEY_STRUCT_SIZE)
    # convert receiver pubkey to secp256k1 object
    receiver_spend_pubkey_obj = pubkey_parse(receiver_spend_pubkey)

    retval = secplib.secp256k1_silentpayments_sender_create_output_pubkey(
        secp_context, result_xonly_pubkey, shared_secret, receiver_spend_pubkey_obj, k)
    assert retval == 1

    return xonly_pubkey_serialize(result_xonly_pubkey)

"""
typedef struct {
    secp256k1_pubkey label;
    secp256k1_pubkey label_negated;
} secp256k1_silentpayments_label_data;

int secp256k1_silentpayments_receiver_create_scanning_data(
    const secp256k1_context *ctx,
    int *direct_match,
    unsigned char *t_k,
    secp256k1_silentpayments_label_data *label_data,
    const unsigned char *shared_secret33,
    const secp256k1_pubkey *receiver_spend_pubkey,
    unsigned int k,
    const secp256k1_xonly_pubkey *tx_output
);
"""
secplib.secp256k1_silentpayments_receiver_scan_output.argtypes = [
    c_voidp, POINTER(c_int), c_voidp, c_voidp, c_voidp, c_voidp, c_uint, c_voidp
]
secplib.secp256k1_silentpayments_receiver_scan_output.restype = c_int
def silentpayments_receiver_scan_output(shared_secret, receiver_spend_pubkey, k, tx_output):
    # TODO: we are assuming here that the structs are packed, but i guess there is no guarantee?
    result_label_data = create_string_buffer(2 * SECP256K1_PUBKEY_STRUCT_SIZE)
    result_t_k = create_string_buffer(32)
    result_direct_match = c_int()

    receiver_spend_pubkey_obj = pubkey_parse(receiver_spend_pubkey)
    tx_output_obj = xonly_pubkey_parse(tx_output)

    retval = secplib.secp256k1_silentpayments_receiver_scan_output(
        secp_context, pointer(result_direct_match), result_t_k, result_label_data,
        shared_secret, receiver_spend_pubkey_obj, k, tx_output_obj)
    assert retval == 1

    label1, label2 = result_label_data[:64], result_label_data[64:]

    if result_direct_match:
        return True, bytes(result_t_k), None, None
    else:
        return False, bytes(result_t_k), pubkey_serialize(label1), pubkey_serialize(label2)

"""
int secp256k1_silentpayments_create_output_seckey(
    const secp256k1_context *ctx,
    unsigned char *output_seckey,
    const unsigned char *receiver_spend_seckey,
    const unsigned char *t_k,
    const unsigned char *label_tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);
);
"""
secplib.secp256k1_silentpayments_create_output_seckey.argtypes = [
    c_voidp, c_voidp, c_voidp, c_voidp, c_voidp
]
secplib.secp256k1_silentpayments_create_output_seckey.restype = c_int
def silentpayments_create_output_seckey(receiver_spend_seckey, t_k, label_tweak=None):
    result_seckey = create_string_buffer(32)
    retval = secplib.secp256k1_silentpayments_create_output_seckey(
        secp_context, result_seckey, receiver_spend_seckey, t_k, label_tweak)
    assert retval == 1
    return bytes(result_seckey)
