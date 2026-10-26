/*
 * ph8_vm_set_request(): fill HTTP superglobals from an already parsed request.
 *
 * This file is #included at the very END of vm.c, because it uses static
 * helpers of that file: VmExtractSuper(), VmHashmapInsert(),
 * VmHttpSplitEncodedQuery(), VmHttpPorcessCookie().
 *
 * NOT covered yet (see notes at the bottom):
 *   - raw body access (php://input)
 *   - array syntax in field names (a[]=1, a[b]=2) for urlencoded and multipart
 */



/* Multipart limits: small forms with small files. */
#define MP_MAX_PARTS  32
#define MP_MAX_HDR    1024
#define MP_MAX_NAME   256
#define MP_MAX_FILE   (1024 * 1024)   /* per file; bigger -> $_FILES[..]["error"] = 1, no content */

#define SUPER_MAP(vm_, lit_)  VmSuperMap((vm_), (lit_), sizeof(lit_) - 1)
static ph7_hashmap *VmSuperMap(ph7_vm *pVm, const char *zName, sxu32 nByte) {
  ph7_value *pValue = VmExtractSuper(pVm, zName, nByte);
  if (pValue == 0 || (pValue->iFlags & MEMOBJ_HASHMAP) == 0) {
    return 0;
  }
  return (ph7_hashmap *)pValue->x.pOther;
}

static void ReqSet(ph7_hashmap *pMap, const char *zKey, int nKey, const char *zVal, size_t nVal) {
  if (pMap) {
    VmHashmapInsert(pMap, zKey, nKey, zVal, (int)nVal);
  }
}

#define REQ_HDR(h_, lit_)  ReqHdrIs((h_), (lit_), sizeof(lit_) - 1)
static int ReqHdrIs(const ph8_http_header *h, const char *zName, size_t nName) {
  return h->name != 0 && h->name_len == nName && SyStrnicmp(h->name, zName, (sxu32)nName) == 0;
}

/* "User-Agent" -> "HTTP_USER_AGENT". The fixed prefix guarantees that a client
 * header can never overwrite REQUEST_METHOD, QUERY_STRING and friends. */
static int ReqServerKey(char *zOut, size_t nOut, const char *zName, size_t nName) {
  static const char zPrefix[] = "HTTP_";
  size_t i, n = sizeof(zPrefix) - 1;
  if (nName == 0 || n + nName >= nOut) {
    return 0;
  }
  memcpy(zOut, zPrefix, n);
  for (i = 0; i < nName; i++) {
    char c = zName[i];
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - 'a' + 'A');
    } else if (c == '-') {
      c = '_';
    }
    zOut[n++] = c;
  }
  zOut[n] = '\0';
  return (int)n;
}

static int ReqIsUrlEncoded(const ph8_http_header *h) {
  static const char zType[] = "application/x-www-form-urlencoded";
  size_t n = sizeof(zType) - 1;
  if (h == 0 || h->value_len < n || SyStrnicmp(h->value, zType, (sxu32)n) != 0) {
    return 0;
  }
  /* accept "...urlencoded" and "...urlencoded; charset=UTF-8" */
  return h->value_len == n || h->value[n] == ';' || h->value[n] == ' ' || h->value[n] == '\t';
}



static int ReqIsMultipart(const ph8_http_header *h) {
  static const char zType[] = "multipart/form-data";
  size_t n = sizeof(zType) - 1;
  if (h == 0 || h->value_len < n || SyStrnicmp(h->value, zType, (sxu32)n) != 0) {
    return 0;
  }
  return h->value_len == n || h->value[n] == ';' || h->value[n] == ' ' || h->value[n] == '\t';
}

/* ---- $_FILES support. Reviewed against hashmap.c / api.c: PH7_NewHashmap() returns a map with
 *      iRef == 1 that belongs to its creator, PH7_HashmapInsert() stores a COPY of a value
 *      (a hashmap value gets +1 iRef), so releasing our temporary afterwards leaves exactly one
 *      owner. $_FILES always exists (PH7_HashmapCreateSuper). Not run against the engine yet. ---- */

