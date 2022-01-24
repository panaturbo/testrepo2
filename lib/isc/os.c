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

#include <inttypes.h>

#include <isc/os.h>
#include <isc/types.h>
#include <isc/util.h>

#include "os_p.h"

static unsigned int isc__os_ncpus = 0;

#ifdef HAVE_SYSCONF

#include <unistd.h>

static inline long
sysconf_ncpus(void) {
#if defined(_SC_NPROCESSORS_ONLN)
	return (sysconf((_SC_NPROCESSORS_ONLN)));
#elif defined(_SC_NPROC_ONLN)
	return (sysconf((_SC_NPROC_ONLN)));
#else  /* if defined(_SC_NPROCESSORS_ONLN) */
	return (0);
#endif /* if defined(_SC_NPROCESSORS_ONLN) */
}
#endif /* HAVE_SYSCONF */

#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
#include <sys/param.h> /* for NetBSD */
#include <sys/sysctl.h>
#include <sys/types.h> /* for FreeBSD */

static int
sysctl_ncpus(void) {
	int ncpu, result;
	size_t len;

	len = sizeof(ncpu);
	result = sysctlbyname("hw.ncpu", &ncpu, &len, 0, 0);
	if (result != -1) {
		return (ncpu);
	}
	return (0);
}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */

static void
ncpus_initialize(void) {
#if defined(HAVE_SYSCONF)
	isc__os_ncpus = sysconf_ncpus();
#endif /* if defined(HAVE_SYSCONF) */
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
	if (isc__os_ncpus <= 0) {
		isc__os_ncpus = sysctl_ncpus();
	}
#endif /* if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME) */
	if (isc__os_ncpus == 0) {
		isc__os_ncpus = 1;
	}
}

unsigned int
isc_os_ncpus(void) {
	return (isc__os_ncpus);
}

void
isc__os_initialize(void) {
	ncpus_initialize();
#if defined(HAVE_SYSCONF) && defined(_SC_LEVEL1_DCACHE_LINESIZE)
	long s = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	RUNTIME_CHECK((size_t)s == (size_t)ISC_OS_CACHELINE_SIZE || s <= 0);
#endif
}

void
isc__os_shutdown(void) {
	/* empty, but defined for completeness */;
}
