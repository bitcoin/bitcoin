#!/usr/bin/env python3
# Copyright (c) 2022 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

import argparse
import sys
import ipaddress
import json
import math
from collections import defaultdict

import asmap

def load_file(input_file):
    try:
        contents = input_file.read()
    except OSError as err:
        sys.exit(f"Input file '{input_file.name}' cannot be read: {err.strerror}.")
    try:
        bin_asmap = asmap.ASMap.from_binary(contents)
    except ValueError:
        bin_asmap = None
    txt_error = None
    entries = None
    try:
        txt_contents = str(contents, encoding="utf-8")
    except UnicodeError:
        txt_error = "invalid UTF-8"
        txt_contents = None
    if txt_contents is not None:
        entries = []
        for line in txt_contents.split("\n"):
            idx = line.find('#')
            if idx >= 0:
                line = line[:idx]
            line = line.lstrip(' ').rstrip(' \t\r\n')
            if len(line) == 0:
                continue
            fields = line.split(' ')
            if len(fields) != 2:
                txt_error = f"unparseable line '{line}'"
                entries = None
                break
            prefix, asn = fields
            if len(asn) <= 2 or asn[:2] != "AS" or any(c < '0' or c > '9' for c in asn[2:]):
                txt_error = f"invalid ASN '{asn}'"
                entries = None
                break
            try:
                net = ipaddress.ip_network(prefix)
            except ValueError:
                txt_error = f"invalid network '{prefix}'"
                entries = None
                break
            entries.append((asmap.net_to_prefix(net), int(asn[2:])))
    if entries is not None and bin_asmap is not None and len(contents) > 0:
        sys.exit(f"Input file '{input_file.name}' is ambiguous.")
    if entries is not None:
        state = asmap.ASMap()
        state.update_multi(entries)
        return state
    if bin_asmap is not None:
        return bin_asmap
    sys.exit(f"Input file '{input_file.name}' is neither a valid binary asmap file nor valid text input ({txt_error}).")


def save_binary(output_file, state, fill):
    contents = state.to_binary(fill=fill)
    try:
        output_file.write(contents)
        output_file.close()
    except OSError as err:
        sys.exit(f"Output file '{output_file.name}' cannot be written to: {err.strerror}.")

def save_text(output_file, state, fill, overlapping):
    for prefix, asn in state.to_entries(fill=fill, overlapping=overlapping):
        net = asmap.prefix_to_net(prefix)
        try:
            print(f"{net} AS{asn}", file=output_file)
        except OSError as err:
            sys.exit(f"Output file '{output_file.name}' cannot be written to: {err.strerror}.")
    try:
        output_file.close()
    except OSError as err:
        sys.exit(f"Output file '{output_file.name}' cannot be written to: {err.strerror}.")

