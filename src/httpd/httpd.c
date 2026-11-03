/*
 * PH7-ESP32 — embeddable PHP bytecode compiler & virtual machine,
 * ported and maintained for ESP32/FreeRTOS, Linux and Windows(Cygwin).
 *
 * Copyright (C) 2026-present Viacheslav Logunov <vvb333007@gmail.com>
 *
 * Licensed under the Symisc Public License (modified BSD with mandatory
 * source disclosure, clause 3); commercial licensing available from
 * Symisc Systems. Full terms: see LICENSE. Contacts: see CONTACTS.
 */

/*
 * PH8 HTTP server. It uses FreeRTOS API (PH8 provides one, a limited set of function, on Linux and Windows/Cygwin)
 *
 * How it works:
 *   There are N+1 tasks started: 1 task is so-called "acceptor" who listens for new connections, establishes one
 *   and wakes up one of N remaining tasks ( "workers"). Workers handle the connection: send static files or execute PHP scripts
 *   All tasks  are created once and never deleted (no task create/delete per connection).
 *
 *   Transport is behind ph8_conn (and corresponding methods ->read/->write/->close). This abstraction added in order to
 *   add SSL support. Rigght now only "plain" HTTP connections are supported (no HTTPS yet). Functions that handle plain connections
 *   all have _plain in their name (e.g. conn_init_plain() )
 *
 *   All workers busy -> acceptor answers 503 and closes.
 *
 *   Request: goes through picohttpparser first; body via Content-Length or chunked
 *   (phr_decode_chunked), "Expect: 100-continue" supported.
 *
 *   If requests resolves to executable script, then a VM is created, source code compiled
 *   The request is parsed to PHP variables (Superglobals _GET/_SERVER/etc) and those variables are 
 *   installed into a freshly compiled VM; VM is executed.
 *
 *   Response: goes through a buffer of fixed size; if size is known and it fits then send
 *   with Content-Length; chunked otherwise.
 *
 *   Always "Connection: close"
 */

#include "httpd.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <fcntl.h>

/* FreeRTOS/ESP32 has mmap() provided by TARFS library */
#if defined( __CYGWIN__) || defined(__linux__)
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

#include "ph7.h"

/* Cygwin/Linux: these are emulated (see src/cygwin)*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "ph8httpd";

#define PH8_HTTPD_BACKLOG 4 /* Max pending connections for listen() */

#define MAX_HEADERS   32        /* max number of headers per request is MAX_HEADERS-1. 32 is used as "Too Many Headers" sentinel */
#define LOG_QUERY_MAX 64        /* too long for logging */
#define RESP_HDR_CAP  512       /* max room for response headers */
#define CHUNK_PREFIX  10        /* room before data for "<hex>\r\n" (8 hex digits max) */

#ifndef O_BINARY
#define O_BINARY 0                  /* Cygwin: never let a text-mode mount mangle files */
#endif

#define SITE_ROOT_MAX   64          /* docroot max size */
#define SITE_DEC_MAX    256         /* decoded request path */
#define SITE_FS_MAX     384         
#define SITE_READ_CHUNK 1024        /* read()/write() fallback chunk */

/* Globals: we only need 1 engine and likely just 1 docroot, so keep this data global */
static ph7  *g_engine = NULL;
static char  g_root[SITE_ROOT_MAX]; /* no trailing slash */
static size_t g_root_len;

typedef struct ph8_conn ph8_conn;


/* Response: buffered, Content-Length or chunked                       
*/
struct ph8_resp {
    ph8_conn *conn;           /* assotiated connection */
    int       status;         /* response code , e.g. 200 or 404 */
    bool      head_sent;      /* true if headers were sent already; no headers can be added*/
    bool      chunked;        /* framing actually in use (decided at head time) */
    bool      chunked_ok;     /* client is HTTP/1.1 */
    bool      head_only;      /* HEAD request: never send a body */
    bool      has_ctype;      /* Saw "Content-Type:" ? */
    bool      failed;         /* write error: client gone */
    bool      sealed;         /* body already sent by send_static, no more output */
    bool      aborted;        /* script failed, see ph8_resp_abort() */
    char      hdr[RESP_HDR_CAP];
    size_t    hdr_len;
    uint8_t  *buf;            /* data lives at buf + CHUNK_PREFIX */
    size_t    cap;            /* buffer capacity */
    size_t    used;           /* how many bytes is used */
};


/* Helpers. Alot of them. If you are reading this code in attempt to understand what 
 * it does - scroll down to public API and trace from there
 */


/* Send HTTP error reply. This one called when something went horrible wrong
 * so we do not want to generate full HTML page in response
 */
static void reply_error(ph8_resp_t *rs, int code, const char *msg) {

    ph8_resp_status(rs, code);
    ph8_resp_header(rs, "Content-Type", "text/plain; charset=utf-8");
    ph8_resp_printf(rs, "%d %s\n", code, msg);
}

