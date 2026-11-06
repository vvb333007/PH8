<?php

class SQLite3 {

  private $rDb = null;   // resource from sqlite3_open(), null when closed

  public function __construct($zFilename) {
    $this->open($zFilename);
  }

  public function __destruct() {
    $this->close();
  }

  public function open($zFilename) {
    $this->close();

    $rDb = sqlite3_open($zFilename);
    if ($rDb === false) {
      throw new Exception('Unable to open database');
    }
    $this->rDb = $rDb;
  }

  public function close() {
    if ($this->rDb === null) {
      return false;
    }
    $bResult = sqlite3_close($this->rDb);
    $this->rDb = null;
    return $bResult;
  }

  /* Not in PHP: lets SQLite3Stmt and SQLite3Result know whether their statements are still alive */
  public function isOpen() {
    return $this->rDb !== null;
  }

  public function exec($zSql) {
    return sqlite3_exec($this->rDb, $zSql);
  }

  /*
   * $bEntireRow = false: first column of the first row, null when there are no rows
   * $bEntireRow = true:  first row as an associative array, empty array when there are no rows
   * Both: false on error
   */
  public function querySingle($zSql, $bEntireRow = false) {
    if (!$bEntireRow) {
      return sqlite3_query_single($this->rDb, $zSql);
    }

    $oResult = $this->query($zSql);
    if ($oResult === false) {
      return false;
    }
    $aRow = $oResult->fetchArray(SQLITE3_ASSOC);
    $oResult->finalize();

    if ($aRow === false) {
      return array();
    }
    return $aRow;
  }

  public function query($zSql) {
    $rStmt = sqlite3_prepare($this->rDb, $zSql);
    if ($rStmt === false) {
      return false;
    }
    if (!sqlite3_execute($rStmt)) {
      sqlite3_finalize($rStmt);
      return false;
    }
    /* The statement belongs to the result: it is finalized together with it */
    return new SQLite3Result($rStmt, true, $this);
  }

  public function prepare($zSql) {
    $rStmt = sqlite3_prepare($this->rDb, $zSql);
    if ($rStmt === false) {
      return false;
    }
    return new SQLite3Stmt($rStmt, $this);
  }

  public function lastInsertRowID() {
    return sqlite3_last_insert_rowid($this->rDb);
  }

  public function changes() {
    return sqlite3_changes($this->rDb);
  }

  public function lastErrorCode() {
    return sqlite3_last_error_code($this->rDb);
  }

  public function lastErrorMsg() {
    return sqlite3_last_error_msg($this->rDb);
  }
}


/* Created by SQLite3::prepare(), the constructor is not meant to be called from scripts */
class SQLite3Stmt {

  private $rStmt = null;   // resource from sqlite3_prepare(), null when closed
  private $oDb;            // owning SQLite3 object, to know if the connection is still open

  public function __construct($rStmt, $oDb) {
    $this->rStmt = $rStmt;
    $this->oDb = $oDb;
  }

  public function __destruct() {
    $this->close();
  }

  /* "name" becomes ":name", names that already have a prefix and numbers stay as they are */
  private function normalizeParam($mParam) {
    if (!is_string($mParam)) {
      return (int)$mParam;
    }
    $zFirst = substr($mParam, 0, 1);
    if ($zFirst == ':' || $zFirst == '@' || $zFirst == '$') {
      return $mParam;
    }
    return ':' . $mParam;
  }

  /* $iType = 0 means "choose by the type of the value" */
  //public function bindValue($mParam, $mValue, $iType = 0) {
//    return sqlite3_bind_value($this->rStmt, $this->normalizeParam($mParam), $mValue, $iType);
//  }
  // TODO: move reset to bind_value
  public function bindValue($mParam, $mValue, $iType = 0) {
    /* A statement which ran to the end refuses new values until it is reset */
    sqlite3_reset($this->rStmt);
    return sqlite3_bind_value($this->rStmt, $this->normalizeParam($mParam), $mValue, $iType);
  }

  public function execute() {
    if (!sqlite3_execute($this->rStmt)) {
      return false;
    }
    /* The result shares the statement, it must not finalize it */
    return new SQLite3Result($this->rStmt, false, $this->oDb);
  }

  public function reset() {
    return sqlite3_reset($this->rStmt);
  }

  public function clear() {
    return sqlite3_clear_bindings($this->rStmt);
  }

  public function close() {
    if ($this->rStmt === null) {
      return true;
    }
    $bResult = true;
    /* When the connection is already closed, SQLite has finalized the statement itself */
    if ($this->oDb->isOpen()) {
      $bResult = sqlite3_finalize($this->rStmt);
    }
    $this->rStmt = null;
    return $bResult;
  }

  public function paramCount() {
    return sqlite3_bind_parameter_count($this->rStmt);
  }

  public function getSQL() {
    return sqlite3_sql($this->rStmt);
  }

  public function readOnly() {
    return sqlite3_stmt_readonly($this->rStmt);
  }
}


/* Created by SQLite3::query() and SQLite3Stmt::execute() */
class SQLite3Result {

  private $rStmt;
  private $bOwnsStmt;      // true for SQLite3::query(): the statement dies with the result
  private $oDb;
  private $bFinalized = false;

  public function __construct($rStmt, $bOwnsStmt, $oDb) {
    $this->rStmt = $rStmt;
    $this->bOwnsStmt = $bOwnsStmt;
    $this->oDb = $oDb;
  }

  public function __destruct() {
    if ($this->bOwnsStmt) {
      $this->finalize();
    }
  }

  public function numColumns() {
    return sqlite3_num_columns($this->rStmt);
  }

  public function columnName($iColumn) {
    return sqlite3_column_name($this->rStmt, $iColumn);
  }

  /* Type of the value in the row which fetchArray() will return next */
  public function columnType($iColumn) {
    return sqlite3_column_type($this->rStmt, $iColumn);
  }

  public function fetchArray($iMode = SQLITE3_BOTH) {
    return sqlite3_fetch_array($this->rStmt, $iMode);
  }

  /* Start over: the C layer ends the result set on reset, so the statement is executed again */
  public function reset() {
    return sqlite3_execute($this->rStmt);
  }

  public function finalize() {
    if ($this->bFinalized) {
      return true;
    }
    $this->bFinalized = true;

    if (!$this->bOwnsStmt) {
      /* The statement belongs to SQLite3Stmt: only stop the iteration */
      return sqlite3_reset($this->rStmt);
    }

    $bResult = true;
    if ($this->oDb->isOpen()) {
      $bResult = sqlite3_finalize($this->rStmt);
    }
    $this->rStmt = null;
    return $bResult;
  }
}

?>