static void ReqInt(ph7_hashmap *pMap, const char *zKey, sxu32 nKey, sxi64 iVal) {
  ph7_value sKey, sVal;
  PH7_MemObjInitFromString(pMap->pVm, &sKey, 0);
  PH7_MemObjStringAppend(&sKey, zKey, nKey);
  PH7_MemObjInitFromInt(pMap->pVm, &sVal, iVal);
  PH7_HashmapInsert(pMap, &sKey, &sVal);
  PH7_MemObjRelease(&sKey);
  PH7_MemObjRelease(&sVal);
}

/* Insert pChild (a fresh hashmap, we own its initial reference) under zKey */
static void ReqMapInto(ph7_hashmap *pParent, const char *zKey, sxu32 nKey, ph7_hashmap *pChild) {
  ph7_value sKey, sVal;
  PH7_MemObjInitFromString(pParent->pVm, &sKey, 0);
  PH7_MemObjStringAppend(&sKey, zKey, nKey);
  PH7_MemObjInitFromArray(pParent->pVm, &sVal, pChild);   /* no extra reference: ours is the initial iRef == 1 */
  PH7_HashmapInsert(pParent, &sKey, &sVal); /* stores a copy: the container takes its own reference */
  PH7_MemObjRelease(&sKey);
  PH7_MemObjRelease(&sVal);                 /* drops ours */
}

typedef struct {
  ph7_vm *pVm;
  ph7_hashmap *pPost, *pRequest, *pFiles;
} ReqMp;

/*
 * $_FILES["field"] = [name, type, size, error, tmp_name, content]
 *   - small files (<= 1 MB) are handed over as a string in "content": there is no
 *     temporary file, so no flash wear and no filesystem dependency. tmp_name is "".
 *     Save with file_put_contents($dst, $_FILES['f']['content']).
 *   - error: 0 ok, 1 file too big (no content), 4 no file chosen (UPLOAD_ERR_*)
 */
static int ReqMpPart(const ph8_mp_part_t *p, void *pUser) {
  ReqMp *m = (ReqMp *)pUser;
  ph7_hashmap *pEntry;
  int err;

  if (p->name_len == 0) {
    return 0;
  }
  if (p->filename == 0) {                       /* ordinary field -> $_POST and $_REQUEST */
    ReqSet(m->pPost, p->name, (int)p->name_len, (const char *)p->data, p->data_len);
    ReqSet(m->pRequest, p->name, (int)p->name_len, (const char *)p->data, p->data_len);
    return 0;
  }
  if (m->pFiles == 0 && (m->pFiles = SUPER_MAP(m->pVm, "_FILES")) == 0) {
    return 1;                                   /* out of memory: abort */
  }
  err = (p->filename_len == 0 && p->data_len == 0) ? 4 : (p->data_len > MP_MAX_FILE ? 1 : 0);
  pEntry = PH7_NewHashmap(m->pVm, 0, 0);
  if (pEntry == 0) {
    return 1;
  }
  ReqSet(pEntry, "name", 4, p->filename, p->filename_len);
  ReqSet(pEntry, "type", 4, p->ctype ? p->ctype : "", p->ctype ? p->ctype_len : 0);
  ReqSet(pEntry, "tmp_name", 8, "", 0);
  ReqInt(pEntry, "error", 5, err);
  ReqInt(pEntry, "size", 4, err ? 0 : (sxi64)p->data_len);
  if (err == 0) {
    ReqSet(pEntry, "content", 7, (const char *)p->data, p->data_len);
  }
  ReqMapInto(m->pFiles, p->name, (sxu32)p->name_len, pEntry);
  return 0;
}

