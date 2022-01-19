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

#if HAVE_CMOCKA

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include <isc/result.h>
#include <isc/util.h>

/* convert result to identifier string */
static void
isc_result_toid_test(void **state) {
	const char *id;

	UNUSED(state);

	id = isc_result_toid(ISC_R_SUCCESS);
	assert_string_equal("ISC_R_SUCCESS", id);

	id = isc_result_toid(ISC_R_FAILURE);
	assert_string_equal("ISC_R_FAILURE", id);
}

/* convert result to description string */
static void
isc_result_totext_test(void **state) {
	const char *str;

	UNUSED(state);

	str = isc_result_totext(ISC_R_SUCCESS);
	assert_string_equal("success", str);

	str = isc_result_totext(ISC_R_FAILURE);
	assert_string_equal("failure", str);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(isc_result_toid_test),
		cmocka_unit_test(isc_result_totext_test),
	};

	return (cmocka_run_group_tests(tests, NULL, NULL));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (SKIPPED_TEST_EXIT_CODE);
}

#endif /* if HAVE_CMOCKA */
