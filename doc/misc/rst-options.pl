#!/usr/bin/perl

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use warnings;
use strict;

if (@ARGV < 1) {
	print STDERR <<'END';
usage:
    perl rst-options.pl options_file >named.conf.rst
END
	exit 1;
}

my $FILE = shift;

open (FH, "<", $FILE) or die "Can't open $FILE";

print <<END;
.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

END

print <<END;
.. highlight: console

.. iscman:: named.conf

named.conf - configuration file for **named**
---------------------------------------------

Synopsis
~~~~~~~~

:program:`named.conf`

Description
~~~~~~~~~~~

:file:`named.conf` is the configuration file for :iscman:`named`.
Statements are enclosed in braces and terminated with a semi-colon.
Clauses in the statements are also semi-colon terminated. The usual
comment styles are supported:

C style: /\\* \\*/

 C++ style: // to end of line

Unix style: # to end of line

END

# skip preamble
my $preamble = 0;
while (<FH>) {
	if (m{^\s*$}) {
		last if $preamble > 0;
	} else {
		$preamble++;
	}
}

my $UNDERLINE;

my $blank = 0;
while (<FH>) {
	if (m{// not.*implemented} || m{// obsolete} ||
	    m{// ancient} || m{// test.*only})
	{
		next;
	}

	s{ // not configured}{};
	s{ // non-operational}{};
	s{ (// )*may occur multiple times}{};
	s{<([a-z0-9_-]+)>}{$1}g;
	s{ // deprecated,*}{// deprecated};
	s{[[]}{[}g;
	s{[]]}{]}g;
	s{        }{\t}g;
	if (m{^([a-z0-9-]+) }) {
		my $HEADING = uc $1;
		$UNDERLINE = $HEADING;
		$UNDERLINE =~ s/./^/g;
		print $HEADING . "\n";
		print $UNDERLINE . "\n\n";
		if ($HEADING eq "TRUSTED-KEYS") {
		    print "Deprecated - see DNSSEC-KEYS.\n\n";
		}
		if ($HEADING eq "MANAGED-KEYS") {
		    print "See DNSSEC-KEYS.\n\n" ;
		}
		print "::\n\n";
	}

	if (m{^\s*$}) {
		if (!$blank) {
			print "\n";
			$blank = 1;
		}
		next;
	} else {
		$blank = 0;
	}
	print "  " . $_;

}

print "ZONE\n";
$UNDERLINE = "ZONE";
$UNDERLINE =~ s/./^/g;
print $UNDERLINE . "\n\n";
print "Any of these zone statements can also be set inside the view statement.\n\n";

print <<END;
.. include:: ../../doc/misc/primary.zoneopt.rst
.. include:: ../../doc/misc/secondary.zoneopt.rst
.. include:: ../../doc/misc/mirror.zoneopt.rst
.. include:: ../../doc/misc/forward.zoneopt.rst
.. include:: ../../doc/misc/hint.zoneopt.rst
.. include:: ../../doc/misc/redirect.zoneopt.rst
.. include:: ../../doc/misc/static-stub.zoneopt.rst
.. include:: ../../doc/misc/stub.zoneopt.rst
.. include:: ../../doc/misc/delegation-only.zoneopt.rst
.. include:: ../../doc/misc/in-view.zoneopt.rst

Files
~~~~~

|named_conf|

See Also
~~~~~~~~

:iscman:`named(8) <named>`, :iscman:`named-checkconf(8) <named-checkconf>`, :iscman:`rndc(8) <rndc>`, :iscman:`rndc-confgen(8) <rndc-confgen>`, :iscman:`tsig-keygen(8) <tsig-keygen>`, BIND 9 Administrator Reference Manual.

END
