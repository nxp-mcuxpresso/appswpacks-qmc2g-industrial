/*
 *
 * Copyright 2018 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NX_LOG_DEFAULT_CONFIG_H
#define NX_LOG_DEFAULT_CONFIG_H

#include "qmc_features_config.h"

#if defined(FEATURE_SE_DEBUG_ENABLED) && (FEATURE_SE_DEBUG_ENABLED == 0)
#   ifndef FLOW_SILENT
#      define FLOW_SILENT
#   endif
#endif

/* See Plug & Trust Middleware Docuemntation --> stack --> Logging
   for more information */

/*
 * - 1 => Enable Debug level logging - for all.
 * - 0 => Disable Debug level logging.  This has to be
 *        enabled individually by other logging
 *        header/source files */
#define NX_LOG_ENABLE_DEFAULT_DEBUG 0

/* Same as NX_LOG_ENABLE_DEFAULT_DEBUG but for Info Level */
#define NX_LOG_ENABLE_DEFAULT_INFO 1

/* Same as NX_LOG_ENABLE_DEFAULT_DEBUG but for Warn Level */
#define NX_LOG_ENABLE_DEFAULT_WARN 1

/* Same as NX_LOG_ENABLE_DEFAULT_DEBUG but for Error Level.
 * Ideally, this shoudl alwasy be kept enabled */
#define NX_LOG_ENABLE_DEFAULT_ERROR 1


/* Release - retail build */
#ifdef FLOW_SILENT
#undef NX_LOG_ENABLE_DEFAULT_DEBUG
#undef NX_LOG_ENABLE_DEFAULT_INFO
#undef NX_LOG_ENABLE_DEFAULT_WARN
#undef NX_LOG_ENABLE_DEFAULT_ERROR

#define NX_LOG_ENABLE_DEFAULT_DEBUG 0
#define NX_LOG_ENABLE_DEFAULT_INFO 0
#define NX_LOG_ENABLE_DEFAULT_WARN 0
#define NX_LOG_ENABLE_DEFAULT_ERROR 0
#endif

#endif /* NX_LOG_DEFAULT_CONFIG_H */
