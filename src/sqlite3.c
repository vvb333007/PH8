/*
 * SQLite wrapper for PH8: skeleton.
 *
 * The wrapper is a set of PHP-visible functions which mirror the SQLite C API
 * (sqlite3_open, sqlite3_prepare, sqlite3_bind_value, ...). A database and a
 * statement are passed to the script as resources. The SQLite3 / SQLite3Stmt /
 * SQLite3Result classes of PHP can later be put on top of these functions,
 * every method of the class maps to exactly one function here.
 *
 * ASSUMPTIONS, check them against your ph7.h:
 *   - function registration:    ph7_create_function(), ph7_create_constant()
 *   - resources:                ph7_result_resource(), ph7_value_to_resource(),
 *                               ph7_value_is_resource()
 *   - the user data given to ph7_create_function() is returned by
 *     ph7_context_user_data()
 *   - arrays and scalars:       ph7_context_new_array(), ph7_context_new_scalar(),
 *                               ph7_array_add_strkey_elem(), ph7_array_add_intkey_elem(),
 *                               ph7_value_reset_string_cursor(), ph7_result_value()
 *
 * Design notes:
 *   - Resources carry no type, so every structure starts with a magic number.
 *   - All wrapper structures are allocated from the VM allocator (pVm->sAllocator),
 *     so the VM arena owns their memory. The memory of SQLite itself is NOT
 *     covered by it: SQLite has its own global allocator (sqlite3_config()).
 *   - Because of that every connection a script opened must be closed before
 *     the script run ends. All handles are registered in a per-VM structure
 *     (ph7_sqlite) and PH8_sqlite_reset() closes everything the script forgot
 *     to close. Call it when a script run finishes (also when the same VM is
 *     going to run a script again) and before the VM is released.
 *   - A closed handle is not freed at once (the script still holds the pointer).
 *     It is only marked as closed and freed in PH8_sqlite_reset().
 *   - Result rows are read with one row look-ahead: execute() makes the first
 *     sqlite3_step(), every fetch_array() returns the pending row and steps once
 *     more. This way execute() also works for INSERT/UPDATE statements.
 */

#include "ph7int.h"  /* needed for the VM allocator: pVm->sAllocator */
#include "sqlite3.h" /* SQLite3 library header */

/* PH8's MemObj of 'resource' type carries no type of what it is under the hood */
enum {
  PH8_SQLITE_DB_MAGIC   = 0x53514C44u, /* 'SQLD' */
  PH8_SQLITE_STMT_MAGIC = 0x53514C53u, /* 'SQLS' */
};

/* Fetch modes, same values as SQLITE3_ASSOC, SQLITE3_NUM and SQLITE3_BOTH in PHP 
*/
enum {
  PH8_SQLITE_ASSOC = 1,
  PH8_SQLITE_NUM   = 2,
  PH8_SQLITE_BOTH  = 3,
};

//typedef struct ph7_sqlite ph7_sqlite;
typedef struct PhSqliteDb PhSqliteDb;
typedef struct PhSqliteStmt PhSqliteStmt;

/* SQLite3 statement resource 
*/
struct PhSqliteStmt {
  unsigned int nMagic;
  sqlite3_stmt *pStmt;  /* NULL when finalized */
  PhSqliteDb *pOwner;
  PhSqliteStmt *pNext;  /* next statement of the same connection */
  int iStepRc;          /* result of the last sqlite3_step(): SQLITE_ROW means an unread row is pending */
};

/* SQLite3 DB resource 
*/
struct PhSqliteDb {
  unsigned int nMagic;
  sqlite3 *pDb;         /* NULL when closed */
  ph7_sqlite *pVm;
  PhSqliteDb *pNext;    /* next connection of the same VM */
  PhSqliteStmt *pFirstStmt;
};

struct ph7_sqlite {
  PhSqliteDb *pFirstDb;
  SyMemBackend *pAllocator; /* &pVm->sAllocator of the owning VM */
};


