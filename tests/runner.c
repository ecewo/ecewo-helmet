#include "ecewo-mock.h"
#include "ecewo-helmet.h"
#include "tests.h"
#include "tester.h"

void setup_all_routes(void) {
  get("/secure", handler_helmet_test);
}

int main(void) {
  if (mock_init(setup_all_routes) != 0) {
    printf("ERROR: Failed to initialize mock server\n");
    return 1;
  }

  helmet_init(NULL);

  RUN_TEST(test_helmet_default_headers);
  RUN_TEST(test_helmet_custom_config);

  mock_cleanup();
  return 0;
}