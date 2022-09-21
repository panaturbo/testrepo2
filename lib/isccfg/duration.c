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

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/ttl.h>

#include <isccfg/duration.h>

/*
 * isccfg_duration_fromtext initially taken from OpenDNSSEC code base.
 * Modified to fit the BIND 9 code.
 */
isc_result_t
isccfg_duration_fromtext(isc_textregion_t *source,
			 isccfg_duration_t *duration) {
	char buf[DURATION_MAXLEN];
	char *P, *X, *T, *W, *str;
	bool not_weeks = false;
	int i;

	/*
	 * Copy the buffer as it may not be NULL terminated.
	 * Anyone having a duration longer than 63 characters is crazy.
	 */
	if (source->length > sizeof(buf) - 1) {
		return (ISC_R_BADNUMBER);
	}
	/* Copy source->length bytes and NULL terminate. */
	snprintf(buf, sizeof(buf), "%.*s", (int)source->length, source->base);
	str = buf;

	/* Clear out duration. */
	for (i = 0; i < 7; i++) {
		duration->parts[i] = 0;
	}
	duration->iso8601 = false;
	duration->unlimited = false;

	/* Every duration starts with 'P' */
	if (toupper((unsigned char)str[0]) != 'P') {
		return (ISC_R_BADNUMBER);
	}
	P = str;

	/* Record the time indicator. */
	T = strpbrk(str, "Tt");

	/* Record years. */
	X = strpbrk(str, "Yy");
	if (X != NULL) {
		duration->parts[0] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Record months. */
	X = strpbrk(str, "Mm");

	/*
	 * M could be months or minutes. This is months if there is no time
	 * part, or this M indicator is before the time indicator.
	 */
	if (X != NULL && (T == NULL || (size_t)(X - P) < (size_t)(T - P))) {
		duration->parts[1] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Record days. */
	X = strpbrk(str, "Dd");
	if (X != NULL) {
		duration->parts[3] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Time part? */
	if (T != NULL) {
		str = T;
		not_weeks = true;
	}

	/* Record hours. */
	X = strpbrk(str, "Hh");
	if (X != NULL && T != NULL) {
		duration->parts[4] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Record minutes. */
	X = strpbrk(str, "Mm");

	/*
	 * M could be months or minutes. This is minutes if there is a time
	 * part and the M indicator is behind the time indicator.
	 */
	if (X != NULL && T != NULL && (size_t)(X - P) > (size_t)(T - P)) {
		duration->parts[5] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Record seconds. */
	X = strpbrk(str, "Ss");
	if (X != NULL && T != NULL) {
		duration->parts[6] = atoi(str + 1);
		str = X;
		not_weeks = true;
	}

	/* Or is the duration configured in weeks? */
	W = strpbrk(buf, "Ww");
	if (W != NULL) {
		if (not_weeks) {
			/* Mix of weeks and other indicators is not allowed */
			return (ISC_R_BADNUMBER);
		} else {
			duration->parts[2] = atoi(str + 1);
			str = W;
		}
	}

	/* Deal with trailing garbage. */
	if (str[1] != '\0') {
		return (ISC_R_BADNUMBER);
	}

	duration->iso8601 = true;
	return (ISC_R_SUCCESS);
}

isc_result_t
isccfg_parse_duration(isc_textregion_t *source, isccfg_duration_t *duration) {
	isc_result_t result;

	REQUIRE(duration != NULL);

	duration->unlimited = false;
	result = isccfg_duration_fromtext(source, duration);
	if (result == ISC_R_BADNUMBER) {
		/* Fallback to dns_ttl_fromtext. */
		uint32_t ttl;
		result = dns_ttl_fromtext(source, &ttl);
		if (result == ISC_R_SUCCESS) {
			/*
			 * With dns_ttl_fromtext() the information on optional
			 * units is lost, and is treated as seconds from now on.
			 */
			duration->iso8601 = false;
			duration->parts[6] = ttl;
		}
	}

	return (result);
}

uint32_t
isccfg_duration_toseconds(const isccfg_duration_t *duration) {
	uint32_t seconds = 0;

	REQUIRE(duration != NULL);

	seconds += duration->parts[6];		   /* Seconds */
	seconds += duration->parts[5] * 60;	   /* Minutes */
	seconds += duration->parts[4] * 3600;	   /* Hours */
	seconds += duration->parts[3] * 86400;	   /* Days */
	seconds += duration->parts[2] * 86400 * 7; /* Weeks */
	/*
	 * The below additions are not entirely correct
	 * because days may very per month and per year.
	 */
	seconds += duration->parts[1] * 86400 * 31;  /* Months */
	seconds += duration->parts[0] * 86400 * 365; /* Years */
	return (seconds);
}
