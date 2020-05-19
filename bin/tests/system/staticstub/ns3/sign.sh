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

. $SYSTEMTESTTOP/conf.sh

zone=example.
infile=example.db.in
zonefile=example.db

(cd ../ns4 && $SHELL -e sign.sh )

cp ../ns4/dsset-sub.example$TP .

keyname1=`$KEYGEN -q -a RSASHA256 -b 1024 -n zone $zone`
keyname2=`$KEYGEN -q -a RSASHA256 -b 2048 -f KSK -n zone $zone`
cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -g -o $zone $zonefile > /dev/null

# Configure the resolving server with a trusted key.
keyfile_to_static_ds $keyname2 > trusted.conf

zone=undelegated
infile=undelegated.db.in
zonefile=undelegated.db
keyname1=`$KEYGEN -q -a RSASHA256 -b 1024 -n zone $zone`
keyname2=`$KEYGEN -q -a RSASHA256 -b 2048 -f KSK -n zone $zone`
cat $infile $keyname1.key $keyname2.key > $zonefile

$SIGNER -g -o $zone $zonefile > /dev/null

keyfile_to_static_ds $keyname2 >> trusted.conf
cp trusted.conf ../ns2/trusted.conf
