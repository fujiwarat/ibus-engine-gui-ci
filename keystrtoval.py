# vim:set et sts=4 sw=4:
# -*- coding: utf-8 -*-
#
# ibuskeystrtoval
#
# Copyright (c) 2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (c) 2021 Red Hat, Inc.
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
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

from gi import require_version as gi_require_version
gi_require_version('IBus', '1.0')

from gi.repository import IBus

'''
    const struct {
        const gchar *prefix;
        const gint len;
    } prefix [] = {
        { "keycode ", sizeof ("keycode ") - 1 },
'''

def main():
    keyvals = list()
    states = list()
    for key in dir(IBus):
        if key.startswith('KEY_'):
            keyvals.append(key)
    for key in dir(IBus.ModifierType):
        if key.endswith('_MASK'):
            states.append(key)
    print('#ifndef __IBUS_ENGINE_GUI_CI_KEYVAL_STR_TO_VAL_H_')
    print('#define __IBUS_ENGINE_GUI_CI_KEYVAL_STR_TO_VAL_H_')
    print('')
    print('#include <ibus.h>')
    print('')

    print('''
const struct {
    const char *key;
    const int val;
} IBusKeyvalStroToVal [] = {''')
    for key in keyvals:
        print("    { \"IBUS_%s\", IBUS_%s }," % (key, key))
    print("    { NULL, 0 }")
    print('};');

    print('''
const struct {
    const char *key;
    const guint32 val;
} IBusStateStrToVal [] = {''')
    for key in states:
        print("    { \"IBUS_%s\", IBUS_%s }," % (key, key))
    print("    { NULL, 0 }")
    print('};');
    print('')
    print('#endif')

if __name__ == '__main__':
    main()

