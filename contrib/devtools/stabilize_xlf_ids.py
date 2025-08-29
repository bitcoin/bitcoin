#!/usr/bin/env python3
"""
Stabilize XLF ids by replacing sequential ids with stable content hashes.

The ID is derived from the Qt context ("resname") and the English source text.
Plural forms keep the original "[N]" suffix.
"""

import hashlib
import sys
import os
import re
import xml.etree.ElementTree as ET

PLURAL_SUFFIX = re.compile(r'(\[\d+])$')


def source_text(tu):
    source = tu.find('./{*}source')
    assert source is not None, "trans-unit missing <source>"
    return ''.join(source.itertext())


def stable_id(resname, text):
    return hashlib.sha256(f"{resname}\0{text}".encode('utf-8')).hexdigest()


def build_id_mapping(path):
    root = ET.parse(path).getroot()

    mapping = {}
    for g in root.findall('.//{*}group[@resname]'):
        resname = g.get('resname')
        for tu in g.findall('.//{*}trans-unit'):
            old_id = tu.get('id')
            assert old_id is not None, "trans-unit missing id attribute"
            assert old_id not in mapping, f"duplicate input id detected: {old_id}"
            plural = PLURAL_SUFFIX.search(old_id)
            suffix = plural.group(1) if plural else ''
            mapping[old_id] = f"{stable_id(resname, source_text(tu))}{suffix}"

    if len(mapping) != len(root.findall('.//{*}trans-unit')):
        raise SystemExit("trans-unit missing group resname")

    if len(mapping) != len(set(mapping.values())):
        raise SystemExit("duplicate output ids detected")
    return mapping


def stabilize_ids(old_path, new_path, mapping):
    os.replace(new_path, old_path)  # we don't need the old translation units anymore
    with open(old_path, 'r+', encoding='utf-8') as f:
        content = re.sub(r'\bid="([^"]+)"', lambda m: f'id="{mapping.get(m.group(1), m.group(1))}"', f.read())
        f.seek(0)
        f.write(content)
        f.truncate()


def verify_unique_ids(path):
    ids = [tu.get('id') for tu in ET.parse(path).getroot().findall('.//{*}trans-unit')]
    if len(ids) != len(set(ids)):
        raise SystemExit("duplicate id detected")


def main():
    if len(sys.argv) != 3:
        print("Usage: stabilize_xlf_ids.py <old_xlf> <new_xlf>", file=sys.stderr)
        sys.exit(1)

    _, old_path, new_path = sys.argv
    mapping = build_id_mapping(new_path)

    stabilize_ids(old_path, new_path, mapping)
    verify_unique_ids(old_path)

    print(f"[stats] units={len(mapping)}", file=sys.stderr)


if __name__ == "__main__":
    main()
