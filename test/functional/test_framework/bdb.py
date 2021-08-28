#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Utilities for working directly with the wallet's BDB database file

This is specific to the configuration of BDB used in this project:
    - pagesize: 4096 bytes
    - Outer database contains single subdatabase named 'main'
    - btree
    - btree leaf pages

Each key-value pair is two entries in a btree leaf. The first is the key, the one that follows
is the value. And so on. Note that the entry data is itself not in the correct order. Instead
entry offsets are stored in the correct order and those offsets are needed to then retrieve
the data itself.

Page format can be found in BDB source code dbinc/db_page.h
This only implements the deserialization of btree metadata pages and normal btree pages. Overflow
pages are not implemented but may be needed in the future if dealing with wallets with large
transactions.

`db_dump -da wallet.dat` is useful to see the data in a wallet.dat BDB file
"""

import struct

# Important constants
PAGESIZE = 4096
OUTER_META_PAGE = 0
INNER_META_PAGE = 2

# Page type values
BTREE_INTERNAL = 3
BTREE_LEAF = 5
BTREE_META = 9

# Some magic numbers for sanity checking
BTREE_MAGIC = 0x053162
DB_VERSION = 9

# Deserializes a leaf page into a dict.
# Btree internal pages have the same header, for those, return None.
# For the btree leaf pages, deserialize them and put all the data into a dict
def dump_leaf_page(data):
    page_info = {}
    page_header = data[0:26]
    _, pgno, prev_pgno, next_pgno, entries, hf_offset, level, pg_type = struct.unpack('QIIIHHBB', page_header)
    page_info['pgno'] = pgno
    page_info['prev_pgno'] = prev_pgno
    page_info['next_pgno'] = next_pgno
    page_info['hf_offset'] = hf_offset
    page_info['level'] = level
    page_info['pg_type'] = pg_type
    page_info['entry_offsets'] = struct.unpack('{}H'.format(entries), data[26:26 + entries * 2])
    page_info['entries'] = []

    if pg_type == BTREE_INTERNAL:
        # Skip internal pages. These are the internal nodes of the btree and don't contain anything relevant to us
        return None

    assert pg_type == BTREE_LEAF, 'A non-btree leaf page has been encountered while dumping leaves'

    for i in range(0, entries):
        offset = page_info['entry_offsets'][i]
        entry = {'offset': offset}
        page_data_header = data[offset:offset + 3]
        e_len, pg_type = struct.unpack('HB', page_data_header)
        entry['len'] = e_len
        entry['pg_type'] = pg_type
        entry['data'] = data[offset + 3:offset + 3 + e_len]
        page_info['entries'].append(entry)

    return page_info

# Deserializes a btree metadata page into a dict.
# Does a simple sanity check on the magic value, type, and version
def dump_meta_page(page):
    # metadata page
    # general metadata
    metadata = {}
    meta_page = page[0:72]
    _, pgno, magic, version, pagesize, encrypt_alg, pg_type, metaflags, _, free, last_pgno, nparts, key_count, record_count, flags, uid = struct.unpack('QIIIIBBBBIIIIII20s', meta_page)
    metadata['pgno'] = pgno
    metadata['magic'] = magic
    metadata['version'] = version
    metadata['pagesize'] = pagesize
    metadata['encrypt_alg'] = encrypt_alg
    metadata['pg_type'] = pg_type
    metadata['metaflags'] = metaflags
    metadata['free'] = free
    metadata['last_pgno'] = last_pgno
    metadata['nparts'] = nparts
    metadata['key_count'] = key_count
    metadata['record_count'] = record_count
    metadata['flags'] = flags
    metadata['uid'] = uid.hex().encode()

    assert magic == BTREE_MAGIC, 'bdb magic does not match bdb btree magic'
    assert pg_type == BTREE_META, 'Metadata page is not a btree metadata page'
    assert version == DB_VERSION, 'Database too new'

    # btree metadata
    btree_meta_page = page[72:512]
    _, minkey, re_len, re_pad, root, _, crypto_magic, _, iv, chksum = struct.unpack('IIIII368sI12s16s20s', btree_meta_page)
    metadata['minkey'] = minkey
    metadata['re_len'] = re_len
    metadata['re_pad'] = re_pad
    metadata['root'] = root
    metadata['crypto_magic'] = crypto_magic
    metadata['iv'] = iv.hex().encode()
    metadata['chksum'] = chksum.hex().encode()

    return metadata

# Given the dict from dump_leaf_page, get the key-value pairs and put them into a dict
def extract_kv_pairs(page_data):
    out = {}
    last_key = None
    for i, entry in enumerate(page_data['entries']):
        # By virtue of these all being pairs, even number entries are keys, and odd are values
        if i % 2 == 0:
            out[entry['data']] = b''
            last_key = entry['data']
        else:
            out[last_key] = entry['data']
    return out

# Extract the key-value pairs of the BDB file given in filename
def dump_bdb_kv(filename):
    # Read in the BDB file and start deserializing it
    pages = []
    with open(filename, 'rb') as f:
        data = f.read(PAGESIZE)
        while len(data) > 0:
            pages.append(data)
            data = f.read(PAGESIZE)

    # Sanity check the meta pages
    dump_meta_page(pages[OUTER_META_PAGE])
    dump_meta_page(pages[INNER_META_PAGE])

    # Fetch the kv pairs from the leaf pages
    kv = {}
    for i in range(3, len(pages)):
        info = dump_leaf_page(pages[i])
        if info is not None:
            info_kv = extract_kv_pairs(info)
            kv = {**kv, **info_kv}
    return kv
