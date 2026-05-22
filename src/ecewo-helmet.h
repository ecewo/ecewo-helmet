// Copyright 2025-2026 Savas Sahin <savashn@proton.me>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef ECEWO_HELMET_H
#define ECEWO_HELMET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "ecewo.h"
#include "ecewo-helmet-export.h"

/**
 * Opaque helmet configuration builder.
 *
 * Created with ecewo_helmet_config_new(); customize via the
 * ecewo_helmet_config_set_*() setters; then either install it on an app via
 * ecewo_helmet_install(), or discard it via ecewo_helmet_config_free(). Once
 * installed, the configuration is consumed and the caller must NOT free it -
 * its lifetime is taken over by the app.
 *
 * The struct definition is intentionally private so that fields can be added or
 * changed without breaking the plugin's ABI.
 */
typedef struct ecewo_helmet_config_s ecewo_helmet_config_t;

// ---------------------------------------------------------------------------
// CONFIGURATION BUILDER
// ---------------------------------------------------------------------------

/** Allocate a new configuration pre-populated with secure defaults:
 *  CSP "default-src 'self'", HSTS max-age 31536000 (no subdomains/preload),
 *  X-Frame-Options "SAMEORIGIN", Referrer-Policy
 *  "strict-origin-when-cross-origin", X-XSS-Protection "0", nosniff on,
 *  ie_no_open on. Returns NULL on allocation failure. The caller owns the
 *  handle until it is passed to ecewo_helmet_install(). */
ECEWO_HELMET_EXPORT ecewo_helmet_config_t *ecewo_helmet_config_new(void);

/** Free a configuration that has not yet been installed.
 *  After ecewo_helmet_install() the configuration is owned by the app and must
 *  not be freed by the caller. */
ECEWO_HELMET_EXPORT void ecewo_helmet_config_free(ecewo_helmet_config_t *config);

/** Set the Content-Security-Policy header value. Pass NULL to disable the
 *  header. The string is copied. Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_config_set_csp(ecewo_helmet_config_t *config, const char *csp);

/** Set the Strict-Transport-Security parameters. `max_age` is the value in
 *  seconds as a string (e.g. "31536000"); pass NULL to disable the header.
 *  The string is copied. Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_config_set_hsts(ecewo_helmet_config_t *config,
                                                     const char *max_age,
                                                     bool include_subdomains,
                                                     bool preload);

/** Set the X-Frame-Options header value (e.g. "DENY", "SAMEORIGIN").
 *  Pass NULL to disable the header. The string is copied.
 *  Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_config_set_frame_options(ecewo_helmet_config_t *config, const char *frame_options);

/** Set the Referrer-Policy header value. Pass NULL to disable the header.
 *  The string is copied. Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_config_set_referrer_policy(ecewo_helmet_config_t *config, const char *referrer_policy);

/** Set the X-XSS-Protection header value (e.g. "0", "1; mode=block").
 *  Pass NULL to disable the header. The string is copied.
 *  Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_config_set_xss_protection(ecewo_helmet_config_t *config, const char *xss_protection);

/** Enable or disable the X-Content-Type-Options: nosniff header. */
ECEWO_HELMET_EXPORT void ecewo_helmet_config_set_nosniff(ecewo_helmet_config_t *config, bool enabled);

/** Enable or disable the X-Download-Options: noopen header. */
ECEWO_HELMET_EXPORT void ecewo_helmet_config_set_ie_no_open(ecewo_helmet_config_t *config, bool enabled);

// ---------------------------------------------------------------------------
// INSTALLATION
// ---------------------------------------------------------------------------

/** Install the configuration on `app` and register the helmet middleware
 *  globally. On success the configuration is consumed - its values are copied
 *  into the app arena and the handle must not be freed or accessed again. On
 *  failure the configuration is also freed; do not use it again.
 *
 *  Each app may have at most one helmet installation; a second call returns -1.
 *
 *  When `config` is NULL, secure defaults are used (see ecewo_helmet_config_new).
 *  Returns 0 on success, -1 on error. */
ECEWO_HELMET_EXPORT int ecewo_helmet_install(ecewo_app_t *app, ecewo_helmet_config_t *config);

#ifdef __cplusplus
}
#endif

#endif
