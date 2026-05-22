#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-helmet.h"
#include "tester.h"
#include <string.h>

static void handler_secure(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_send_text(res, 200, "Helmet OK");
}

int test_custom_headers(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/secure"
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Helmet OK", res.body);

  ASSERT_EQ_STR("default-src 'none'", mock_get_header(&res, "Content-Security-Policy"));
  ASSERT_EQ_STR("max-age=63072000; includeSubDomains; preload",
                mock_get_header(&res, "Strict-Transport-Security"));
  ASSERT_EQ_STR("DENY", mock_get_header(&res, "X-Frame-Options"));
  ASSERT_EQ_STR("no-referrer", mock_get_header(&res, "Referrer-Policy"));
  ASSERT_EQ_STR("1; mode=block", mock_get_header(&res, "X-XSS-Protection"));
  ASSERT_EQ_STR("nosniff", mock_get_header(&res, "X-Content-Type-Options"));

  // ie_no_open was disabled, so this header must be absent.
  ASSERT_NULL(mock_get_header(&res, "X-Download-Options"));

  free_request(&res);
  RETURN_OK();
}

static void setup_routes(ecewo_app_t *app) {
  ecewo_helmet_config_t *config = ecewo_helmet_config_new();
  if (!config) {
    fprintf(stderr, "ERROR: ecewo_helmet_config_new failed\n");
    abort();
  }

  ecewo_helmet_config_set_csp(config, "default-src 'none'");
  ecewo_helmet_config_set_hsts(config, "63072000", true, true);
  ecewo_helmet_config_set_frame_options(config, "DENY");
  ecewo_helmet_config_set_referrer_policy(config, "no-referrer");
  ecewo_helmet_config_set_xss_protection(config, "1; mode=block");
  ecewo_helmet_config_set_ie_no_open(config, false);

  // install() consumes the config; do not free or reuse it afterwards.
  if (ecewo_helmet_install(app, config) != 0) {
    fprintf(stderr, "ERROR: ecewo_helmet_install failed\n");
    abort();
  }

  ECEWO_GET(app, "/secure", handler_secure);
}

int main(void) {
  if (mock_init(setup_routes) != 0) {
    printf("ERROR: Failed to initialize mock server\n");
    return 1;
  }

  RUN_TEST(test_custom_headers);

  mock_cleanup();
  return 0;
}
