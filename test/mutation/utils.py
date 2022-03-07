import os
import re
import pathlib
import random


BASE_PATH = str(pathlib.Path().resolve())


def replace_value(code, regex_to_search, regex_to_mutate, to_be_mutated):
    search_regex = re.search(regex_to_search, to_be_mutated)
    if code == "RANDOM_INT":
        regex_to_mutate = regex_to_mutate.replace("RANDOM_INT", str(random.randint(0, 3000)))
    elif code == "LESS_THAN":
        search_regex = re.search(regex_to_search, to_be_mutated)
        if int(search_regex.group(2)) == 0:
            print(search_regex.group(1))
            return False
        new_value = str(int(int(search_regex.group(2)) * 0.7))
        regex_to_mutate = regex_to_mutate.replace("LESS_THAN", new_value)
    else:
        search_regex = re.search(regex_to_search, to_be_mutated)
        new_value = str(int(int(search_regex.group(2)) * 1.3))
        regex_to_mutate = regex_to_mutate.replace("GREATER_THAN", new_value)

    return regex_to_mutate


def mkdir_mutation_folder():
    path = os.path.join(BASE_PATH, 'test/mutation/mutators')
    if not os.path.isdir(f'{BASE_PATH}/test/mutation/mutators'):
        os.mkdir(path)
