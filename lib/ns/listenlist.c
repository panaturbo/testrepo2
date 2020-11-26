/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/util.h>

#include <dns/acl.h>

#include <ns/listenlist.h>

static void
destroy(ns_listenlist_t *list);

isc_result_t
ns_listenelt_create(isc_mem_t *mctx, in_port_t port, isc_dscp_t dscp,
		    dns_acl_t *acl, bool tls, const char *key, const char *cert,
		    ns_listenelt_t **target) {
	ns_listenelt_t *elt = NULL;
	isc_result_t result = ISC_R_SUCCESS;
	REQUIRE(target != NULL && *target == NULL);
	elt = isc_mem_get(mctx, sizeof(*elt));
	elt->mctx = mctx;
	ISC_LINK_INIT(elt, link);
	elt->port = port;
	elt->dscp = dscp;
	elt->acl = acl;
	elt->sslctx = NULL;
	if (tls) {
		result = isc_nm_tls_create_server_ctx(key, cert, &elt->sslctx);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}
	*target = elt;
	return (ISC_R_SUCCESS);
}

void
ns_listenelt_destroy(ns_listenelt_t *elt) {
	if (elt->acl != NULL) {
		dns_acl_detach(&elt->acl);
	}
	if (elt->sslctx != NULL) {
		SSL_CTX_free(elt->sslctx);
		elt->sslctx = NULL;
	}
	isc_mem_put(elt->mctx, elt, sizeof(*elt));
}

isc_result_t
ns_listenlist_create(isc_mem_t *mctx, ns_listenlist_t **target) {
	ns_listenlist_t *list = NULL;
	REQUIRE(target != NULL && *target == NULL);
	list = isc_mem_get(mctx, sizeof(*list));
	list->mctx = mctx;
	list->refcount = 1;
	ISC_LIST_INIT(list->elts);
	*target = list;
	return (ISC_R_SUCCESS);
}

static void
destroy(ns_listenlist_t *list) {
	ns_listenelt_t *elt, *next;
	for (elt = ISC_LIST_HEAD(list->elts); elt != NULL; elt = next) {
		next = ISC_LIST_NEXT(elt, link);
		ns_listenelt_destroy(elt);
	}
	isc_mem_put(list->mctx, list, sizeof(*list));
}

void
ns_listenlist_attach(ns_listenlist_t *source, ns_listenlist_t **target) {
	INSIST(source->refcount > 0);
	source->refcount++;
	*target = source;
}

void
ns_listenlist_detach(ns_listenlist_t **listp) {
	ns_listenlist_t *list = *listp;
	*listp = NULL;
	INSIST(list->refcount > 0);
	list->refcount--;
	if (list->refcount == 0) {
		destroy(list);
	}
}

isc_result_t
ns_listenlist_default(isc_mem_t *mctx, in_port_t port, isc_dscp_t dscp,
		      bool enabled, ns_listenlist_t **target) {
	isc_result_t result;
	dns_acl_t *acl = NULL;
	ns_listenelt_t *elt = NULL;
	ns_listenlist_t *list = NULL;

	REQUIRE(target != NULL && *target == NULL);
	if (enabled) {
		result = dns_acl_any(mctx, &acl);
	} else {
		result = dns_acl_none(mctx, &acl);
	}
	if (result != ISC_R_SUCCESS) {
		goto cleanup;
	}

	result = ns_listenelt_create(mctx, port, dscp, acl, false, NULL, NULL,
				     &elt);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_acl;
	}

	result = ns_listenlist_create(mctx, &list);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_listenelt;
	}

	ISC_LIST_APPEND(list->elts, elt, link);

	*target = list;
	return (ISC_R_SUCCESS);

cleanup_listenelt:
	ns_listenelt_destroy(elt);
cleanup_acl:
	dns_acl_detach(&acl);
cleanup:
	return (result);
}