/* Handle management                                                        
 *  Finalize all statements of a connection and close it. The structures stay allocated.
*/
static void PhSqliteCloseDb(PhSqliteDb *pDb) {

  PhSqliteStmt *pStmt;

  if (pDb->pDb == NULL) {
    return;
  }
  for (pStmt = pDb->pFirstStmt; pStmt != NULL; pStmt = pStmt->pNext) {
    if (pStmt->pStmt != NULL) {
      sqlite3_finalize(pStmt->pStmt);
      pStmt->pStmt = NULL;
    }
  }
  sqlite3_close(pDb->pDb);
  pDb->pDb = NULL;
}

/* Allocate a zeroed chunk from the VM allocator 
*/
static void *PhSqliteAlloc(ph7_sqlite *pState, sxu32 nBytes) {
  void *pMem = SyMemBackendPoolAlloc(pState->pAllocator, nBytes);

  if (pMem != NULL) {
    SyZero(pMem, nBytes);
  }
  return pMem;
}

static void PhSqliteFree(ph7_sqlite *pState, void *pMem) {
  SyMemBackendPoolFree(pState->pAllocator, pMem);
}

/* Create the per-VM state in the VM allocator 
*/
static ph7_sqlite *PhSqliteVmCreate(ph7_vm *pVm) {

  ph7_sqlite *pState = (ph7_sqlite *)SyMemBackendPoolAlloc(&pVm->sAllocator, sizeof(ph7_sqlite));

  if (pState != NULL) {
    pState->pFirstDb = NULL;
    pState->pAllocator = &pVm->sAllocator;
  }
  return pState;
}

/*
 * Close and free every connection and statement the script opened.
 * The state itself stays valid, it belongs to the VM arena.
 * Call it when a script run finishes and before the VM is released.
 */
void ph7_sqlite_reset(ph7_sqlite *pState) {
  PhSqliteDb *pDb;
  PhSqliteDb *pNextDb;
  PhSqliteStmt *pStmt;
  PhSqliteStmt *pNextStmt;

  if (pState == NULL) {
    return;
  }
  for (pDb = pState->pFirstDb; pDb != NULL; pDb = pNextDb) {
    pNextDb = pDb->pNext;
    PhSqliteCloseDb(pDb);
    for (pStmt = pDb->pFirstStmt; pStmt != NULL; pStmt = pNextStmt) {
      pNextStmt = pStmt->pNext;
      PhSqliteFree(pState, pStmt);
    }
    PhSqliteFree(pState, pDb);
  }
  pState->pFirstDb = NULL;
}

/* Get an open connection from an argument. Throws a warning and returns NULL if it is not one. */
static PhSqliteDb *PhSqliteGetDb(ph7_context *pCtx, ph7_value *pArg) {
  PhSqliteDb *pDb;

  if (!ph7_value_is_resource(pArg)) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Expecting a database resource");
    return NULL;
  }
  pDb = (PhSqliteDb *)ph7_value_to_resource(pArg);
  if (pDb == NULL || pDb->nMagic != PH8_SQLITE_DB_MAGIC) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Not a database resource");
    return NULL;
  }
  if (pDb->pDb == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Database is already closed");
    return NULL;
  }
  return pDb;
}

/* Get a live statement from an argument */
static PhSqliteStmt *PhSqliteGetStmt(ph7_context *pCtx, ph7_value *pArg) {
  PhSqliteStmt *pStmt;

  if (!ph7_value_is_resource(pArg)) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Expecting a statement resource");
    return NULL;
  }
  pStmt = (PhSqliteStmt *)ph7_value_to_resource(pArg);
  if (pStmt == NULL || pStmt->nMagic != PH8_SQLITE_STMT_MAGIC) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Not a statement resource");
    return NULL;
  }
  if (pStmt->pStmt == NULL) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "Statement is already finalized");
    return NULL;
  }
  return pStmt;
}

/* Return false when the script passed too few arguments */
static int PhSqliteCheckArgs(ph7_context *pCtx, int nArg, int nMinimum, const char *zFunc) {
  if (nArg < nMinimum) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "%s() expects at least %d argument(s)", zFunc, nMinimum);
    return 0;
  }
  return 1;
}


/* Value conversion                                                         */


