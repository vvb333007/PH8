#ifndef PH8_HTTPD_H
#define PH8_HTTPD_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "picohttpparser.h"
#include "ph7.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    const char              *method;
    size_t                   method_len;
    const char              *target;
    size_t                   target_len;  /* raw request-target "/path?query", not decoded */
    const char              *path;
    size_t                   path_len;    /* target without the query, not decoded */
    const char              *query;
    size_t                   query_len;   /* after '?'; NULL when there is no '?' */
    int                      minor_version;              /* 0 = HTTP/1.0, 1 = HTTP/1.1 */
    const struct phr_header *headers;
    size_t                   num_headers;
    const uint8_t           *body;
    size_t                   body_len;   /* NUL-terminated, de-chunked */
} ph8_request_t;



/* ---- Response (opaque) ---------------------------------------------
 * Headers can be set until the first byte leaves the output buffer.
 * Small responses go out with Content-Length, larger/streamed ones
 * with Transfer-Encoding: chunked (HTTP/1.0 clients: close-delimited).
 * All functions return 0 on success, <0 if the client is gone.      */

typedef struct ph8_resp ph8_resp_t;

void ph8_resp_status(ph8_resp_t *r, int code);
int  ph8_resp_header(ph8_resp_t *r, const char *name, const char *value);
int  ph8_resp_write (ph8_resp_t *r, const void *data, size_t len);
int  ph8_resp_printf(ph8_resp_t *r, const char *fmt, ...)
                     __attribute__((format(printf, 2, 3)));
int  ph8_resp_flush (ph8_resp_t *r);  /* push headers + buffered data now */

/* Zero-copy: sends data[0..len) as the ENTIRE body (Content-Length, no chunking),
 * straight from the caller's memory (e.g. an mmap()'ed TARFS file). Only valid
 * as the first output of a response; if something was already written it
 * degrades to a normal copying ph8_resp_write(). After a successful call
 * the response is sealed: further writes return -1. */
int  ph8_resp_send_static(ph8_resp_t *r, const void *data, size_t len);

/* The script failed. Buffered output is dropped. If nothing was sent yet the
 * client gets a clean 500. Otherwise the response is NOT terminated (no final
 * 0-chunk) and the connection is closed, so the client sees a failed transfer
 * instead of a silently truncated "successful" page. Further writes return -1. */
void ph8_resp_abort(ph8_resp_t *r);

/* ---- Server --------------------------------------------------------- */

typedef void (*ph8_handler_t)(const ph8_request_t *req, ph8_resp_t *resp);

/* Allowed request methods (cfg.method_mask). Known but not allowed -> 405 + Allow,
 * unknown method token -> 501. Both are answered before any body is read. */
/* Largest file accepted in a multipart form (cfg.max_body_size = this + 64 KiB of form overhead). */
#define PH8_UPLOAD_MAX (1024 * 1024)

#define PH8_M_GET      (1u << 0)
#define PH8_M_HEAD     (1u << 1)
#define PH8_M_POST     (1u << 2)
#define PH8_M_PUT      (1u << 3)
#define PH8_M_DELETE   (1u << 4)
#define PH8_M_PATCH    (1u << 5)
#define PH8_M_OPTIONS  (1u << 6)
#define PH8_M_CONNECT  (1u << 7)
#define PH8_M_TRACE    (1u << 8)
#define PH8_M_DEFAULT  (PH8_M_GET | PH8_M_HEAD | PH8_M_POST | PH8_M_PUT | \
                        PH8_M_DELETE | PH8_M_PATCH | PH8_M_OPTIONS)

typedef struct {
    ph7          *engine;          
    char const   *docroot;
    uint16_t      port;
    uint8_t       max_workers;     /* worker pool = max concurrent connections */
    uint32_t      worker_stack;    /* worker stack size. must be kept as small as posiible 8KiB seems reasonable for ESP32 */
    uint8_t       worker_prio;     /* worker task priority */
    uint8_t       acceptor_prio;   /* acceptor task priority */
    uint32_t      req_buf_size;    /* request line + headers limit             */
    uint32_t      max_body_size;   /* request body limit (-> 413)              */
    uint32_t      out_buf_size;    /* output buffer = size of one chunk        */
    uint32_t      io_timeout_ms;   /* SO_RCVTIMEO / SO_SNDTIMEO limits. TODO: slowlori protection */
    ph8_handler_t handler;         /* Must not be NULL */
    uint16_t      method_mask;     /* PH8_M_* bits, 0 = PH8_M_DEFAULT          */
    const char *const *allowed_hosts; /* NULL-terminated list of Host names
                                       * ("192.168.1.5", "dev.local", "[::1]"),
                                       * port ignored. NULL = accept any Host
                                       */
} ph8_httpd_cfg_t;

#define PH8_HTTPD_CFG_DEFAULT() { \
    .engine        = NULL, \
    .docroot       = "/", \
    .port          = 80,          \
    .max_workers   = 4,           \
    .worker_stack  = 8192,        \
    .worker_prio   = 5,           \
    .acceptor_prio = 6,           \
    .req_buf_size  = 4096,        \
    .max_body_size = PH8_UPLOAD_MAX + 64 * 1024, /* FileSize+Headers */ \
    .out_buf_size  = 2048,        \
    .io_timeout_ms = 5000,        \
    .handler       = NULL,        \
    .method_mask   = PH8_M_DEFAULT, \
    .allowed_hosts = NULL,        \
}

esp_err_t ph8_httpd_start(const ph8_httpd_cfg_t *cfg);




/* Static files + PH8 scripts on top of ph8_httpd.
 *
 *   /dir/            -> dir/index.php, then dir/index.html
 *   /dir             -> 301 to "./dir/"
 *   *.php            -> executed (extension compared case-insensitively)
 *   anything else    -> served as a file (GET/HEAD only), mmap() with a
 *                       read()/write() fallback
 */

/* engine: ONE ph7 engine shared by all workers (must be thread-safe).
 * docroot: directory with the site, at most 63 characters, no trailing '/' needed.
 * Returns 0, or -1 if docroot is not a directory / too long. */
int  ph8_site_init(ph7 *engine, const char *docroot);

/* Use as cfg.handler. */
void ph8_site_handler(const ph8_request_t *req, ph8_resp_t *resp);


#ifdef __cplusplus
}
#endif

#endif /* PH8_HTTPD_H */
