#!/usr/bin/env python3
# Copyright (c) 2022 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

import argparse
import sys
import ipaddress
import math

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
    else:
        parser.print_help()
        sys.exit("No command provided.")

if __name__ == '__main__':
    main()