/* Store column iCol of the current row into a PH8 value, preserving the SQLite type */
static void PhSqliteColumnToValue(ph7_value *pValue, sqlite3_stmt *pStmt, int iCol) {
  const char *zData;
  int nBytes;

  switch (sqlite3_column_type(pStmt, iCol)) {
    case SQLITE_INTEGER:
      ph7_value_int64(pValue, sqlite3_column_int64(pStmt, iCol));
      break;
    case SQLITE_FLOAT:
      ph7_value_double(pValue, sqlite3_column_double(pStmt, iCol));
      break;
    case SQLITE_NULL:
      ph7_value_null(pValue);
      break;
    case SQLITE_BLOB:
      zData = (const char *)sqlite3_column_blob(pStmt, iCol);
      nBytes = sqlite3_column_bytes(pStmt, iCol);
      ph7_value_string(pValue, zData != NULL ? zData : "", nBytes);
      break;
    default: /* SQLITE_TEXT */
      zData = (const char *)sqlite3_column_text(pStmt, iCol);
      nBytes = sqlite3_column_bytes(pStmt, iCol);
      ph7_value_string(pValue, zData != NULL ? zData : "", nBytes);
      break;
  }
}

/*
 * Bind a PH8 value to a statement parameter.
 * iType is one of SQLITE_INTEGER, SQLITE_FLOAT, SQLITE_TEXT, SQLITE_BLOB, SQLITE_NULL
 * or 0, which means "choose by the type of the value".
 * SQLITE_TRANSIENT makes SQLite copy the data, the value can change before sqlite3_step().
 */
static int PhSqliteBindValue(sqlite3_stmt *pStmt, int iParam, ph7_value *pValue, int iType) {
  const char *zData;
  int nBytes;

  if (iType == 0) {
    if (ph7_value_is_null(pValue)) {
      iType = SQLITE_NULL;
    } else if (ph7_value_is_int(pValue) || ph7_value_is_bool(pValue)) {
      iType = SQLITE_INTEGER;
    } else if (ph7_value_is_float(pValue)) {
      iType = SQLITE_FLOAT;
    } else {
      iType = SQLITE_TEXT;
    }
  }

  switch (iType) {
    case SQLITE_INTEGER:
      return sqlite3_bind_int64(pStmt, iParam, ph7_value_to_int64(pValue));
    case SQLITE_FLOAT:
      return sqlite3_bind_double(pStmt, iParam, ph7_value_to_double(pValue));
    case SQLITE_NULL:
      return sqlite3_bind_null(pStmt, iParam);
    case SQLITE_BLOB:
      zData = ph7_value_to_string(pValue, &nBytes);
      return sqlite3_bind_blob(pStmt, iParam, zData, nBytes, SQLITE_TRANSIENT);
    default: /* SQLITE_TEXT */
      zData = ph7_value_to_string(pValue, &nBytes);
      return sqlite3_bind_text(pStmt, iParam, zData, nBytes, SQLITE_TRANSIENT);
  }
}


/* Script-visible functions                                                 */


/* resource|false sqlite3_open(string $zPath) */
static int PH8_sqlite3_open(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  ph7_sqlite *pVm = (ph7_sqlite *)ph7_context_user_data(pCtx);
  PhSqliteDb *pDb;
  const char *zPath;
  int nPathLen;
  int iRc;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_open")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  zPath = ph7_value_to_string(apArg[0], &nPathLen);

  pDb = (PhSqliteDb *)PhSqliteAlloc(pVm, sizeof(PhSqliteDb));
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb->nMagic = PH8_SQLITE_DB_MAGIC;
  pDb->pVm = pVm;

  iRc = sqlite3_open(zPath, &pDb->pDb);
  if (iRc != SQLITE_OK) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "Cannot open database: %s",
                                   pDb->pDb != NULL ? sqlite3_errmsg(pDb->pDb) : "out of memory");
    if (pDb->pDb != NULL) {
      sqlite3_close(pDb->pDb);
    }
    PhSqliteFree(pVm, pDb);
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  /* Register the connection, so it can be closed when the VM goes away */
  pDb->pNext = pVm->pFirstDb;
  pVm->pFirstDb = pDb;

  ph7_result_resource(pCtx, pDb);
  return PH7_OK;
}

/* bool sqlite3_close(resource $oDb) */
static int PH8_sqlite3_close(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_close")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  PhSqliteCloseDb(pDb);
  ph7_result_bool(pCtx, 1);
  return PH7_OK;
}

