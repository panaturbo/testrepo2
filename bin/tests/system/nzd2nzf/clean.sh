#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

rm -f ./*/named.conf
rm -f ./*/named.run
rm -f ./*/named.memstats
rm -f dig.out.*
rm -f rndc.out*
rm -f ns*/*.nzf
rm -f ns*/*.nzd ns*/*.nzd-lock
rm -f ns*/managed-keys.bind*
