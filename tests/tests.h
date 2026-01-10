#ifndef ECEWO_COOKIE_TESTS
#define ECEWO_COOKIE_TESTS

#include "ecewo.h"

void handler_helmet_test(Req *req, Res *res);

int test_helmet_default_headers(void);
int test_helmet_custom_config(void);

#endif