/* bool sqlite3_exec(resource $oDb, string $zSql) */
static int PH8_sqlite3_exec(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;
  const char *zSql;
  char *zErrMsg = NULL;
  int nSqlLen;
  int iRc;

  if (!PhSqliteCheckArgs(pCtx, nArg, 2, "sqlite3_exec")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  zSql = ph7_value_to_string(apArg[1], &nSqlLen);

  iRc = sqlite3_exec(pDb->pDb, zSql, NULL, NULL, &zErrMsg);
  if (iRc != SQLITE_OK) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "sqlite3_exec(): %s", zErrMsg != NULL ? zErrMsg : "unknown error");
    sqlite3_free(zErrMsg);
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_bool(pCtx, 1);
  return PH7_OK;
}

/* mixed sqlite3_query_single(resource $oDb, string $zSql): first column of the first row, null if no rows, false on error */
static int PH8_sqlite3_query_single(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;
  sqlite3_stmt *pStmt = NULL;
  ph7_value *pValue;
  const char *zSql;
  int nSqlLen;
  int iRc;

  if (!PhSqliteCheckArgs(pCtx, nArg, 2, "sqlite3_query_single")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  zSql = ph7_value_to_string(apArg[1], &nSqlLen);

  iRc = sqlite3_prepare_v2(pDb->pDb, zSql, nSqlLen, &pStmt, NULL);
  if (iRc != SQLITE_OK) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "sqlite3_query_single(): %s", sqlite3_errmsg(pDb->pDb));
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  iRc = sqlite3_step(pStmt);
  if (iRc == SQLITE_ROW) {
    pValue = ph7_context_new_scalar(pCtx);
    if (pValue != NULL) {
      PhSqliteColumnToValue(pValue, pStmt, 0);
      ph7_result_value(pCtx, pValue);
    } else {
      ph7_result_bool(pCtx, 0);
    }
  } else if (iRc == SQLITE_DONE) {
    ph7_result_null(pCtx);
  } else {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "sqlite3_query_single(): %s", sqlite3_errmsg(pDb->pDb));
    ph7_result_bool(pCtx, 0);
  }

  sqlite3_finalize(pStmt);
  return PH7_OK;
}

/* resource|false sqlite3_prepare(resource $oDb, string $zSql) */
static int PH8_sqlite3_prepare(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;
  PhSqliteStmt *pStmt;
  const char *zSql;
  int nSqlLen;
  int iRc;

  if (!PhSqliteCheckArgs(pCtx, nArg, 2, "sqlite3_prepare")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  zSql = ph7_value_to_string(apArg[1], &nSqlLen);

  pStmt = (PhSqliteStmt *)PhSqliteAlloc(pDb->pVm, sizeof(PhSqliteStmt));
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt->nMagic = PH8_SQLITE_STMT_MAGIC;
  pStmt->pOwner = pDb;
  pStmt->iStepRc = SQLITE_DONE;

  iRc = sqlite3_prepare_v2(pDb->pDb, zSql, nSqlLen, &pStmt->pStmt, NULL);
  if (iRc != SQLITE_OK) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "sqlite3_prepare(): %s", sqlite3_errmsg(pDb->pDb));
    PhSqliteFree(pDb->pVm, pStmt);
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  /* Register the statement in its connection */
  pStmt->pNext = pDb->pFirstStmt;
  pDb->pFirstStmt = pStmt;

  ph7_result_resource(pCtx, pStmt);
  return PH7_OK;
}

