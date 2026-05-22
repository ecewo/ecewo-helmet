#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-helmet.h"
#include "tester.h"
#include <string.h>

static void handler_secure(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_send_text(res, 200, "Helmet OK");
}

int test_disabled_headers(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/secure"
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Helmet OK", res.body);

  // Disabled headers must be absent.
  ASSERT_NULL(mock_get_header(&res, "Content-Security-Policy"));
  ASSERT_NULL(mock_get_header(&res, "Strict-Transport-Security"));
  ASSERT_NULL(mock_get_header(&res, "X-Content-Type-Options"));

  // Headers left at their defaults must still be present.
  ASSERT_EQ_STR("SAMEORIGIN", mock_get_header(&res, "X-Frame-Options"));
  ASSERT_EQ_STR("noopen", mock_get_header(&res, "X-Download-Options"));

  free_request(&res);
  RETURN_OK();
}

static void setup_routes(ecewo_app_t *app) {
  ecewo_helmet_config_t *config = ecewo_helmet_config_new();
  if (!config) {
    fprintf(stderr, "ERROR: ecewo_helmet_config_new failed\n");
    abort();
  }

  // Passing NULL disables the corresponding header.
  ecewo_helmet_config_set_csp(config, NULL);
  ecewo_helmet_config_set_hsts(config, NULL, false, false);
  ecewo_helmet_config_set_nosniff(config, false);

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

  RUN_TEST(test_disabled_headers);

  mock_cleanup();
  return 0;
}