/* Convert '[0-9a-fA-F]' to decimal 
*/
static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Percent-decode request path into dst. (no query!!!)
 * Returns the length, -1 for a bad path (e.g. path is not absolute, bad %XX, control
 * characters or '\' (Cygwin treats it as separator), '..' blocks etc),
 * -2 if it does not fit. 
 */
static int decode_path(char *dst, size_t cap, const char *s, size_t n)
{
    size_t o = 0,   /* output cursor */
           i,       /* input cursor */
           j;

    /* Empty string or not absolute path */
    if (NULL == dst || NULL == s || 0 == n || '/' != s[0] )
        return -1;
    
    for (i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        /* decode %xy */
        if (c == '%') {
            int h, l;
            /* premature end */
            if (i + 2 >= n) {
                return -1;
            }
            h = hexval(s[i + 1]);
            l = hexval(s[i + 2]);
            /* bad hexadecimal numbers */
            if (h < 0 || l < 0) {
                return -1;
            }
            c = (unsigned char)((h << 4) | l);
            i += 2;
        }
        /* Bad characters (control, slash and DEL)*/
        if (c < 0x20 || c == 0x7f || c == '\\')
            return -1;
        
        /* No space left in output buffer, path too long */
        if (o + 1 >= cap) {
            return -2;
        }
        dst[o++] = (char)c;
    }
    /* nul termination */
    dst[o] = '\0';

    /* reject ".." segments */
    for (i = 0; i < o; i = j + 1) {
        j = i;
        while (j < o && dst[j] != '/') {
            j++;
        }
        if (j - i == 2 && dst[i] == '.' && dst[i + 1] == '.') {
            return -1;
        }
    }
    return (int)o;
}

/* Extract file extension (e.g. 'php' or 'jpg')
 * by traversing path backwards
 */
static const char *ext_of(const char *path) {

    if (path == NULL || *path == '\0')
      return "";

    const char *slash = strrchr(path, '/');
    const char *dot = strrchr(path, '.');

    return (dot != NULL && (slash == NULL || dot > slash)) ? dot + 1 : "";
}

/* Coose "Content-Type" according to a file extension
 *
 */
static const char *mime_of(const char *path) {

    static const struct { 

      const char *ext;   /* File extension */
      const char *type;  /* Corresponding MIME type */

    } mimes[] = {

        { "html", "text/html; charset=utf-8" },       
        { "htm",  "text/html; charset=utf-8" },
        { "md",   "text/markdown; charset=utf-8" },
        { "css",  "text/css; charset=utf-8" },        
        { "js",   "text/javascript; charset=utf-8" },
        { "mjs",  "text/javascript; charset=utf-8" }, 
        { "txt",  "text/plain; charset=utf-8" },
        { "csv",  "text/csv; charset=utf-8" },        
        { "json", "application/json" },
        { "xml",  "application/xml" },                
        { "svg",  "image/svg+xml" },
        { "png",  "image/png" },                      
        { "jpg",  "image/jpeg" },
        { "jpeg", "image/jpeg" },                     
        { "gif",  "image/gif" },
        { "webp", "image/webp" },                     
        { "ico",  "image/x-icon" },
        { "woff", "font/woff" },                      
        { "woff2", "font/woff2" },
        { "pdf",  "application/pdf" },                
        { "wasm", "application/wasm" },

    };

    /* Extract extension of the requested resource */
    const char *ext = ext_of(path);

    /* Find corresponding entry, fallback to octet-stream if nothing found */
    if (*ext != '\0')
      for (int i = 0; i < sizeof(mimes) / sizeof(mimes[0]); i++) 
        if (strcasecmp(ext, mimes[i].ext) == 0)
          return mimes[i].type;
    
    return "application/octet-stream";
}
/* Check if `rq` was sent with method `m`
 * e.g. `method_is(rq,"HEAD");`
 */
static int method_is(const ph8_request_t *rq, const char *m) {

  if (m == NULL || *m == '\0')
    m = "";

  int n = strlen(m);
  return rq->method_len == n && memcmp(rq->method, m, n) == 0;
}


/*
 * Choose index file when directory is requested
 * fs ends with '/'. Appends the first existing index file; updates st. 
 * (st is a stat() result on a directory )
 */
static int pick_index(char *fs, size_t cap, struct stat *st) {

    static const char *const names[] = { 

      "index.php",   /* Has priority over .html and .md */
      "index.html",
      "index.htm",   /* Support for old naming convention */
      "INDEX.md",
      "README.md",   /* Last resort */
    };

    size_t l = strlen(fs);

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (l + strlen(names[i]) >= cap) {
            continue;
        }
        strcpy(fs + l, names[i]);
        /* stat() again, check if it a file */
        if (stat(fs, st) == 0 && S_ISREG(st->st_mode)) {
            return 1;
        }
    }
    /* NUL-terminate */
    fs[l] = '\0';
    return 0;
}

/* Send redirect.
 *
 * "/dir" -> 301 "./dir/". The Location is relative and starts with "./", so 
 * paths like "//evil.com" are harmless
 */
static void redirect_to_dir(const ph8_request_t *rq, ph8_resp_t *rs)
{
    const char *seg = rq->path;
    char loc[320];
    int n;

    for (size_t i = 0; i < rq->path_len; i++) {
        if (rq->path[i] == '/') {
            seg = rq->path + i + 1;
        }
    }
    n = snprintf(loc, sizeof(loc), "./%.*s/%s%.*s",
                 (int)(rq->path + rq->path_len - seg), seg,
                 rq->query_len > 0 ? "?" : "",
                 rq->query_len > 0 ? (int)rq->query_len : 0, rq->query_len > 0 ? rq->query : "");
    if (n < 0 || (size_t)n >= sizeof(loc)) {
        reply_error(rs, 414, "URI Too Long");
        return;
    }
    ph8_resp_status(rs, 301);
    ph8_resp_header(rs, "Location", loc);
    ph8_resp_header(rs, "Content-Type", "text/plain; charset=utf-8");
    ph8_resp_printf(rs, "301 Moved Permanently\n");
}


/* Static files
 * We try mmap() and fallback to read/write if mmap() is not supported
 * Cygwin target has mmap(); FreeRTOS/ESP32 has mmap() only in TARFS
 *
*/
static void serve_static(ph8_resp_t *rs, int is_head, const char *path) {

    struct stat st;
    size_t size;
    void *p;
    int fd = open(path, O_RDONLY | O_BINARY);

    if (fd < 0) {
        reply_error(rs, errno == EACCES ? 403 : 404, errno == EACCES ? "Forbidden" : "Not Found");
        return;
    }
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        reply_error(rs, 404, "Not Found");
        return;
    }

    /* Ready to stream: sett 200 OK, extract and send MIME header */
    ph8_resp_status(rs, 200);
    ph8_resp_header(rs, "Content-Type", mime_of(path));

    size = (size_t)st.st_size;
    if (size == 0) {                               /* mmap() of 0 bytes is EINVAL */
        close(fd);
        ph8_resp_send_static(rs, "", 0);
        return;
    }
    p = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (p != MAP_FAILED) {
        close(fd);                                 /* the mapping outlives the descriptor */
        ph8_resp_send_static(rs, p, size);         /* synchronous: safe to unmap right after */
        munmap(p, size);
        return;
    }

    /* Fallback: read()/write() in small chunks. */
    if (is_head) {
        ph8_resp_flush(rs);                        /* headers only, do not read the file */
    } else {
        char buf[SITE_READ_CHUNK];
        for (;;) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ph8_resp_abort(rs);                /* do not pretend a truncated file is complete */
                break;
            }
            if (n == 0 || ph8_resp_write(rs, buf, (size_t)n) < 0) {
                break;                             /* EOF or client gone */
            }
        }
    }
    close(fd);
}


/* Scripts 
 * PH8 Engine uses concept of an "output consumer" - a callback which is called every time 
 * script output anything, e.g. `echo 'Hello';` is processed by an output consumer. 
 * This callback is registered for every VM.
 */


/* VM output consumer: everything the script prints goes into the HTTP response. 
 * The same function can be used as an error consumer; Error consumer is set per engine, not per VM
 *
 */
inline static int out_consumer(const void *data, unsigned int len, void *user) {
    return ph8_resp_write((ph8_resp_t *)user, data, len) < 0 ? SXERR_ABORT : SXRET_OK;
}


/* Called by HTTP handler (ph8_site_handler())  to execute .php scripts
 *
 */
