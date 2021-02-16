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

#include <libgen.h>
#include <unistd.h>
#include <uv.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <isc/atomic.h>
#include <isc/buffer.h>
#include <isc/condition.h>
#include <isc/log.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/once.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/stdtime.h>
#include <isc/thread.h>
#include <isc/util.h>

#include "netmgr-int.h"
#include "uv-compat.h"

#define TLS_BUF_SIZE 65536

static isc_result_t
tls_error_to_result(int tls_err) {
	switch (tls_err) {
	case SSL_ERROR_ZERO_RETURN:
		return (ISC_R_EOF);
	default:
		return (ISC_R_UNEXPECTED);
	}
}

static void
tls_do_bio(isc_nmsocket_t *sock);

static void
tls_close_direct(isc_nmsocket_t *sock);

static void
async_tls_do_bio(isc_nmsocket_t *sock);

/*
 * The socket is closing, outerhandle has been detached, listener is
 * inactive, or the netmgr is closing: any operation on it should abort
 * with ISC_R_CANCELED.
 */
static bool
inactive(isc_nmsocket_t *sock) {
	return (!isc__nmsocket_active(sock) || atomic_load(&sock->closing) ||
		sock->outerhandle == NULL ||
		(sock->listener != NULL &&
		 !isc__nmsocket_active(sock->listener)) ||
		atomic_load(&sock->mgr->closing));
}

static void
update_result(isc_nmsocket_t *sock, const isc_result_t result) {
	LOCK(&sock->lock);
	sock->result = result;
	SIGNAL(&sock->cond);
	if (!atomic_load(&sock->active)) {
		WAIT(&sock->scond, &sock->lock);
	}
	UNLOCK(&sock->lock);
	if (sock->parent) {
		LOCK(&sock->parent->lock);
		sock->parent->result = result;
		UNLOCK(&sock->parent->lock);
	}
}

static void
tls_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *cbarg) {
	isc_nmsocket_tls_send_req_t *send_req =
		(isc_nmsocket_tls_send_req_t *)cbarg;
	isc_nmsocket_t *sock = send_req->tlssock;
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(VALID_NMSOCK(sock));

	/*  XXXWPK TODO */
	UNUSED(eresult);

	isc_mem_put(handle->sock->mgr->mctx, send_req->data.base,
		    send_req->data.length);
	isc_mem_put(handle->sock->mgr->mctx, send_req, sizeof(*send_req));

	sock->tlsstream.nsending--;
	async_tls_do_bio(sock);
	isc__nmsocket_detach(&sock);
}

static void
tls_failed_read_cb(isc_nmsocket_t *sock, isc_nmhandle_t *handle,
		   const isc_result_t result, const bool close) {
	REQUIRE(VALID_NMSOCK(sock));

	if (!sock->tlsstream.server &&
	    (sock->tlsstream.state == TLS_INIT ||
	     sock->tlsstream.state == TLS_HANDSHAKE) &&
	    sock->connect_cb != NULL)
	{
		INSIST(handle == NULL);
		handle = isc__nmhandle_get(sock, NULL, NULL);
		sock->connect_cb(handle, result, sock->connect_cbarg);
		update_result(sock, result);
		isc__nmsocket_clearcb(sock);
		isc_nmhandle_detach(&handle);
	} else if (sock->recv_cb != NULL) {
		isc__nm_uvreq_t *req = NULL;
		req = isc__nm_uvreq_get(sock->mgr, sock);
		req->cb.recv = sock->recv_cb;
		req->cbarg = sock->recv_cbarg;
		req->handle = NULL;
		if (handle) {
			REQUIRE(VALID_NMHANDLE(handle));
			isc_nmhandle_attach(handle, &req->handle);
		} else {
			req->handle = isc__nmhandle_get(sock, NULL, NULL);
		}
		isc__nmsocket_clearcb(sock);
		isc__nm_readcb(sock, req, result);
	}
	sock->tlsstream.state = TLS_ERROR;

	if (close) {
		isc__nmsocket_prep_destroy(sock);
	}
}

