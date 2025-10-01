#!/usr/bin/env python3
# Copyright (c) 2020-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Utilities for working directly with the wallet's BDB database file

This is specific to the configuration of BDB used in this project:
    - Outer database contains single subdatabase named 'main'
    - btree
    - btree internal, leaf and overflow pages

Each key-value pair is two entries in a btree leaf, which optionally refers to overflow pages
if the data doesn't fit into a single page. The first entry is the key, the one that follows
is the value. And so on. Note that the entry data is itself not in the correct order. Instead
entry offsets are stored in the correct order and those offsets are needed to then retrieve
the data itself. Note that this implementation currently only supports reading databases that
are in the same endianness as the host.

Page format can be found in BDB source code dbinc/db_page.h

`db_dump -da wallet.dat` is useful to see the data in a wallet.dat BDB file
"""

import struct

# Important constants
PAGE_HEADER_SIZE = 26
OUTER_META_PAGE = 0

# Page type values
BTREE_INTERNAL = 3
BTREE_LEAF = 5
OVERFLOW_DATA = 7
BTREE_META = 9

# Record type values
RECORD_KEYDATA = 1
RECORD_OVERFLOW_DATA = 3

# Some magic numbers for sanity checking
BTREE_MAGIC = 0x053162
DB_VERSION = 9
SUBDATABASE_NAME = b'main'

# Deserializes an internal, leaf or overflow page into a dict.
# In addition to the common page header fields, the result contains an 'entries'
# array of dicts with the following fields, depending on the page type:
#     internal page [BTREE_INTERNAL]:
#         - 'page_num': referenced page number (used to find further pages to process)
#     leaf page [BTREE_LEAF]:
#         - 'record_type': record type, must be RECORD_KEYDATA or RECORD_OVERFLOW_DATA
#         - 'data': binary data (key or value payload),  if record type is RECORD_KEYDATA
#         - 'page_num': referenced overflow page number, if record type is RECORD_OVERFLOW_DATA
#     overflow page [OVERFLOW_DATA]:
#         - 'data': binary data (part of key or value payload)
def dump_page(data):
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

    assert pg_type in (BTREE_INTERNAL, BTREE_LEAF, OVERFLOW_DATA)

    if pg_type == OVERFLOW_DATA:
        assert entries == 1
        page_info['entries'].append({'data': data[26:26 + hf_offset]})
        return page_info

    for i in range(0, entries):
        entry = {}
        offset = page_info['entry_offsets'][i]
        record_header = data[offset:offset + 3]
        offset += 3
        e_len, record_type = struct.unpack('HB', record_header)

        if pg_type == BTREE_INTERNAL:
            assert record_type == RECORD_KEYDATA
            internal_record_data = data[offset:offset + 9]
            _, page_num, _ = struct.unpack('=BII', internal_record_data)
            entry['page_num'] = page_num
        elif pg_type == BTREE_LEAF:
            assert record_type in (RECORD_KEYDATA, RECORD_OVERFLOW_DATA)
            entry['record_type'] = record_type
            if record_type == RECORD_KEYDATA:
                entry['data'] = data[offset:offset + e_len]
            elif record_type == RECORD_OVERFLOW_DATA:
                overflow_record_data = data[offset:offset + 9]
                _, page_num, _ = struct.unpack('=BII', overflow_record_data)
                entry['page_num'] = page_num

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
def extract_kv_pairs(page_data, pages):
    out = {}
    last_key = None
    for i, entry in enumerate(page_data['entries']):
        data = b''
        if entry['record_type'] == RECORD_KEYDATA:
            data = entry['data']
        elif entry['record_type'] == RECORD_OVERFLOW_DATA:
            next_page = entry['page_num']
            while next_page != 0:
                opage = pages[next_page]
                opage_info = dump_page(opage)
                data += opage_info['entries'][0]['data']
                next_page = opage_info['next_pgno']

        # By virtue of these all being pairs, even number entries are keys, and odd are values
        if i % 2 == 0:
            out[entry['data']] = b''
            last_key = data
        else:
            out[last_key] = data
    return out

# Extract the key-value pairs of the BDB file given in filename
def dump_bdb_kv(filename):
    # Read in the BDB file and start deserializing it
    pages = []
    with open(filename, 'rb') as f:
        # Determine pagesize first
        data = f.read(PAGE_HEADER_SIZE)
        pagesize = struct.unpack('I', data[20:24])[0]
        assert pagesize in (512, 1024, 2048, 4096, 8192, 16384, 32768, 65536)

        # Read rest of first page
        data += f.read(pagesize - PAGE_HEADER_SIZE)
        assert len(data) == pagesize

        # Read all remaining pages
        while len(data) > 0:
            pages.append(data)
            data = f.read(pagesize)

    # Sanity check the meta pages, read root page
    outer_meta_info = dump_meta_page(pages[OUTER_META_PAGE])
    root_page_info = dump_page(pages[outer_meta_info['root']])
    assert root_page_info['pg_type'] == BTREE_LEAF
    assert len(root_page_info['entries']) == 2
    assert root_page_info['entries'][0]['data'] == SUBDATABASE_NAME
    assert len(root_page_info['entries'][1]['data']) == 4
    inner_meta_page = int.from_bytes(root_page_info['entries'][1]['data'], 'big')
    inner_meta_info = dump_meta_page(pages[inner_meta_page])

    # Fetch the kv pairs from the pages
    kv = {}
    pages_to_process = [inner_meta_info['root']]
    while len(pages_to_process) > 0:
        curr_page_no = pages_to_process.pop()
        assert curr_page_no <= outer_meta_info['last_pgno']
        info = dump_page(pages[curr_page_no])
        assert info['pg_type'] in (BTREE_INTERNAL, BTREE_LEAF)
        if info['pg_type'] == BTREE_INTERNAL:
            for entry in info['entries']:
                pages_to_process.append(entry['page_num'])
        elif info['pg_type'] == BTREE_LEAF:
            info_kv = extract_kv_pairs(info, pages)
            kv = {**kv, **info_kv}
    return kv
