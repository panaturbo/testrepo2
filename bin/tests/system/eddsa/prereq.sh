#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -e

. ../conf.sh

supported=0
if $SHELL ../testcrypto.sh ed25519; then
	supported=1
fi
if $SHELL ../testcrypto.sh ed448; then
	supported=1
fi

[ "$supported" -eq 1 ] || exit 1
