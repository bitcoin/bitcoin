#!/usr/bin/env python3
import hashlib
import json
import sys

import bech32m
import ripemd160

NUMS_H = bytes.fromhex("50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0")
MAX_INPUTS_PER_TEST_CASE = 3
MAX_OUTPUTS_PER_TEST_CASE = 4
MAX_PERMUTATIONS_PER_SENDING_TEST_CASE = 12

def sha256(s):
    return hashlib.sha256(s).digest()

def hash160(s):
    return ripemd160.ripemd160(sha256(s))

def smallest_outpoint(outpoints):
    serialized_outpoints = [bytes.fromhex(txid)[::-1] + n.to_bytes(4, 'little') for txid, n in outpoints]
    return sorted(serialized_outpoints)[0]

def decode_silent_payments_address(address):
    _, data = bech32m.decode("sp", address)
    data = bytes(data)  # convert from list to bytes
    assert len(data) == 66
    return data[:33], data[33:]

def is_p2tr(s):  # OP_1 OP_PUSHBYTES_32 <32 bytes>
    return (len(s) == 34) and (s[0] == 0x51) and (s[1] == 0x20)

def is_p2wpkh(s):  # OP_0 OP_PUSHBYTES_20 <20 bytes>
    return (len(s) == 22) and (s[0] == 0x00) and (s[1] == 0x14)

def is_p2sh(s):  # OP_HASH160 OP_PUSHBYTES_20 <20 bytes> OP_EQUAL
    return (len(s) == 23) and (s[0] == 0xA9) and (s[1] == 0x14) and (s[-1] == 0x87)

def is_p2pkh(s):  # OP_DUP OP_HASH160 OP_PUSHBYTES_20 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
    return (len(s) == 25) and (s[0] == 0x76) and (s[1] == 0xA9) and (s[2] == 0x14) and \
        (s[-2] == 0x88) and (s[-1] == 0xAC)

def get_pubkey_from_input(spk, script_sig, witness):
    # build witness stack from raw witness data
    witness_stack = []
    no_witness_items = 0
    if len(witness) > 0:
        no_witness_items = witness[0]
        witness = witness[1:]
    for i in range(no_witness_items):
        item_len = witness[0]
        witness_stack.append(witness[1:item_len+1])
        witness = witness[item_len+1:]

    if is_p2pkh(spk):
        spk_pkh = spk[3:3 + 20]
        for i in range(len(script_sig), 0, -1):
            if i - 33 >= 0:
                pk = script_sig[i - 33:i]
                if hash160(pk) == spk_pkh:
                    return pk
    elif is_p2sh(spk) and is_p2wpkh(script_sig[1:]):
        pubkey = witness_stack[-1]
        if len(pubkey) == 33:
            return pubkey
    elif is_p2wpkh(spk):
        # the witness must contain two items and the second item is the pubkey
        pubkey = witness_stack[-1]
        if len(pubkey) == 33:
            return pubkey
    elif is_p2tr(spk):
        if len(witness_stack) > 1 and witness_stack[-1][0] == 0x50:
            witness_stack.pop()
        if len(witness_stack) > 1:  # script-path spend?
            control_block = witness_stack[-1]
            internal_key = control_block[1:33]
            if internal_key == NUMS_H:  # skip
                return b''
        return spk[2:]

    return b''

def to_c_array(x):
    if x == "":
        return ""
    s = ',0x'.join(a+b for a,b in zip(x[::2], x[1::2]))
    return "0x" + s

def emit_key_material(comment, keys, include_count=False):
    global out
    if include_count:
        out += f"        {len(keys)}," + "\n"
    out += f"        {{ /* {comment} */" + "\n"
    for i in range(MAX_INPUTS_PER_TEST_CASE):
        out += "            "
        if i < len(keys):
            out += "{"
            out += to_c_array(keys[i])
            out += "}"
        else:
            out += '""'
        out += ",\n"
    out +=  "        },\n"

