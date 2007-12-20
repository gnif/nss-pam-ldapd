#!/bin/sh

# test_myldap.sh - simple wrapper test script for test_myldap
#
# Copyright (C) 2007 Arthur de Jong
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301 USA

# This script expects to be run in an environment where an LDAP server
# is available at the location specified in nss-ldapd-test.conf in
# this directory.

set -e

# get LDAP config
cfgfile="$srcdir/nss-ldapd-test.conf"
uri=`sed -n 's/^uri *//p' "$cfgfile"`
base="dc=test,dc=tld"

# try to fetch the base DN (fail with exit 77 to indicate problem)
ldapsearch -b "$base" -s base -x -H "$uri" > /dev/null 2>&1 || {
  echo "LDAP server $uri not available for $base"
  exit 77
}
echo "using LDAP server $uri"

# just execute test_myldap
exec ./test_myldap "$cfgfile"