/* SXRET_OK, SXERR_SYNTAX (malformed -> answer 400), SXERR_LIMIT (-> 413), SXERR_MEM */
static int ReqParseMultipart(ph7_vm *pVm, const ph8_http_header *pCType, const uint8_t *body, size_t n) {
  ph8_mp_limits_t lim;
  char zBoundary[80];
  ReqMp m;
  int rc;

  if (ph8_mp_boundary(pCType->value, pCType->value_len, zBoundary, sizeof(zBoundary)) != 0) {
    return SXERR_SYNTAX;
  }
  lim.max_parts = MP_MAX_PARTS;
  lim.max_header_len = MP_MAX_HDR;
  lim.max_name_len = MP_MAX_NAME;
  m.pVm = pVm;
  m.pPost = SUPER_MAP(pVm, "_POST");
  m.pRequest = SUPER_MAP(pVm, "_REQUEST");
  m.pFiles = 0;
  rc = ph8_mp_parse(body, n, zBoundary, &lim, ReqMpPart, &m);
  if (rc >= 0) {
    return SXRET_OK;
  }
  return rc == PH8_MP_LIMIT ? SXERR_LIMIT : (rc == PH8_MP_ABORT ? SXERR_MEM : SXERR_SYNTAX);
}

/*
 * Empties the request superglobals so the same VM can serve another request.
 * Uses PH7_HashmapRelease(map, FALSE): the same primitive as array_erase().
 * $_SERVER["SCRIPT_FILENAME"] is set once by PH7_HashmapCreateSuper and is kept.
 *
 * NOTE: this is only part of reusing a VM. PH7_VmReset() keeps global variables,
 * constants, static members and the include_once list, so prefer a fresh VM per request.
 */
int ph8_vm_clear_request(ph7_vm *pVm) {
  static const char *const aNames[] = { "_SERVER", "_HEADER", "_COOKIE", "_GET", "_POST", "_FILES", "_REQUEST" };
  char zScript[256];
  int nScript = 0;
  ph7_value *pSrv;
  size_t i;

  if (pVm == 0) {
    return SXERR_INVALID;
  }
  pSrv = VmExtractSuper(pVm, "_SERVER", sizeof("_SERVER") - 1);
  if (pSrv && (pSrv->iFlags & MEMOBJ_HASHMAP)) {
    ph7_value *pSf = ph7_array_fetch(pSrv, "SCRIPT_FILENAME", -1);
    if (pSf) {
      int n = 0;
      const char *z = ph7_value_to_string(pSf, &n);
      if (n > 0 && n < (int)sizeof(zScript)) {
        memcpy(zScript, z, (size_t)n);
        nScript = n;
      }
    }
  }
  for (i = 0; i < sizeof(aNames) / sizeof(aNames[0]); i++) {
    ph7_value *pV = VmExtractSuper(pVm, aNames[i], (sxu32)strlen(aNames[i]));
    if (pV && (pV->iFlags & MEMOBJ_HASHMAP)) {
      ph7_hashmap *pMap = (ph7_hashmap *)pV->x.pOther;
      if (pMap->nEntry > 0) {
        PH7_HashmapRelease(pMap, FALSE);
      }
    }
  }
  if (nScript > 0) {
    ReqSet(SUPER_MAP(pVm, "_SERVER"), "SCRIPT_FILENAME", 15, zScript, (size_t)nScript);
  }
  return SXRET_OK;
}

