#include "ecewo.h"
#include "ecewo-mock.h"
#include "ecewo-helmet.h"
#include "tester.h"
#include <string.h>

static void handler_secure(ecewo_request_t *req, ecewo_response_t *res) {
  (void)req;
  ecewo_send_text(res, 200, "Helmet OK");
}

int test_default_headers(void) {
  MockParams params = {
    .method = MOCK_GET,
    .path = "/secure"
  };

  MockResponse res = request(&params);

  ASSERT_EQ(200, res.status_code);
  ASSERT_EQ_STR("Helmet OK", res.body);

  ASSERT_EQ_STR("default-src 'self'", mock_get_header(&res, "Content-Security-Policy"));
  ASSERT_EQ_STR("max-age=31536000", mock_get_header(&res, "Strict-Transport-Security"));
  ASSERT_EQ_STR("SAMEORIGIN", mock_get_header(&res, "X-Frame-Options"));
  ASSERT_EQ_STR("nosniff", mock_get_header(&res, "X-Content-Type-Options"));
  ASSERT_EQ_STR("0", mock_get_header(&res, "X-XSS-Protection"));
  ASSERT_EQ_STR("strict-origin-when-cross-origin", mock_get_header(&res, "Referrer-Policy"));
  ASSERT_EQ_STR("noopen", mock_get_header(&res, "X-Download-Options"));

  free_request(&res);
  RETURN_OK();
}

static void setup_routes(ecewo_app_t *app) {
  // NULL config installs the secure defaults.
  if (ecewo_helmet_install(app, NULL) != 0) {
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

  RUN_TEST(test_default_headers);

  mock_cleanup();
  return 0;
}
