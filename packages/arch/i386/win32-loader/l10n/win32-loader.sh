#!/bin/bash -e
#
# l10n support for win32-loader
# Copyright (C) 2007  Robert Millan <rmh@aybabtu.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# make sure gettext output is in UTF-8
unset LC_CTYPE
while read a b ; do
  if [ "$b" = "UTF-8" ] ; then
    export LC_CTYPE=$a
    break
  fi
done < /etc/locale.gen
if ! [ "$LC_CTYPE" ] ; then
  echo "Run \"sudo dpkg-reconfigure locales\" and add at least one UTF-8 locale there."
  exit 1
fi

. /usr/bin/gettext.sh
export TEXTDOMAIN=win32-loader
export TEXTDOMAINDIR=${PWD}/locale

# translate:
# This must be the string used by Windows to represent your
# language's charset.  If you don't know, check [wine]/tools/wmc/lang.c,
# or http://www.microsoft.com/globaldev/reference/WinCP.mspx
#
# IMPORTANT: In the rest of this file, only the subset of UTF-8 that can be
# converted to this charset should be used.
charset=`gettext windows-1252`

export LANGUAGE
./win32-loader | iconv -f utf-8 -t ${charset}
