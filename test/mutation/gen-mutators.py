#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import pathlib
import re
import consts
from utils import mkdir_mutation_folder, replace_value


FILES = consts.FILES
REGEX_OPERATORS = consts.REGEX_OPERATORS
REPLACE_OPERATORS = consts.REPLACE_OPERATORS
REPLACEMENT_IN_REGEX = consts.REPLACEMENT_IN_REGEX

BASE_PATH = str(pathlib.Path().resolve())
BASE_MUT = f'{BASE_PATH}/test/mutation/mutators'


def mutate(file_to_mutate):
    input_file = f'{BASE_PATH}/{file_to_mutate}'

    file_name = file_to_mutate.split('/')
    file_name = file_name[len(file_name) - 1].split('.')[0]

    with open(input_file, 'r', encoding="utf8") as source_code:
        source_code = source_code.readlines()
        num_lines = len(source_code)

    OPERATORS = REGEX_OPERATORS + REPLACE_OPERATORS
    i = 0

    for operator in OPERATORS:
        lines_num_list = list(range(1, num_lines - 1))
        for line_num in lines_num_list:
            lines = source_code.copy()
            line_before_mutation = lines[line_num]

            if line_before_mutation.lstrip().startswith(("//", "*", "assert", "/*")):
                continue

            if operator in REGEX_OPERATORS:
                if re.search(operator[0], line_before_mutation):
                    print(f"Before: {line_before_mutation}")
                    operator_sub = operator[1]
                    res = [op for op in REPLACEMENT_IN_REGEX if(op in operator[1])]
                    if bool(res):
                        operator_sub = replace_value(res[0], operator[0], operator[1], line_before_mutation)
                        if not operator_sub:
                            continue
                    line_mutated = re.sub(operator[0], operator_sub, line_before_mutation)
                    lines[line_num] = f'{line_before_mutation[:-len(line_before_mutation.lstrip())]}{line_mutated}'
                else:
                    continue
            elif operator in REPLACE_OPERATORS:
                if operator[0] in line_before_mutation:
                    print(f"Before: {line_before_mutation}")
                    line_mutated = line_before_mutation.replace(operator[0], operator[1])
                    lines[line_num] = f'{line_before_mutation[:-len(line_before_mutation.lstrip())]}{line_mutated}'
                else:
                    continue

            mutator_file = f'{BASE_MUT}/{file_name}.mutant.{i}.cpp'
            with open(mutator_file, 'w', encoding="utf8") as file:
                print(f"After: {line_mutated}")
                file.writelines(lines)
                i = i + 1


if __name__ == "__main__":
    mkdir_mutation_folder()

    for file in FILES:
        mutate(file)
