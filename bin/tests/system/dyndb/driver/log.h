/*
 * Copyright (C) 2009--2015  Red Hat ; see COPYRIGHT for license
 */

#pragma once

#include <isc/error.h>
#include <isc/result.h>

#include <dns/log.h>

#define fatal_error(...) isc_error_fatal(__FILE__, __LINE__, __VA_ARGS__)

#define log_error_r(fmt, ...) \
	log_error(fmt ": %s", ##__VA_ARGS__, isc_result_totext(result))

#define log_error(format, ...) log_write(ISC_LOG_ERROR, format, ##__VA_ARGS__)

#define log_info(format, ...) log_write(ISC_LOG_INFO, format, ##__VA_ARGS__)

void
log_write(int level, const char *format, ...) ISC_FORMAT_PRINTF(2, 3);