static void
async_tls_do_bio(isc_nmsocket_t *sock) {
	isc__netievent_tlsdobio_t *ievent =
		isc__nm_get_netievent_tlsdobio(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

static void
tls_do_bio(isc_nmsocket_t *sock) {
	isc_result_t result = ISC_R_SUCCESS;
	int pending, tls_err = 0;
	int rv;
	isc__nm_uvreq_t *req;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	/* We will resume read if TLS layer wants us to */
	if (sock->outerhandle != NULL) {
		REQUIRE(VALID_NMHANDLE(sock->outerhandle));
		isc_nm_pauseread(sock->outerhandle);
	}

	if (sock->tlsstream.state == TLS_INIT) {
		(void)SSL_do_handshake(sock->tlsstream.ssl);
		sock->tlsstream.state = TLS_HANDSHAKE;
	} else if (sock->tlsstream.state == TLS_ERROR) {
		result = ISC_R_FAILURE;
		goto low_level_error;
	} else if (sock->tlsstream.state == TLS_CLOSED) {
		return;
	}

	/* Data from TLS to client */
	char buf[1];
	if (sock->tlsstream.state == TLS_IO && sock->recv_cb != NULL &&
	    !atomic_load(&sock->readpaused))
	{
		(void)SSL_peek(sock->tlsstream.ssl, buf, 1);
		while ((pending = SSL_pending(sock->tlsstream.ssl)) > 0) {
			if (pending > TLS_BUF_SIZE) {
				pending = TLS_BUF_SIZE;
			}
			isc_region_t region = {
				isc_mem_get(sock->mgr->mctx, pending), pending
			};
			isc_region_t dregion;
			memset(region.base, 0, region.length);
			rv = SSL_read(sock->tlsstream.ssl, region.base,
				      region.length);
			/* Pending succeded, so should read */
			RUNTIME_CHECK(rv == pending);
			dregion = (isc_region_t){ region.base, rv };
			sock->recv_cb(sock->statichandle, ISC_R_SUCCESS,
				      &dregion, sock->recv_cbarg);
			isc_mem_put(sock->mgr->mctx, region.base,
				    region.length);
		}
	}

	/* Peek to move the session forward */
	(void)SSL_peek(sock->tlsstream.ssl, buf, 1);

	/* Data from TLS to network */
	pending = BIO_pending(sock->tlsstream.app_bio);
	if (pending > 0) {
		/*TODO Should we keep the track of these requests in a list? */
		isc_nmsocket_tls_send_req_t *send_req = NULL;
		if (pending > TLS_BUF_SIZE) {
			pending = TLS_BUF_SIZE;
		}
		send_req = isc_mem_get(sock->mgr->mctx, sizeof(*send_req));
		send_req->data.base = isc_mem_get(sock->mgr->mctx, pending);
		send_req->data.length = pending;
		send_req->tlssock = NULL;
		isc__nmsocket_attach(sock, &send_req->tlssock);
		rv = BIO_read(sock->tlsstream.app_bio, send_req->data.base,
			      pending);
		/* There's something pending, read must succeed */
		RUNTIME_CHECK(rv == pending);
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nm_send(sock->outerhandle, &send_req->data, tls_senddone,
			    send_req);
		/* We'll continue in tls_senddone */
		return;
	}

	/* Get the potential error code */
	rv = SSL_peek(sock->tlsstream.ssl, buf, 1);

	if (rv < 0) {
		tls_err = SSL_get_error(sock->tlsstream.ssl, rv);
	}

	/* Only after doing the IO we can check if SSL handshake is done */
	if (sock->tlsstream.state == TLS_HANDSHAKE &&
	    SSL_is_init_finished(sock->tlsstream.ssl) == 1)
	{
		isc_nmhandle_t *tlshandle = isc__nmhandle_get(sock, NULL, NULL);
		if (sock->tlsstream.server) {
			sock->listener->accept_cb(sock->statichandle,
						  ISC_R_SUCCESS,
						  sock->listener->accept_cbarg);
		} else {
			sock->connect_cb(tlshandle, ISC_R_SUCCESS,
					 sock->connect_cbarg);
			update_result(tlshandle->sock, ISC_R_SUCCESS);
		}
		isc_nmhandle_detach(&tlshandle);
		sock->tlsstream.state = TLS_IO;
		async_tls_do_bio(sock);
		return;
	}

	switch (tls_err) {
	case 0:
		return;
	case SSL_ERROR_WANT_WRITE:
		if (sock->tlsstream.nsending == 0) {
			/*
			 * Launch tls_do_bio asynchronously. If we're sending
			 * already the send callback will call it.
			 */
			async_tls_do_bio(sock);
		} else {
			return;
		}
		break;
	case SSL_ERROR_WANT_READ:
		INSIST(VALID_NMHANDLE(sock->outerhandle));
		isc_nm_resumeread(sock->outerhandle);
		break;
	default:
		result = tls_error_to_result(tls_err);
		goto error;
	}

	while ((req = ISC_LIST_HEAD(sock->tlsstream.sends)) != NULL) {
		INSIST(VALID_UVREQ(req));
		rv = SSL_write(sock->tlsstream.ssl, req->uvbuf.base,
			       req->uvbuf.len);
		if (rv < 0) {
			if (sock->tlsstream.nsending == 0) {
				async_tls_do_bio(sock);
			}
			return;
		}
		if (rv != (int)req->uvbuf.len) {
			if (!sock->tlsstream.server &&
			    (sock->tlsstream.state == TLS_HANDSHAKE ||
			     TLS_INIT))
			{
				isc_nmhandle_t *tlshandle =
					isc__nmhandle_get(sock, NULL, NULL);
				sock->connect_cb(tlshandle, result,
						 sock->connect_cbarg);
				update_result(tlshandle->sock, result);
				isc_nmhandle_detach(&tlshandle);
			}
			sock->tlsstream.state = TLS_ERROR;
			async_tls_do_bio(sock);
			return;
		}
		ISC_LIST_UNLINK(sock->tlsstream.sends, req, link);
		req->cb.send(sock->statichandle, ISC_R_SUCCESS, req->cbarg);
		isc__nm_uvreq_put(&req, sock);
	}

	return;

error:
	isc_log_write(isc_lctx, ISC_LOGCATEGORY_GENERAL, ISC_LOGMODULE_NETMGR,
		      ISC_LOG_ERROR, "SSL error in BIO: %d %s", tls_err,
		      isc_result_totext(result));
low_level_error:
	if (sock->tlsstream.state == TLS_HANDSHAKE) {
		isc_nmhandle_t *tlshandle = isc__nmhandle_get(sock, NULL, NULL);
		if (!sock->tlsstream.server) {
			sock->connect_cb(tlshandle, result,
					 sock->connect_cbarg);
			update_result(tlshandle->sock, result);
		}
		isc_nmhandle_detach(&tlshandle);
	} else if (sock->tlsstream.state == TLS_IO) {
		if (ISC_LIST_HEAD(sock->tlsstream.sends) != NULL) {
			while ((req = ISC_LIST_HEAD(sock->tlsstream.sends)) !=
			       NULL) {
				req->cb.send(sock->statichandle, result,
					     req->cbarg);
				ISC_LIST_UNLINK(sock->tlsstream.sends, req,
						link);
				isc__nm_uvreq_put(&req, sock);
			}
		} else if (sock->recv_cb != NULL) {
			tls_failed_read_cb(sock, sock->statichandle, result,
					   false);
		} else {
			tls_close_direct(sock);
		}
	}
	sock->tlsstream.state = TLS_ERROR;
}

static void
tls_readcb(isc_nmhandle_t *handle, isc_result_t result, isc_region_t *region,
	   void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;
	int rv;

	REQUIRE(VALID_NMSOCK(tlssock));
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(tlssock->tid == isc_nm_tid());
	if (result != ISC_R_SUCCESS) {
		tls_failed_read_cb(tlssock, tlssock->statichandle, result,
				   true);
		return;
	}
	rv = BIO_write(tlssock->tlsstream.app_bio, region->base,
		       region->length);

	if (rv != (int)region->length) {
		/* XXXWPK log it? */
		tlssock->tlsstream.state = TLS_ERROR;
	}
	tls_do_bio(tlssock);
}

static isc_result_t
initialize_tls(isc_nmsocket_t *sock, bool server) {
	REQUIRE(sock->tid == isc_nm_tid());

	if (BIO_new_bio_pair(&(sock->tlsstream.ssl_bio), TLS_BUF_SIZE,
			     &(sock->tlsstream.app_bio), TLS_BUF_SIZE) != 1)
	{
		SSL_free(sock->tlsstream.ssl);
		return (ISC_R_TLSERROR);
	}

	SSL_set_bio(sock->tlsstream.ssl, sock->tlsstream.ssl_bio,
		    sock->tlsstream.ssl_bio);
	if (server) {
		SSL_set_accept_state(sock->tlsstream.ssl);
	} else {
		SSL_set_connect_state(sock->tlsstream.ssl);
	}
	sock->tlsstream.nsending = 0;
	isc_nm_read(sock->outerhandle, tls_readcb, sock);
	tls_do_bio(sock);
	return (ISC_R_SUCCESS);
}

static isc_result_t
tlslisten_acceptcb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *tlslistensock = (isc_nmsocket_t *)cbarg;
	isc_nmsocket_t *tlssock = NULL;
	int r;

	/* If accept() was unsuccessful we can't do anything */
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(VALID_NMSOCK(tlslistensock));
	REQUIRE(tlslistensock->type == isc_nm_tlslistener);

	/*
	 * We need to create a 'wrapper' tlssocket for this connection.
	 */
	tlssock = isc_mem_get(handle->sock->mgr->mctx, sizeof(*tlssock));
	isc__nmsocket_init(tlssock, handle->sock->mgr, isc_nm_tlssocket,
			   handle->sock->iface);

	/* We need to initialize SSL now to reference SSL_CTX properly */
	tlssock->tlsstream.ctx = tlslistensock->tlsstream.ctx;
	tlssock->tlsstream.ssl = SSL_new(tlssock->tlsstream.ctx);
	ISC_LIST_INIT(tlssock->tlsstream.sends);
	if (tlssock->tlsstream.ssl == NULL) {
		update_result(tlssock, ISC_R_TLSERROR);
		atomic_store(&tlssock->closed, true);
		isc__nmsocket_detach(&tlssock);
		return (ISC_R_TLSERROR);
	}

	tlssock->extrahandlesize = tlslistensock->extrahandlesize;
	isc__nmsocket_attach(tlslistensock, &tlssock->listener);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	tlssock->peer = handle->sock->peer;
	tlssock->read_timeout = atomic_load(&handle->sock->mgr->init);
	tlssock->tid = isc_nm_tid();
	tlssock->tlsstream.server = true;
	tlssock->tlsstream.state = TLS_INIT;

	r = uv_timer_init(&tlssock->mgr->workers[isc_nm_tid()].loop,
			  &tlssock->timer);
	RUNTIME_CHECK(r == 0);

	tlssock->timer.data = tlssock;
	tlssock->timer_initialized = true;
	tlssock->tlsstream.ctx = tlslistensock->tlsstream.ctx;

	result = initialize_tls(tlssock, true);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	/* TODO: catch failure code, detach tlssock, and log the error */

	return (result);
}

isc_result_t
isc_nm_listentls(isc_nm_t *mgr, isc_nmiface_t *iface,
		 isc_nm_accept_cb_t accept_cb, void *accept_cbarg,
		 size_t extrahandlesize, int backlog, isc_quota_t *quota,
		 SSL_CTX *sslctx, isc_nmsocket_t **sockp) {
	isc_result_t result;
	isc_nmsocket_t *tlssock = isc_mem_get(mgr->mctx, sizeof(*tlssock));
	isc_nmsocket_t *tsock = NULL;

	REQUIRE(VALID_NM(mgr));

	isc__nmsocket_init(tlssock, mgr, isc_nm_tlslistener, iface);
	tlssock->result = ISC_R_DEFAULT;
	tlssock->accept_cb = accept_cb;
	tlssock->accept_cbarg = accept_cbarg;
	tlssock->extrahandlesize = extrahandlesize;
	tlssock->tlsstream.ctx = sslctx;
	tlssock->tlsstream.ssl = NULL;

	/*
	 * tlssock will be a TLS 'wrapper' around an unencrypted stream.
	 * We set tlssock->outer to a socket listening for a TCP connection.
	 */
	result = isc_nm_listentcp(mgr, iface, tlslisten_acceptcb, tlssock,
				  extrahandlesize, backlog, quota,
				  &tlssock->outer);
	if (result != ISC_R_SUCCESS) {
		atomic_store(&tlssock->closed, true);
		isc__nmsocket_detach(&tlssock);
		return (result);
	}

	/* wait for listen result */
	isc__nmsocket_attach(tlssock->outer, &tsock);
	LOCK(&tlssock->outer->lock);
	while (tlssock->outer->rchildren != tlssock->outer->nchildren) {
		WAIT(&tlssock->outer->cond, &tlssock->outer->lock);
	}
	result = tlssock->outer->result;
	tlssock->result = result;
	atomic_store(&tlssock->active, true);
	INSIST(tlssock->outer->tlsstream.tlslistener == NULL);
	isc__nmsocket_attach(tlssock, &tlssock->outer->tlsstream.tlslistener);
	BROADCAST(&tlssock->outer->scond);
	UNLOCK(&tlssock->outer->lock);
	isc__nmsocket_detach(&tsock);
	INSIST(result != ISC_R_DEFAULT);

	if (result == ISC_R_SUCCESS) {
		atomic_store(&tlssock->listening, true);
		*sockp = tlssock;
	}

	return (result);
}

void
isc__nm_async_tlssend(isc__networker_t *worker, isc__netievent_t *ev0) {
	int rv;
	isc__netievent_tlssend_t *ievent = (isc__netievent_tlssend_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc__nm_uvreq_t *req = ievent->req;
	ievent->req = NULL;
	REQUIRE(VALID_UVREQ(req));
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	if (inactive(sock)) {
		req->cb.send(req->handle, ISC_R_CANCELED, req->cbarg);
		isc__nm_uvreq_put(&req, sock);
		return;
	}
	if (!ISC_LIST_EMPTY(sock->tlsstream.sends)) {
		/* We're not the first */
		ISC_LIST_APPEND(sock->tlsstream.sends, req, link);
		tls_do_bio(sock);
		return;
	}

	rv = SSL_write(sock->tlsstream.ssl, req->uvbuf.base, req->uvbuf.len);
	if (rv < 0) {
		/*
		 * We might need to read, we might need to write, or the
		 * TLS socket might be dead - in any case, we need to
		 * enqueue the uvreq and let the TLS BIO layer do the rest.
		 */
		ISC_LIST_APPEND(sock->tlsstream.sends, req, link);
		tls_do_bio(sock);
		return;
	}
	if (rv != (int)req->uvbuf.len) {
		sock->tlsstream.state = TLS_ERROR;
		async_tls_do_bio(sock);
		return;
	}
	req->cb.send(sock->statichandle, ISC_R_SUCCESS, req->cbarg);
	isc__nm_uvreq_put(&req, sock);
	tls_do_bio(sock);
	return;
}

void
isc__nm_tls_send(isc_nmhandle_t *handle, const isc_region_t *region,
		 isc_nm_cb_t cb, void *cbarg) {
	isc__netievent_tlssend_t *ievent = NULL;
	isc__nm_uvreq_t *uvreq = NULL;
	isc_nmsocket_t *sock = NULL;

	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_tlssocket);

	if (inactive(sock)) {
		cb(handle, ISC_R_CANCELED, cbarg);
		return;
	}

	uvreq = isc__nm_uvreq_get(sock->mgr, sock);
	isc_nmhandle_attach(handle, &uvreq->handle);
	uvreq->cb.send = cb;
	uvreq->cbarg = cbarg;

	uvreq->uvbuf.base = (char *)region->base;
	uvreq->uvbuf.len = region->length;

	/*
	 * We need to create an event and pass it using async channel
	 */
	ievent = isc__nm_get_netievent_tlssend(sock->mgr, sock, uvreq);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlsstartread(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsstartread_t *ievent =
		(isc__netievent_tlsstartread_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;

	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	tls_do_bio(sock);
}

void
isc__nm_tls_read(isc_nmhandle_t *handle, isc_nm_recv_cb_t cb, void *cbarg) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	REQUIRE(handle->sock->statichandle == handle);
	REQUIRE(handle->sock->tid == isc_nm_tid());

	isc__netievent_tlsstartread_t *ievent = NULL;
	isc_nmsocket_t *sock = handle->sock;

	if (inactive(sock)) {
		cb(handle, ISC_R_NOTCONNECTED, NULL, cbarg);
		return;
	}

	sock->recv_cb = cb;
	sock->recv_cbarg = cbarg;

	ievent = isc__nm_get_netievent_tlsstartread(sock->mgr, sock);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_tls_pauseread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	isc_nmsocket_t *sock = handle->sock;

	atomic_store(&sock->readpaused, true);
}

void
isc__nm_tls_resumeread(isc_nmhandle_t *handle) {
	REQUIRE(VALID_NMHANDLE(handle));
	REQUIRE(VALID_NMSOCK(handle->sock));
	isc_nmsocket_t *sock = handle->sock;

	atomic_store(&sock->readpaused, false);
	async_tls_do_bio(sock);
}

static void
timer_close_cb(uv_handle_t *handle) {
	isc_nmsocket_t *sock = (isc_nmsocket_t *)uv_handle_get_data(handle);
	tls_close_direct(sock);
}

static void
tls_close_direct(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->tid == isc_nm_tid());

	/* if (!sock->tlsstream.server) { */
	/* 	INSIST(sock->tlsstream.state != TLS_HANDSHAKE && */
	/* 	       sock->tlsstream.state != TLS_INIT); */
	/* } */

	sock->tlsstream.state = TLS_CLOSING;

	if (sock->timer_running) {
		uv_timer_stop(&sock->timer);
		sock->timer_running = false;
	}

	/* We don't need atomics here, it's all in single network thread
	 */
	if (sock->timer_initialized) {
		/*
		 * We need to fire the timer callback to clean it up,
		 * it will then call us again (via detach) so that we
		 * can finally close the socket.
		 */
		sock->timer_initialized = false;
		uv_timer_stop(&sock->timer);
		uv_close((uv_handle_t *)&sock->timer, timer_close_cb);
	} else {
		/*
		 * At this point we're certain that there are no
		 * external references, we can close everything.
		 */
		if (sock->outerhandle != NULL) {
			isc_nm_pauseread(sock->outerhandle);
			isc_nmhandle_detach(&sock->outerhandle);
		}
		if (sock->listener != NULL) {
			isc__nmsocket_detach(&sock->listener);
		}
		if (sock->tlsstream.ssl != NULL) {
			SSL_free(sock->tlsstream.ssl);
			sock->tlsstream.ssl = NULL;
			/* These are destroyed when we free SSL* */
			sock->tlsstream.ctx = NULL;
			sock->tlsstream.ssl_bio = NULL;
		}
		if (sock->tlsstream.app_bio != NULL) {
			BIO_free(sock->tlsstream.app_bio);
			sock->tlsstream.app_bio = NULL;
		}
		sock->tlsstream.state = TLS_CLOSED;
		atomic_store(&sock->closed, true);
		isc__nmsocket_detach(&sock);
	}
}

void
isc__nm_tls_close(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlssocket);

	if (!atomic_compare_exchange_strong(&sock->closing, &(bool){ false },
					    true)) {
		return;
	}

	if (sock->tid == isc_nm_tid()) {
		tls_close_direct(sock);
	} else {
		isc__netievent_tlsclose_t *ievent =
			isc__nm_get_netievent_tlsclose(sock->mgr, sock);
		isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
				       (isc__netievent_t *)ievent);
	}
}

