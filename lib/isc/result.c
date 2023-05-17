/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stddef.h>
#include <stdlib.h>

#include <isc/once.h>
#include <isc/util.h>

static const char *description[ISC_R_NRESULTS] = {
	[ISC_R_SUCCESS] = "success",
	[ISC_R_NOMEMORY] = "out of memory",
	[ISC_R_TIMEDOUT] = "timed out",
	[ISC_R_NOTHREADS] = "no available threads",
	[ISC_R_ADDRNOTAVAIL] = "address not available",
	[ISC_R_ADDRINUSE] = "address in use",
	[ISC_R_NOPERM] = "permission denied",
	[ISC_R_NOCONN] = "no pending connections",
	[ISC_R_NETUNREACH] = "network unreachable",
	[ISC_R_HOSTUNREACH] = "host unreachable",
	[ISC_R_NETDOWN] = "network down",
	[ISC_R_HOSTDOWN] = "host down",
	[ISC_R_CONNREFUSED] = "connection refused",
	[ISC_R_NORESOURCES] = "not enough free resources",
	[ISC_R_EOF] = "end of file",
	[ISC_R_BOUND] = "socket already bound",
	[ISC_R_RELOAD] = "reload",
	[ISC_R_LOCKBUSY] = "lock busy",
	[ISC_R_EXISTS] = "already exists",
	[ISC_R_NOSPACE] = "ran out of space",
	[ISC_R_CANCELED] = "operation canceled",
	[ISC_R_NOTBOUND] = "socket is not bound",
	[ISC_R_SHUTTINGDOWN] = "shutting down",
	[ISC_R_NOTFOUND] = "not found",
	[ISC_R_UNEXPECTEDEND] = "unexpected end of input",
	[ISC_R_FAILURE] = "failure",
	[ISC_R_IOERROR] = "I/O error",
	[ISC_R_NOTIMPLEMENTED] = "not implemented",
	[ISC_R_UNBALANCED] = "unbalanced parentheses",
	[ISC_R_NOMORE] = "no more",
	[ISC_R_INVALIDFILE] = "invalid file",
	[ISC_R_BADBASE64] = "bad base64 encoding",
	[ISC_R_UNEXPECTEDTOKEN] = "unexpected token",
	[ISC_R_QUOTA] = "quota reached",
	[ISC_R_UNEXPECTED] = "unexpected error",
	[ISC_R_ALREADYRUNNING] = "already running",
	[ISC_R_IGNORE] = "ignore",
	[ISC_R_MASKNONCONTIG] = "address mask not contiguous",
	[ISC_R_FILENOTFOUND] = "file not found",
	[ISC_R_FILEEXISTS] = "file already exists",
	[ISC_R_NOTCONNECTED] = "socket is not connected",
	[ISC_R_RANGE] = "out of range",
	[ISC_R_NOENTROPY] = "out of entropy",
	[ISC_R_MULTICAST] = "invalid use of multicast address",
	[ISC_R_NOTFILE] = "not a file",
	[ISC_R_NOTDIRECTORY] = "not a directory",
	[ISC_R_EMPTY] = "queue is empty",
	[ISC_R_FAMILYMISMATCH] = "address family mismatch",
	[ISC_R_FAMILYNOSUPPORT] = "address family not supported",
	[ISC_R_BADHEX] = "bad hex encoding",
	[ISC_R_TOOMANYOPENFILES] = "too many open files",
	[ISC_R_NOTBLOCKING] = "not blocking",
	[ISC_R_UNBALANCEDQUOTES] = "unbalanced quotes",
	[ISC_R_INPROGRESS] = "operation in progress",
	[ISC_R_CONNECTIONRESET] = "connection reset",
	[ISC_R_SOFTQUOTA] = "soft quota reached",
	[ISC_R_BADNUMBER] = "not a valid number",
	[ISC_R_DISABLED] = "disabled",
	[ISC_R_MAXSIZE] = "max size",
	[ISC_R_BADADDRESSFORM] = "invalid address format",
	[ISC_R_BADBASE32] = "bad base32 encoding",
	[ISC_R_UNSET] = "unset",
	[ISC_R_MULTIPLE] = "multiple",
	[ISC_R_WOULDBLOCK] = "would block",
	[ISC_R_COMPLETE] = "complete",
	[ISC_R_CRYPTOFAILURE] = "crypto failure",
	[ISC_R_DISCQUOTA] = "disc quota",
	[ISC_R_DISCFULL] = "disc full",
	[ISC_R_DEFAULT] = "default",
	[ISC_R_IPV4PREFIX] = "IPv4 prefix",
	[ISC_R_TLSERROR] = "TLS error",
	[ISC_R_TLSBADPEERCERT] = "TLS peer certificate verification failed",
	[ISC_R_HTTP2ALPNERROR] = "ALPN for HTTP/2 failed",
	[ISC_R_DOTALPNERROR] = "ALPN for DoT failed",
	[ISC_R_INVALIDPROTO] = "invalid protocol",

	[DNS_R_LABELTOOLONG] = "label too long",
	[DNS_R_BADESCAPE] = "bad escape",
	[DNS_R_EMPTYLABEL] = "empty label",
	[DNS_R_BADDOTTEDQUAD] = "bad dotted quad",
	[DNS_R_INVALIDNS] = "invalid NS owner name (wildcard)",
	[DNS_R_UNKNOWN] = "unknown class/type",
	[DNS_R_BADLABELTYPE] = "bad label type",
	[DNS_R_BADPOINTER] = "bad compression pointer",
	[DNS_R_TOOMANYHOPS] = "too many hops",
	[DNS_R_DISALLOWED] = "disallowed (by application policy)",
	[DNS_R_EXTRATOKEN] = "extra input text",
	[DNS_R_EXTRADATA] = "extra input data",
	[DNS_R_TEXTTOOLONG] = "text too long",
	[DNS_R_NOTZONETOP] = "not at top of zone",
	[DNS_R_SYNTAX] = "syntax error",
	[DNS_R_BADCKSUM] = "bad checksum",
	[DNS_R_BADAAAA] = "bad IPv6 address",
	[DNS_R_NOOWNER] = "no owner",
	[DNS_R_NOTTL] = "no ttl",
	[DNS_R_BADCLASS] = "bad class",
	[DNS_R_NAMETOOLONG] = "name too long",
	[DNS_R_PARTIALMATCH] = "partial match",
	[DNS_R_NEWORIGIN] = "new origin",
	[DNS_R_UNCHANGED] = "unchanged",
	[DNS_R_BADTTL] = "bad ttl",
	[DNS_R_NOREDATA] = "more data needed/to be rendered",
	[DNS_R_CONTINUE] = "continue",
	[DNS_R_DELEGATION] = "delegation",
	[DNS_R_GLUE] = "glue",
	[DNS_R_DNAME] = "dname",
	[DNS_R_CNAME] = "cname",
	[DNS_R_BADDB] = "bad database",
	[DNS_R_ZONECUT] = "zonecut",
	[DNS_R_BADZONE] = "bad zone",
	[DNS_R_MOREDATA] = "more data",
	[DNS_R_UPTODATE] = "up to date",
	[DNS_R_TSIGVERIFYFAILURE] = "tsig verify failure",
	[DNS_R_TSIGERRORSET] = "tsig indicates error",
	[DNS_R_SIGINVALID] = "RRSIG failed to verify",
	[DNS_R_SIGEXPIRED] = "RRSIG has expired",
	[DNS_R_SIGFUTURE] = "RRSIG validity period has not begun",
	[DNS_R_KEYUNAUTHORIZED] = "key is unauthorized to sign data",
	[DNS_R_INVALIDTIME] = "invalid time",
	[DNS_R_EXPECTEDTSIG] = "expected a TSIG or SIG(0)",
	[DNS_R_UNEXPECTEDTSIG] = "did not expect a TSIG or SIG(0)",
	[DNS_R_INVALIDTKEY] = "TKEY is unacceptable",
	[DNS_R_HINT] = "hint",
	[DNS_R_DROP] = "drop",
	[DNS_R_NOTLOADED] = "zone not loaded",
	[DNS_R_NCACHENXDOMAIN] = "ncache nxdomain",
	[DNS_R_NCACHENXRRSET] = "ncache nxrrset",
	[DNS_R_WAIT] = "wait",
	[DNS_R_NOTVERIFIEDYET] = "not verified yet",
	[DNS_R_NOIDENTITY] = "no identity",
	[DNS_R_NOJOURNAL] = "no journal",
	[DNS_R_ALIAS] = "alias",
	[DNS_R_USETCP] = "use TCP",
	[DNS_R_NOVALIDSIG] = "no valid RRSIG",
	[DNS_R_NOVALIDNSEC] = "no valid NSEC",
	[DNS_R_NOTINSECURE] = "insecurity proof failed",
	[DNS_R_UNKNOWNSERVICE] = "unknown service",
	[DNS_R_RECOVERABLE] = "recoverable error occurred",
	[DNS_R_UNKNOWNOPT] = "unknown opt attribute record",
	[DNS_R_UNEXPECTEDID] = "unexpected message id",
	[DNS_R_SEENINCLUDE] = "seen include file",
	[DNS_R_NOTEXACT] = "not exact",
	[DNS_R_BLACKHOLED] = "address blackholed",
	[DNS_R_BADALG] = "bad algorithm",
	[DNS_R_METATYPE] = "invalid use of a meta type",
	[DNS_R_CNAMEANDOTHER] = "CNAME and other data",
	[DNS_R_SINGLETON] = "multiple RRs of singleton type",
	[DNS_R_HINTNXRRSET] = "hint nxrrset",
	[DNS_R_NOMASTERFILE] = "no master file configured",
	[DNS_R_UNKNOWNPROTO] = "unknown protocol",
	[DNS_R_CLOCKSKEW] = "clocks are unsynchronized",
	[DNS_R_BADIXFR] = "IXFR failed",
	[DNS_R_NOTAUTHORITATIVE] = "not authoritative",
	[DNS_R_NOVALIDKEY] = "no valid KEY",
	[DNS_R_OBSOLETE] = "obsolete",
	[DNS_R_FROZEN] = "already frozen",
	[DNS_R_UNKNOWNFLAG] = "unknown flag",
	[DNS_R_EXPECTEDRESPONSE] = "expected a response",
	[DNS_R_NOVALIDDS] = "no valid DS",
	[DNS_R_NSISADDRESS] = "NS is an address",
	[DNS_R_REMOTEFORMERR] = "received FORMERR",
	[DNS_R_TRUNCATEDTCP] = "truncated TCP response",
	[DNS_R_LAME] = "lame server detected",
	[DNS_R_UNEXPECTEDRCODE] = "unexpected RCODE",
	[DNS_R_UNEXPECTEDOPCODE] = "unexpected OPCODE",
	[DNS_R_CHASEDSSERVERS] = "chase DS servers",
	[DNS_R_EMPTYNAME] = "empty name",
	[DNS_R_EMPTYWILD] = "empty wild",
	[DNS_R_BADBITMAP] = "bad bitmap",
	[DNS_R_FROMWILDCARD] = "from wildcard",
	[DNS_R_BADOWNERNAME] = "bad owner name (check-names)",
	[DNS_R_BADNAME] = "bad name (check-names)",
	[DNS_R_DYNAMIC] = "dynamic zone",
	[DNS_R_UNKNOWNCOMMAND] = "unknown command",
	[DNS_R_MUSTBESECURE] = "must-be-secure",
	[DNS_R_COVERINGNSEC] = "covering NSEC record returned",
	[DNS_R_MXISADDRESS] = "MX is an address",
	[DNS_R_DUPLICATE] = "duplicate query",
	[DNS_R_INVALIDNSEC3] = "invalid NSEC3 owner name (wildcard)",
	[DNS_R_NOTPRIMARY] = "not primary",
	[DNS_R_BROKENCHAIN] = "broken trust chain",
	[DNS_R_EXPIRED] = "expired",
	[DNS_R_NOTDYNAMIC] = "not dynamic",
	[DNS_R_BADEUI] = "bad EUI",
	[DNS_R_NTACOVERED] = "covered by negative trust anchor",
	[DNS_R_BADCDS] = "bad CDS",
	[DNS_R_BADCDNSKEY] = "bad CDNSKEY",
	[DNS_R_OPTERR] = "malformed OPT option",
	[DNS_R_BADDNSTAP] = "malformed DNSTAP data",
	[DNS_R_BADTSIG] = "TSIG in wrong location",
	[DNS_R_BADSIG0] = "SIG(0) in wrong location",
	[DNS_R_TOOMANYRECORDS] = "too many records",
	[DNS_R_VERIFYFAILURE] = "verify failure",
	[DNS_R_ATZONETOP] = "at top of zone",
	[DNS_R_NOKEYMATCH] = "no matching key found",
	[DNS_R_TOOMANYKEYS] = "too many keys matching",
	[DNS_R_KEYNOTACTIVE] = "key is not actively signing",
	[DNS_R_NSEC3ITERRANGE] = "NSEC3 iterations out of range",
	[DNS_R_NSEC3SALTRANGE] = "NSEC3 salt length too high",
	[DNS_R_NSEC3BADALG] = "cannot use NSEC3 with key algorithm",
	[DNS_R_NSEC3RESALT] = "NSEC3 resalt",
	[DNS_R_INCONSISTENTRR] = "inconsistent resource record",
	[DNS_R_NOALPN] = "no ALPN",

	[DST_R_UNSUPPORTEDALG] = "algorithm is unsupported",
	[DST_R_CRYPTOFAILURE] = "crypto failure",
	[DST_R_NOCRYPTO] = "built with no crypto support",
	[DST_R_NULLKEY] = "illegal operation for a null key",
	[DST_R_INVALIDPUBLICKEY] = "public key is invalid",
	[DST_R_INVALIDPRIVATEKEY] = "private key is invalid",
	[DST_R_WRITEERROR] = "error occurred writing key to disk",
	[DST_R_INVALIDPARAM] = "invalid algorithm specific parameter",
	[DST_R_SIGNFAILURE] = "sign failure",
	[DST_R_VERIFYFAILURE] = "verify failure",
	[DST_R_NOTPUBLICKEY] = "not a public key",
	[DST_R_NOTPRIVATEKEY] = "not a private key",
	[DST_R_KEYCANNOTCOMPUTESECRET] = "not a key that can compute a secret",
	[DST_R_COMPUTESECRETFAILURE] = "failure computing a shared secret",
	[DST_R_NORANDOMNESS] = "no randomness available",
	[DST_R_BADKEYTYPE] = "bad key type",
	[DST_R_NOENGINE] = "no engine",
	[DST_R_EXTERNALKEY] = "illegal operation for an external key",

	[DNS_R_NOERROR] = "NOERROR",
	[DNS_R_FORMERR] = "FORMERR",
	[DNS_R_SERVFAIL] = "SERVFAIL",
	[DNS_R_NXDOMAIN] = "NXDOMAIN",
	[DNS_R_NOTIMP] = "NOTIMP",
	[DNS_R_REFUSED] = "REFUSED",
	[DNS_R_YXDOMAIN] = "YXDOMAIN",
	[DNS_R_YXRRSET] = "YXRRSET",
	[DNS_R_NXRRSET] = "NXRRSET",
	[DNS_R_NOTAUTH] = "NOTAUTH",
	[DNS_R_NOTZONE] = "NOTZONE",
	[DNS_R_RCODE11] = "<rcode 11>",
	[DNS_R_RCODE12] = "<rcode 12>",
	[DNS_R_RCODE13] = "<rcode 13>",
	[DNS_R_RCODE14] = "<rcode 14>",
	[DNS_R_RCODE15] = "<rcode 15>",
	[DNS_R_BADVERS] = "BADVERS",

	[ISCCC_R_UNKNOWNVERSION] = "unknown version",
	[ISCCC_R_SYNTAX] = "syntax error",
	[ISCCC_R_BADAUTH] = "bad auth",
	[ISCCC_R_EXPIRED] = "expired",
	[ISCCC_R_CLOCKSKEW] = "clock skew",
	[ISCCC_R_DUPLICATE] = "duplicate",
};

