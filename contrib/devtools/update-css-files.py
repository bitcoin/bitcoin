#!/usr/bin/env python3
#
# update-css-files.py creates color analyse files in css/colors and updates the
# `<colors></colors>` section in all css files.
#
# Copyright (c) 2020 The Dash Core developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from pathlib import Path
import re
import subprocess
import sys

MATCH_REPLACE = '<colors>.+?</colors>'
MATCH_COLORS = '#(?:[0-9a-fA-F]{2}){2,4}|#(?:[0-9a-f]{1}){3}'

def error(msg):
    exit('\nERROR: {}\n'.format(msg))

def parse_css(file_css):
    # Temporarily
    state = 0
    selectors = []

    # Results
    by_attribute = {}
    by_color = {}

    for line in file_css.read_text().splitlines():

        if line == '':
            continue

        # start of a comment
        if state == 0 and line.startswith('/*'):
            if '*/' in line:
                state = 0
            else:
                state = 1
        # we are in a comment section
        elif state == 1:
            # end of the comment
            if '*/' in line:
                state = 0
            else:
                continue
        # first line of multiple selector
        elif (state == 0 or state == 2) and ',' in line:
            state = 2
        # first line of single selector or end of multiple
        elif (state == 0 or state == 2) and '{' in line:
            state = 3
        # end of element
        elif state == 4 and line == '}':
            state = 0

        if state == 0 and len(selectors):
            selectors = []

        if state == 2:
            selector = line.split(",")[0].strip(' ')
            selectors.append(selector)

        if state == 3:
            selector = line.split("{")[0].strip(' ')
            selectors.append(selector)
            state = 4
            continue

        if state == 4:
            matched_colors = re.findall(MATCH_COLORS, line)

            if len(matched_colors) > 1:
                error("Multiple colors in a line.\n\n  {}\n\nSeems to be an invalid file!".format(line))
            elif len(matched_colors) == 1:
                matched_color = matched_colors[0]
                element = line.split(":")[0].strip(' ')

                if not matched_color in by_color:
                    by_color[matched_color] = []

                by_color[matched_color].append(element)

                entry = element + " " + matched_color

                if not entry in by_attribute:
                    by_attribute[entry] = []

                by_attribute[entry].extend(selectors)

    def sort_color(color):
        tmp = color[0].replace('#', '0x')
        return int(tmp, 0)

    def remove_duplicates(l):
        no_duplicates = []
        [no_duplicates.append(i) for i in l if not no_duplicates.count(i)]
        return no_duplicates

    colors = []

    # sort colors just by hex value
    if len(by_color):
        colors = sorted(by_color.items(), key=lambda x: sort_color(x))

    for k, l in by_attribute.items():
        by_attribute[k] = remove_duplicates(l)

    for k, l in by_color.items():
        by_color[k] = remove_duplicates(l)

    return {'fileName': file_css.stem, 'byAttribute': by_attribute, 'byColor': by_color, 'colors': colors}


def create_color_file(content, commit):

    str_result = "Color analyse of " +\
                 content['fileName'] + ".css " + \
                 "by " + \
                 Path(__file__).name + \
                 " for commit " + \
                 commit + \
                 "\n\n"

    if not len(content['colors']):
        return None

    str_result += "# Used colors\n\n"
    for c in content['colors']:
        str_result += c[0] + '\n'

    str_result += "\n# Grouped by attribute\n"

    for k, v in content['byAttribute'].items():
        str_result += '\n' + k + '\n'
        for val in v:
            str_result += '  ' + val + '\n'

    str_result += "\n# Grouped by color\n"

    for k, v in content['byColor'].items():
        str_result += '\n' + k + '\n'
        for val in v:
            str_result += '  ' + val + '\n'

    return str_result

if __name__ == '__main__':

    if len(sys.argv) > 1:
        error('No argument required!')

    try:
        css_folder_path = Path(__file__).parent.absolute() / Path('../../src/qt/res/css/')
        css_folder_path = css_folder_path.resolve(strict=True)
    except Exception:
        error("Path doesn't exist: {}".format(css_folder_path))

    if not len(list(css_folder_path.glob('*.css'))):
        error("No .css files found in {}".format(css_folder_path))

    results = [parse_css(x) for x in css_folder_path.glob('*.css') if x.is_file()]

    colors_folder_path = css_folder_path / Path('colors/')
    if not colors_folder_path.is_dir():
        try:
            colors_folder_path.mkdir()
        except Exception:
            error("Can't create new folder: {}".format(colors_folder_path))

    commit = subprocess.check_output(['git', '-C', css_folder_path, 'rev-parse', '--short', 'HEAD']).decode("utf-8")

    for r in results:

        # Update the css file
        css_file = css_folder_path / Path(r['fileName'] + '.css')
        css_content = css_file.read_text()
        to_replace = re.findall(MATCH_REPLACE, css_content, re.DOTALL)

        str_result = "\n# Used colors in {}.css for commit {}\n".format(r['fileName'], commit)
        for c in r['colors']:
            str_result += c[0] + '\n'

        str_replace = "<colors>\n{}\n</colors>".format(str_result)
        css_content = css_content.replace(to_replace[0], str_replace)
        css_file.write_text(css_content)

        # Write the <css>_color.txt files
        str_result = create_color_file(r, commit)

        if str_result is not None:
            color_file = colors_folder_path / Path(r['fileName'] + '_css_colors.txt')
            color_file.write_text(str_result)

            print('\n{}.css -> {} created!'.format(r['fileName'], color_file))
        else:
            print('\n{}.css -> No colors found..'.format(r['fileName'] + ".css"))