static void run_script(const ph8_request_t *rq, ph8_resp_t *rs, const char *path, const char *script_name) {

    ph8_http_request hr = { 0 };
    ph7_vm *vm = NULL;
    int rc;

    /* Compile requested script, create a VM */
    rc = ph7_compile_file(g_engine, path, &vm, 0);
    if (rc != PH7_OK) {                            /* details go to the engine's error output */
        if (vm != NULL) {
            ph7_vm_release(vm);
        }
        reply_error(rs, 500, "Script compile error");
        return;
    }

#if 0
  TODO:
  if (g_srv.cfg)
  ph7_vm_config(vm,PH7_VM_CONFIG_ERR_REPORT);
#endif

    hr.method = rq->method;
    hr.method_len = rq->method_len;
    hr.target = rq->target;
    hr.target_len = rq->target_len;
    hr.minor_version = rq->minor_version;
    hr.headers = (const ph8_http_header *)rq->headers;
    hr.num_headers = rq->num_headers;
    hr.body = rq->body;
    hr.body_len = rq->body_len;


    /* Set variables: $_SERVER/$_GET/$_POST/$_FILES... incl. multipart
     * Save a reference to `rs` in the VM: this way we can access it from FFI functions whenever we 
     * want to manipulate raw response. this is required to implement `header()` and 
     * `http_status_code()` PHP functions
     */
    rc = ph8_vm_set_request(vm, &hr, rq, rs);

    if (rc != SXRET_OK) {
        if (rc == SXERR_SYNTAX) {
            reply_error(rs, 400, "Bad Request");
        } else if (rc == SXERR_LIMIT) {
            reply_error(rs, 413, "Payload Too Large");
        } else {
            reply_error(rs, 500, "Internal Server Error");
        }
        ph7_vm_release(vm);
        return;
    }

    /* Configure docroot and scriptname; configure import_path and set an output consumer callback
     */
    ph7_vm_config(vm, PH7_VM_CONFIG_SERVER_ATTR, "DOCUMENT_ROOT", g_root, -1);
    ph7_vm_config(vm, PH7_VM_CONFIG_SERVER_ATTR, "SCRIPT_NAME", script_name, -1);
    ph7_vm_config(vm, PH7_VM_CONFIG_IMPORT_PATH, g_root);
    ph7_vm_config(vm, PH7_VM_CONFIG_OUTPUT, out_consumer, rs);
    

    /* Scripts are likely to produce HTML. Headers can still be set
     * until the first output buffer is flushed. Set default content-type header to be text/html
     */
    ph8_resp_status(rs, 200);
    ph8_resp_header(rs, "Content-Type", "text/html; charset=utf-8");
    
    /* Execute the script.
     * Final response flush is done in the caller of tthis function (in the worker)
     */
    rc = ph7_vm_exec(vm, NULL);
    if (rc != PH7_OK) {
        /* clean 500 if nothing was sent, else drop the connection */
        ph8_resp_abort(rs);                        
    }
    ph7_vm_release(vm);
}



/* Main HTTPD handler:
 * It is called by the worker; `rs` is fresh and empty, `rq` is filled with parsed request
 * Perform sanity checks on input (path), serve static content via mmap() (with read/write fallback if mmap() is not supported),
 * If .php file is requested - setup PHP environment variables (_GET/_SERVER/_POST/ cookies etc) and execute script
 * Once script is finished - flush all what is left in the buffer and close connection; return to IDLE
 */

    /* TODO: hooks:
     * HTTP server must provide API for registering custom URI/handler pairs
     * e.g. PH8_HttpUriHandler(const char *zUri,const char *zMethod, int (* cbHandler)(void *));
      PH8_HttpUriHandler("/upload_firmware","GET", uf_show_index);
      PH8_HttpUriHandler("/upload_firmware","POST", uf_burn);
     * Callback returns:
         0 - Continue normal processing
        -1 - URI was consumed by a hook, reply generated. Finish processing.

    */

void ph8_site_handler(const ph8_request_t *rq, ph8_resp_t *rs) {

    /* Local variables ~600 bytes*/
    char dec[SITE_DEC_MAX];
    char path[SITE_FS_MAX];
    struct stat st;
    int n;

    n = decode_path(dec, sizeof(dec), rq->path, rq->path_len);
    if (n == -1) {
        reply_error(rs, 400, "Bad Request");
        return;
    }


    if (n == -2 || g_root_len + (size_t)n >= sizeof(path)) {
        reply_error(rs, 414, "URI Too Long");
        return;
    }
    memcpy(path, g_root, g_root_len);
    memcpy(path + g_root_len, dec, (size_t)n + 1);

#if 0
    /* Hooks can update `path` or/and process request and generate reply.
    * Negative return value means that request was fully processed by the hooks so we 
    * have nothing to do
    */
    if (ph8_http_execute_hooks(rq,rs,path) < 0)
      return ;
#endif    

    if (stat(path, &st) < 0) {
        reply_error(rs, errno == EACCES ? 403
                                        : 404,
                        errno == EACCES ? "Forbidden"
                                        : "Not Found");
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        if (rq->path[rq->path_len - 1] != '/') {
            redirect_to_dir(rq, rs);
            return;
        }
        if (!pick_index(path, sizeof(path), &st)) {
            reply_error(rs, 403, "Directory listing is not allowed");     /* no directory listings */
            return;
        }
    }
    if (!S_ISREG(st.st_mode)) {
        reply_error(rs, 404, "Not a regular file");
        return;
    }

    /* Php scripts can accept any method */

    if (strcasecmp(ext_of(path), "php") == 0) {
        run_script(rq, rs,
                   path,                /* full path */
                   path + g_root_len);  /* script name */
        return;
    }

    /* For static content we restrict to GET and HEAD
    */
    if (!method_is(rq, "GET") && !method_is(rq, "HEAD")) {
        ph8_resp_header(rs, "Allow", "GET, HEAD");
        reply_error(rs, 405, "Method Not Allowed");
        return;
    }
    serve_static(rs, method_is(rq, "HEAD"), path);
}




static void *SyOSHeapAlloc(sxu32 nByte); /* Declared in src/lib.c */

/* Memory helpers: prefer PSRAM for big buffers
 */
static void *big_alloc(size_t n) {
  return heap_caps_malloc_prefer(n, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_DEFAULT);
}

static void *big_realloc(void *p, size_t n) {
  return heap_caps_realloc_prefer(p, n, 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT, MALLOC_CAP_DEFAULT);
}


/* Transport abstraction
 * Instead of plain read/write on a socket we use ->read() and ->write() on a ph8_conn
 * The only reason for this extra layer is upcoming support for SSL
*/

struct ph8_conn {
    int   fd;
    void *ctx;                                         /* TLS session later */
    int  (*read)(ph8_conn *c, void *buf, size_t len);  /* <0 error/timeout, 0 EOF */
    int  (*write)(ph8_conn *c, const void *buf, size_t len);
    void (*close)(ph8_conn *c);
};

static int plain_read(ph8_conn *c, void *buf, size_t len) {
    /*TODO: add global recv timeout to filter out slowloris: client who send 1 byte every RCV_TIMO
          can hold the connection indefinitely long
    */
    return recv(c->fd, buf, len, 0);
}

static int plain_write(ph8_conn *c, const void *buf, size_t len){

    return send(c->fd, buf, len, 0);
}

/* Lingering close: half-close, then drain briefly. Without it, closing a
 * socket that still has unread request data (e.g. after a 413) makes the
 * stack send RST and the client may never see our response. */
