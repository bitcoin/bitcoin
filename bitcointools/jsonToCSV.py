#!/usr/bin/env python
#
# Reads an array of JSON objects and writes out CSV-format,
# with key names in first row.
# Columns will be union of all keys in the objects.
#

import csv
import json
import sys

json_string = sys.stdin.read()
json_array = json.loads(json_string)

columns = set()
for item in json_array:
  columns.update(set(item))

writer = csv.writer(sys.stdout)
writer.writerow(list(columns))
for item in json_array:
  row = []
  for c in columns:
    if c in item: row.append(str(item[c]))
    else: row.append('')
  writer.writerow(row)
  