def emit_recipient_addr_material(recipient_addresses):
    global out
    out += f"        {len(recipient_addresses)}," + "\n"
    out +=  "        { /* recipient pubkeys (address data) */\n"
    for i in range(MAX_OUTPUTS_PER_TEST_CASE):
        out += "            {\n"
        if i < len(recipient_addresses):
            B_scan, B_spend = decode_silent_payments_address(recipient_addresses[i])
            out += "                {"
            out += to_c_array(B_scan.hex())
            out += "},\n"
            out += "                {"
            out += to_c_array(B_spend.hex())
            out += "},\n"
        else:
            out += '                "",\n'
            out += '                "",\n'
        out += "            }"
        out += ",\n"
    out += "        },\n"

def emit_sending_outputs(comment, output_sets, include_count=False):
    global out
    if include_count:
        out += f"        {len(output_sets)}," + "\n"
        out += f"        {len(output_sets[0])}," + "\n"
    if comment:
        out += f"        {{ /* {comment} */" + "\n"
    else:
        out += "         {\n"
    for i in range(MAX_PERMUTATIONS_PER_SENDING_TEST_CASE):
        if i < len(output_sets):
            emit_outputs(comment=None, outputs=output_sets[i], include_count=False, spacing=12)
        else:
            emit_outputs(comment=None, outputs=[], include_count=False, spacing=12)
    out += "        }"
    out += ","
    out += "\n"

def emit_outputs(comment, outputs, include_count=False, last=False, spacing=8):
    global out
    if include_count:
        out += spacing*" " + f"{len(outputs)}," + "\n"
    if comment:
        out += spacing*" " + f"{{ /* {comment} */" + "\n"
    else:
        out += spacing*" " + "{\n"
    for i in range(MAX_OUTPUTS_PER_TEST_CASE):
        if i < len(outputs):
            out += spacing*" " + "    {"
            out += to_c_array(outputs[i])
            out += "}"
        else:
            out += spacing*' ' + '    ""'
        out += ",\n"
    out += spacing*" " + "}"
    if not last:
        out += ","
    out += "\n"

filename_input = sys.argv[1]
with open(filename_input) as f:
    test_vectors = json.load(f)

out = ""
num_vectors = 0

for test_nr, test_vector in enumerate(test_vectors):
    # determine input private and public keys, grouped into plain and taproot/x-only
    input_plain_seckeys = []
    input_taproot_seckeys = []
    input_plain_pubkeys = []
    input_xonly_pubkeys = []
    outpoints = []
    for i in test_vector['sending'][0]['given']['vin']:
        pub_key = get_pubkey_from_input(bytes.fromhex(i['prevout']['scriptPubKey']['hex']),
            bytes.fromhex(i['scriptSig']), bytes.fromhex(i['txinwitness']))
        if len(pub_key) == 33:  # regular input
            input_plain_seckeys.append(i['private_key'])
            input_plain_pubkeys.append(pub_key.hex())
        elif len(pub_key) == 32:  # taproot input
            input_taproot_seckeys.append(i['private_key'])
            input_xonly_pubkeys.append(pub_key.hex())
        outpoints.append((i['txid'], i['vout']))
    if len(input_plain_pubkeys) == 0 and len(input_xonly_pubkeys) == 0:
        continue

    num_vectors += 1
    out += f"    /* ----- {test_vector['comment']} ({num_vectors}) ----- */\n"
    out +=  "    {\n"

    outpoint_L = smallest_outpoint(outpoints).hex()
    emit_key_material("input plain seckeys", input_plain_seckeys, include_count=True)
    emit_key_material("input plain pubkeys", input_plain_pubkeys)
    emit_key_material("input taproot seckeys", input_taproot_seckeys, include_count=True)
    emit_key_material("input x-only pubkeys", input_xonly_pubkeys)
    out += "        /* smallest outpoint */\n"
    out += "        {"
    out += to_c_array(outpoint_L)
    out += "},\n"

    # emit recipient pubkeys (address data)
    emit_recipient_addr_material(test_vector['sending'][0]['given']['recipients'])
    # emit recipient outputs
    emit_sending_outputs("recipient outputs", test_vector['sending'][0]['expected']['outputs'], include_count=True)

    # emit recipient scan/spend seckeys
    recv_test_given = test_vector['receiving'][0]['given']
    recv_test_expected = test_vector['receiving'][0]['expected']
    out += "        /* recipient data (scan and spend seckeys) */\n"
    out += "        {" + f"{to_c_array(recv_test_given['key_material']['scan_priv_key'])}" + "},\n"
    out += "        {" + f"{to_c_array(recv_test_given['key_material']['spend_priv_key'])}" + "},\n"

    # emit recipient to-scan outputs, labels and expected-found outputs
    emit_outputs("outputs to scan", recv_test_given['outputs'], include_count=True)
    labels = recv_test_given['labels']
    out += f"        {len(labels)}, " + "{"
    for i in range(4):
        if i < len(labels):
            out += f"{labels[i]}"
        else:
            out += "0xffffffff"
        if i != 3:
            out += ", "
    out += "}, /* labels */\n"
    expected_pubkeys = [o['pub_key'] for o in recv_test_expected['outputs']]
    expected_tweaks = [o['priv_key_tweak'] for o in recv_test_expected['outputs']]
    expected_signatures = [o['signature'] for o in recv_test_expected['outputs']]
    out += "        /* expected output data (pubkeys and seckey tweaks) */\n"
    emit_outputs("", expected_pubkeys, include_count=True)
    emit_outputs("", expected_tweaks)
    emit_outputs("", expected_signatures, last=True)

    out += "    }"
    if test_nr != len(test_vectors)-1:
        out += ","
    out += "\n\n"