static void plain_close(ph8_conn *c) {
    shutdown(c->fd, SHUT_WR);
    struct timeval tv = { 
      .tv_sec = 0, 
      .tv_usec = 200000 /* 0.2 sec */
    };
    setsockopt(c->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char tmp[128];
    for (int i = 0; i < 128; i++) {          /* at most 16 KB, 0.2x128 = 26 seconds for slowloris */
        if (recv(c->fd, tmp, sizeof(tmp), 0) <= 0) {
            break;
        }
    }
    close(c->fd);
    c->fd = -1;
}

static void conn_init_plain(ph8_conn *c, int fd)
{
    c->fd    = fd;
    c->ctx   = NULL;
    c->read  = plain_read;
    c->write = plain_write;
    c->close = plain_close;
}

static int conn_write_all(ph8_conn *c, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        int n = c->write(c, p, len);
        if (n <= 0) {
            return -1;
        }
        p   += n;
        len -= (size_t)n;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Server state                                                        */
/* ------------------------------------------------------------------ */

static struct {
    ph8_httpd_cfg_t   cfg;
    int               listen_fd;
    QueueHandle_t     queue;       /* int fd */
    SemaphoreHandle_t free_slots;  /* counting: number of idle workers */
} g_srv;

typedef struct {
    int      id;
    uint8_t *reqbuf;               /* request line + headers (+ first body bytes) */
    uint8_t *outbuf;               /* response buffer, CHUNK_PREFIX + out_buf_size + 2 */
} worker_t;

/* ------------------------------------------------------------------ */
/* Plain error responses (before/instead of handler)                   */
/* ------------------------------------------------------------------ */

static const char *reason(int code)
{
    switch (code) {
    case 100: return "Continue";
    case 200: return "OK";
    case 201: return "Created";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 411: return "Length Required";
    case 413: return "Payload Too Large";
    case 414: return "URI Too Long";
    case 415: return "Unsupported Media Type";
    case 417: return "Expectation Failed";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 503: return "Service Unavailable";
    default:  return "Unknown";
    }
}

/* Zero or more complete header lines, each ending with CRLF or NULL
*/
static void send_error_ex(ph8_conn *c, int code, const char *extra)
{
    char buf[384]; /* TODO: no magic numbers! */
    const char *rs = reason(code);
    int n = snprintf(buf, sizeof(buf),
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "%s"
                     "\r\n%s\n",
                     code, rs, (int)strlen(rs) + 1, extra ? extra : "", rs);
    if (n > 0) {
        conn_write_all(c, buf, (size_t)n < sizeof(buf) ? (size_t)n : sizeof(buf) - 1);
    }
}

/* Same as above but no `extra`
*/
inline static void send_error(ph8_conn *c, int code) {
    send_error_ex(c, code, NULL);
}


/* Initialize response struct. Must be deinitialized
 * once script is finished
 */
static void resp_init(ph8_resp_t *r, ph8_conn *c, uint8_t *buf, size_t cap, bool http11, bool head_only) {
    memset(r, 0, sizeof(*r));
    r->conn       = c;
    r->status     = 200;
    r->chunked_ok = http11;
    r->head_only  = head_only;
    r->buf        = buf;
    r->cap        = cap;
}

void ph8_resp_status(ph8_resp_t *r, int code) {
    if (!r->head_sent)
        r->status = code;
    else {
      /* TODO: a visible warning (at least in DEVEL mode) */
    }
}

#if 0
/* Add/replace headers
 * Headers are stored as an plain buffer, so replacement procedure includes left shift (to erase old value)
 * and than appending new header to the end:
 *
 * Replacing Header1:Value with Header1:Vaalluuee
 *
 *  1. Initial: "Header1:Value\r\nHeader2:Value"
 *  2. After left shift: "Header2:Value"
 *  3. Append: "Header2:Value\r\nHeader1:Vaalluuee"
 *
 * If `replace` is not set, then header is just appended
 */
int ph8_resp_header(ph8_resp_t *r, const char *name, const char *value, bool replace) {

    /* Headers were sent, can not add any more */
    if (r->head_sent) {
        return -1;
    }

    /* framing headers are ours; also block header injection */
    if (strcasecmp(name, "Content-Length") == 0 ||
        strcasecmp(name, "Transfer-Encoding") == 0 ||
        strcasecmp(name, "Connection") == 0 ||
        strpbrk(name, "\r\n:") || strpbrk(value, "\r\n")) {
        return -1;
    }

    size_t nl = strlen(name), vl = strlen(value);
    size_t old_pos = 0, old_len = 0;

    /* Find an existing header */
    if (replace) {
        size_t pos = 0;

        while (pos < r->hdr_len) {
            char *line = r->hdr + pos;
            char *end = memchr(line, '\n', r->hdr_len - pos);

            if (!end) {
                return -1;
            }

            size_t line_len = (size_t)(end - line) + 1;
            char *colon = memchr(line, ':', line_len);

            if (colon &&
                (size_t)(colon - line) == nl &&
                strncasecmp(line, name, nl) == 0) {
                old_pos = pos;
                old_len = line_len;
                break;
            }

            pos += line_len;
        }
    }

    /* Check capacity after accounting for the old header */
    if (r->hdr_len - old_len + nl + vl + 4 > RESP_HDR_CAP) {
        return -1;
    }

    /* Remove the old header, shifting the remaining headers left */
    if (old_len) {
        memmove(r->hdr + old_pos,
                r->hdr + old_pos + old_len,
                r->hdr_len - old_pos - old_len);

        r->hdr_len -= old_len;
    }

    /* Append the new header */
    memcpy(r->hdr + r->hdr_len, name, nl);
    r->hdr_len += nl;

    r->hdr[r->hdr_len++] = ':';
    r->hdr[r->hdr_len++] = ' ';

    memcpy(r->hdr + r->hdr_len, value, vl);
    r->hdr_len += vl;

    r->hdr[r->hdr_len++] = '\r';
    r->hdr[r->hdr_len++] = '\n';

    /* Got "Content-Type"?  record that */
    if (strcasecmp(name, "Content-Type") == 0) {
        r->has_ctype = true;
    }

    return 0;
}
#else
int ph8_resp_header(ph8_resp_t *r, const char *name, const char *value)
{
    if (r->head_sent) {
        return -1;
    }
    /* framing headers are ours; also block header injection */
    if (strcasecmp(name, "Content-Length") == 0 ||
        strcasecmp(name, "Transfer-Encoding") == 0 ||
        strcasecmp(name, "Connection") == 0 ||
        strpbrk(name, "\r\n:") || strpbrk(value, "\r\n")) {
        return -1;
    }
    size_t nl = strlen(name), vl = strlen(value);
    if (r->hdr_len + nl + vl + 4 > RESP_HDR_CAP) {
        return -1;
    }
    memcpy(r->hdr + r->hdr_len, name, nl);   r->hdr_len += nl;
    r->hdr[r->hdr_len++] = ':';
    r->hdr[r->hdr_len++] = ' ';
    memcpy(r->hdr + r->hdr_len, value, vl);  r->hdr_len += vl;
    r->hdr[r->hdr_len++] = '\r';
    r->hdr[r->hdr_len++] = '\n';
    if (strcasecmp(name, "Content-Type") == 0) {
        r->has_ctype = true;
    }
    return 0;
}
#endif

/* Build status line + headers + framing and send them in one write
*/
static int resp_send_head(ph8_resp_t *r, bool chunked, bool have_len, size_t clen)
{
    char out[RESP_HDR_CAP + 192];
    int  n = snprintf(out, sizeof(out),
                      "HTTP/1.1 %d %s\r\nConnection: close\r\n",
                      r->status, reason(r->status));
    memcpy(out + n, r->hdr, r->hdr_len);
    n += (int)r->hdr_len;
    if (!r->has_ctype) {
        n += snprintf(out + n, sizeof(out) - n, "Content-Type: text/plain; charset=utf-8\r\n");
    }
    if (chunked) {
        n += snprintf(out + n, sizeof(out) - n, "Transfer-Encoding: chunked\r\n\r\n");
    } else if (have_len) {
        n += snprintf(out + n, sizeof(out) - n, "Content-Length: %u\r\n\r\n", (unsigned)clen);
    } else {
        n += snprintf(out + n, sizeof(out) - n, "\r\n");   /* close-delimited body */
    }
    r->head_sent = true;
    r->chunked   = chunked;
    if (conn_write_all(r->conn, out, (size_t)n) < 0) {
        r->failed = true;
        return -1;
    }
    return 0;
}

/* Send buffered data as one chunk (or raw, if no chunked framing)
*/
static int resp_emit(ph8_resp_t *r) {

    if (r->used == 0 || r->head_only)
        return 0;
    
    size_t n = r->used;
    r->used = 0;

    uint8_t *data = r->buf + CHUNK_PREFIX;

    int rc;

    if (!r->chunked) {
        rc = conn_write_all(r->conn, data, n);
    } else {
        /* "<hex>\r\n" is written right-aligned in front of the data, and
         * "\r\n" right behind it, so the whole chunk goes out in one send. */
        char tmp[CHUNK_PREFIX + 1];
        int  hl = snprintf(tmp, sizeof(tmp), "%zx\r\n", n);
        uint8_t *start = data - hl;
        memcpy(start, tmp, (size_t)hl);
        data[n]     = '\r';
        data[n + 1] = '\n';
        rc = conn_write_all(r->conn, start, (size_t)hl + n + 2);
    }
    if (rc < 0) {
        r->failed = true;
    }
    return rc;
}

static int resp_flush_full(ph8_resp_t *r)
{
    if (!r->head_sent) {
        if (resp_send_head(r, r->chunked_ok, false, 0) < 0) {
            return -1;
        }
    }
    return resp_emit(r);
}

/* Write to the internal buffer, flushing as necessary
 *
 */
int ph8_resp_write(ph8_resp_t *r, const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    if (r->failed || r->sealed || r->aborted) {
        return -1;
    }
    while (len > 0) {
        if (r->used == r->cap && resp_flush_full(r) < 0) {
            return -1;
        }
        size_t n = r->cap - r->used;
        if (n > len) {
            n = len;
        }
        memcpy(r->buf + CHUNK_PREFIX + r->used, p, n);
        r->used += n;
        p       += n;
        len     -= n;
    }
    return 0;
}

/* formatted ph8_resp_write()
 *
 */
int ph8_resp_printf(ph8_resp_t *r, const char *fmt, ...) {
    char    tmp[256];
    va_list ap;

    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return -1;
    }
    if ((size_t)n < sizeof(tmp)) {
        return ph8_resp_write(r, tmp, (size_t)n);
    }
    char *big = malloc((size_t)n + 1);
    if (!big) {
        return -1;
    }
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    int rc = ph8_resp_write(r, big, (size_t)n);
    free(big);
    return rc;
}

/*
 *
 *
 */
int ph8_resp_flush(ph8_resp_t *r) {

    if (r->failed || r->aborted) {
        return -1;
    }
    if (!r->head_sent || r->used > 0) {
        return resp_flush_full(r);
    }
    return 0;
}

/* Mark response as bad, so subsequent operations on `r` always fail
 *
 */
void ph8_resp_abort(ph8_resp_t *r) {
    r->aborted = true;
    r->used = 0;                    /* drop buffered output */
}

/* Send static file.
 *
 * Slow path:
 *
 *  If headers were sent or if there is any data queued in output buffer: fallback to 
 *  ph8_resp_write() path. This path copies data into internal buffer and flushes it 
 *  when it gets full.
 *
 * Fast path:
 *
 *  For mmap()ed files  or any arbitrary in-memory data arrays we write buffer to the 
 *  client connection directly.
 */
int ph8_resp_send_static(ph8_resp_t *r, const void *data, size_t len) {
    if (r->failed || r->sealed || r->aborted) {
        return -1;
    }
    if (r->head_sent || r->used > 0) {
        /* Too late for Content-Length: fall back to the copying path. 
           TODO: can we just update Content-Length header in place takibg r->used into account?
        */
        return ph8_resp_write(r, data, len);
    }
    r->sealed = true;
    /* Send header, disable chunked */
    if (resp_send_head(r, false, true, len) < 0) {
        return -1;
    }
    /* No body, no pro^H^H^H we are done */
    if (r->head_only || len == 0) {
        return 0;
    }
    /* In one go */
    if (conn_write_all(r->conn, data, len) < 0) {
        r->failed = true;
        return -1;
    }
    return 0;
}
/* This one is called by the worker when script is finished
 * Send unsent data or signal errors
 */
static void resp_finish(ph8_resp_t *r) {

    if (r->aborted) {
        /* Nothing sent yet: a clean 500 */
        if (!r->head_sent && !r->failed) {
            send_error(r->conn, 500);
        } else {
         /*Otherwise leave the response unterminated (no final 0): the connection is closed by the
         * worker and client receives an error, not a truncated "OK". 
         */
        }
        return;
    }
    if (r->failed || r->sealed) {
        return;
    }
    if (!r->head_sent) {
        /* Whole response fit into the buffer: plain Content-Length. */
        if (resp_send_head(r, false, true, r->used) < 0) {
            return;
        }
        if (!r->head_only && r->used > 0) {
            if (conn_write_all(r->conn, r->buf + CHUNK_PREFIX, r->used) < 0) {
                r->failed = true;
            }
        }
        r->used = 0;
        return;
    }
    if (resp_emit(r) < 0) {
        return;
    }
    /* Final 0 */
    if (r->chunked && !r->head_only) {
        conn_write_all(r->conn, "0\r\n\r\n", 5);
    }
}


/* Request body. */


/*
 * Helpers
*/
static const struct phr_header *find_hdr(const struct phr_header *h, size_t n, const char *name)
{
    size_t nl = strlen(name);
    for (size_t i = 0; i < n; i++) {
        if (h[i].name && h[i].name_len == nl && strncasecmp(h[i].name, name, nl) == 0) {
            return &h[i];
        }
    }
    return NULL;
}

static int count_hdr(const struct phr_header *h, size_t n, const char *name)
{
    size_t nl = strlen(name);
    int cnt = 0;
    for (size_t i = 0; i < n; i++) {
        if (h[i].name && h[i].name_len == nl && strncasecmp(h[i].name, name, nl) == 0) {
            cnt++;
        }
    }
    return cnt;
}

static bool parse_content_length(const struct phr_header *h, size_t *out)
{
    if (h->value_len == 0 || h->value_len > 9) {      /* <= 999 999 999, no overflow */
        return false;
    }
    size_t v = 0;
    for (size_t i = 0; i < h->value_len; i++) {
        char ch = h->value[i];
        if (ch < '0' || ch > '9') {
            return false;
        }
        v = v * 10 + (size_t)(ch - '0');
    }
    *out = v;
    return true;
}

#define SEND_CONTINUE(c) \
    conn_write_all((c), "HTTP/1.1 100 Continue\r\n\r\n", sizeof("HTTP/1.1 100 Continue\r\n\r\n") - 1)

/*
 * Reads the request body. `pre` = body bytes that already arrived together
 * with the headers.
 * Returns 0 on success (*body is NULL when there is no body, else malloc'd
 * and NUL-terminated), an HTTP status code to answer with, or -1 if the
 * connection is dead (answer nothing).
 */
static int read_body(ph8_conn *c, const struct phr_header *h, size_t nh, int minor,
                     const uint8_t *pre, size_t pre_len,
                     uint8_t **body, size_t *body_len) {
    const size_t max_body = g_srv.cfg.max_body_size;
    const struct phr_header *te = find_hdr(h, nh, "transfer-encoding");
    const struct phr_header *ex = find_hdr(h, nh, "expect");
    int    ncl = count_hdr(h, nh, "content-length");
    bool   expect100 = false;
    int    err = 0;
    uint8_t *buf = NULL;

    *body = NULL;
    *body_len = 0;

    if (ex) {
        if (ex->value_len == 12 && strncasecmp(ex->value, "100-continue", 12) == 0) {
            expect100 = (minor >= 1);
        } else {
            return 417;
        }
    }

    if (te || ncl) {
        const struct phr_header *ce = find_hdr(h, nh, "content-encoding");
        if (ce && !(ce->value_len == 8 && strncasecmp(ce->value, "identity", 8) == 0)) {
            return 415;             /* gzip/deflate/br request bodies are not decoded */
        }
    }

    if (te) {
        /* ---- Transfer-Encoding: chunked ---- */
        if (ncl || minor < 1) {
            return 400;                     /* smuggling guard / 1.0 can't chunk */
        }
        if (!(te->value_len == 7 && strncasecmp(te->value, "chunked", 7) == 0)) {
            return 501;
        }
        const size_t hard_cap = max_body + 1024;   /* raw framing overhead slack */
        if (pre_len + 1 > hard_cap) {
            return 413;
        }
        size_t cap = pre_len + 2048;
        if (cap > hard_cap) {
            cap = hard_cap;
        }
        buf = big_alloc(cap);
        if (!buf) {
            return 500;
        }
        memcpy(buf, pre, pre_len);

        struct phr_chunked_decoder dec;
        memset(&dec, 0, sizeof(dec));
        dec.consume_trailer = 1;

        size_t decoded = 0;       /* decoded bytes at buf[0..decoded)            */
        size_t pending = pre_len; /* raw bytes sitting at buf[decoded..] to feed */
        bool   sent100 = false;

        for (;;) {
            if (pending > 0) {
                size_t sz = pending;
                ssize_t r = phr_decode_chunked(&dec, (char *)buf + decoded, &sz);
                decoded += sz;
                if (r == -1) { err = 400; goto fail; }
                if (decoded > max_body) { err = 413; goto fail; }
                if (r >= 0) {
                    break;                          /* complete */
                }
            }
            if (expect100 && !sent100) {
                if (SEND_CONTINUE(c) < 0) { err = -1; goto fail; }
                sent100 = true;
            }
            if (cap - decoded - 1 < 512) {          /* grow */
                size_t ncap = cap * 2;
                if (ncap > hard_cap) {
                    ncap = hard_cap;
                }
                if (ncap - decoded - 1 < 1) { err = 413; goto fail; }
                uint8_t *np = big_realloc(buf, ncap);
                if (!np) { err = 500; goto fail; }
                buf = np;
                cap = ncap;
            }
            int n = c->read(c, buf + decoded, cap - decoded - 1);
            if (n <= 0) { err = -1; goto fail; }
            pending = (size_t)n;
        }
        buf[decoded] = 0;
        *body = buf;
        *body_len = decoded;
        return 0;
    }

    if (ncl == 0) {
        return 0;                                   /* no body */
    }
    if (ncl > 1) {
        return 400;
    }

    /* ---- Content-Length ---- */
    size_t cl;
    if (!parse_content_length(find_hdr(h, nh, "content-length"), &cl)) {
        return 400;
    }
    if (cl > max_body) {
        return 413;
    }
    if (cl == 0) {
        return 0;
    }
    buf = big_alloc(cl + 1);
    if (!buf) {
        return 500;
    }
    size_t have = pre_len < cl ? pre_len : cl;
    memcpy(buf, pre, have);
    if (have < cl && expect100) {
        if (SEND_CONTINUE(c) < 0) { err = -1; goto fail; }
    }
    while (have < cl) {
        int n = c->read(c, buf + have, cl - have);
        if (n <= 0) { err = -1; goto fail; }
        have += (size_t)n;
    }
    buf[cl] = 0;
    *body = buf;
    *body_len = cl;
    return 0;

fail:
    free(buf);
    return err;
}



/* Connection handling
*/


typedef struct {
    const char *name;
    uint16_t    bit;
} method_def_t;

static const method_def_t k_methods[] = {
    { "GET", PH8_M_GET },         { "HEAD", PH8_M_HEAD },       { "POST", PH8_M_POST },
    { "PUT", PH8_M_PUT },         { "DELETE", PH8_M_DELETE },   { "PATCH", PH8_M_PATCH },
    { "OPTIONS", PH8_M_OPTIONS }, { "CONNECT", PH8_M_CONNECT }, { "TRACE", PH8_M_TRACE },
};
#define N_METHODS (sizeof(k_methods) / sizeof(k_methods[0]))

/* Host header against cfg.allowed_hosts. The port is ignored; IPv6 literals
 * are compared with their brackets, e.g. "[::1]". 
 */
static bool host_allowed(const struct phr_header *host)
{
    const char *const *list = g_srv.cfg.allowed_hosts;
    const char *v = host->value;
    size_t n = host->value_len;

    if (list == NULL) {
        return true;
    }
    if (n > 0 && v[0] == '[') {
        const char *e = memchr(v, ']', n);
        if (e == NULL) {
            return false;
        }
        n = (size_t)(e - v) + 1;
    } else {
        const char *colon = memchr(v, ':', n);
        if (colon != NULL) {
            n = (size_t)(colon - v);
        }
    }
    for (; *list != NULL; list++) {
        if (strlen(*list) == n && strncasecmp(*list, v, n) == 0) {
            return true;
        }
    }
    return false;
}

/*
 * Cheap checks done before any body is read.
 * Returns 0, or the status to answer with (405 also fills `allow` with a
 * complete "Allow: ...\r\n" header line).
 */
static int check_request(const char *m, size_t ml, int minor,
                         const struct phr_header *h, size_t nh,
                         char *allow, size_t allow_n)
{
    uint16_t bit = 0;
    size_t i;
    int nhost;

    for (i = 0; i < N_METHODS; i++) {
        if (strlen(k_methods[i].name) == ml && memcmp(k_methods[i].name, m, ml) == 0) {
            bit = k_methods[i].bit;
            break;
        }
    }
    if (bit == 0) {
        return 501;                         /* not a method we know at all */
    }
    if ((g_srv.cfg.method_mask & bit) == 0) {
        size_t o = (size_t)snprintf(allow, allow_n, "Allow:");
        const char *sep = " ";
        for (i = 0; i < N_METHODS && o < allow_n; i++) {
            if (g_srv.cfg.method_mask & k_methods[i].bit) {
                o += (size_t)snprintf(allow + o, allow_n - o, "%s%s", sep, k_methods[i].name);
                sep = ", ";
            }
        }
        if (o < allow_n) {
            snprintf(allow + o, allow_n - o, "\r\n");
        }
        return 405;
    }

    nhost = count_hdr(h, nh, "host");
    if (nhost > 1 || (nhost == 0 && minor >= 1)) {
        return 400;                         /* RFC 9112: exactly one Host is required in 1.1 */
    }
    if (nhost == 1 && !host_allowed(find_hdr(h, nh, "host"))) {
        return 403;                         /* DNS rebinding / wrong name */
    }
    return 0;
}

/* This is what worker does on new connection.
 */
static void handle_connection(worker_t *w, ph8_conn *c) {

    const char *method, *path;
    size_t      method_len, path_len, prev_len = 0, len = 0, nh;
    int         minor_version, pret;
    struct phr_header headers[MAX_HEADERS]; /* 16/24 bytes times 32 */

    /* Read data from the client and submit it to PicoHttpParser
     * Once decoded, perform basic checks on the request; Allocate response and execute HTTPD handler
     */
    for (;;) {

        /* Read data from a client, put into internal request buffer */
        int n = c->read(c, w->reqbuf + len, g_srv.cfg.req_buf_size - len);
        if (n <= 0) {
            return;                         /* timeout, reset or EOF */
        }
        prev_len = len;
        len += (size_t)n;

        /* Binary garbage as the first byte is almost always a TLS CilentHello
         * on a plain-HTTP port; Answering in plaintext is useless, just close
         */
        if (prev_len == 0) {
            uint8_t b = w->reqbuf[0];
            if ((b < 0x20 && b != '\r' && b != '\n') || b >= 0x80) {
                ESP_LOGW(TAG, "[w%d] non-HTTP data (first byte 0x%02x), closing", w->id, b);
                return;
            }
        }

        nh = MAX_HEADERS;
        pret = phr_parse_request((const char *)w->reqbuf, len,
                                 &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &nh, prev_len);
        if (pret > 0) {
            break;                          /* headers complete */
        }

        /* PicoParser can not parse received data */
        if (pret == -1) {
            /* picohttpparser leaves the number of parsed headers in nh;
             * MAX_HEADERS there means "too many headers". */
            send_error(c, nh == MAX_HEADERS ? 431 : 400);
            return;
        }

        /* We hit the request buffer limit when reading from client: it is a fatal error;
        * See if we had \n in the received stream. If there was no \n that means URI was too big.
        * Otherwise it is the header block that does not fit
        */
        if (len == g_srv.cfg.req_buf_size) {
            send_error(c, memchr(w->reqbuf, '\n', len) ? 431 : 414);
            return;
        }
    }

    /* Basic checks for the request: check if method is allowed; 
     * check if Host is allowed (there are allowed hosts list in g_srv.cfg)
     */
    {
        char allow[96];
        int code = check_request(method, method_len, minor_version, headers, nh, allow, sizeof(allow));
        if (code != 0) {
            send_error_ex(c, code, code == 405 ? allow : NULL);
            return;
        }
    }

    /* So far so good - read the body
     */
    uint8_t *body = NULL;
    size_t   body_len = 0;
    int rc = read_body(c, headers, nh, minor_version, w->reqbuf + pret, len - (size_t)pret, &body, &body_len);
    if (rc != 0) {
        if (rc > 0) {
            send_error(c, rc);
        }
        return;
    }

    /* Split "/path?query" once, here: the handler and the VM get both parts. */
    const char *qmark = memchr(path, '?', path_len);
    size_t plen = qmark ? (size_t)(qmark - path) : path_len;
    size_t qlen = qmark ? path_len - plen - 1 : 0;

    ESP_LOGI(TAG, "[w%d] %.*s %.*s%s%.*s%s (body %u)", w->id,
             (int)method_len, method,
             (int)plen, path,
             qmark ? " ?" : "",
             (int)(qlen > LOG_QUERY_MAX ? LOG_QUERY_MAX : qlen), qmark ? qmark + 1 : "",
             qlen > LOG_QUERY_MAX ? "..." : "",
             (unsigned)body_len);

    /* Initialize `rq` (a 'Request' buffer)
     * Contains parsed HTTP request
     */
    ph8_request_t rq = {
        .method = method,
        .method_len = method_len,
        .target = path,
        .target_len = path_len,
        .path = path,
        .path_len = plen,
        .query = qmark ? qmark + 1 : NULL,
        .query_len = qlen,
        .minor_version = minor_version,
        .headers = headers,
        .num_headers = nh,
        .body = body ? body : (const uint8_t *)"",
        .body_len = body_len,
    };

    /* Initialize a response buffer `rs`
     * This is where all script output go
     */
    ph8_resp_t rs = { 0 };

    resp_init(&rs, c, w->outbuf, g_srv.cfg.out_buf_size, minor_version >= 1, method_is(&rq,"HEAD"));


   /* and call our handler */
    g_srv.cfg.handler(&rq, &rs);

    /* send whats left, close connection, free memory */
    resp_finish(&rs);

    free(body);
}


/* Tasks
 *  There are 1 listening task (acceptor) and number of processing tasks (workers)
 * All of them are created whn HTTP server starts; Workers go IDLE (hanging on a queue semaphore)
 * While acceptor wait for connections, initializes ph8_conn structure for every new connection 
 * and wakes one of worker threads up, sending a socket via queue
 */


/* A worker task; Sleeps all day, only wakes up when there is something new in the queue
*/
static void worker_task(void *arg) {

    worker_t *w = (worker_t *)arg;
    int fd;

    for (;;) {
        /* Block here if there are no connections pending */
        if (xQueueReceive(g_srv.queue, &fd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        /* Set socket options for a client */
        struct timeval tv = {
            .tv_sec  = g_srv.cfg.io_timeout_ms / 1000,
            .tv_usec = (g_srv.cfg.io_timeout_ms % 1000) * 1000,
        };
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        /* Allocate PLAIN connection (not TLS)*/
        ph8_conn conn;
        conn_init_plain(&conn, fd);   /* TLS: handshake here, swap vtable */

        /* Handle connection; this is where all magic happens */
        handle_connection(w, &conn);

        conn.close(&conn);

        /* block on the semaphore. worker is idle again */
        xSemaphoreGive(g_srv.free_slots);   
    }
}

/* Listen and accept connections. Wake workers up to handle connections
 */
static void acceptor_task(void *arg) {

  arg = arg;

  for (;;) {

    struct sockaddr_in6 src; // for future extensions 
    socklen_t slen = sizeof(src);

    int fd = accept(g_srv.listen_fd, (struct sockaddr *)&src, &slen);
    if (fd < 0) {
      ESP_LOGW(TAG, "accept failed: errno %d", errno);
      vTaskDelay(pdMS_TO_TICKS(500));   /* e.g. out of sockets, don't spin */
      continue;
    }

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    /* Take semaphore with a 0 timeout to check if there are idle workers waiting for a client */
    if (xSemaphoreTake(g_srv.free_slots, 0) != pdTRUE) {

      /* No idle worker. Never block the acceptor here: no lingering.
       * Create a temporary connection just to send the error message and close it
       */
       ph8_conn c;
       conn_init_plain(&c, fd);
       send_error(&c, 503);
       /* TODO: grateful close via shutdown()? */
       close(fd);
       continue;
    }

     /* Send fd to the queue, waking up one of the workers. First one wins, others continue to sleep */
    if (xQueueSend(g_srv.queue, &fd, 0) != pdTRUE) {
      /* cannot happen: queue len == workers */
      ESP_LOGE(TAG, "xQueueSend() : drop client, queue full");
      close(fd);                        
      xSemaphoreGive(g_srv.free_slots);
    }
  }
}


/* Start HTTP server: create & bind socket, create acceptor and workers tasks
 * Parse configuration (check docroot)
 */
esp_err_t ph8_httpd_start(const ph8_httpd_cfg_t *cfg) {

  struct stat st;
  size_t n;

  /* Sanity checks */
  if (!cfg || !cfg->handler || !cfg->docroot || !cfg->engine) {
    ESP_LOGE(TAG, "required configuration parameter is NULL");
    return ESP_FAIL;
  }

  /* Copy configuration to a global variable */
  g_srv.cfg = *cfg;

  /* Attach the PHP engine instance to the HTTPD server.
   * Set document root directory
   */
  n = strlen(g_srv.cfg.docroot);
  while (n > 1 && g_srv.cfg.docroot[n - 1] == '/') 
    n--;
    
  if (n == 0 || n >= SITE_ROOT_MAX) {
     ESP_LOGE(TAG, "malformed docroot");
     return ESP_FAIL;
  }
    
  memcpy(g_root, g_srv.cfg.docroot, n);
  g_root[n] = '\0';

  if (stat(g_root, &st) < 0 || !S_ISDIR(st.st_mode)) {
    ESP_LOGE(TAG, "docroot does not exists or is not a directory");
    return ESP_FAIL;
  }

  g_root_len = n;
  g_engine = g_srv.cfg.engine;

  /* Set up safe defaults */
  if (g_srv.cfg.method_mask == 0)
    g_srv.cfg.method_mask = PH8_M_DEFAULT;
    

  int j = 0;                  /* Number of worker tasks actually started */
  n = g_srv.cfg.max_workers;  /* Number of worker tasks we want to start */

  if (n > 127) /* just to limit array size below */
    n = 127;
  TaskHandle_t tasks[n];

  memset(tasks,0,sizeof(tasks));

  if (NULL != (g_srv.queue = xQueueCreate(n, sizeof(int)))) {
    if (NULL != (g_srv.free_slots = xSemaphoreCreateCounting(n, n))) {

      /* Create listener */
      if ( 0 <= (g_srv.listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP))) {

        int one = 1;
        setsockopt(g_srv.listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        struct sockaddr_in addr = {
          .sin_family      = AF_INET,
          .sin_port        = htons(g_srv.cfg.port),
          .sin_addr.s_addr = htonl(INADDR_ANY),
        };

        if (bind(g_srv.listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
          if (0 == listen(g_srv.listen_fd, PH8_HTTPD_BACKLOG )) {
            /* Spawn workers */
            
            for (size_t i = 0; i < n; i++) { // -Wsign-compare is annoying!
              worker_t *w = calloc(1, sizeof(*w));
              if (w != NULL) {

                w->id     = i;
                w->reqbuf = big_alloc(g_srv.cfg.req_buf_size);
                w->outbuf = big_alloc(CHUNK_PREFIX + g_srv.cfg.out_buf_size + 2);

                if (w->reqbuf != NULL && w->outbuf != NULL) {

                  char name[16];
                  snprintf(name, sizeof(name), "ph8w%d", (int)i);

                  if (xTaskCreate(worker_task, name, g_srv.cfg.worker_stack, w, g_srv.cfg.worker_prio, &tasks[j]) == pdPASS) {
                    j++;
                    continue ;
                  }

                  ESP_LOGE(TAG, "can not start worker: xTaskCreate() failed");
                  
                }

                if (w->reqbuf) free(w->reqbuf);
                if (w->reqbuf) free(w->outbuf);
                free(w);
              }
            }

            if (j > 0) {
              /* Spawn the acceptor if we have spawned at least one worker */
              if (xTaskCreate(acceptor_task, "ph8acc", 4096, NULL, g_srv.cfg.acceptor_prio, &tasks[j]) == pdPASS) {
                ESP_LOGI(TAG, "[%s], listening on :%u, %d workers",g_root,  g_srv.cfg.port, (int)n);
                return ESP_OK;
              } else {
                ESP_LOGI(TAG, "xTaskCreate(): failed to spawn acceptor");
              }
            } else {
              ESP_LOGI(TAG, "No workers were spawn");
            }
          } else {
            ESP_LOGI(TAG, "listen(): system call failed, errno = %d", errno);
          }
        } else {
          ESP_LOGI(TAG, "bind(): system call failed, errno = %d", errno);
        }
      } else {
        ESP_LOGI(TAG, "socket(): system call failed, errno = %d", errno);
      }
    } else {
      ESP_LOGI(TAG, "xSemaphoreCreateCounting(): system call failed, errno = %d", errno);
    }
  } else {
    ESP_LOGI(TAG, "xQueueCreate(): system call failed, errno = %d", errno);
  }


  /* Deallocate system resources: tasks, semaphore and a queue */
  ESP_LOGE(TAG, "HTTP server failed to start");

  /* Close listening socket */
  if (g_srv.listen_fd >= 0) {
    close(g_srv.listen_fd);
    g_srv.listen_fd = -1;
  }

  /* Kill tasks */
  for (n = 0; n < g_srv.cfg.max_workers; n++)
    if (tasks[n] != NULL)
      vTaskDelete(tasks[n]);

  /* Delete semaphore and a queue. Tasks must be dead by this time */
  if (g_srv.free_slots != NULL) {
    vSemaphoreDelete(g_srv.free_slots);
    g_srv.free_slots = NULL;
  }

  if (g_srv.queue != NULL) {
    vQueueDelete(g_srv.queue);
    g_srv.queue = NULL;
  }

  return ESP_FAIL;
}
