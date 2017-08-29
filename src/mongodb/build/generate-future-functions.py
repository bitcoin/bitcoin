#!/usr/bin/env python
#
# Copyright 2015 MongoDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generate test functions for use with mock_server_t.

Defines functions like future_cursor_next in future-functions.h and
future-functions.c, which defer a libmongoc operation to a background thread
via functions like background_cursor_next. Also defines functions like
future_value_set_bson_ptr and future_value_get_bson_ptr which support the
future / background functions, and functions like future_get_bson_ptr which
wait for a future to resolve, then return its value.

These future functions are used in conjunction with mock_server_t to
conveniently test libmongoc wire protocol operations.

Written for Python 2.6+, requires Jinja 2 for templating.
"""

from collections import namedtuple
from os.path import basename, dirname, join as joinpath, normpath

from jinja2 import Environment, FileSystemLoader  # Please "pip install jinja2".


this_dir = dirname(__file__)
template_dir = joinpath(this_dir, 'future_function_templates')
mock_server_dir = normpath(joinpath(this_dir, '../tests/mock_server'))

# Add additional types here. Use typedefs for derived types so they can
# be named with one symbol.
typedef = namedtuple("typedef", ["name", "typedef"])

# These are typedef'ed if necessary in future-value.h, and added to the union
# of possible future_value_t.value types. future_value_t getters and setters
# are generated for all types, as well as future_t getters.
typedef_list = [
    # Fundamental.
    typedef("bool", None),
    typedef("char_ptr", "char *"),
    typedef("char_ptr_ptr", "char **"),
    typedef("int", None),
    typedef("int64_t", None),
    typedef("size_t", None),
    typedef("ssize_t", None),
    typedef("uint32_t", None),

    # Const fundamental.
    typedef("const_char_ptr", "const char *"),

    # libbson.
    typedef("bson_error_ptr", "bson_error_t *"),
    typedef("bson_ptr", "bson_t *"),

    # Const libbson.
    typedef("const_bson_ptr", "const bson_t *"),
    typedef("const_bson_ptr_ptr", "const bson_t **"),

    # libmongoc.
    typedef("mongoc_bulk_operation_ptr", "mongoc_bulk_operation_t *"),
    typedef("mongoc_client_ptr", "mongoc_client_t *"),
    typedef("mongoc_collection_ptr", "mongoc_collection_t *"),
    typedef("mongoc_cursor_ptr", "mongoc_cursor_t *"),
    typedef("mongoc_database_ptr", "mongoc_database_t *"),
    typedef("mongoc_gridfs_file_ptr", "mongoc_gridfs_file_t *"),
    typedef("mongoc_gridfs_ptr", "mongoc_gridfs_t *"),
    typedef("mongoc_insert_flags_t", None),
    typedef("mongoc_iovec_ptr", "mongoc_iovec_t *"),
    typedef("mongoc_query_flags_t", None),
    typedef("const_mongoc_index_opt_t", "const mongoc_index_opt_t *"),
    typedef("mongoc_server_description_ptr", "mongoc_server_description_t *"),
    typedef("mongoc_ss_optype_t", None),
    typedef("mongoc_topology_ptr", "mongoc_topology_t *"),
    typedef("mongoc_write_concern_ptr", "mongoc_write_concern_t *"),

    # Const libmongoc.
    typedef("const_mongoc_find_and_modify_opts_ptr", "const mongoc_find_and_modify_opts_t *"),
    typedef("const_mongoc_iovec_ptr", "const mongoc_iovec_t *"),
    typedef("const_mongoc_read_prefs_ptr", "const mongoc_read_prefs_t *"),
    typedef("const_mongoc_write_concern_ptr", "const mongoc_write_concern_t *"),
]

type_list = [T.name for T in typedef_list]
type_list_with_void = type_list + ['void']

param = namedtuple("param", ["type_name", "name"])
future_function = namedtuple("future_function", ["ret_type", "name", "params"])

# Add additional functions to be tested here. For a name like "cursor_next", we
# generate two functions: future_cursor_next to prepare the future_t and launch
# a background thread, and background_cursor_next to run on the thread and
# resolve the future.
future_functions = [
    future_function("uint32_t",
                    "mongoc_bulk_operation_execute",
                    [param("mongoc_bulk_operation_ptr", "bulk"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_client_command_simple",
                    [param("mongoc_client_ptr", "client"),
                     param("const_char_ptr", "db_name"),
                     param("const_bson_ptr", "command"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_client_read_command_with_opts",
                    [param("mongoc_client_ptr", "client"),
                     param("const_char_ptr", "db_name"),
                     param("const_bson_ptr", "command"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("const_bson_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_client_write_command_with_opts",
                    [param("mongoc_client_ptr", "client"),
                     param("const_char_ptr", "db_name"),
                     param("const_bson_ptr", "command"),
                     param("const_bson_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_client_read_write_command_with_opts",
                    [param("mongoc_client_ptr", "client"),
                     param("const_char_ptr", "db_name"),
                     param("const_bson_ptr", "command"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("const_bson_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("void",
                    "mongoc_client_kill_cursor",
                    [param("mongoc_client_ptr", "client"),
                     param("int64_t", "cursor_id")]),

    future_function("mongoc_cursor_ptr",
                    "mongoc_collection_aggregate",
                    [param("mongoc_collection_ptr", "collection"),
                     param("mongoc_query_flags_t", "flags"),
                     param("const_bson_ptr", "pipeline"),
                     param("const_bson_ptr", "options"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs")]),

    future_function("int64_t",
                    "mongoc_collection_count",
                    [param("mongoc_collection_ptr", "collection"),
                     param("mongoc_query_flags_t", "flags"),
                     param("const_bson_ptr", "query"),
                     param("int64_t", "skip"),
                     param("int64_t", "limit"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("bson_error_ptr", "error")]),

    future_function("int64_t",
                    "mongoc_collection_count_with_opts",
                    [param("mongoc_collection_ptr", "collection"),
                     param("mongoc_query_flags_t", "flags"),
                     param("const_bson_ptr", "query"),
                     param("int64_t", "skip"),
                     param("int64_t", "limit"),
                     param("const_bson_ptr", "opts"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_create_index_with_opts",
                    [param("mongoc_collection_ptr", "collection"),
                     param("const_bson_ptr", "keys"),
                     param("const_mongoc_index_opt_t", "opt"),
                     param("bson_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_find_and_modify_with_opts",
                    [param("mongoc_collection_ptr", "collection"),
                     param("const_bson_ptr", "query"),
                     param("const_mongoc_find_and_modify_opts_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_find_and_modify",
                    [param("mongoc_collection_ptr", "collection"),
                     param("const_bson_ptr", "query"),
                     param("const_bson_ptr", "sort"),
                     param("const_bson_ptr", "update"),
                     param("const_bson_ptr", "fields"),
                     param("bool", "_remove"),
                     param("bool", "upsert"),
                     param("bool", "_new"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("mongoc_cursor_ptr",
                    "mongoc_collection_find_indexes",
                    [param("mongoc_collection_ptr", "collection"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_stats",
                    [param("mongoc_collection_ptr", "collection"),
                     param("const_bson_ptr", "options"),
                     param("bson_ptr", "stats"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_insert",
                    [param("mongoc_collection_ptr", "collection"),
                     param("mongoc_insert_flags_t", "flags"),
                     param("const_bson_ptr", "document"),
                     param("const_mongoc_write_concern_ptr", "write_concern"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_read_write_command_with_opts",
                    [param("mongoc_collection_ptr", "collection"),
                     param("const_bson_ptr", "command"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("const_bson_ptr", "opts"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_collection_insert_bulk",
                    [param("mongoc_collection_ptr", "collection"),
                     param("mongoc_insert_flags_t", "flags"),
                     param("const_bson_ptr_ptr", "documents"),
                     param("uint32_t", "n_documents"),
                     param("const_mongoc_write_concern_ptr", "write_concern"),
                     param("bson_error_ptr", "error")]),

    future_function("void",
                    "mongoc_cursor_destroy",
                    [param("mongoc_cursor_ptr", "cursor")]),

    future_function("bool",
                    "mongoc_cursor_next",
                    [param("mongoc_cursor_ptr", "cursor"),
                     param("const_bson_ptr_ptr", "doc")]),

    future_function("char_ptr_ptr",
                    "mongoc_client_get_database_names",
                    [param("mongoc_client_ptr", "client"),
                     param("bson_error_ptr", "error")]),

    future_function("mongoc_server_description_ptr",
                    "mongoc_client_select_server",
                    [param("mongoc_client_ptr", "client"),
                     param("bool", "for_writes"),
                     param("const_mongoc_read_prefs_ptr", "prefs"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_database_command_simple",
                    [param("mongoc_database_ptr", "database"),
                     param("bson_ptr", "command"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("bson_ptr", "reply"),
                     param("bson_error_ptr", "error")]),

    future_function("char_ptr_ptr",
                    "mongoc_database_get_collection_names",
                    [param("mongoc_database_ptr", "database"),
                     param("bson_error_ptr", "error")]),

    future_function("ssize_t",
                    "mongoc_gridfs_file_readv",
                    [param("mongoc_gridfs_file_ptr", "file"),
                     param("mongoc_iovec_ptr", "iov"),
                     param("size_t", "iovcnt"),
                     param("size_t", "min_bytes"),
                     param("uint32_t", "timeout_msec")]),

    future_function("mongoc_gridfs_file_ptr",
                    "mongoc_gridfs_find_one",
                    [param("mongoc_gridfs_ptr", "gridfs"),
                     param("const_bson_ptr", "query"),
                     param("bson_error_ptr", "error")]),

    future_function("bool",
                    "mongoc_gridfs_file_remove",
                    [param("mongoc_gridfs_file_ptr", "file"),
                     param("bson_error_ptr", "error")]),

    future_function("int",
                    "mongoc_gridfs_file_seek",
                    [param("mongoc_gridfs_file_ptr", "file"),
                     param("int64_t", "delta"),
                     param("int", "whence")]),

    future_function("ssize_t",
                    "mongoc_gridfs_file_writev",
                    [param("mongoc_gridfs_file_ptr", "file"),
                     param("const_mongoc_iovec_ptr", "iov"),
                     param("size_t", "iovcnt"),
                     param("uint32_t", "timeout_msec")]),

    future_function("mongoc_gridfs_file_ptr",
                    "mongoc_gridfs_find_one_with_opts",
                    [param("mongoc_gridfs_ptr", "gridfs"),
                     param("const_bson_ptr", "filter"),
                     param("const_bson_ptr", "opts"),
                     param("bson_error_ptr", "error")]),

    future_function("mongoc_server_description_ptr",
                    "mongoc_topology_select",
                    [param("mongoc_topology_ptr", "topology"),
                     param("mongoc_ss_optype_t", "optype"),
                     param("const_mongoc_read_prefs_ptr", "read_prefs"),
                     param("bson_error_ptr", "error")]),

    future_function("mongoc_gridfs_ptr",
                    "mongoc_client_get_gridfs",
                    [param("mongoc_client_ptr", "client"),
                     param("const_char_ptr", "db"),
                     param("const_char_ptr", "prefix"),
                     param("bson_error_ptr", "error")]),
]


for fn in future_functions:
    if fn.ret_type not in type_list_with_void:
        raise Exception('bad type "%s"\n\nin %s' % (fn.ret_type, fn))
    for p in fn.params:
        if p.type_name not in type_list:
            raise Exception('bad type "%s"\n\nin %s' % (p.type_name, fn))


header_comment = """/**************************************************
 *
 * Generated by build/%s.
 *
 * DO NOT EDIT THIS FILE.
 *
 *************************************************/
/* clang-format off */""" % basename(__file__)


def future_function_name(fn):
    if fn.name.startswith('mongoc'):
        # E.g. future_cursor_next().
        return 'future' + fn.name[len('mongoc'):]
    else:
        # E.g. future__mongoc_client_kill_cursor().
        return 'future_' + fn.name


env = Environment(loader=FileSystemLoader(template_dir))
env.filters['future_function_name'] = future_function_name


files = ["future.h",
         "future.c",
         "future-value.h",
         "future-value.c",
         "future-functions.h",
         "future-functions.c"]


for file_name in files:
    print(file_name)
    with open(joinpath(mock_server_dir, file_name), 'w+') as f:
        t = env.get_template(file_name + ".template")
        f.write(t.render(globals()))
