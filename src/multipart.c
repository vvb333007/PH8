#include "ph7int.h"

#include <string.h>

#define MP_BOUNDARY_MAX 70          /* RFC 2046 */

static char lc(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static int is_ws(char c)
{
    return c == ' ' || c == '\t';
}

/* a[0..n) equals lit, ASCII case-insensitive */
static int ieq_n(const char *a, size_t n, const char *lit)
{
    size_t i;
    if (strlen(lit) != n) {
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (lc(a[i]) != lc(lit[i])) {
            return 0;
        }
    }
    return 1;
}

static int iprefix(const char *a, size_t n, const char *lit)
{
    size_t i, m = strlen(lit);
    if (n < m) {
        return 0;
    }
    for (i = 0; i < m; i++) {
        if (lc(a[i]) != lc(lit[i])) {
            return 0;
        }
    }
    return 1;
}

/*
 * Finds parameter `key` in a header value like:  form-data; name="a"; filename="b"
 * Matching is by whole parameter name, so "name" does not match "filename" or
 * "filename*". Quoted values may contain ';' and \" or \\ escapes (returned raw).
 */
static int mp_param(const char *v, size_t n, const char *key, const char **out, size_t *out_n)
{
    const char *end = v + n;
    const char *p;

    if (n == 0 || (p = memchr(v, ';', n)) == NULL) {
        return 0;
    }
    p++;
    while (p < end) {
        const char *k0, *k1, *v0, *v1;

        while (p < end && is_ws(*p)) {
            p++;
        }
        k0 = p;
        while (p < end && *p != '=' && *p != ';') {
            p++;
        }
        k1 = p;
        while (k1 > k0 && is_ws(k1[-1])) {
            k1--;
        }
        if (p >= end || *p == ';') {            /* parameter without a value */
            if (p < end) {
                p++;
            }
            continue;
        }
        p++;                                    /* '=' */
        while (p < end && is_ws(*p)) {
            p++;
        }
        if (p < end && *p == '"') {
            p++;
            v0 = p;
            while (p < end && *p != '"') {
                if (*p == '\\' && p + 1 < end && (p[1] == '"' || p[1] == '\\')) {
                    p++;
                }
                p++;
            }
            v1 = p;
            if (p < end) {
                p++;                            /* closing quote */
            }
        } else {
            v0 = p;
            while (p < end && *p != ';') {
                p++;
            }
            v1 = p;
            while (v1 > v0 && is_ws(v1[-1])) {
                v1--;
            }
        }
        if (ieq_n(k0, (size_t)(k1 - k0), key)) {
            *out = v0;
            *out_n = (size_t)(v1 - v0);
            return 1;
        }
        while (p < end && *p != ';') {
            p++;
        }
        if (p < end) {
            p++;
        }
    }
    return 0;
}

int ph8_mp_boundary(const char *ct, size_t n, char *out, size_t out_n)
{
    static const char type[] = "multipart/form-data";
    const size_t tl = sizeof(type) - 1;
    const char *b;
    size_t bn, i;

    if (ct == NULL || out == NULL || out_n == 0 || !iprefix(ct, n, type)) {
        return PH8_MP_BAD;
    }
    if (n > tl && ct[tl] != ';' && !is_ws(ct[tl])) {
        return PH8_MP_BAD;                      /* e.g. "multipart/form-datax" */
    }
    if (!mp_param(ct, n, "boundary", &b, &bn)) {
        return PH8_MP_BAD;
    }
    if (bn == 0 || bn > MP_BOUNDARY_MAX || bn >= out_n) {
        return PH8_MP_BAD;
    }
    for (i = 0; i < bn; i++) {
        unsigned char c = (unsigned char)b[i];
        /* printable ASCII only; no quotes or backslashes (keeps escaping out of the picture) */
        if (c < 0x20 || c > 0x7e || c == '"' || c == '\\') {
            return PH8_MP_BAD;
        }
    }
    if (b[bn - 1] == ' ') {
        return PH8_MP_BAD;
    }
    memcpy(out, b, bn);
    out[bn] = '\0';
    return 0;
}

/* First occurrence of delim (which starts with '\r') in s[0..n), or NULL. */
static const uint8_t *find_delim(const uint8_t *s, size_t n, const char *delim, size_t dn)
{
    const uint8_t *p = s, *end = s + n;

    while ((size_t)(end - p) >= dn) {
        p = memchr(p, '\r', (size_t)(end - p) - dn + 1);
        if (p == NULL) {
            return NULL;
        }
        if (memcmp(p, delim, dn) == 0) {
            return p;
        }
        p++;
    }
    return NULL;
}

typedef struct {
    const char *disp;  size_t disp_n;
    const char *ctype; size_t ctype_n;
} mp_hdrs_t;

/* Header block of one part (without the final CRLF CRLF). Strict: CRLF line
 * ends only, no obs-fold. Returns 0 or PH8_MP_BAD. */
static int parse_headers(const char *h, size_t n, mp_hdrs_t *out)
{
    const char *end = h + n;

    memset(out, 0, sizeof(*out));
    while (h < end) {
        size_t rem = (size_t)(end - h), i = 0, line_n;
        const char *line = h, *colon, *name, *val;
        size_t name_n, val_n;

        while (i + 1 < rem && !(h[i] == '\r' && h[i + 1] == '\n')) {
            i++;
        }
        if (i + 1 < rem) {
            line_n = i;
            h += i + 2;
        } else {
            line_n = rem;
            h = end;
        }
        if (line_n == 0 || is_ws(line[0]) || memchr(line, '\r', line_n) || memchr(line, '\n', line_n)) {
            return PH8_MP_BAD;
        }
        colon = memchr(line, ':', line_n);
        if (colon == NULL) {
            return PH8_MP_BAD;
        }
        name = line;
        name_n = (size_t)(colon - line);
        while (name_n > 0 && is_ws(name[name_n - 1])) {
            name_n--;
        }
        if (name_n == 0) {
            return PH8_MP_BAD;
        }
        val = colon + 1;                        /* value = (colon+1 .. end of line), trimmed */
        val_n = (size_t)((line + line_n) - val);
        while (val_n > 0 && is_ws(*val)) {
            val++;
            val_n--;
        }
        while (val_n > 0 && is_ws(val[val_n - 1])) {
            val_n--;
        }
        if (ieq_n(name, name_n, "content-disposition")) {
            out->disp = val;
            out->disp_n = val_n;
        } else if (ieq_n(name, name_n, "content-type")) {
            out->ctype = val;
            out->ctype_n = val_n;
        }
    }
    return 0;
}

int ph8_mp_parse(const uint8_t *body, size_t len, const char *boundary,
                 const ph8_mp_limits_t *lim, ph8_mp_cb cb, void *user)
{
    char delim[4 + MP_BOUNDARY_MAX + 1];        /* "\r\n--" + boundary */
    size_t bl, dl, pos;
    size_t max_parts = 32, max_hdr = 1024, max_name = 256;
    size_t nparts = 0;
    int delivered = 0;

    if (body == NULL || boundary == NULL || cb == NULL) {
        return PH8_MP_BAD;
    }
    if (lim != NULL) {
        max_parts = lim->max_parts ? lim->max_parts : max_parts;
        max_hdr   = lim->max_header_len ? lim->max_header_len : max_hdr;
        max_name  = lim->max_name_len ? lim->max_name_len : max_name;
    }
    bl = strlen(boundary);
    if (bl == 0 || bl > MP_BOUNDARY_MAX) {
        return PH8_MP_BAD;
    }
    memcpy(delim, "\r\n--", 4);
    memcpy(delim + 4, boundary, bl);
    dl = 4 + bl;

    /* First delimiter: at the very start (no CRLF before it) or after a preamble. */
    if (len >= dl - 2 && memcmp(body, delim + 2, dl - 2) == 0) {
        pos = dl - 2;
    } else {
        const uint8_t *m = find_delim(body, len, delim, dl);
        if (m == NULL) {
            return PH8_MP_BAD;
        }
        pos = (size_t)(m - body) + dl;
    }

    for (;;) {
        const uint8_t *hs, *he, *data, *next;
        size_t hlen, data_start;
        mp_hdrs_t H;
        const char *nm = NULL, *fn = NULL;
        size_t nn = 0, fnn = 0;
        int has_fn;

        /* After a delimiter: "--" ends the body; else transport padding, then CRLF. */
        if (len - pos >= 2 && body[pos] == '-' && body[pos + 1] == '-') {
            return delivered;                   /* epilogue is ignored */
        }
        while (pos < len && is_ws((char)body[pos])) {
            pos++;
        }
        if (len - pos < 2 || body[pos] != '\r' || body[pos + 1] != '\n') {
            return PH8_MP_BAD;
        }
        pos += 2;

        if (++nparts > max_parts) {
            return PH8_MP_LIMIT;
        }

        /* Header block: empty (CRLF right away) or up to CRLF CRLF. */
        hs = body + pos;
        if (len - pos >= 2 && hs[0] == '\r' && hs[1] == '\n') {
            hlen = 0;
            data_start = pos + 2;
        } else {
            size_t scan = len - pos;
            if (scan > max_hdr + 4) {
                scan = max_hdr + 4;
            }
            he = find_delim(hs, scan, "\r\n\r\n", 4);
            if (he == NULL) {
                return (len - pos > max_hdr + 4) ? PH8_MP_LIMIT : PH8_MP_BAD;
            }
            hlen = (size_t)(he - hs);
            data_start = pos + hlen + 4;
        }
        if (parse_headers((const char *)hs, hlen, &H) != 0) {
            return PH8_MP_BAD;
        }

        data = body + data_start;
        next = find_delim(data, len - data_start, delim, dl);
        if (next == NULL) {
            return PH8_MP_BAD;                  /* truncated body */
        }

        if (H.disp != NULL && iprefix(H.disp, H.disp_n, "form-data")
            && mp_param(H.disp, H.disp_n, "name", &nm, &nn)) {
            ph8_mp_part_t part;

            has_fn = mp_param(H.disp, H.disp_n, "filename", &fn, &fnn);
            if (nn > max_name || (has_fn && fnn > max_name)) {
                return PH8_MP_LIMIT;
            }
            part.name = nm;
            part.name_len = nn;
            part.filename = has_fn ? fn : NULL;
            part.filename_len = has_fn ? fnn : 0;
            part.ctype = H.ctype;
            part.ctype_len = H.ctype_n;
            part.data = data;
            part.data_len = (size_t)(next - data);
            if (cb(&part, user) != 0) {
                return PH8_MP_ABORT;
            }
            delivered++;
        }
        pos = (size_t)(next - body) + dl;
    }
}