/* bool sqlite3_bind_value(resource $oStmt, int|string $mParam, mixed $mValue [, int $iType]) */
static int PH8_sqlite3_bind_value(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteStmt *pStmt;
  int iParam;
  int iType = 0;
  int iRc;

  if (!PhSqliteCheckArgs(pCtx, nArg, 3, "sqlite3_bind_value")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt = PhSqliteGetStmt(pCtx, apArg[0]);
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  if (ph7_value_is_string(apArg[1])) {
    /* Named parameter, the name must include the prefix, i.e. ':name' */
    iParam = sqlite3_bind_parameter_index(pStmt->pStmt, ph7_value_to_string(apArg[1], NULL));
  } else {
    iParam = ph7_value_to_int(apArg[1]);
  }
  if (iParam <= 0 || iParam > sqlite3_bind_parameter_count(pStmt->pStmt)) {
    ph7_context_throw_error(pCtx, PH7_CTX_WARNING, "sqlite3_bind_value(): no such parameter");
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  if (nArg > 3) {
    iType = ph7_value_to_int(apArg[3]);
  }

  iRc = PhSqliteBindValue(pStmt->pStmt, iParam, apArg[2], iType);
  ph7_result_bool(pCtx, iRc == SQLITE_OK);
  return PH7_OK;
}

/* bool sqlite3_execute(resource $oStmt): restart the statement and make the first step */
static int PH8_sqlite3_execute(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteStmt *pStmt;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_execute")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt = PhSqliteGetStmt(pCtx, apArg[0]);
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  sqlite3_reset(pStmt->pStmt);
  pStmt->iStepRc = sqlite3_step(pStmt->pStmt);

  if (pStmt->iStepRc != SQLITE_ROW && pStmt->iStepRc != SQLITE_DONE) {
    ph7_context_throw_error_format(pCtx, PH7_CTX_WARNING, "sqlite3_execute(): %s", sqlite3_errmsg(pStmt->pOwner->pDb));
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_bool(pCtx, 1);
  return PH7_OK;
}

/* bool sqlite3_reset(resource $oStmt) */
static int PH8_sqlite3_reset(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteStmt *pStmt;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_reset")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt = PhSqliteGetStmt(pCtx, apArg[0]);
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  sqlite3_reset(pStmt->pStmt);
  pStmt->iStepRc = SQLITE_DONE;
  ph7_result_bool(pCtx, 1);
  return PH7_OK;
}

/* array|false sqlite3_fetch_array(resource $oStmt [, int $iMode = 3]) */
static int PH8_sqlite3_fetch_array(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteStmt *pStmt;
  ph7_value *pArray;
  ph7_value *pValue;
  int iMode = PH8_SQLITE_BOTH;
  int nColumns;
  int iCol;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_fetch_array")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt = PhSqliteGetStmt(pCtx, apArg[0]);
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  if (nArg > 1) {
    iMode = ph7_value_to_int(apArg[1]);
  }

  /* No pending row: the result set is exhausted */
  if (pStmt->iStepRc != SQLITE_ROW) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  /* Both values are released automatically when the call returns */
  pArray = ph7_context_new_array(pCtx);
  pValue = ph7_context_new_scalar(pCtx);
  if (pArray == NULL || pValue == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }

  nColumns = sqlite3_column_count(pStmt->pStmt);
  for (iCol = 0; iCol < nColumns; iCol++) {
    PhSqliteColumnToValue(pValue, pStmt->pStmt, iCol);
    if (iMode & PH8_SQLITE_NUM) {
      ph7_array_add_intkey_elem(pArray, iCol, pValue);
    }
    if (iMode & PH8_SQLITE_ASSOC) {
      ph7_array_add_strkey_elem(pArray, sqlite3_column_name(pStmt->pStmt, iCol), pValue);
    }
    ph7_value_reset_string_cursor(pValue);
  }
  ph7_result_value(pCtx, pArray);

  /* The row is copied, now it is safe to move on to the next one */
  pStmt->iStepRc = sqlite3_step(pStmt->pStmt);
  return PH7_OK;
}

/* bool sqlite3_finalize(resource $oStmt) */
static int PH8_sqlite3_finalize(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteStmt *pStmt;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_finalize")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pStmt = PhSqliteGetStmt(pCtx, apArg[0]);
  if (pStmt == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  sqlite3_finalize(pStmt->pStmt);
  pStmt->pStmt = NULL;
  ph7_result_bool(pCtx, 1);
  return PH7_OK;
}

/* int sqlite3_last_insert_rowid(resource $oDb) */
static int PH8_sqlite3_last_insert_rowid(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_last_insert_rowid")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_int64(pCtx, sqlite3_last_insert_rowid(pDb->pDb));
  return PH7_OK;
}

/* int sqlite3_changes(resource $oDb) */
static int PH8_sqlite3_changes(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_changes")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_int(pCtx, sqlite3_changes(pDb->pDb));
  return PH7_OK;
}

/* string sqlite3_last_error_msg(resource $oDb) */
static int PH8_sqlite3_last_error_msg(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_last_error_msg")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_string(pCtx, sqlite3_errmsg(pDb->pDb), -1);
  return PH7_OK;
}

/* int sqlite3_last_error_code(resource $oDb) */
static int PH8_sqlite3_last_error_code(ph7_context *pCtx, int nArg, ph7_value **apArg) {
  PhSqliteDb *pDb;

  if (!PhSqliteCheckArgs(pCtx, nArg, 1, "sqlite3_last_error_code")) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  pDb = PhSqliteGetDb(pCtx, apArg[0]);
  if (pDb == NULL) {
    ph7_result_bool(pCtx, 0);
    return PH7_OK;
  }
  ph7_result_int(pCtx, sqlite3_errcode(pDb->pDb));
  return PH7_OK;
}


/* Registration                                                             */




static const struct {

  const char *zName;
  int (*xFunc)(ph7_context *, int, ph7_value **);

} aSqliteFuncs[] = {

  { "sqlite3_open",             PH8_sqlite3_open },
  { "sqlite3_close",            PH8_sqlite3_close },
  { "sqlite3_exec",             PH8_sqlite3_exec },
  { "sqlite3_query_single",     PH8_sqlite3_query_single },
  { "sqlite3_prepare",          PH8_sqlite3_prepare },
  { "sqlite3_bind_value",       PH8_sqlite3_bind_value },
  { "sqlite3_execute",          PH8_sqlite3_execute },
  { "sqlite3_reset",            PH8_sqlite3_reset },
  { "sqlite3_fetch_array",      PH8_sqlite3_fetch_array },
  { "sqlite3_finalize",         PH8_sqlite3_finalize },
  { "sqlite3_last_insert_rowid", PH8_sqlite3_last_insert_rowid },
  { "sqlite3_changes",          PH8_sqlite3_changes },
  { "sqlite3_last_error_msg",   PH8_sqlite3_last_error_msg },
  { "sqlite3_last_error_code",  PH8_sqlite3_last_error_code },

};


/* The type constants have the same values in PHP and in SQLite, so they can be taken from sqlite3.h */
static const struct {

  const char *zName;
  int         nValue;

} aSqliteConsts[] = {

  { "SQLITE3_ASSOC",   PH8_SQLITE_ASSOC },
  { "SQLITE3_NUM",     PH8_SQLITE_NUM },
  { "SQLITE3_BOTH",    PH8_SQLITE_BOTH },
  { "SQLITE3_INTEGER", SQLITE_INTEGER },
  { "SQLITE3_FLOAT",   SQLITE_FLOAT },
  { "SQLITE3_TEXT",    SQLITE_TEXT },
  { "SQLITE3_BLOB",    SQLITE_BLOB },
  { "SQLITE3_NULL",    SQLITE_NULL },

};

/* Constant callback: the value travels in the user data pointer */
static void PH8_sqlite_const(ph7_value *pValue, void *pUserData) {
  ph7_value_int(pValue, (int)(size_t)pUserData);
}

/*
 * Register the wrapper in a VM. Call it right after the VM is created.
 * On success *ppState receives the state which must be passed to
 * PH8_sqlite_reset() when a script run finishes and before the VM is released.
 */
int ph7_sqlite_init(ph7_vm *pVm, ph7_sqlite **ppState) {
  ph7_sqlite *pState;
  int nIndex;
  int iRc;

  pState = PhSqliteVmCreate(pVm);
  if (pState == NULL) {
    return PH7_NOMEM;
  }

  for (nIndex = 0; nIndex < (int)(sizeof(aSqliteFuncs) / sizeof(aSqliteFuncs[0])); nIndex++) {
    iRc = ph7_create_function(pVm, aSqliteFuncs[nIndex].zName, aSqliteFuncs[nIndex].xFunc, pState);
    if (iRc != PH7_OK) {
      PhSqliteFree(pState, pState);
      return iRc;
    }
  }

  for (nIndex = 0; nIndex < (int)(sizeof(aSqliteConsts) / sizeof(aSqliteConsts[0])); nIndex++) {
    iRc = ph7_create_constant(pVm, aSqliteConsts[nIndex].zName, PH8_sqlite_const, (void *)(size_t)aSqliteConsts[nIndex].nValue);
    if (iRc != PH7_OK) {
      PhSqliteFree(pState, pState);
      return iRc;
    }
  }

  *ppState = pState;
  return PH7_OK;
}
