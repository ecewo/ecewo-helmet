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

#include "ecewo-helmet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HELMET_DEFAULT_CSP "default-src 'self'"
#define HELMET_DEFAULT_HSTS_MAX_AGE "31536000"
#define HELMET_DEFAULT_FRAME_OPTIONS "SAMEORIGIN"
#define HELMET_DEFAULT_REFERRER_POLICY "strict-origin-when-cross-origin"
#define HELMET_DEFAULT_XSS_PROTECTION "0"

// ---------------------------------------------------------------------------
// Configuration builder (heap-backed; lifetime ends at install or free).
// A NULL string slot means the corresponding header is disabled.
// ---------------------------------------------------------------------------

struct ecewo_helmet_config_s {
  char *csp;
  char *hsts_max_age;
  bool hsts_subdomains;
  bool hsts_preload;
  char *frame_options;
  char *referrer_policy;
  char *xss_protection;
  bool nosniff;
  bool ie_no_open;
};

// ---------------------------------------------------------------------------
// Installed state (lives in the app arena, owned by the app).
// All header values are pre-composed so the middleware allocates nothing.
// ---------------------------------------------------------------------------

typedef struct helmet_state_s {
  const char *csp;
  const char *hsts; // fully composed value, or NULL when disabled
  const char *frame_options;
  const char *referrer_policy;
  const char *xss_protection;
  bool nosniff;
  bool ie_no_open;
} helmet_state_t;

// Address of this static is used as the per-app data key. Using a file-static
// guarantees uniqueness across plugins without coordination.
static int helmet_state_key;

static helmet_state_t *helmet_state_for(const ecewo_app_t *app) {
  if (!app)
    return NULL;
  return (helmet_state_t *)ecewo_get_app_data(app, &helmet_state_key);
}

// ---------------------------------------------------------------------------
// Builder helpers
// ---------------------------------------------------------------------------

static char *xstrdup(const char *s) {
  if (!s)
    return NULL;
  size_t n = strlen(s);
  char *out = malloc(n + 1);
  if (!out)
    return NULL;
  memcpy(out, s, n + 1);
  return out;
}

// Replace *slot with a copy of value (or NULL). Returns -1 on allocation
// failure, leaving the old value untouched.
static int replace_str(char **slot, const char *value) {
  if (!value) {
    free(*slot);
    *slot = NULL;
    return 0;
  }
  char *copy = xstrdup(value);
  if (!copy)
    return -1;
  free(*slot);
  *slot = copy;
  return 0;
}

ecewo_helmet_config_t *ecewo_helmet_config_new(void) {
  ecewo_helmet_config_t *c = calloc(1, sizeof(*c));
  if (!c)
    return NULL;

  c->csp = xstrdup(HELMET_DEFAULT_CSP);
  c->hsts_max_age = xstrdup(HELMET_DEFAULT_HSTS_MAX_AGE);
  c->frame_options = xstrdup(HELMET_DEFAULT_FRAME_OPTIONS);
  c->referrer_policy = xstrdup(HELMET_DEFAULT_REFERRER_POLICY);
  c->xss_protection = xstrdup(HELMET_DEFAULT_XSS_PROTECTION);
  c->hsts_subdomains = false;
  c->hsts_preload = false;
  c->nosniff = true;
  c->ie_no_open = true;

  if (!c->csp || !c->hsts_max_age || !c->frame_options || !c->referrer_policy || !c->xss_protection) {
    ecewo_helmet_config_free(c);
    return NULL;
  }

  return c;
}

void ecewo_helmet_config_free(ecewo_helmet_config_t *config) {
  if (!config)
    return;
  free(config->csp);
  free(config->hsts_max_age);
  free(config->frame_options);
  free(config->referrer_policy);
  free(config->xss_protection);
  free(config);
}

int ecewo_helmet_config_set_csp(ecewo_helmet_config_t *config, const char *csp) {
  if (!config)
    return -1;
  return replace_str(&config->csp, csp);
}

int ecewo_helmet_config_set_hsts(ecewo_helmet_config_t *config,
                                 const char *max_age,
                                 bool include_subdomains,
                                 bool preload) {
  if (!config)
    return -1;
  if (replace_str(&config->hsts_max_age, max_age) != 0)
    return -1;
  config->hsts_subdomains = include_subdomains;
  config->hsts_preload = preload;
  return 0;
}

int ecewo_helmet_config_set_frame_options(ecewo_helmet_config_t *config, const char *frame_options) {
  if (!config)
    return -1;
  return replace_str(&config->frame_options, frame_options);
}