void
isc__nm_async_tlsclose(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsclose_t *ievent = (isc__netievent_tlsclose_t *)ev0;

	REQUIRE(ievent->sock->tid == isc_nm_tid());
	UNUSED(worker);

	tls_close_direct(ievent->sock);
}

void
isc__nm_tls_stoplistening(isc_nmsocket_t *sock) {
	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(sock->type == isc_nm_tlslistener);

	atomic_store(&sock->listening, false);
	atomic_store(&sock->closed, true);
	sock->recv_cb = NULL;
	sock->recv_cbarg = NULL;
	if (sock->tlsstream.ssl != NULL) {
		SSL_free(sock->tlsstream.ssl);
		sock->tlsstream.ssl = NULL;
		sock->tlsstream.ctx = NULL;
	}

	if (sock->outer != NULL) {
		isc_nm_stoplistening(sock->outer);
		isc__nmsocket_detach(&sock->outer);
	}
}

isc_result_t
isc_nm_tlsconnect(isc_nm_t *mgr, isc_nmiface_t *local, isc_nmiface_t *peer,
		  isc_nm_cb_t cb, void *cbarg, SSL_CTX *ctx,
		  unsigned int timeout, size_t extrahandlesize) {
	isc_nmsocket_t *nsock = NULL, *tsock = NULL;
	isc__netievent_tlsconnect_t *ievent = NULL;
	isc_result_t result = ISC_R_DEFAULT;

	REQUIRE(VALID_NM(mgr));

	nsock = isc_mem_get(mgr->mctx, sizeof(*nsock));
	isc__nmsocket_init(nsock, mgr, isc_nm_tlssocket, local);
	nsock->extrahandlesize = extrahandlesize;
	nsock->result = ISC_R_DEFAULT;
	nsock->connect_cb = cb;
	nsock->connect_cbarg = cbarg;
	nsock->connect_timeout = timeout;
	nsock->tlsstream.ctx = ctx;

	ievent = isc__nm_get_netievent_tlsconnect(mgr, nsock);
	ievent->local = local->addr;
	ievent->peer = peer->addr;
	ievent->ctx = ctx;

	isc__nmsocket_attach(nsock, &tsock);
	if (isc__nm_in_netthread()) {
		nsock->tid = isc_nm_tid();
		isc__nm_async_tlsconnect(&mgr->workers[nsock->tid],
					 (isc__netievent_t *)ievent);
		isc__nm_put_netievent_tlsconnect(mgr, ievent);
	} else {
		nsock->tid = isc_random_uniform(mgr->nworkers);
		isc__nm_enqueue_ievent(&mgr->workers[nsock->tid],
				       (isc__netievent_t *)ievent);
	}

	LOCK(&nsock->lock);
	result = nsock->result;
	while (result == ISC_R_DEFAULT) {
		WAIT(&nsock->cond, &nsock->lock);
		result = nsock->result;
	}
	atomic_store(&nsock->active, true);
	BROADCAST(&nsock->scond);
	UNLOCK(&nsock->lock);
	INSIST(VALID_NMSOCK(nsock));
	isc__nmsocket_detach(&tsock);

	INSIST(result != ISC_R_DEFAULT);

	return (result);
}