STRUCT_DEFINITIONS = f"""
#define MAX_INPUTS_PER_TEST_CASE  {MAX_INPUTS_PER_TEST_CASE}
#define MAX_OUTPUTS_PER_TEST_CASE {MAX_OUTPUTS_PER_TEST_CASE}
#define MAX_PERMUTATIONS_PER_SENDING_TEST_CASE {MAX_PERMUTATIONS_PER_SENDING_TEST_CASE}

struct bip352_recipient_addressdata {{
    unsigned char scan_pubkey[33];
    unsigned char spend_pubkey[33];
}};

struct bip352_test_vector {{
    /* Inputs (private keys / public keys + smallest outpoint) */
    size_t num_plain_inputs;
    unsigned char plain_seckeys[MAX_INPUTS_PER_TEST_CASE][32];
    unsigned char plain_pubkeys[MAX_INPUTS_PER_TEST_CASE][33];

    size_t num_taproot_inputs;
    unsigned char taproot_seckeys[MAX_INPUTS_PER_TEST_CASE][32];
    unsigned char xonly_pubkeys[MAX_INPUTS_PER_TEST_CASE][32];

    unsigned char outpoint_smallest[36];

    /* Given sender data (pubkeys encoded per output address to send to) */
    size_t num_outputs;
    struct bip352_recipient_addressdata recipient_pubkeys[MAX_OUTPUTS_PER_TEST_CASE];

    /* Expected sender data */
    size_t num_output_sets;
    size_t num_recipient_outputs;
    unsigned char recipient_outputs[MAX_PERMUTATIONS_PER_SENDING_TEST_CASE][MAX_OUTPUTS_PER_TEST_CASE][32];

    /* Given recipient data */
    unsigned char scan_seckey[32];
    unsigned char spend_seckey[32];
    size_t num_to_scan_outputs;
    unsigned char to_scan_outputs[MAX_OUTPUTS_PER_TEST_CASE][32];
    size_t num_labels;
    unsigned int label_integers[MAX_OUTPUTS_PER_TEST_CASE];

    /* Expected recipient data */
    size_t num_found_output_pubkeys;
    unsigned char found_output_pubkeys[MAX_OUTPUTS_PER_TEST_CASE][32];
    unsigned char found_seckey_tweaks[MAX_OUTPUTS_PER_TEST_CASE][32];
    unsigned char found_signatures[MAX_OUTPUTS_PER_TEST_CASE][64];
}};
"""

print("/* Note: this file was autogenerated using tests_silentpayments_generate.py. Do not edit. */")
print(f"#define SECP256K1_SILENTPAYMENTS_NUMBER_TESTVECTORS ({num_vectors})")

print(STRUCT_DEFINITIONS)

print("static const struct bip352_test_vector bip352_test_vectors[SECP256K1_SILENTPAYMENTS_NUMBER_TESTVECTORS] = {")
print(out, end='')
print("};")
