/*
 * PH7-ESP32 — embeddable PHP bytecode compiler & virtual machine,
 * ported and maintained for ESP32/FreeRTOS, Linux and Windows(Cygwin).
 *
 * Copyright (C) 2011-2018 Symisc Systems <http://ph7.symisc.net/>
 * Copyright (C) 2026-present Viacheslav Logunov <vvb333007@gmail.com>
 *
 * Licensed under the Symisc Public License (modified BSD with mandatory
 * source disclosure, clause 3); commercial licensing available from
 * Symisc Systems. Full terms: see LICENSE. Contacts: see CONTACTS.
 */

#pragma once
/*
 * multipart/form-data parser, in-memory and zero-copy: the whole body is already
 * in RAM (the server enforces max_body_size), parts are delivered as pointers
 * into it. No allocation, no globals, safe to call from many tasks at once.
 */

#include <stddef.h>
#include <stdint.h>

#define PH8_MP_OK      0
#define PH8_MP_BAD    (-1)   /* malformed body or boundary */
#define PH8_MP_LIMIT  (-2)   /* a limit from ph8_mp_limits_t was exceeded */
#define PH8_MP_ABORT  (-3)   /* the callback returned non-zero */

typedef struct {
    const char    *name;     size_t name_len;      /* raw bytes between the quotes, NOT unescaped */
    const char    *filename; size_t filename_len;  /* NULL: ordinary field; len 0: empty file input */
    const char    *ctype;    size_t ctype_len;     /* the part's Content-Type, or NULL */
    const uint8_t *data;     size_t data_len;
} ph8_mp_part_t;

/* Return non-zero to stop parsing (-> PH8_MP_ABORT). All pointers are valid
 * only during the call (they point into the request body). */
typedef int (*ph8_mp_cb)(const ph8_mp_part_t *part, void *user);

typedef struct {
    size_t max_parts;        /* parts in one body            (0 = 32)   */
    size_t max_header_len;   /* header block of ONE part     (0 = 1024) */
    size_t max_name_len;     /* field name and filename      (0 = 256)  */
} ph8_mp_limits_t;

/* Extracts the boundary from a "multipart/form-data; boundary=..." value
 * (quoted or not) into `out` (NUL-terminated). 0 on success, PH8_MP_BAD otherwise. */
int ph8_mp_boundary(const char *content_type, size_t len, char *out, size_t out_n);

/* Returns the number of parts delivered to `cb` (>= 0) or a negative PH8_MP_* code.
 * `lim` may be NULL (defaults). Parts without a Content-Disposition "name" are skipped. */
int ph8_mp_parse(const uint8_t *body, size_t len, const char *boundary,
                 const ph8_mp_limits_t *lim, ph8_mp_cb cb, void *user);