static void
tls_connect_cb(isc_nmhandle_t *handle, isc_result_t result, void *cbarg) {
	isc_nmsocket_t *tlssock = (isc_nmsocket_t *)cbarg;

	REQUIRE(VALID_NMSOCK(tlssock));

	if (result != ISC_R_SUCCESS) {
		tlssock->connect_cb(handle, result, tlssock->connect_cbarg);
		update_result(tlssock, result);
		tls_close_direct(tlssock);
		return;
	}

	INSIST(VALID_NMHANDLE(handle));

	tlssock->peer = isc_nmhandle_peeraddr(handle);
	isc_nmhandle_attach(handle, &tlssock->outerhandle);
	result = initialize_tls(tlssock, false);
	if (result != ISC_R_SUCCESS) {
		tlssock->connect_cb(handle, result, tlssock->connect_cbarg);
		update_result(tlssock, result);
		tls_close_direct(tlssock);
		return;
	}
}

void
isc__nm_async_tlsconnect(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlsconnect_t *ievent =
		(isc__netievent_tlsconnect_t *)ev0;
	isc_nmsocket_t *tlssock = ievent->sock;
	isc_result_t result;
	int r;
	isc_nmhandle_t *tlshandle = NULL;

	UNUSED(worker);

	/*
	 * We need to initialize SSL now to reference SSL_CTX properly.
	 */
	tlssock->tlsstream.ssl = SSL_new(tlssock->tlsstream.ctx);
	if (tlssock->tlsstream.ssl == NULL) {
		result = ISC_R_TLSERROR;
		goto error;
	}

	tlssock->tid = isc_nm_tid();
	r = uv_timer_init(&tlssock->mgr->workers[isc_nm_tid()].loop,
			  &tlssock->timer);
	RUNTIME_CHECK(r == 0);

	tlssock->timer.data = tlssock;
	tlssock->timer_initialized = true;
	tlssock->tlsstream.state = TLS_INIT;

	result = isc_nm_tcpconnect(worker->mgr, (isc_nmiface_t *)&ievent->local,
				   (isc_nmiface_t *)&ievent->peer,
				   tls_connect_cb, tlssock,
				   tlssock->connect_timeout, 0);
	if (result != ISC_R_SUCCESS) {
		goto error;
	}
	return;
error:
	tlshandle = isc__nmhandle_get(tlssock, NULL, NULL);
	atomic_store(&tlssock->closed, true);
	tlssock->connect_cb(tlshandle, result, tlssock->connect_cbarg);
	isc_nmhandle_detach(&tlshandle);
	update_result(tlssock, result);
	tls_close_direct(tlssock);
}

