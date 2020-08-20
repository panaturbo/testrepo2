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

#if HAVE_CMOCKA

#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UNIT_TESTING
#include <cmocka.h>
#include <maxminddb.h>

#include <isc/print.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/geoip.h>

#include "../geoip2.c"
#include "dnstest.h"

/* Use GeoIP2 databases from the 'geoip2' system test */
#define TEST_GEOIP_DATA "../../../bin/tests/system/geoip2/data"

static dns_geoip_databases_t geoip;

static MMDB_s geoip_country, geoip_city, geoip_as, geoip_isp, geoip_domain;

static void
load_geoip(const char *dir);
static void
close_geoip(void);

static int
_setup(void **state) {
	isc_result_t result;

	UNUSED(state);

	result = dns_test_begin(NULL, false);
	assert_int_equal(result, ISC_R_SUCCESS);

	/* Use databases from the geoip system test */
	load_geoip(TEST_GEOIP_DATA);

	return (0);
}

static int
_teardown(void **state) {
	UNUSED(state);

	close_geoip();

	dns_test_end();

	return (0);
}

static MMDB_s *
open_geoip2(const char *dir, const char *dbfile, MMDB_s *mmdb) {
	char pathbuf[PATH_MAX];
	int ret;

	snprintf(pathbuf, sizeof(pathbuf), "%s/%s", dir, dbfile);
	ret = MMDB_open(pathbuf, MMDB_MODE_MMAP, mmdb);
	if (ret == MMDB_SUCCESS) {
		return (mmdb);
	}

	return (NULL);
}

static void
load_geoip(const char *dir) {
	geoip.country = open_geoip2(dir, "GeoIP2-Country.mmdb", &geoip_country);
	geoip.city = open_geoip2(dir, "GeoIP2-City.mmdb", &geoip_city);
	geoip.as = open_geoip2(dir, "GeoLite2-ASN.mmdb", &geoip_as);
	geoip.isp = open_geoip2(dir, "GeoIP2-ISP.mmdb", &geoip_isp);
	geoip.domain = open_geoip2(dir, "GeoIP2-Domain.mmdb", &geoip_domain);
}

static void
close_geoip(void) {
	MMDB_close(&geoip_country);
	MMDB_close(&geoip_city);
	MMDB_close(&geoip_as);
	MMDB_close(&geoip_isp);
	MMDB_close(&geoip_domain);
}

static bool
/* Check if an MMDB entry of a given subtype exists for the given IP */
entry_exists(dns_geoip_subtype_t subtype, const char *addr) {
	struct in6_addr in6;
	struct in_addr in4;
	isc_netaddr_t na;
	MMDB_s *db;

	if (inet_pton(AF_INET6, addr, &in6) == 1) {
		isc_netaddr_fromin6(&na, &in6);
	} else if (inet_pton(AF_INET, addr, &in4) == 1) {
		isc_netaddr_fromin(&na, &in4);
	} else {
		INSIST(0);
		ISC_UNREACHABLE();
	}

	db = geoip2_database(&geoip, fix_subtype(&geoip, subtype));

	return (db != NULL && get_entry_for(db, &na) != NULL);
}

/*
 * Baseline test - check if get_entry_for() works as expected, i.e. that its
 * return values are consistent with the contents of the test MMDBs found in
 * bin/tests/system/geoip2/data/ (10.53.0.1 and fd92:7065:b8e:ffff::1 should be
 * present in all databases, 192.0.2.128 should only be present in the country
 * database, ::1 should be absent from all databases).
 */
static void
baseline(void **state) {
	dns_geoip_subtype_t subtype;

	UNUSED(state);

	subtype = dns_geoip_city_name;

	assert_true(entry_exists(subtype, "10.53.0.1"));
	assert_false(entry_exists(subtype, "192.0.2.128"));
	assert_true(entry_exists(subtype, "fd92:7065:b8e:ffff::1"));
	assert_false(entry_exists(subtype, "::1"));

	subtype = dns_geoip_country_name;

	assert_true(entry_exists(subtype, "10.53.0.1"));
	assert_true(entry_exists(subtype, "192.0.2.128"));
	assert_true(entry_exists(subtype, "fd92:7065:b8e:ffff::1"));
	assert_false(entry_exists(subtype, "::1"));

	subtype = dns_geoip_domain_name;

	assert_true(entry_exists(subtype, "10.53.0.1"));
	assert_false(entry_exists(subtype, "192.0.2.128"));
	assert_true(entry_exists(subtype, "fd92:7065:b8e:ffff::1"));
	assert_false(entry_exists(subtype, "::1"));

	subtype = dns_geoip_isp_name;

	assert_true(entry_exists(subtype, "10.53.0.1"));
	assert_false(entry_exists(subtype, "192.0.2.128"));
	assert_true(entry_exists(subtype, "fd92:7065:b8e:ffff::1"));
	assert_false(entry_exists(subtype, "::1"));

	subtype = dns_geoip_as_asnum;

	assert_true(entry_exists(subtype, "10.53.0.1"));
	assert_false(entry_exists(subtype, "192.0.2.128"));
	assert_true(entry_exists(subtype, "fd92:7065:b8e:ffff::1"));
	assert_false(entry_exists(subtype, "::1"));
}

