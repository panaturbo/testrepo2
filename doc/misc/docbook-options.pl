#!/usr/bin/perl
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use warnings;
use strict;
use Time::Piece;

if (@ARGV < 1) {
	print STDERR <<'END';
usage:
    perl docbook-options.pl options_file [YYYY/MM/DD] >named.conf.docbook
END
	exit 1;
}

my $FILE = shift;

my $DATE;
if (@ARGV >= 2) {
	$DATE = shift
} else {
	$DATE = `git log --max-count=1 --date=short --format='%cd' $FILE` or die "unable to determine last modification date of '$FILE'; specify on command line\nexiting";
}
chomp $DATE;

open (FH, "<", $FILE) or die "Can't open $FILE";

my $t = Time::Piece->new();
my $year = $t->year;

print <<END;
<!--
 - Copyright (C) 2004-$year Internet Systems Consortium, Inc. ("ISC")
 -
 - This Source Code Form is subject to the terms of the Mozilla Public
 - License, v. 2.0. If a copy of the MPL was not distributed with this
 - file, You can obtain one at http://mozilla.org/MPL/2.0/.
-->

<!-- Generated by doc/misc/docbook-options.pl -->

<refentry xmlns:db="http://docbook.org/ns/docbook" version="5.0" xml:id="man.named.conf">
  <info>
    <date>$DATE</date>
  </info>
  <refentryinfo>
    <corpname>ISC</corpname>
    <corpauthor>Internet Systems Consortium, Inc.</corpauthor>
  </refentryinfo>

  <refmeta>
    <refentrytitle><filename>named.conf</filename></refentrytitle>
    <manvolnum>5</manvolnum>
    <refmiscinfo>BIND9</refmiscinfo>
  </refmeta>

  <refnamediv>
    <refname><filename>named.conf</filename></refname>
    <refpurpose>configuration file for <command>named</command></refpurpose>
  </refnamediv>

  <docinfo>
    <copyright>
END

for (my $y = 2004; $y <= $year; $y++) {
    print "      <year>$y</year>\n";
}

print <<END;
      <holder>Internet Systems Consortium, Inc. ("ISC")</holder>
    </copyright>
  </docinfo>

  <refsynopsisdiv>
    <cmdsynopsis sepchar=" ">
      <command>named.conf</command>
    </cmdsynopsis>
  </refsynopsisdiv>

  <refsection><info><title>DESCRIPTION</title></info>

    <para><filename>named.conf</filename> is the configuration file
      for
      <command>named</command>.  Statements are enclosed
      in braces and terminated with a semi-colon.  Clauses in
      the statements are also semi-colon terminated.  The usual
      comment styles are supported:
    </para>
    <para>
      C style: /* */
    </para>
    <para>
      C++ style: // to end of line
    </para>
    <para>
      Unix style: # to end of line
    </para>
  </refsection>

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
	s{<([a-z0-9_-]+)>}{<replaceable>$1</replaceable>}g;
	s{ // deprecated,*}{// deprecated};
	s{[[]}{[}g;
	s{[]]}{]}g;
	s{        }{\t}g;
	if (m{^([a-z0-9-]+) }) {
		my $HEADING = uc $1;
		print <<END;
  <refsection><info><title>$HEADING</title></info>
END

                if ($1 eq "trusted-keys") {
                        print <<END;
  <para>Deprecated - see DNSSEC-KEYS.</para>
END
                }

                if ($1 eq "managed-keys") {
                        print <<END;
  <para>See DNSSEC-KEYS.</para>
END
                }

		print <<END;
    <literallayout class="normal">
END
        }

	if (m{^\s*$} && !$blank) {
		$blank = 1;
		print <<END;
</literallayout>
  </refsection>
END
	} else {
		$blank = 0;
	}
	print;
}

print <<END;
  <refsection><info><title>FILES</title></info>

    <para><filename>/etc/named.conf</filename>
    </para>
  </refsection>

  <refsection><info><title>SEE ALSO</title></info>

    <para><citerefentry>
	<refentrytitle>ddns-confgen</refentrytitle><manvolnum>8</manvolnum>
      </citerefentry>,
      <citerefentry>
	<refentrytitle>named</refentrytitle><manvolnum>8</manvolnum>
      </citerefentry>,
      <citerefentry>
	<refentrytitle>named-checkconf</refentrytitle><manvolnum>8</manvolnum>
      </citerefentry>,
      <citerefentry>
	<refentrytitle>rndc</refentrytitle><manvolnum>8</manvolnum>
      </citerefentry>,
      <citerefentry>
	<refentrytitle>rndc-confgen</refentrytitle><manvolnum>8</manvolnum>
      </citerefentry>,
      <citetitle>BIND 9 Administrator Reference Manual</citetitle>.
    </para>
  </refsection>

</refentry>
END
