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

.. _supported_os:

Supported Platforms
-------------------

Current support status of various platforms and BIND 9 versions can be
found in the ISC Knowledgebase:

https://kb.isc.org/docs/supported-platforms

In general, this version of BIND will build and run on any
POSIX-compliant system with a C11-compliant C compiler, BSD-style
sockets with RFC-compliant IPv6 support, POSIX-compliant threads, and
the :ref:`required libraries <build_dependencies>`.

The following C11 features are used in BIND 9:

-  Atomic operations support, either in the form of C11 atomics or
   ``__atomic`` builtin operations.

-  Thread Local Storage support, either in the form of C11
   ``_Thread_local``/``thread_local``, or the ``__thread`` GCC
   extension.

The C11 variants are preferred.

ISC regularly tests BIND on many operating systems and architectures,
but lacks the resources to test all of them. Consequently, ISC is only
able to offer support on a “best effort” basis for some.

Regularly tested platforms
~~~~~~~~~~~~~~~~~~~~~~~~~~

As of Jan 2022, BIND 9.18 is fully supported and regularly tested on the
following systems:

-  Debian 9, 10, 11
-  Ubuntu LTS 18.04, 20.04
-  Fedora 35
-  Red Hat Enterprise Linux / CentOS / Oracle Linux 7, 8
-  FreeBSD 12.3, 13.0
-  OpenBSD 7.0
-  Alpine Linux 3.15

The amd64, i386, armhf and arm64 CPU architectures are all fully
supported.

Best effort
~~~~~~~~~~~

The following are platforms on which BIND is known to build and run. ISC
makes every effort to fix bugs on these platforms, but may be unable to
do so quickly due to lack of hardware, less familiarity on the part of
engineering staff, and other constraints. None of these are tested
regularly by ISC.

-  macOS 10.12+
-  Solaris 11
-  NetBSD
-  Other Linux distributions still supported by their vendors, such as:

   -  Ubuntu 20.10+
   -  Gentoo
   -  Arch Linux

-  OpenWRT/LEDE 17.01+
-  Other CPU architectures (mips, mipsel, sparc, …)

Community maintained
~~~~~~~~~~~~~~~~~~~~

These systems may not all have the required dependencies for building
BIND easily available, although it will be possible in many cases to
compile those directly from source. The community and interested parties
may wish to help with maintenance, and we welcome patch contributions,
although we cannot guarantee that we will accept them. All contributions
will be assessed against the risk of adverse effect on officially
supported platforms.

-  Platforms past or close to their respective EOL dates, such as:

   -  Ubuntu 14.04, 16.04 (Ubuntu ESM releases are not supported)
   -  CentOS 6
   -  Debian Jessie
   -  FreeBSD 10.x, 11.x

Unsupported Platforms
---------------------

These are platforms on which BIND 9.18 is known *not* to build or run:

-  Platforms without at least OpenSSL 1.0.2
-  Windows
-  Solaris 10 and older
-  Platforms that don’t support IPv6 Advanced Socket API (RFC 3542)
-  Platforms that don’t support atomic operations (via compiler or
   library)
-  Linux without NPTL (Native POSIX Thread Library)
-  Platforms on which ``libuv`` cannot be compiled