static const char *identifier[ISC_R_NRESULTS] = {
	[ISC_R_SUCCESS] = "ISC_R_SUCCESS",
	[ISC_R_NOMEMORY] = "ISC_R_NOMEMORY",
	[ISC_R_TIMEDOUT] = "ISC_R_TIMEDOUT",
	[ISC_R_NOTHREADS] = "ISC_R_NOTHREADS",
	[ISC_R_ADDRNOTAVAIL] = "ISC_R_ADDRNOTAVAIL",
	[ISC_R_ADDRINUSE] = "ISC_R_ADDRINUSE",
	[ISC_R_NOPERM] = "ISC_R_NOPERM",
	[ISC_R_NOCONN] = "ISC_R_NOCONN",
	[ISC_R_NETUNREACH] = "ISC_R_NETUNREACH",
	[ISC_R_HOSTUNREACH] = "ISC_R_HOSTUNREACH",
	[ISC_R_NETDOWN] = "ISC_R_NETDOWN",
	[ISC_R_HOSTDOWN] = "ISC_R_HOSTDOWN",
	[ISC_R_CONNREFUSED] = "ISC_R_CONNREFUSED",
	[ISC_R_NORESOURCES] = "ISC_R_NORESOURCES",
	[ISC_R_EOF] = "ISC_R_EOF",
	[ISC_R_BOUND] = "ISC_R_BOUND",
	[ISC_R_RELOAD] = "ISC_R_RELOAD",
	[ISC_R_LOCKBUSY] = "ISC_R_LOCKBUSY",
	[ISC_R_EXISTS] = "ISC_R_EXISTS",
	[ISC_R_NOSPACE] = "ISC_R_NOSPACE",
	[ISC_R_CANCELED] = "ISC_R_CANCELED",
	[ISC_R_NOTBOUND] = "ISC_R_NOTBOUND",
	[ISC_R_SHUTTINGDOWN] = "ISC_R_SHUTTINGDOWN",
	[ISC_R_NOTFOUND] = "ISC_R_NOTFOUND",
	[ISC_R_UNEXPECTEDEND] = "ISC_R_UNEXPECTEDEND",
	[ISC_R_FAILURE] = "ISC_R_FAILURE",
	[ISC_R_IOERROR] = "ISC_R_IOERROR",
	[ISC_R_NOTIMPLEMENTED] = "ISC_R_NOTIMPLEMENTED",
	[ISC_R_UNBALANCED] = "ISC_R_UNBALANCED",
	[ISC_R_NOMORE] = "ISC_R_NOMORE",
	[ISC_R_INVALIDFILE] = "ISC_R_INVALIDFILE",
	[ISC_R_BADBASE64] = "ISC_R_BADBASE64",
	[ISC_R_UNEXPECTEDTOKEN] = "ISC_R_UNEXPECTEDTOKEN",
	[ISC_R_QUOTA] = "ISC_R_QUOTA",
	[ISC_R_UNEXPECTED] = "ISC_R_UNEXPECTED",
	[ISC_R_ALREADYRUNNING] = "ISC_R_ALREADYRUNNING",
	[ISC_R_IGNORE] = "ISC_R_IGNORE",
	[ISC_R_MASKNONCONTIG] = "ISC_R_MASKNONCONTIG",
	[ISC_R_FILENOTFOUND] = "ISC_R_FILENOTFOUND",
	[ISC_R_FILEEXISTS] = "ISC_R_FILEEXISTS",
	[ISC_R_NOTCONNECTED] = "ISC_R_NOTCONNECTED",
	[ISC_R_RANGE] = "ISC_R_RANGE",
	[ISC_R_NOENTROPY] = "ISC_R_NOENTROPY",
	[ISC_R_MULTICAST] = "ISC_R_MULTICAST",
	[ISC_R_NOTFILE] = "ISC_R_NOTFILE",
	[ISC_R_NOTDIRECTORY] = "ISC_R_NOTDIRECTORY",
	[ISC_R_EMPTY] = "ISC_R_EMPTY",
	[ISC_R_FAMILYMISMATCH] = "ISC_R_FAMILYMISMATCH",
	[ISC_R_FAMILYNOSUPPORT] = "ISC_R_FAMILYNOSUPPORT",
	[ISC_R_BADHEX] = "ISC_R_BADHEX",
	[ISC_R_TOOMANYOPENFILES] = "ISC_R_TOOMANYOPENFILES",
	[ISC_R_NOTBLOCKING] = "ISC_R_NOTBLOCKING",
	[ISC_R_UNBALANCEDQUOTES] = "ISC_R_UNBALANCEDQUOTES",
	[ISC_R_INPROGRESS] = "ISC_R_INPROGRESS",
	[ISC_R_CONNECTIONRESET] = "ISC_R_CONNECTIONRESET",
	[ISC_R_SOFTQUOTA] = "ISC_R_SOFTQUOTA",
	[ISC_R_BADNUMBER] = "ISC_R_BADNUMBER",
	[ISC_R_DISABLED] = "ISC_R_DISABLED",
	[ISC_R_MAXSIZE] = "ISC_R_MAXSIZE",
	[ISC_R_BADADDRESSFORM] = "ISC_R_BADADDRESSFORM",
	[ISC_R_BADBASE32] = "ISC_R_BADBASE32",
	[ISC_R_UNSET] = "ISC_R_UNSET",
	[ISC_R_MULTIPLE] = "ISC_R_MULTIPLE",
	[ISC_R_WOULDBLOCK] = "ISC_R_WOULDBLOCK",
	[ISC_R_COMPLETE] = "ISC_R_COMPLETE",
	[ISC_R_CRYPTOFAILURE] = "ISC_R_CRYPTOFAILURE",
	[ISC_R_DISCQUOTA] = "ISC_R_DISCQUOTA",
	[ISC_R_DISCFULL] = "ISC_R_DISCFULL",
	[ISC_R_DEFAULT] = "ISC_R_DEFAULT",
	[ISC_R_IPV4PREFIX] = "ISC_R_IPV4PREFIX",
	[ISC_R_TLSERROR] = "ISC_R_TLSERROR",
	[ISC_R_TLSBADPEERCERT] = "ISC_R_TLSBADPEERCERT",
	[ISC_R_HTTP2ALPNERROR] = "ISC_R_HTTP2ALPNERROR",
	[ISC_R_DOTALPNERROR] = "ISC_R_DOTALPNERROR",
	[DNS_R_LABELTOOLONG] = "DNS_R_LABELTOOLONG",
	[DNS_R_BADESCAPE] = "DNS_R_BADESCAPE",
	[DNS_R_EMPTYLABEL] = "DNS_R_EMPTYLABEL",
	[DNS_R_BADDOTTEDQUAD] = "DNS_R_BADDOTTEDQUAD",
	[DNS_R_INVALIDNS] = "DNS_R_INVALIDNS",
	[DNS_R_UNKNOWN] = "DNS_R_UNKNOWN",
	[DNS_R_BADLABELTYPE] = "DNS_R_BADLABELTYPE",
	[DNS_R_BADPOINTER] = "DNS_R_BADPOINTER",
	[DNS_R_TOOMANYHOPS] = "DNS_R_TOOMANYHOPS",
	[DNS_R_DISALLOWED] = "DNS_R_DISALLOWED",
	[DNS_R_EXTRATOKEN] = "DNS_R_EXTRATOKEN",
	[DNS_R_EXTRADATA] = "DNS_R_EXTRADATA",
	[DNS_R_TEXTTOOLONG] = "DNS_R_TEXTTOOLONG",
	[DNS_R_NOTZONETOP] = "DNS_R_NOTZONETOP",
	[DNS_R_SYNTAX] = "DNS_R_SYNTAX",
	[DNS_R_BADCKSUM] = "DNS_R_BADCKSUM",
	[DNS_R_BADAAAA] = "DNS_R_BADAAAA",
	[DNS_R_NOOWNER] = "DNS_R_NOOWNER",
	[DNS_R_NOTTL] = "DNS_R_NOTTL",
	[DNS_R_BADCLASS] = "DNS_R_BADCLASS",
	[DNS_R_NAMETOOLONG] = "DNS_R_NAMETOOLONG",
	[DNS_R_PARTIALMATCH] = "DNS_R_PARTIALMATCH",
	[DNS_R_NEWORIGIN] = "DNS_R_NEWORIGIN",
	[DNS_R_UNCHANGED] = "DNS_R_UNCHANGED",
	[DNS_R_BADTTL] = "DNS_R_BADTTL",
	[DNS_R_NOREDATA] = "DNS_R_NOREDATA",
	[DNS_R_CONTINUE] = "DNS_R_CONTINUE",
	[DNS_R_DELEGATION] = "DNS_R_DELEGATION",
	[DNS_R_GLUE] = "DNS_R_GLUE",
	[DNS_R_DNAME] = "DNS_R_DNAME",
	[DNS_R_CNAME] = "DNS_R_CNAME",
	[DNS_R_BADDB] = "DNS_R_BADDB",
	[DNS_R_ZONECUT] = "DNS_R_ZONECUT",
	[DNS_R_BADZONE] = "DNS_R_BADZONE",
	[DNS_R_MOREDATA] = "DNS_R_MOREDATA",
	[DNS_R_UPTODATE] = "DNS_R_UPTODATE",
	[DNS_R_TSIGVERIFYFAILURE] = "DNS_R_TSIGVERIFYFAILURE",
	[DNS_R_TSIGERRORSET] = "DNS_R_TSIGERRORSET",
	[DNS_R_SIGINVALID] = "DNS_R_SIGINVALID",
	[DNS_R_SIGEXPIRED] = "DNS_R_SIGEXPIRED",
	[DNS_R_SIGFUTURE] = "DNS_R_SIGFUTURE",
	[DNS_R_KEYUNAUTHORIZED] = "DNS_R_KEYUNAUTHORIZED",
	[DNS_R_INVALIDTIME] = "DNS_R_INVALIDTIME",
	[DNS_R_EXPECTEDTSIG] = "DNS_R_EXPECTEDTSIG",
	[DNS_R_UNEXPECTEDTSIG] = "DNS_R_UNEXPECTEDTSIG",
	[DNS_R_INVALIDTKEY] = "DNS_R_INVALIDTKEY",
	[DNS_R_HINT] = "DNS_R_HINT",
	[DNS_R_DROP] = "DNS_R_DROP",
	[DNS_R_NOTLOADED] = "DNS_R_NOTLOADED",
	[DNS_R_NCACHENXDOMAIN] = "DNS_R_NCACHENXDOMAIN",
	[DNS_R_NCACHENXRRSET] = "DNS_R_NCACHENXRRSET",
	[DNS_R_WAIT] = "DNS_R_WAIT",
	[DNS_R_NOTVERIFIEDYET] = "DNS_R_NOTVERIFIEDYET",
	[DNS_R_NOIDENTITY] = "DNS_R_NOIDENTITY",
	[DNS_R_NOJOURNAL] = "DNS_R_NOJOURNAL",
	[DNS_R_ALIAS] = "DNS_R_ALIAS",
	[DNS_R_USETCP] = "DNS_R_USETCP",
	[DNS_R_NOVALIDSIG] = "DNS_R_NOVALIDSIG",
	[DNS_R_NOVALIDNSEC] = "DNS_R_NOVALIDNSEC",
	[DNS_R_NOTINSECURE] = "DNS_R_NOTINSECURE",
	[DNS_R_UNKNOWNSERVICE] = "DNS_R_UNKNOWNSERVICE",
	[DNS_R_RECOVERABLE] = "DNS_R_RECOVERABLE",
	[DNS_R_UNKNOWNOPT] = "DNS_R_UNKNOWNOPT",
	[DNS_R_UNEXPECTEDID] = "DNS_R_UNEXPECTEDID",
	[DNS_R_SEENINCLUDE] = "DNS_R_SEENINCLUDE",
	[DNS_R_NOTEXACT] = "DNS_R_NOTEXACT",
	[DNS_R_BLACKHOLED] = "DNS_R_BLACKHOLED",
	[DNS_R_BADALG] = "DNS_R_BADALG",
	[DNS_R_METATYPE] = "DNS_R_METATYPE",
	[DNS_R_CNAMEANDOTHER] = "DNS_R_CNAMEANDOTHER",
	[DNS_R_SINGLETON] = "DNS_R_SINGLETON",
	[DNS_R_HINTNXRRSET] = "DNS_R_HINTNXRRSET",
	[DNS_R_NOMASTERFILE] = "DNS_R_NOMASTERFILE",
	[DNS_R_UNKNOWNPROTO] = "DNS_R_UNKNOWNPROTO",
	[DNS_R_CLOCKSKEW] = "DNS_R_CLOCKSKEW",
	[DNS_R_BADIXFR] = "DNS_R_BADIXFR",
	[DNS_R_NOTAUTHORITATIVE] = "DNS_R_NOTAUTHORITATIVE",
	[DNS_R_NOVALIDKEY] = "DNS_R_NOVALIDKEY",
	[DNS_R_OBSOLETE] = "DNS_R_OBSOLETE",
	[DNS_R_FROZEN] = "DNS_R_FROZEN",
	[DNS_R_UNKNOWNFLAG] = "DNS_R_UNKNOWNFLAG",
	[DNS_R_EXPECTEDRESPONSE] = "DNS_R_EXPECTEDRESPONSE",
	[DNS_R_NOVALIDDS] = "DNS_R_NOVALIDDS",
	[DNS_R_NSISADDRESS] = "DNS_R_NSISADDRESS",
	[DNS_R_REMOTEFORMERR] = "DNS_R_REMOTEFORMERR",
	[DNS_R_TRUNCATEDTCP] = "DNS_R_TRUNCATEDTCP",
	[DNS_R_LAME] = "DNS_R_LAME",
	[DNS_R_UNEXPECTEDRCODE] = "DNS_R_UNEXPECTEDRCODE",
	[DNS_R_UNEXPECTEDOPCODE] = "DNS_R_UNEXPECTEDOPCODE",
	[DNS_R_CHASEDSSERVERS] = "DNS_R_CHASEDSSERVERS",
	[DNS_R_EMPTYNAME] = "DNS_R_EMPTYNAME",
	[DNS_R_EMPTYWILD] = "DNS_R_EMPTYWILD",
	[DNS_R_BADBITMAP] = "DNS_R_BADBITMAP",
	[DNS_R_FROMWILDCARD] = "DNS_R_FROMWILDCARD",
	[DNS_R_BADOWNERNAME] = "DNS_R_BADOWNERNAME",
	[DNS_R_BADNAME] = "DNS_R_BADNAME",
	[DNS_R_DYNAMIC] = "DNS_R_DYNAMIC",
	[DNS_R_UNKNOWNCOMMAND] = "DNS_R_UNKNOWNCOMMAND",
	[DNS_R_MUSTBESECURE] = "DNS_R_MUSTBESECURE",
	[DNS_R_COVERINGNSEC] = "DNS_R_COVERINGNSEC",
	[DNS_R_MXISADDRESS] = "DNS_R_MXISADDRESS",
	[DNS_R_DUPLICATE] = "DNS_R_DUPLICATE",
	[DNS_R_INVALIDNSEC3] = "DNS_R_INVALIDNSEC3",
	[DNS_R_NOTPRIMARY] = "DNS_R_NOTPRIMARY",
	[DNS_R_BROKENCHAIN] = "DNS_R_BROKENCHAIN",
	[DNS_R_EXPIRED] = "DNS_R_EXPIRED",
	[DNS_R_NOTDYNAMIC] = "DNS_R_NOTDYNAMIC",
	[DNS_R_BADEUI] = "DNS_R_BADEUI",
	[DNS_R_NTACOVERED] = "DNS_R_NTACOVERED",
	[DNS_R_BADCDS] = "DNS_R_BADCDS",
	[DNS_R_BADCDNSKEY] = "DNS_R_BADCDNSKEY",
	[DNS_R_OPTERR] = "DNS_R_OPTERR",
	[DNS_R_BADDNSTAP] = "DNS_R_BADDNSTAP",
	[DNS_R_BADTSIG] = "DNS_R_BADTSIG",
	[DNS_R_BADSIG0] = "DNS_R_BADSIG0",
	[DNS_R_TOOMANYRECORDS] = "DNS_R_TOOMANYRECORDS",
	[DNS_R_VERIFYFAILURE] = "DNS_R_VERIFYFAILURE",
	[DNS_R_ATZONETOP] = "DNS_R_ATZONETOP",
	[DNS_R_NOKEYMATCH] = "DNS_R_NOKEYMATCH",
	[DNS_R_TOOMANYKEYS] = "DNS_R_TOOMANYKEYS",
	[DNS_R_KEYNOTACTIVE] = "DNS_R_KEYNOTACTIVE",
	[DNS_R_NSEC3ITERRANGE] = "DNS_R_NSEC3ITERRANGE",
	[DNS_R_NSEC3SALTRANGE] = "DNS_R_NSEC3SALTRANGE",
	[DNS_R_NSEC3BADALG] = "DNS_R_NSEC3BADALG",
	[DNS_R_NSEC3RESALT] = "DNS_R_NSEC3RESALT",
	[DNS_R_INCONSISTENTRR] = "DNS_R_INCONSISTENTRR",
	[DNS_R_NOALPN] = "DNS_R_NOALPN",

	[DST_R_UNSUPPORTEDALG] = "DST_R_UNSUPPORTEDALG",
	[DST_R_CRYPTOFAILURE] = "DST_R_CRYPTOFAILURE",
	[DST_R_NOCRYPTO] = "DST_R_NOCRYPTO",
	[DST_R_NULLKEY] = "DST_R_NULLKEY",
	[DST_R_INVALIDPUBLICKEY] = "DST_R_INVALIDPUBLICKEY",
	[DST_R_INVALIDPRIVATEKEY] = "DST_R_INVALIDPRIVATEKEY",
	[DST_R_WRITEERROR] = "DST_R_WRITEERROR",
	[DST_R_INVALIDPARAM] = "DST_R_INVALIDPARAM",
	[DST_R_SIGNFAILURE] = "DST_R_SIGNFAILURE",
	[DST_R_VERIFYFAILURE] = "DST_R_VERIFYFAILURE",
	[DST_R_NOTPUBLICKEY] = "DST_R_NOTPUBLICKEY",
	[DST_R_NOTPRIVATEKEY] = "DST_R_NOTPRIVATEKEY",
	[DST_R_KEYCANNOTCOMPUTESECRET] = "DST_R_KEYCANNOTCOMPUTESECRET",
	[DST_R_COMPUTESECRETFAILURE] = "DST_R_COMPUTESECRETFAILURE",
	[DST_R_NORANDOMNESS] = "DST_R_NORANDOMNESS",
	[DST_R_BADKEYTYPE] = "DST_R_BADKEYTYPE",
	[DST_R_NOENGINE] = "DST_R_NOENGINE",
	[DST_R_EXTERNALKEY] = "DST_R_EXTERNALKEY",

	[DNS_R_NOERROR] = "DNS_R_NOERROR",
	[DNS_R_FORMERR] = "DNS_R_FORMERR",
	[DNS_R_SERVFAIL] = "DNS_R_SERVFAIL",
	[DNS_R_NXDOMAIN] = "DNS_R_NXDOMAIN",
	[DNS_R_NOTIMP] = "DNS_R_NOTIMP",
	[DNS_R_REFUSED] = "DNS_R_REFUSED",
	[DNS_R_YXDOMAIN] = "DNS_R_YXDOMAIN",
	[DNS_R_YXRRSET] = "DNS_R_YXRRSET",
	[DNS_R_NXRRSET] = "DNS_R_NXRRSET",
	[DNS_R_NOTAUTH] = "DNS_R_NOTAUTH",
	[DNS_R_NOTZONE] = "DNS_R_NOTZONE",
	[DNS_R_RCODE11] = "DNS_R_RCODE11",
	[DNS_R_RCODE12] = "RNS_R_RCODE12",
	[DNS_R_RCODE13] = "DNS_R_RCODE13",
	[DNS_R_RCODE14] = "DNS_R_RCODE14",
	[DNS_R_RCODE15] = "DNS_R_RCODE15",
	[DNS_R_BADVERS] = "DNS_R_BADVERS",

	[ISCCC_R_UNKNOWNVERSION] = "ISCCC_R_UNKNOWNVERSION",
	[ISCCC_R_SYNTAX] = "ISCCC_R_SYNTAX",
	[ISCCC_R_BADAUTH] = "ISCCC_R_BADAUTH",
	[ISCCC_R_EXPIRED] = "ISCCC_R_EXPIRED",
	[ISCCC_R_CLOCKSKEW] = "ISCCC_R_CLOCKSKEW",
	[ISCCC_R_DUPLICATE] = "ISCCC_R_DUPLICATE",
};

STATIC_ASSERT((DNS_R_SERVFAIL - DNS_R_NOERROR == 2),
	      "DNS_R_NOERROR has wrong value");

STATIC_ASSERT((DNS_R_BADVERS - DNS_R_NOERROR == 16),
	      "DNS_R_BADVERS has wrong value");

STATIC_ASSERT((ISC_R_NRESULTS < INT32_MAX), "result.h enum too big");

const char *
isc_result_totext(isc_result_t result) {
	return (description[result]);
}

const char *
isc_result_toid(isc_result_t result) {
	return (identifier[result]);
}