int ph8_vm_set_request(ph7_vm *pVm, const ph8_http_request *rq) {
  const ph8_http_header *pCType = 0;
  ph7_hashmap *pServer, *pHeader;
  const char *zQuery;
  SyString sPart;
  SyBlob sWorker;
  char zKey[128];
  size_t nPath, i;
  int rc = SXRET_OK;

  if (pVm == 0 || rq == 0 || rq->method == 0 || rq->target == 0) {
    return SXERR_INVALID;
  }
  if (pVm->nMagic != PH7_VM_RUN) {
    return SXERR_CORRUPT; /* not compiled yet, or already executing */
  }
  pServer = SUPER_MAP(pVm, "_SERVER");
  pHeader = SUPER_MAP(pVm, "_HEADER"); /* Symisc extension, may be absent */
  if (pServer == 0) {
    return SXERR_NOTFOUND;
  }

  /* A fresh VM is assumed. To serve another request with the same VM call ph8_vm_clear_request() first. */

  /* Split "/path?query". No decoding and no normalization here: the router is
   * responsible for percent-decoding and rejecting ".." before touching files. */
  zQuery = memchr(rq->target, '?', rq->target_len);
  nPath = zQuery ? (size_t)(zQuery - rq->target) : rq->target_len;

  SyBlobInit(&sWorker, &pVm->sAllocator);

  /* ---- $_SERVER ---- */
  ReqSet(pServer, "SERVER_PROTOCOL", -1, rq->minor_version >= 1 ? "HTTP/1.1" : "HTTP/1.0", 8);
  ReqSet(pServer, "REQUEST_METHOD", -1, rq->method, rq->method_len);
  ReqSet(pServer, "REQUEST_URI", -1, rq->target, rq->target_len);
  /* PH7 compatibility: it stored the path part here */
  ReqSet(pServer, "PATH_INFO", -1, rq->target, nPath);
  ReqSet(pServer, "ORIG_PATH_INFO", -1, rq->target, nPath);
  /* Always present, empty when there is no query (like PHP). Unlike PH7: for every method */
  ReqSet(pServer, "QUERY_STRING", -1, zQuery ? zQuery + 1 : "", zQuery ? rq->target_len - nPath - 1 : 0);

  /* ---- headers: $_SERVER[HTTP_*], $_HEADER[Original-Name], $_COOKIE ---- */
  for (i = 0; i < rq->num_headers; i++) {
    const ph8_http_header *h = &rq->headers[i];
    if (h->name == 0 || h->name_len == 0) {
      continue; /* obs-fold continuation: deprecated (RFC 9112) and a smuggling vector, ignored */
    }
    ReqSet(pHeader, h->name, (int)h->name_len, h->value, h->value_len);

    if (REQ_HDR(h, "Content-Type")) {
      pCType = h;
      ReqSet(pServer, "CONTENT_TYPE", -1, h->value, h->value_len); /* CGI names: no HTTP_ prefix */
    } else if (REQ_HDR(h, "Content-Length")) {
      ReqSet(pServer, "CONTENT_LENGTH", -1, h->value, h->value_len);
    } else {
      int nKey = ReqServerKey(zKey, sizeof(zKey), h->name, h->name_len);
      if (nKey > 0) {
        ReqSet(pServer, zKey, nKey, h->value, h->value_len);
      }
    }
    if (REQ_HDR(h, "Authorization")) { /* PH7 compatibility */
      ReqSet(pServer, "PHP_AUTH_DIGEST", -1, h->value, h->value_len);
      ReqSet(pServer, "PHP_AUTH", -1, h->value, h->value_len);
    }
    if (REQ_HDR(h, "Cookie") && h->value_len > 0) {
      VmHttpPorcessCookie(pVm, &sWorker, h->value, (sxu32)h->value_len);
    }
  }

  /* ---- $_GET (+$_REQUEST) ---- */
  if (zQuery && rq->target_len - nPath > 1) {
    SyStringInitFromBuf(&sPart, zQuery + 1, (sxu32)(rq->target_len - nPath - 1));
    VmHttpSplitEncodedQuery(pVm, &sPart, &sWorker, FALSE);
  }

  /* ---- body: $_POST (+$_REQUEST) and $_FILES ---- */
  if (rq->method_len == 4 && memcmp(rq->method, "POST", 4) == 0
      && rq->body != 0 && rq->body_len > 0) {
    if (ReqIsUrlEncoded(pCType)) {
      SyStringInitFromBuf(&sPart, (const char *)rq->body, (sxu32)rq->body_len);
      VmHttpSplitEncodedQuery(pVm, &sPart, &sWorker, TRUE);
    } else if (ReqIsMultipart(pCType)) {
      rc = ReqParseMultipart(pVm, pCType, rq->body, rq->body_len);
    }
  }

  SyBlobRelease(&sWorker);
  return rc;
}

/*
 * Notes:
 *  - Reusing a VM: ph8_vm_clear_request() clears the superglobals, but ph7_vm_reset() keeps global
 *    variables, define()d constants, static members and the include_once list. Without
 *    clearing those, request N can see request N-1's data: use a VM per request.
 *  - php://input: add `const uint8_t *pReqBody; sxu32 nReqBody;` to ph7_vm, store
 *    rq->body there and expose it through a read-only stream.
 *  - Duplicate headers: the last one wins. RFC 9110 says join with ", " (except Cookie).
 */
