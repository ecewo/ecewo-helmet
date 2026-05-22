# ecewo-helmet

The `ecewo-helmet` plugin is for automaticaly setting security headers.

## Table of Contents

1. [Installation](#installation)
2. [Default Helmet Configuration](#default-helmet-configuration)
3. [Custom Helmet Configuration](#custom-helmet-configuration)
4. [Configuration Options](#configuration-options)
5. [CSP Directive Reference](#csp-directive-reference)
6. [Security Headers Explained](#security-headers-explained)
    1. [Content Security Policy (CSP)](#content-security-policy-csp)
    2. [HTTP Strict Transport Security (HSTS)](#http-strict-transport-security-hsts)
    3. [Frame Options](#frame-options)
    4. [Referrer Policy](#referrer-policy)

## Installation

Add to your `CMakeLists.txt`:

```sh
ecewo_add(helmet)

target_link_libraries(app PRIVATE
    ecewo::ecewo
    ecewo::helmet
)
```

## Default Helmet Configuration

Default configuration is enough for many applications. If you would like to customize it, see [Configuration Options](#configuration-options).

```c
#include "ecewo.h"
#include "ecewo-helmet.h"
#include <stdio.h>

void example_handler(ecewo_request_t *req, ecewo_response_t *res)
{
    ecewo_send_text(res, 200, "Hello");
}

int main(void)
{
    ecewo_app_t *app = ecewo_create();
    if (!app)
    {
        fprintf(stderr, "Failed to create app\n");
        return 1;
    }

    // Enable security headers with defaults (pass NULL for the config)
    ecewo_helmet_install(app, NULL);
    // Response headers:
    // Content-Security-Policy: default-src 'self'
    // Strict-Transport-Security: max-age=31536000
    // X-Frame-Options: SAMEORIGIN
    // X-Content-Type-Options: nosniff
    // X-XSS-Protection: 0
    // Referrer-Policy: strict-origin-when-cross-origin
    // X-Download-Options: noopen

    ECEWO_GET(app, "/", example_handler);

    ecewo_listen(app, 3000);
    return 0;
}
```

The configuration is an **opaque handle** rather than a public struct, so the
plugin's ABI stays stable even as new options are added. You build it with the
`ecewo_helmet_config_*()` setters and hand it to `ecewo_helmet_install()`, which
takes ownership of it.

**About the default CSP**

Content Security Policy is the most effective protection against XSS attacks, but it's also the most complex header to configure. The default is the strict `default-src 'self'`, which only permits resources from the same origin. Most applications will need to relax it for their own assets (CDNs, analytics, inline styles, etc.).

**To customize CSP (recommended for production):**
```c
ecewo_helmet_config_t *config = ecewo_helmet_config_new();
ecewo_helmet_config_set_csp(config, "default-src 'self'; script-src 'self' https://trusted-cdn.com");

ecewo_helmet_install(app, config); // consumes config
```

See the [CSP Directive Reference](#csp-directive-reference) section for more information.

## Custom Helmet Configuration

```c
#include "ecewo.h"
#include "ecewo-helmet.h"
#include <stdio.h>

int main(void)
{
    ecewo_app_t *app = ecewo_create();
    if (!app)
    {
        fprintf(stderr, "Failed to create app\n");
        return 1;
    }

    ecewo_helmet_config_t *config = ecewo_helmet_config_new();

    // Content Security Policy - Controls what resources can be loaded
    ecewo_helmet_config_set_csp(config,
        "default-src 'self'; "                                                 // Only allow resources from same origin by default
        "script-src 'self' https://www.googletagmanager.com 'unsafe-inline'; " // Allow scripts from self, Google Tag Manager, and inline scripts
        "style-src 'self' 'unsafe-inline'; "                                   // Allow styles from self and inline styles (for dynamic styling)
        "img-src 'self' data: https:; "                                        // Allow images from self, data URIs, and any HTTPS source
        "connect-src 'self' https://www.google-analytics.com");                // Allow AJAX/fetch to self and Google Analytics

    // HTTP Strict Transport Security - Force HTTPS for 2 years, all subdomains,
    // and submit to the browser preload list (permanent, irreversible!)
    ecewo_helmet_config_set_hsts(config, "63072000", true, true);

    // Clickjacking Protection - never allow this site to be embedded in iframes
    ecewo_helmet_config_set_frame_options(config, "DENY");

    // Privacy - never send the referrer header to any destination
    ecewo_helmet_config_set_referrer_policy(config, "no-referrer");

    // XSS Protection - disable the legacy filter (rely on CSP instead)
    ecewo_helmet_config_set_xss_protection(config, "0");

    // MIME Type Security - force browsers to honor the declared content-type
    ecewo_helmet_config_set_nosniff(config, true);

    // IE-specific Security - prevent IE from executing downloads in-context
    ecewo_helmet_config_set_ie_no_open(config, true);

    // Enable security headers with the custom config (consumes config)
    ecewo_helmet_install(app, config);

    ECEWO_GET(app, "/", example_handler);

    ecewo_listen(app, 3000);
    return 0;
}
```

> [!NOTE]
>
> `ecewo_helmet_install()` takes ownership of the config: its values are copied
> into the app arena and the handle is freed for you. Do not free or reuse it
> afterwards. If you build a config but decide not to install it, release it with
> `ecewo_helmet_config_free()`.
>
> Pass `NULL` to any string setter (e.g. `ecewo_helmet_config_set_csp(config, NULL)`
> or `ecewo_helmet_config_set_hsts(config, NULL, false, false)`) to disable that
> header entirely.

## Configuration Options

A fresh config from `ecewo_helmet_config_new()` starts with these secure
defaults. Override any of them with the corresponding setter.

| Setter                                  | Default                             | Header                   |
|-----------------------------------------|-------------------------------------|--------------------------|
| `ecewo_helmet_config_set_csp`           | `"default-src 'self'"`              | Content-Security-Policy  |
| `ecewo_helmet_config_set_hsts`          | `"31536000"`, no subdomains/preload | Strict-Transport-Security|
| `ecewo_helmet_config_set_frame_options` | `"SAMEORIGIN"`                      | X-Frame-Options          |
| `ecewo_helmet_config_set_referrer_policy` | `"strict-origin-when-cross-origin"` | Referrer-Policy        |
| `ecewo_helmet_config_set_xss_protection`| `"0"`                               | X-XSS-Protection         |
| `ecewo_helmet_config_set_nosniff`       | `true`                              | X-Content-Type-Options   |
| `ecewo_helmet_config_set_ie_no_open`    | `true`                              | X-Download-Options       |

## CSP Directive Reference

| Directive       | Controls                          | Example Values                                 |
|-----------------|-----------------------------------|------------------------------------------------|
| `default-src`   | Default for all resource types    | `'self'`, `'none'`                             |
| `script-src`    | JavaScript sources                | `'self'`, `https://cdn.com`, `'unsafe-inline'` |
| `style-src`     | CSS sources                       | `'self'`, `'unsafe-inline'`                    |
| `img-src`       | Image sources                     | `'self'`, `data:`, `https:`                    |
| `font-src`      | Font sources                      | `'self'`, `https://fonts.gstatic.com`          |
| `connect-src`   | AJAX/fetch/WebSocket              | `'self'`, `https://api.example.com`            |
| `frame-src`     | `<iframe>` sources                | `'self'`, `https://trusted.com`                |
| `object-src`    | `<object>`, `<embed>`, `<applet>` | `'none'` (recommended)                         |
| `base-uri`      | `<base>` tag                      | `'self'`                                       |
| `form-action`   | Form submission targets           | `'self'`                                       |
| `frame-ancestors` | Sites that can embed this page  | `'none'`, `'self'`                             |

**Special Keywords:**
- `'self'` - Same origin
- `'none'` - Block everything
- `'unsafe-inline'` - Allow inline scripts/styles (less secure)
- `'unsafe-eval'` - Allow `eval()` (dangerous!)
- `data:` - Allow data URIs
- `https:` - Allow any HTTPS source

## Security Headers Explained

### Content Security Policy (CSP)

Prevents XSS attacks by controlling resource loading:

```c
// Strict: only same-origin scripts/styles
ecewo_helmet_config_set_csp(config, "default-src 'self'");

// Allow CDN for scripts
ecewo_helmet_config_set_csp(config, "default-src 'self'; script-src 'self' https://cdn.example.com");

// Allow inline styles (for styled-components, etc.)
ecewo_helmet_config_set_csp(config, "default-src 'self'; style-src 'self' 'unsafe-inline'");

// Allow Google Fonts and Analytics
ecewo_helmet_config_set_csp(config, "default-src 'self'; "
                                    "font-src 'self' https://fonts.gstatic.com; "
                                    "script-src 'self' https://www.googletagmanager.com");
```

### HTTP Strict Transport Security (HSTS)

Forces HTTPS connections:

```c
// Basic: 1 year HTTPS only
ecewo_helmet_config_set_hsts(config, "31536000", false, false);

// With subdomains: include api.example.com, www.example.com
ecewo_helmet_config_set_hsts(config, "31536000", true, false);

// Preload list: browsers always use HTTPS (permanent)
ecewo_helmet_config_set_hsts(config, "63072000", true, true); // Irreversible
```

### Frame Options

Prevents clickjacking:

```c
ecewo_helmet_config_set_frame_options(config, "DENY");       // Never allow iframes
ecewo_helmet_config_set_frame_options(config, "SAMEORIGIN"); // Only same-origin iframes (default)
```

### Referrer Policy

Controls referrer information:

```c
ecewo_helmet_config_set_referrer_policy(config, "no-referrer");                     // Never send referrer
ecewo_helmet_config_set_referrer_policy(config, "strict-origin-when-cross-origin"); // Default
ecewo_helmet_config_set_referrer_policy(config, "same-origin");                     // Only same-origin
```
