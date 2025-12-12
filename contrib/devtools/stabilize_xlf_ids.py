#!/usr/bin/env python3
"""
Stabilize XLF ids by preserving existing ids for unchanged case-insensitive resname#text keys,
and assigning new sequential ids for genuinely new occurrences.
"""

import sys
import os
import re
import xml.etree.ElementTree as ET
from collections import defaultdict
from itertools import count, islice

# Accept plain and plural IDs like _msg862 and _msg862[0]
ID = re.compile(r'^_msg(\d+)(?:\[\d+])?$')


def get_translation_units(root):
    # map: "resname#source-text" -> [ids in encounter order]
    d = defaultdict(list)
    for g in root.findall('.//{*}group[@resname]'):
        for tu in g.findall('.//{*}trans-unit'):
            key = f"{g.get('resname')}#{tu.find('.//{*}source').text.lower()}"
            d[key].append(tu.get('id'))
    return d


def compute_remaps(old_map, new_map):
    return [(nid, oid)
            for key, nids in new_map.items()
            for nid, oid in zip(nids, old_map.get(key, ()))
            if oid != nid]


def compute_assignments(old_map, new_map, last_id):
    return [(nid, f"_msg{next(last_id)}")
            for key, nids in new_map.items()
            for nid in islice(nids, len(old_map.get(key, ())), None)]


def stabilize_ids(old_path, new_path, mapping):
    os.replace(new_path, old_path)  # we don't need the old translation units anymore
    with open(old_path, 'r+', encoding='utf-8') as f:
        content = re.sub(r'\bid="(_msg\d+(?:\[\d+])?)"', lambda m: f'id="{mapping.get(m.group(1), m.group(1))}"', f.read())
        f.seek(0)
        f.write(content)


def verify_unique_ids(path):
    seen = set()
    for tu in ET.parse(path).getroot().findall('.//{*}trans-unit'):
        sid = tu.get('id')
        assert sid not in seen, f"duplicate id detected: {sid}"
        seen.add(sid)


def main():
    if len(sys.argv) != 3:
        print("Usage: stabilize_xlf_ids.py <old_xlf> <new_xlf>", file=sys.stderr)
        sys.exit(1)

    _, old_path, new_path = sys.argv
    old_map, new_map = [get_translation_units(ET.parse(p).getroot()) for p in (old_path, new_path)]
    remaps = compute_remaps(old_map, new_map)

    last_id = max((int(m.group(1)) for ids in old_map.values() for oid in ids if (m := ID.match(oid))))
    assigns = compute_assignments(old_map, new_map, count(last_id + 1))

    stabilize_ids(old_path, new_path, dict(assigns + remaps))
    verify_unique_ids(old_path)

    print(f"[stats] old_units={sum(len(v) for v in old_map.values())} new_units={sum(len(v) for v in new_map.values())}", file=sys.stderr)
    print(f"[stats] new_ids_assigned={len(assigns)}", file=sys.stderr)
    print(f"[stats] id_reassignments={len(remaps)}", file=sys.stderr)


if __name__ == "__main__":
    main()