static bool
do_lookup_string(const char *addr, dns_geoip_subtype_t subtype,
		 const char *string) {
	dns_geoip_elem_t elt;
	struct in_addr in4;
	isc_netaddr_t na;
	int n;

	n = inet_pton(AF_INET, addr, &in4);
	assert_int_equal(n, 1);
	isc_netaddr_fromin(&na, &in4);

	elt.subtype = subtype;
	strlcpy(elt.as_string, string, sizeof(elt.as_string));

	return (dns_geoip_match(&na, &geoip, &elt));
}

static bool
do_lookup_string_v6(const char *addr, dns_geoip_subtype_t subtype,
		    const char *string) {
	dns_geoip_elem_t elt;
	struct in6_addr in6;
	isc_netaddr_t na;
	int n;

	n = inet_pton(AF_INET6, addr, &in6);
	assert_int_equal(n, 1);
	isc_netaddr_fromin6(&na, &in6);

	elt.subtype = subtype;
	strlcpy(elt.as_string, string, sizeof(elt.as_string));

	return (dns_geoip_match(&na, &geoip, &elt));
}

/* GeoIP country matching */
static void
country(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.country == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1", dns_geoip_country_code, "AU");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_country_name,
				 "Australia");
	assert_true(match);

	match = do_lookup_string("192.0.2.128", dns_geoip_country_code, "O1");
	assert_true(match);

	match = do_lookup_string("192.0.2.128", dns_geoip_country_name,
				 "Other");
	assert_true(match);
}

/* GeoIP country (ipv6) matching */
static void
country_v6(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.country == NULL) {
		skip();
	}

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_code, "AU");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_country_name, "Australia");
	assert_true(match);
}

/* GeoIP city (ipv4) matching */
static void
city(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.city == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1", dns_geoip_city_continentcode,
				 "NA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_countrycode, "US");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_countryname,
				 "United States");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_region, "CA");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_regionname,
				 "California");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_name,
				 "Redwood City");
	assert_true(match);

	match = do_lookup_string("10.53.0.1", dns_geoip_city_postalcode,
				 "94063");
	assert_true(match);
}

/* GeoIP city (ipv6) matching */
static void
city_v6(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.city == NULL) {
		skip();
	}

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_continentcode, "NA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countrycode, "US");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_countryname,
				    "United States");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_region, "CA");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_regionname, "California");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_name, "Redwood City");
	assert_true(match);

	match = do_lookup_string_v6("fd92:7065:b8e:ffff::1",
				    dns_geoip_city_postalcode, "94063");
	assert_true(match);
}

/* GeoIP asnum matching */
static void
asnum(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.as == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.3", dns_geoip_as_asnum, "AS100003");
	assert_true(match);
}

/* GeoIP isp matching */
static void
isp(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.isp == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.1", dns_geoip_isp_name,
				 "One Systems, Inc.");
	assert_true(match);
}

/* GeoIP org matching */
static void
org(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.as == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.2", dns_geoip_org_name,
				 "Two Technology Ltd.");
	assert_true(match);
}

/* GeoIP domain matching */
static void
domain(void **state) {
	bool match;

	UNUSED(state);

	if (geoip.domain == NULL) {
		skip();
	}

	match = do_lookup_string("10.53.0.5", dns_geoip_domain_name, "five.es");
	assert_true(match);
}

int
main(void) {
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(baseline),   cmocka_unit_test(country),
		cmocka_unit_test(country_v6), cmocka_unit_test(city),
		cmocka_unit_test(city_v6),    cmocka_unit_test(asnum),
		cmocka_unit_test(isp),	      cmocka_unit_test(org),
		cmocka_unit_test(domain),
	};

	return (cmocka_run_group_tests(tests, _setup, _teardown));
}

#else /* HAVE_CMOCKA */

#include <stdio.h>

int
main(void) {
	printf("1..0 # Skipped: cmocka not available\n");
	return (0);
}

#endif /* HAVE_CMOCKA */
