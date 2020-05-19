/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdio.h>
#include <stdlib.h>

#include <isc/assertions.h>
#include <isc/backtrace.h>
#include <isc/print.h>
#include <isc/result.h>

/*
 * The maximum number of stack frames to dump on assertion failure.
 */
#ifndef BACKTRACE_MAXFRAME
#define BACKTRACE_MAXFRAME 128
#endif /* ifndef BACKTRACE_MAXFRAME */

/*%
 * Forward.
 */
static void
default_callback(const char *, int, isc_assertiontype_t, const char *);

static isc_assertioncallback_t isc_assertion_failed_cb = default_callback;

/*%
 * Public.
 */

/*% assertion failed handler */
/* coverity[+kill] */
void
isc_assertion_failed(const char *file, int line, isc_assertiontype_t type,
		     const char *cond) {
	isc_assertion_failed_cb(file, line, type, cond);
	abort();
	/* NOTREACHED */
}

/*% Set callback. */
void
isc_assertion_setcallback(isc_assertioncallback_t cb) {
	if (cb == NULL) {
		isc_assertion_failed_cb = default_callback;
	} else {
		isc_assertion_failed_cb = cb;
	}
}

/*% Type to Text */
const char *
isc_assertion_typetotext(isc_assertiontype_t type) {
	const char *result;

	/*
	 * These strings have purposefully not been internationalized
	 * because they are considered to essentially be keywords of
	 * the ISC development environment.
	 */
	switch (type) {
	case isc_assertiontype_require:
		result = "REQUIRE";
		break;
	case isc_assertiontype_ensure:
		result = "ENSURE";
		break;
	case isc_assertiontype_insist:
		result = "INSIST";
		break;
	case isc_assertiontype_invariant:
		result = "INVARIANT";
		break;
	default:
		result = "UNKNOWN";
	}
	return (result);
}

/*
 * Private.
 */

static void
default_callback(const char *file, int line, isc_assertiontype_t type,
		 const char *cond) {
	void *tracebuf[BACKTRACE_MAXFRAME];
	int nframes;
	bool have_backtrace = false;
	isc_result_t result;

	result = isc_backtrace_gettrace(tracebuf, BACKTRACE_MAXFRAME, &nframes);
	if (result == ISC_R_SUCCESS && nframes > 0) {
		have_backtrace = true;
	}

	fprintf(stderr, "%s:%d: %s(%s) failed%s\n", file, line,
		isc_assertion_typetotext(type), cond,
		(have_backtrace) ? ", back trace" : ".");

	if (result == ISC_R_SUCCESS) {
#if HAVE_BACKTRACE_SYMBOLS
		char **strs = backtrace_symbols(tracebuf, nframes);
		for (int i = 0; i < nframes; i++) {
			fprintf(stderr, "%s\n", strs[i]);
		}
#else  /* HAVE_BACKTRACE_SYMBOLS */
		for (int i = 0; i < nframes; i++) {
			fprintf(stderr, "#%d %p in ??\n", i, tracebuf[i]);
		}
#endif /* HAVE_BACKTRACE_SYMBOLS */
	}
	fflush(stderr);
}
