#ifndef SHIM_ESP_HEAP_CAPS_H
#define SHIM_ESP_HEAP_CAPS_H
#include <stdlib.h>
/* No PSRAM on the host: every "caps" request is plain malloc/realloc. */
#define MALLOC_CAP_SPIRAM   (1u << 10)
#define MALLOC_CAP_INTERNAL (1u << 11)
#define MALLOC_CAP_8BIT     (1u << 2)
#define MALLOC_CAP_DEFAULT  (1u << 12)
#define heap_caps_malloc(n, caps)                 malloc(n)
#define heap_caps_realloc(p, n, caps)             realloc((p), (n))
#define heap_caps_malloc_prefer(n, num, ...)      malloc(n)
#define heap_caps_realloc_prefer(p, n, num, ...)  realloc((p), (n))
#endif
