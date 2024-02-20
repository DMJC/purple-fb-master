#!/usr/bin/env python3

# Purple - Internet Messaging Library
# Copyright (C) Pidgin Developers <devel@pidgin.im>
#
# Purple is the legal property of its developers, whose names are too numerous
# to list here. Please refer to the COPYRIGHT file distributed with this
# source distribution.
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this library; if not, see <https://www.gnu.org/licenses/>.

import difflib
import os
import sys

HEADER = """\
/*
 * Purple - Internet Messaging Library
 * Copyright (C) Pidgin Developers <devel@pidgin.im>
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here. Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this library; if not, see <https://www.gnu.org/licenses/>.
 */

""".splitlines(keepends=True)

header_lines = len(HEADER)

if len(sys.argv) < 4:
    print('usage: directory template-file file...')
    sys.exit(1)

path = sys.argv[1]

header = ""
with open(os.path.join(path, sys.argv[2]), 'r') as f:
    header = f.read().splitlines(keepends=True)

error = False

debug = False

for filename in sys.argv[3:]:
    file_data = None
    with open(os.path.join(path, filename), 'r') as f:
        file_data = f.read().splitlines(keepends=True)

    # Look for a starting commenting ignoring newlines, if we find code, abort.
    comment_start = False
    comment_end = 0
    for i, line in enumerate(file_data):
        if line.startswith('/*'):
            comment_start = True
        elif comment_start and line.endswith('*/\n'):
            comment_end = i
            continue

        if line.strip() == '':
            continue

        if comment_start and comment_end != 0:
            header_end = i
            break

    if comment_start:
        new_data = HEADER + file_data[header_end:]
    else:
        new_data = HEADER + file_data

    if debug:
        sys.stdout.writelines(new_data[:30])
        print(f'comment_start: {comment_start}')
        print(f'comment_end: {comment_end}')
        print(f'header_end: {header_end}')
        sys.stdout.writelines(file_data[:25])

    diff = [*difflib.unified_diff(file_data, new_data,
                                  fromfile=f'a/{filename}',
                                  tofile=f'b/{filename}')]
    if diff:
        sys.stdout.writelines(diff)
        error = True

    if debug:
        sys.exit(1)

if error:
    sys.exit(1)
