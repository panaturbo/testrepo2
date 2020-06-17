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

#ifndef ISC_PLATFORM_H
#define ISC_PLATFORM_H 1

/*! \file */

/*****
 ***** Platform-dependent defines.
 *****/

/***
 *** Default strerror_r buffer size
 ***/

#define ISC_STRERRORSIZE 128

/***
 *** System limitations
 ***/

#include <limits.h>

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef IOV_MAX
#define IOV_MAX 1024
#endif

/***
 *** Miscellaneous.
 ***/

/***
 ***	Windows dll support.
 ***/

#define LIBISC_EXTERNAL_DATA
#define LIBDNS_EXTERNAL_DATA
#define LIBISCCC_EXTERNAL_DATA
#define LIBISCCFG_EXTERNAL_DATA
#define LIBNS_EXTERNAL_DATA
#define LIBBIND9_EXTERNAL_DATA
#define LIBTESTS_EXTERNAL_DATA

/*
 * Tell emacs to use C mode for this file.
 *
 * Local Variables:
 * mode: c
 * End:
 */

#endif /* ISC_PLATFORM_H */