void
isc__nm_tls_cancelread(isc_nmhandle_t *handle) {
	isc_nmsocket_t *sock = NULL;
	isc__netievent_tlscancel_t *ievent = NULL;

	REQUIRE(VALID_NMHANDLE(handle));

	sock = handle->sock;

	REQUIRE(sock->type == isc_nm_tlssocket);

	ievent = isc__nm_get_netievent_tlscancel(sock->mgr, sock, handle);
	isc__nm_enqueue_ievent(&sock->mgr->workers[sock->tid],
			       (isc__netievent_t *)ievent);
}

void
isc__nm_async_tlscancel(isc__networker_t *worker, isc__netievent_t *ev0) {
	isc__netievent_tlscancel_t *ievent = (isc__netievent_tlscancel_t *)ev0;
	isc_nmsocket_t *sock = ievent->sock;
	isc_nmhandle_t *handle = ievent->handle;

	REQUIRE(VALID_NMSOCK(sock));
	REQUIRE(worker->id == sock->tid);
	REQUIRE(sock->tid == isc_nm_tid());
	UNUSED(worker);

	tls_failed_read_cb(sock, handle, ISC_R_EOF, false);

	if (sock->outerhandle) {
		isc__nm_tcp_cancelread(sock->outerhandle);
	}
}

void
isc__nm_async_tlsdobio(isc__networker_t *worker, isc__netievent_t *ev0) {
	UNUSED(worker);
	isc__netievent_tlsdobio_t *ievent = (isc__netievent_tlsdobio_t *)ev0;
	tls_do_bio(ievent->sock);
}

void
isc__nm_tls_cleanup_data(isc_nmsocket_t *sock) {
	if (sock->tlsstream.tlslistener) {
		REQUIRE(VALID_NMSOCK(sock->tlsstream.tlslistener));
		isc__nmsocket_detach(&sock->tlsstream.tlslistener);
	}
}
