#!/usr/bin/env python3

"""
A script to convert BIP352 test vectors from JSON to a C header.

Usage:

    ./tools/tests_silentpayments_generate.py src/modules/silentpayments/bip352_send_and_receive_test_vectors.json  > ./src/modules/silentpayments/vectors.h
"""

import hashlib
import json
import sys

NUMS_H = bytes.fromhex("50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0")
MAX_INPUTS_PER_TEST_CASE = 3
MAX_OUTPUTS_PER_TEST_CASE = 4
MAX_PERMUTATIONS_PER_SENDING_TEST_CASE = 12

def sha256(s):
    return hashlib.sha256(s).digest()

def smallest_outpoint(outpoints):
    serialized_outpoints = [bytes.fromhex(txid)[::-1] + n.to_bytes(4, 'little') for txid, n in outpoints]
    return sorted(serialized_outpoints)[0]

def is_p2tr(s):  # OP_1 OP_PUSHBYTES_32 <32 bytes>
    return (len(s) == 34) and (s[0] == 0x51) and (s[1] == 0x20)

def get_pubkey_from_input(input_data, pubkey_hex):
    """Extract the correct pubkey for an input, handling NUMS_H filtering and format conversion"""
    spk = bytes.fromhex(input_data['prevout']['scriptPubKey']['hex'])
    pubkey = bytes.fromhex(pubkey_hex)

    if is_p2tr(spk):  # taproot input
        # Check for NUMS_H in witness (should be skipped)
        witness = bytes.fromhex(input_data.get('txinwitness', ''))
        # Parse witness stack
        witness_stack = []
        num_witness_items = 0
        if len(witness) > 0:
            num_witness_items = witness[0]
            witness = witness[1:]
        for i in range(num_witness_items):
            item_len = witness[0]
            witness_stack.append(witness[1:item_len+1])
            witness = witness[item_len+1:]

        # Check for script-path spend with NUMS_H
        if len(witness_stack) > 1 and witness_stack[-1][0] == 0x50:
            witness_stack.pop()
        if len(witness_stack) > 1:  # script-path spend?
            control_block = witness_stack[-1]
            internal_key = control_block[1:33]
            if internal_key == NUMS_H:  # skip
                return b''

        # Convert to x-only (32 bytes) for taproot
        if len(pubkey) == 33:
            pubkey = pubkey[1:]  # Remove prefix byte
        return pubkey
    else:  # regular input - use full compressed pubkey (33 bytes)
        return pubkey

def gen_byte_array(hex):
    assert len(hex) % 2 == 0
    if hex == "":
        return "{0x00}"
    s = ',0x'.join(a + b for a, b in zip(hex[::2], hex[1::2]))
    return "{0x" + s + "}"

def maybe_gen_comment(comment):
    if comment:
        return f" /* {comment} */"
    else:
        return ""

def gen_key_material(keys, comment=None, prepend_count=False):
    assert len(keys) <= MAX_INPUTS_PER_TEST_CASE
    out = ""
    if prepend_count:
        out += f"        {len(keys)},\n"
    out += f"        {{{maybe_gen_comment(comment)}\n"
    for k in keys:
        out += f"            {gen_byte_array(k)},\n"
    if not keys:
        out += '            "",\n'
    out +=  "        },\n"
    return out

def gen_recipient_addr_material(recipients):
    assert len(recipients) <= MAX_OUTPUTS_PER_TEST_CASE
    out = f"        {len(recipients)},\n"
    out += "        { /* recipient pubkeys (address data) */\n"
    for i in range(MAX_OUTPUTS_PER_TEST_CASE):
        out += "            {\n"
        if i < len(recipients):
            # Use the scan_pubkey and spend_pubkey directly from the recipient
            scan_pubkey = bytes.fromhex(recipients[i]['scan_pub_key'])
            spend_pubkey = bytes.fromhex(recipients[i]['spend_pub_key'])

            out += f"                {gen_byte_array(scan_pubkey.hex())},\n"
            out += f"                {gen_byte_array(spend_pubkey.hex())},\n"
        else:
            out += '                "",\n'
            out += '                "",\n'
        out += "            },\n"
    out += "        },\n"
    return out

def gen_sending_outputs(output_sets, comment=None, prepend_count=False):
    assert len(output_sets) <= MAX_PERMUTATIONS_PER_SENDING_TEST_CASE
    out = ""
    if prepend_count:
        out += f"        {len(output_sets)},\n"
        out += f"        {len(output_sets[0])},\n"
    out += f"        {{{maybe_gen_comment(comment)}\n"
    for o in output_sets:
        out += gen_outputs(outputs=o, prepend_count=False, indent=12)
    if not output_sets:
        out += gen_outputs(comment=None, outputs=[], prepend_count=False, indent=12)
    out += "        },\n"
    return out

