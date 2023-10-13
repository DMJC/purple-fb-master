#!/usr/bin/env python3
#
# Purple - Internet Messaging Library
# Copyright (C) Pidgin Developers <devel@pidgin.im>
#
# Purple is the legal property of its developers, whose names are too numerous
# to list here.  Please refer to the COPYRIGHT file distributed with this
# source distribution.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <https://www.gnu.org/licenses/>.

"""
Produce meson-config.h in a build directory.

This should not really be run manually. It is used by Meson as a
post-configuration script to create meson-config.h which for now simply
contains information about the configuration used to create the build
directory.
"""

import html
import json
import os
import shlex
import subprocess
import sys


try:
    introspect = os.environ['MESONINTROSPECT']
except KeyError:
    print('Meson is too old; '
          'it does not set MESONINTROSPECT for postconf scripts.')
    sys.exit(1)
else:
    introspect = shlex.split(introspect)

# This is the top-most Meson build root.
try:
    build_root = os.environ['MESON_BUILD_ROOT']
except KeyError:
    print('Meson is too old; '
          'it does not set MESON_BUILD_ROOT for postconf scripts.')
    sys.exit(1)

# This is the build root for the Pidgin project itself, which may be different
# if it's used as a subproject.
project_build_root = sys.argv[1]


def tostr(obj):
    if isinstance(obj, str):
        return html.escape(repr(obj))
    else:
        return repr(obj)


def normalize_pidgin_option(option):
    """Promote Pidgin-as-subproject options to global options."""
    name = option['name']
    # Option names for subprojects are determined by the wrap file name + the
    # subproject option. Unfortunately, the wrap file name is determined by the
    # superproject, so we can't know what it is. Just assume it is something
    # semi-standard.
    if ':' in name and name.startswith('pidgin'):
        option['name'] = name.split(':')[1]
    return option


conf = subprocess.check_output(introspect + ['--buildoptions', build_root],
                               universal_newlines=True)
conf = json.loads(conf)

# Skip all subproject options, or else the config string explodes.
conf = [normalize_pidgin_option(option) for option in conf]
conf = [option for option in conf if ':' not in option['name']]

settings = ' '.join('{}={}'.format(option['name'], tostr(option['value']))
                    for option in sorted(conf, key=lambda x: x['name']))

with open(os.path.join(project_build_root, 'meson-config.h'), 'w') as f:
    f.write('#define MESON_ARGS "{}"'.format(settings))