def main():
    parser = argparse.ArgumentParser(description="Tool for performing various operations on textual and binary asmap files.")
    subparsers = parser.add_subparsers(title="valid subcommands", dest="subcommand")

    parser_encode = subparsers.add_parser("encode", help="convert asmap data to binary format")
    parser_encode.add_argument('-f', '--fill', dest="fill", default=False, action="store_true",
                               help="permit reassigning undefined network ranges arbitrarily to reduce size")
    parser_encode.add_argument('infile', nargs='?', type=argparse.FileType('rb'), default=sys.stdin.buffer,
                               help="input asmap file (text or binary); default is stdin")
    parser_encode.add_argument('outfile', nargs='?', type=argparse.FileType('wb'), default=sys.stdout.buffer,
                               help="output binary asmap file; default is stdout")

    parser_decode = subparsers.add_parser("decode", help="convert asmap data to text format")
    parser_decode.add_argument('-f', '--fill', dest="fill", default=False, action="store_true",
                               help="permit reassigning undefined network ranges arbitrarily to reduce length")
    parser_decode.add_argument('-n', '--nonoverlapping', dest="overlapping", default=True, action="store_false",
                               help="output strictly non-overall ping network ranges (increases output size)")
    parser_decode.add_argument('infile', nargs='?', type=argparse.FileType('rb'), default=sys.stdin.buffer,
                               help="input asmap file (text or binary); default is stdin")
    parser_decode.add_argument('outfile', nargs='?', type=argparse.FileType('w'), default=sys.stdout,
                               help="output text file; default is stdout")

    parser_diff = subparsers.add_parser("diff", help="compute the difference between two asmap files")
    parser_diff.add_argument('-i', '--ignore-unassigned', dest="ignore_unassigned", default=False, action="store_true",
                             help="ignore unassigned ranges in the first input (useful when second input is filled)")
    parser_diff.add_argument('infile1', type=argparse.FileType('rb'),
                             help="first file to compare (text or binary)")
    parser_diff.add_argument('infile2', type=argparse.FileType('rb'),
                             help="second file to compare (text or binary)")

    parser_diff_addrs = subparsers.add_parser("diff_addrs",
                                              help="compute difference between two asmap files for a set of addresses")
    parser_diff_addrs.add_argument('-s', '--show-addresses', dest="show_addresses", default=False, action="store_true",
                                   help="include reassigned addresses in the output")
    parser_diff_addrs.add_argument("infile1", type=argparse.FileType("rb"),
                                   help="first file to compare (text or binary)")
    parser_diff_addrs.add_argument("infile2", type=argparse.FileType("rb"),
                                   help="second file to compare (text or binary)")
    parser_diff_addrs.add_argument("addrs_file", type=argparse.FileType("r"),
                                   help="address file containing getnodeaddresses output to use in the comparison "
                                   "(make sure to set the count parameter to zero to get all node addresses, "
                                   "e.g. 'bitcoin-cli getnodeaddresses 0 > addrs.json')")
    args = parser.parse_args()
    if args.subcommand is None:
        parser.print_help()
    elif args.subcommand == "encode":
        state = load_file(args.infile)
        save_binary(args.outfile, state, fill=args.fill)
    elif args.subcommand == "decode":
        state = load_file(args.infile)
        save_text(args.outfile, state, fill=args.fill, overlapping=args.overlapping)
    elif args.subcommand == "diff":
        state1 = load_file(args.infile1)
        state2 = load_file(args.infile2)
        ipv4_changed = 0
        ipv6_changed = 0
        for prefix, old_asn, new_asn in state1.diff(state2):
            if args.ignore_unassigned and old_asn == 0:
                continue
            net = asmap.prefix_to_net(prefix)
            if isinstance(net, ipaddress.IPv4Network):
                ipv4_changed += 1 << (32 - net.prefixlen)
            elif isinstance(net, ipaddress.IPv6Network):
                ipv6_changed += 1 << (128 - net.prefixlen)
            if new_asn == 0:
                print(f"# {net} was AS{old_asn}")
            elif old_asn == 0:
                print(f"{net} AS{new_asn} # was unassigned")
            else:
                print(f"{net} AS{new_asn} # was AS{old_asn}")
        ipv4_change_str = "" if ipv4_changed == 0 else f" (2^{math.log2(ipv4_changed):.2f})"
        ipv6_change_str = "" if ipv6_changed == 0 else f" (2^{math.log2(ipv6_changed):.2f})"

        print(
            f"# {ipv4_changed}{ipv4_change_str} IPv4 addresses changed; "
            f"{ipv6_changed}{ipv6_change_str} IPv6 addresses changed"
        )
    elif args.subcommand == "diff_addrs":
        state1 = load_file(args.infile1)
        state2 = load_file(args.infile2)
        address_info = json.load(args.addrs_file)
        addrs = {a["address"] for a in address_info if a["network"] in ["ipv4", "ipv6"]}
        reassignments = defaultdict(list)
        for addr in addrs:
            net = ipaddress.ip_network(addr)
            prefix = asmap.net_to_prefix(net)
            old_asn = state1.lookup(prefix)
            new_asn = state2.lookup(prefix)
            if new_asn != old_asn:
                reassignments[(old_asn, new_asn)].append(addr)
        reassignments = sorted(reassignments.items(), key=lambda item: len(item[1]), reverse=True)
        num_reassignment_type = defaultdict(int)
        for (old_asn, new_asn), reassigned_addrs in reassignments:
            num_reassigned = len(reassigned_addrs)
            num_reassignment_type[(bool(old_asn), bool(new_asn))] += num_reassigned
            old_asn_str = f"AS{old_asn}" if old_asn else "unassigned"
            new_asn_str = f"AS{new_asn}" if new_asn else "unassigned"
            opt = ": " + ", ".join(reassigned_addrs) if args.show_addresses else ""
            print(f"{num_reassigned} address(es) reassigned from {old_asn_str} to {new_asn_str}{opt}")
        num_reassignments = sum(len(addrs) for _, addrs in reassignments)
        share = num_reassignments / len(addrs) if len(addrs) > 0 else 0
        print(f"Summary: {num_reassignments:,} ({share:.2%}) of {len(addrs):,} addresses were reassigned "
              f"(migrations={num_reassignment_type[True, True]}, assignments={num_reassignment_type[False, True]}, "
              f"unassignments={num_reassignment_type[True, False]})")
    else:
        parser.print_help()
        sys.exit("No command provided.")

if __name__ == '__main__':
    main()