def gen_outputs(outputs, comment=None, prepend_count=False, indent=8):
    out = ""
    spaces = indent * " "
    if prepend_count:
        out += spaces + f"{len(outputs)},\n"
    out += f"        {{{maybe_gen_comment(comment)}\n"
    for o in outputs:
        out += spaces + f"    {gen_byte_array(o)},\n"
    if not outputs:
        out += spaces + '    "",\n'
    out += spaces + "},\n"
    return out

def gen_labels(labels):
    assert len(labels) <= MAX_OUTPUTS_PER_TEST_CASE
    out = "        /* labels */\n"
    out += f"        {len(labels)}, {{"
    for i in range(MAX_OUTPUTS_PER_TEST_CASE):
        if i < len(labels):
            out += f"{labels[i]}, "
        else:
            out += "0xffffffff, "
    out += "},\n"
    return out

def gen_preamble(test_vectors):
    out = "/* Note: this file was autogenerated using tests_silentpayments_generate.py. Do not edit. */\n"
    out += f"""
#include <stddef.h>

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
    return out

def gen_test_vectors(test_vectors):
    out = f"#define SECP256K1_SILENTPAYMENTS_NUMBER_TESTVECTORS {len(test_vectors)}\n\n"
    out += "static const struct bip352_test_vector bip352_test_vectors[SECP256K1_SILENTPAYMENTS_NUMBER_TESTVECTORS] = {\n"
    for test_i, test_vector in enumerate(test_vectors):
        # determine input private and public keys, grouped into plain and taproot/x-only
        input_plain_seckeys = []
        input_taproot_seckeys = []
        input_plain_pubkeys = []
        input_xonly_pubkeys = []
        outpoints = []

        pubkey_index = 0
        input_pubkeys_hex = test_vector['sending'][0]['expected']['input_pub_keys']

        for vec in test_vector['sending'][0]['given']['vin']:
            outpoints.append((vec['txid'], vec['vout']))

            if pubkey_index < len(input_pubkeys_hex):
                pubkey = get_pubkey_from_input(vec, input_pubkeys_hex[pubkey_index])
                if len(pubkey) == 33:  # regular input
                    input_plain_seckeys.append(vec['private_key'])
                    input_plain_pubkeys.append(pubkey.hex())
                    pubkey_index += 1
                elif len(pubkey) == 32:  # taproot input
                    input_taproot_seckeys.append(vec['private_key'])
                    input_xonly_pubkeys.append(pubkey.hex())
                    pubkey_index += 1
                # len(pubkey) == 0, it's a NUMS_H input - skip without incrementing

        out += f"    /* ----- {test_vector['comment']} ({test_i + 1}) ----- */\n"
        out +=  "    {\n"

        outpoint_L = smallest_outpoint(outpoints).hex()
        out += gen_key_material(input_plain_seckeys, "input plain seckeys", prepend_count=True)
        out += gen_key_material(input_plain_pubkeys, "input plain pubkeys")
        out += gen_key_material(input_taproot_seckeys, "input taproot seckeys", prepend_count=True)
        out += gen_key_material(input_xonly_pubkeys, "input x-only pubkeys")
        out += "        /* smallest outpoint */\n"
        out += f"        {gen_byte_array(outpoint_L)},\n"

        # emit recipient pubkeys (address data)
        out += gen_recipient_addr_material(test_vector['sending'][0]['given']['recipients'])
        # emit recipient outputs
        out += gen_sending_outputs(test_vector['sending'][0]['expected']['outputs'], "recipient outputs", prepend_count=True)

        # emit recipient scan/spend seckeys
        recv_test_given = test_vector['receiving'][0]['given']
        recv_test_expected = test_vector['receiving'][0]['expected']
        out += "        /* recipient data (scan and spend seckeys) */\n"
        out += f"        {gen_byte_array(recv_test_given['key_material']['scan_priv_key'])},\n"
        out += f"        {gen_byte_array(recv_test_given['key_material']['spend_priv_key'])},\n"

        # emit recipient to-scan outputs, labels and expected-found outputs
        out += gen_outputs(recv_test_given['outputs'], "outputs to scan", prepend_count=True)
        out += gen_labels(recv_test_given['labels'])
        expected_pubkeys = [o['pub_key'] for o in recv_test_expected['outputs']]
        expected_tweaks = [o['priv_key_tweak'] for o in recv_test_expected['outputs']]
        expected_signatures = [o['signature'] for o in recv_test_expected['outputs']]
        out += "        /* expected output data (pubkeys and seckey tweaks) */\n"
        out += gen_outputs(expected_pubkeys, prepend_count=True)
        out += gen_outputs(expected_tweaks)
        out += gen_outputs(expected_signatures)
        out += "    },\n\n"
    out += "};\n"
    return out

def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    filename_input = sys.argv[1]
    with open(filename_input) as f:
        test_vectors = json.load(f)

    print(gen_preamble(test_vectors))
    print(gen_test_vectors(test_vectors), end="")

if __name__ == "__main__":
    main()