int ecewo_helmet_config_set_referrer_policy(ecewo_helmet_config_t *config, const char *referrer_policy) {
  if (!config)
    return -1;
  return replace_str(&config->referrer_policy, referrer_policy);
}

int ecewo_helmet_config_set_xss_protection(ecewo_helmet_config_t *config, const char *xss_protection) {
  if (!config)
    return -1;
  return replace_str(&config->xss_protection, xss_protection);
}

void ecewo_helmet_config_set_nosniff(ecewo_helmet_config_t *config, bool enabled) {
  if (!config)
    return;
  config->nosniff = enabled;
}

void ecewo_helmet_config_set_ie_no_open(ecewo_helmet_config_t *config, bool enabled) {
  if (!config)
    return;
  config->ie_no_open = enabled;
}

// ---------------------------------------------------------------------------
// Middleware
// ---------------------------------------------------------------------------

static void helmet_middleware(ecewo_request_t *req, ecewo_response_t *res, ecewo_next_t next) {
  ecewo_app_t *app = ecewo_req_app(req);
  const helmet_state_t *state = helmet_state_for(app);

  if (!state) {
    next(req, res);
    return;
  }

  if (state->csp)
    ecewo_header_set(res, "Content-Security-Policy", state->csp);

  if (state->hsts)
    ecewo_header_set(res, "Strict-Transport-Security", state->hsts);

  if (state->frame_options)
    ecewo_header_set(res, "X-Frame-Options", state->frame_options);

  if (state->nosniff)
    ecewo_header_set(res, "X-Content-Type-Options", "nosniff");

  if (state->xss_protection)
    ecewo_header_set(res, "X-XSS-Protection", state->xss_protection);

  if (state->referrer_policy)
    ecewo_header_set(res, "Referrer-Policy", state->referrer_policy);

  if (state->ie_no_open)
    ecewo_header_set(res, "X-Download-Options", "noopen");

  next(req, res);
}

// ---------------------------------------------------------------------------
// Install
// ---------------------------------------------------------------------------

int ecewo_helmet_install(ecewo_app_t *app, ecewo_helmet_config_t *config) {
  if (!config) {
    config = ecewo_helmet_config_new();
    if (!config)
      return -1;
  }

  if (!app) {
    ecewo_helmet_config_free(config);
    return -1;
  }

  if (helmet_state_for(app)) {
    fprintf(stderr, "[ecewo-helmet] helmet is already installed on this app\n");
    ecewo_helmet_config_free(config);
    return -1;
  }

  ecewo_arena_t *arena = ecewo_app_arena(app);
  if (!arena) {
    ecewo_helmet_config_free(config);
    return -1;
  }

  helmet_state_t *state = ecewo_alloc(arena, sizeof(*state));
  if (!state) {
    ecewo_helmet_config_free(config);
    return -1;
  }
  memset(state, 0, sizeof(*state));

  state->nosniff = config->nosniff;
  state->ie_no_open = config->ie_no_open;
  state->csp = config->csp ? ecewo_strdup(arena, config->csp) : NULL;
  state->frame_options = config->frame_options ? ecewo_strdup(arena, config->frame_options) : NULL;
  state->referrer_policy = config->referrer_policy ? ecewo_strdup(arena, config->referrer_policy) : NULL;
  state->xss_protection = config->xss_protection ? ecewo_strdup(arena, config->xss_protection) : NULL;

  if ((config->csp && !state->csp) || (config->frame_options && !state->frame_options) || (config->referrer_policy && !state->referrer_policy) || (config->xss_protection && !state->xss_protection)) {
    fprintf(stderr, "[ecewo-helmet] arena allocation failed during install\n");
    ecewo_helmet_config_free(config);
    return -1;
  }

  // Pre-compose the HSTS value so the middleware allocates nothing per request.
  if (config->hsts_max_age) {
    state->hsts = ecewo_sprintf(arena, "max-age=%s%s%s",
                                config->hsts_max_age,
                                config->hsts_subdomains ? "; includeSubDomains" : "",
                                config->hsts_preload ? "; preload" : "");
    if (!state->hsts) {
      fprintf(stderr, "[ecewo-helmet] arena allocation failed during install\n");
      ecewo_helmet_config_free(config);
      return -1;
    }
  }

  ecewo_set_app_data(app, &helmet_state_key, state);

  if (ecewo_use(app, NULL, helmet_middleware) != 0) {
    ecewo_helmet_config_free(config);
    return -1;
  }

  ecewo_helmet_config_free(config);
  return 0;
}
