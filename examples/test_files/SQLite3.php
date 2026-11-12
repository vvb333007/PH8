<?php


include 'ext/sqlite3';


/*
 * Test of the SQLite3 / SQLite3Stmt / SQLite3Result classes.
 * Put the class definitions in front of it (or include them) and run.
 *
 * Some tests provoke errors on purpose (bad SQL, closed handles),
 * so warnings from the wrapper in the output are expected.
 * Only the lines starting with FAIL and the final summary matter.
 */

$iPassed = 0;
$iFailed = 0;

function TestCheck($zName, $bOk) {
  global $iPassed, $iFailed;

  if ($bOk) {
    $iPassed++;
  } else {
    $iFailed++;
    echo "FAIL: ", $zName, "\n";
  }
}

/* Read the whole result as an array of rows, NUM mode */
function FetchAllRows($oResult) {
  $aRows = array();

  while (($aRow = $oResult->fetchArray(SQLITE3_NUM)) !== false) {
    $aRows[] = $aRow;
  }
  return $aRows;
}


/* ------------------------------------------------------------------ */
/* 1. Open, exec, rowid, changes                                      */
/* ------------------------------------------------------------------ */

$oDb = new SQLite3(':memory:');

TestCheck('open: connection is open', $oDb->isOpen());

TestCheck('exec: create table',
  $oDb->exec('CREATE TABLE t (id INTEGER PRIMARY KEY, name TEXT, score REAL, note TEXT)'));
TestCheck('exec: insert rows',
  $oDb->exec("INSERT INTO t VALUES (1, 'alice', 1.5, NULL);
              INSERT INTO t VALUES (2, 'bob',   2.5, 'x');
              INSERT INTO t VALUES (3, 'carol', 3.5, 'y');"));

TestCheck('lastInsertRowID after multi insert', $oDb->lastInsertRowID() == 3);

$oDb->exec('UPDATE t SET score = score + 1 WHERE id <= 2');
TestCheck('changes after update', $oDb->changes() == 2);

TestCheck('exec: bad sql returns false', $oDb->exec('THIS IS NOT SQL') === false);
TestCheck('lastErrorCode is not zero after an error', $oDb->lastErrorCode() != 0);
TestCheck('lastErrorMsg is not empty after an error', strlen($oDb->lastErrorMsg()) > 0);

$oDb->exec('SELECT 1');
TestCheck('lastErrorCode is zero after success', $oDb->lastErrorCode() == 0);


/* ------------------------------------------------------------------ */
/* 2. querySingle                                                     */
/* ------------------------------------------------------------------ */

TestCheck('querySingle: count', $oDb->querySingle('SELECT COUNT(*) FROM t') == 3);
TestCheck('querySingle: integer type', is_int($oDb->querySingle('SELECT id FROM t WHERE id = 2')));
TestCheck('querySingle: float type', is_float($oDb->querySingle('SELECT score FROM t WHERE id = 3')));
TestCheck('querySingle: text', $oDb->querySingle('SELECT name FROM t WHERE id = 1') === 'alice');
TestCheck('querySingle: no rows gives null', $oDb->querySingle('SELECT id FROM t WHERE id = 99') === null);
TestCheck('querySingle: NULL value gives null', $oDb->querySingle('SELECT note FROM t WHERE id = 1') === null);
TestCheck('querySingle: bad sql gives false', $oDb->querySingle('SELECT FROM WHERE') === false);
TestCheck('querySingle: explicit false flag', $oDb->querySingle('SELECT name FROM t WHERE id = 1', false) === 'alice');

/* The entire row: associative array */
$aRow = $oDb->querySingle('SELECT id, name, note FROM t WHERE id = 2', true);
TestCheck('querySingle row: is an array with 3 keys', is_array($aRow) && count($aRow) == 3);
TestCheck('querySingle row: values and types', $aRow['id'] === 2 && $aRow['name'] === 'bob' && $aRow['note'] === 'x');

$aRow = $oDb->querySingle('SELECT id, note FROM t WHERE id = 1', true);
TestCheck('querySingle row: NULL value is kept', is_array($aRow) && array_key_exists('note', $aRow) && $aRow['note'] === null);

$aRow = $oDb->querySingle('SELECT id FROM t WHERE id = 99', true);
TestCheck('querySingle row: no rows gives an empty array', is_array($aRow) && count($aRow) == 0);
TestCheck('querySingle row: bad sql gives false', $oDb->querySingle('SELECT FROM WHERE', true) === false);

/* Only the first row is returned, the statement is released */
$aRow = $oDb->querySingle('SELECT id FROM t ORDER BY id', true);
TestCheck('querySingle row: first row of many', is_array($aRow) && $aRow['id'] === 1);
for ($iLoop = 0; $iLoop < 200; $iLoop++) {
  $aRow = $oDb->querySingle('SELECT ' . $iLoop . ' AS n', true);
  if ($aRow['n'] !== $iLoop) {
    break;
  }
}
TestCheck('querySingle row: 200 calls in a loop', $iLoop == 200);


/* ------------------------------------------------------------------ */
/* 3. query, fetch modes, column info                                 */
/* ------------------------------------------------------------------ */

$oResult = $oDb->query('SELECT id, name, score, note FROM t ORDER BY id');
TestCheck('query: returns a result', $oResult instanceof SQLite3Result);
TestCheck('numColumns', $oResult->numColumns() == 4);
TestCheck('columnName 0', $oResult->columnName(0) === 'id');
TestCheck('columnName 3', $oResult->columnName(3) === 'note');
TestCheck('columnName: negative index', $oResult->columnName(-1) === false);
TestCheck('columnName: index out of range', $oResult->columnName(4) === false);

/* The first row is pending, nothing has been fetched yet */
TestCheck('columnType: id is integer', $oResult->columnType(0) == SQLITE3_INTEGER);
TestCheck('columnType: name is text', $oResult->columnType(1) == SQLITE3_TEXT);
TestCheck('columnType: score is float', $oResult->columnType(2) == SQLITE3_FLOAT);
TestCheck('columnType: note is null in the first row', $oResult->columnType(3) == SQLITE3_NULL);
TestCheck('columnType: index out of range', $oResult->columnType(9) === false);

$aRow = $oResult->fetchArray(SQLITE3_ASSOC);
TestCheck('ASSOC: key count', count($aRow) == 4);
TestCheck('ASSOC: values', $aRow['id'] === 1 && $aRow['name'] === 'alice' && $aRow['score'] == 2.5);
TestCheck('ASSOC: null value', $aRow['note'] === null);

/* Now the second row is pending: note is text there */
TestCheck('columnType: pending row changed', $oResult->columnType(3) == SQLITE3_TEXT);

$aRow = $oResult->fetchArray(SQLITE3_NUM);
TestCheck('NUM: key count', count($aRow) == 4);
TestCheck('NUM: values', $aRow[0] === 2 && $aRow[1] === 'bob');

$aRow = $oResult->fetchArray();
TestCheck('BOTH is the default: key count', count($aRow) == 8);
TestCheck('BOTH: both keys give the same value', $aRow[1] === $aRow['name'] && $aRow['name'] === 'carol');

TestCheck('fetchArray: exhausted gives false', $oResult->fetchArray() === false);
TestCheck('columnType: no pending row gives false', $oResult->columnType(0) === false);

TestCheck('reset: rewind', $oResult->reset());
$aRows = FetchAllRows($oResult);
TestCheck('reset: all rows are back', count($aRows) == 3 && $aRows[0][0] === 1 && $aRows[2][0] === 3);

TestCheck('finalize', $oResult->finalize());
TestCheck('finalize twice', $oResult->finalize());

TestCheck('query: bad sql gives false', $oDb->query('SELECT * FROM no_such_table') === false);

/* A query without rows */
$oResult = $oDb->query('SELECT id FROM t WHERE id = 99');
TestCheck('empty result: object is returned', $oResult instanceof SQLite3Result);
TestCheck('empty result: fetchArray gives false', $oResult->fetchArray() === false);
$oResult->finalize();

/* Results of queries which are dropped at once must not pile up or crash */
for ($iLoop = 0; $iLoop < 200; $iLoop++) {
  $oResult = $oDb->query('SELECT ' . $iLoop);
  $aRow = $oResult->fetchArray(SQLITE3_NUM);
  if ($aRow[0] !== $iLoop) {
    TestCheck('loop of queries: wrong value at ' . $iLoop, false);
    break;
  }
  $oResult = null;
}
TestCheck('loop of queries', $iLoop == 200);


/* ------------------------------------------------------------------ */
/* 4. Prepared statements                                             */
/* ------------------------------------------------------------------ */

$oStmt = $oDb->prepare('INSERT INTO t (name, score) VALUES (:name, :score)');
TestCheck('prepare: returns a statement', $oStmt instanceof SQLite3Stmt);
TestCheck('paramCount', $oStmt->paramCount() == 2);
TestCheck('getSQL', $oStmt->getSQL() === 'INSERT INTO t (name, score) VALUES (:name, :score)');
TestCheck('readOnly: insert is not', $oStmt->readOnly() === false);

$oSelect = $oDb->prepare('SELECT name FROM t WHERE id = ?');
TestCheck('readOnly: select is', $oSelect->readOnly() === true);
TestCheck('paramCount: positional', $oSelect->paramCount() == 1);
$oSelect->close();

TestCheck('prepare: bad sql gives false', $oDb->prepare('SELEKT 1') === false);

/* Reuse of one statement: bind, execute, reset */
$aNames = array('dave', 'erin', 'frank');
$iRowsBefore = $oDb->querySingle('SELECT COUNT(*) FROM t');

foreach ($aNames as $iIndex => $zName) {
  $bBound1 = $oStmt->bindValue(':name', $zName);
  $bBound2 = $oStmt->bindValue(':score', 10.0 + $iIndex);
  $oInsertResult = $oStmt->execute();
  TestCheck('reuse: bind ' . $zName, $bBound1 && $bBound2);
  TestCheck('reuse: execute ' . $zName, $oInsertResult instanceof SQLite3Result);
  TestCheck('reuse: reset ' . $zName, $oStmt->reset());
}
TestCheck('reuse: rows were inserted', $oDb->querySingle('SELECT COUNT(*) FROM t') == $iRowsBefore + 3);
TestCheck('reuse: last value',
  $oDb->querySingle("SELECT score FROM t WHERE name = 'frank'") == 12.0);

/* An INSERT gives a result without columns and without rows */
$oInsertResult = $oStmt->execute();
TestCheck('insert result: no columns', $oInsertResult->numColumns() == 0);
TestCheck('insert result: no rows', $oInsertResult->fetchArray() === false);

/* Parameter names with and without the prefix */
TestCheck('bindValue: name without prefix', $oStmt->bindValue('name', 'gina'));
TestCheck('bindValue: name with prefix', $oStmt->bindValue(':score', 1.0));
TestCheck('bindValue: unknown name gives false', $oStmt->bindValue(':nothing', 1) === false);
TestCheck('bindValue: number out of range gives false', $oStmt->bindValue(5, 1) === false);

/* clear() sets every parameter to NULL */
$oStmt->bindValue(':name', 'must be forgotten');
$oStmt->bindValue(':score', 99.0);
TestCheck('clear', $oStmt->clear());
$oStmt->execute();
TestCheck('clear: the row has NULL in both columns',
  $oDb->querySingle('SELECT COUNT(*) FROM t WHERE name IS NULL AND score IS NULL') == 1);
TestCheck('clear: old values did not leak',
  $oDb->querySingle("SELECT COUNT(*) FROM t WHERE name = 'must be forgotten'") == 0);

/* A statement which selects: execute() gives rows */
$oSelect = $oDb->prepare('SELECT id, name FROM t WHERE id >= :min ORDER BY id');
$oSelect->bindValue(':min', 2);
$aRows = FetchAllRows($oSelect->execute());
TestCheck('select statement: row count',
  count($aRows) == $oDb->querySingle('SELECT COUNT(*) FROM t WHERE id >= 2'));
TestCheck('select statement: first row', $aRows[0][0] === 2 && $aRows[0][1] === 'bob');

$oSelect->bindValue(':min', 3);
$aRows = FetchAllRows($oSelect->execute());
TestCheck('select statement: executed again with another value',
  count($aRows) == $oDb->querySingle('SELECT COUNT(*) FROM t WHERE id >= 3'));
TestCheck('select statement: the second run starts at id 3', $aRows[0][0] === 3);

TestCheck('close statement', $oSelect->close());
TestCheck('close statement twice', $oSelect->close());
TestCheck('execute after close gives false', $oSelect->execute() === false);
$oStmt->close();


/* ------------------------------------------------------------------ */
/* 5. Binding types                                                   */
/* ------------------------------------------------------------------ */

/* A column without affinity keeps the type of the bound value as it is */
$oDb->exec('CREATE TABLE v (x)');
$oStmt = $oDb->prepare('INSERT INTO v VALUES (:x)');

$oStmt->bindValue(':x', 42);                       $oStmt->execute();   /* integer, by the value   */
$oStmt->bindValue(':x', 1.5);                      $oStmt->execute();   /* float, by the value     */
$oStmt->bindValue(':x', 'abc');                    $oStmt->execute();   /* text, by the value      */
$oStmt->bindValue(':x', null);                     $oStmt->execute();   /* null, by the value      */
$oStmt->bindValue(':x', true);                     $oStmt->execute();   /* bool counts as integer  */
$oStmt->bindValue(':x', 42, SQLITE3_TEXT);         $oStmt->execute();   /* forced text             */
$oStmt->bindValue(':x', '7', SQLITE3_INTEGER);     $oStmt->execute();   /* forced integer          */
$oStmt->bindValue(':x', 3, SQLITE3_FLOAT);         $oStmt->execute();   /* forced float            */
$oStmt->close();

$aExpected = array(SQLITE3_INTEGER, SQLITE3_FLOAT, SQLITE3_TEXT, SQLITE3_NULL,
                   SQLITE3_INTEGER, SQLITE3_TEXT, SQLITE3_INTEGER, SQLITE3_FLOAT);
$aActual = array();

$oResult = $oDb->query('SELECT x FROM v ORDER BY rowid');
while (true) {
  $mType = $oResult->columnType(0);     /* type of the row which is about to be fetched */
  $aRow = $oResult->fetchArray(SQLITE3_NUM);
  if ($aRow === false) {
    break;
  }
  $aActual[] = $mType;
}
$oResult->finalize();

TestCheck('binding types: row count', count($aActual) == count($aExpected));
TestCheck('binding types: ' . implode(',', $aActual) . ' expected ' . implode(',', $aExpected),
  implode(',', $aActual) == implode(',', $aExpected));


/* ------------------------------------------------------------------ */
/* 6. Result of a statement is independent of the statement's life    */
/* ------------------------------------------------------------------ */

$oStmt = $oDb->prepare('SELECT id FROM t ORDER BY id');
$oResult1 = $oStmt->execute();
TestCheck('execute: first row', $oResult1->fetchArray(SQLITE3_NUM) == array(1));
$oResult1->finalize();                          /* must not finalize the statement */
TestCheck('result finalize leaves the statement alive', $oStmt->execute() instanceof SQLite3Result);
$oStmt->close();


/* ------------------------------------------------------------------ */
/* 7. Closing                                                         */
/* ------------------------------------------------------------------ */

$oStmt = $oDb->prepare('SELECT 1');
$oResult = $oDb->query('SELECT 1');

TestCheck('close database', $oDb->close());
TestCheck('close database twice', $oDb->close() === false);
TestCheck('isOpen after close', $oDb->isOpen() === false);

TestCheck('exec after close gives false', $oDb->exec('SELECT 1') === false);
TestCheck('query after close gives false', $oDb->query('SELECT 1') === false);
TestCheck('prepare after close gives false', $oDb->prepare('SELECT 1') === false);

/* Statements died together with the connection: closing them must be harmless */
TestCheck('close statement after the database', $oStmt->close());
TestCheck('finalize result after the database', $oResult->finalize());

/* Opening a path which cannot exist must throw */
$bThrown = false;
try {
  $oBad = new SQLite3('/no/such/directory/test.db');
} catch (Exception $oException) {
  global $bThrown;   /* the catch block has its own scope in PH8 */
  $bThrown = true;
}
TestCheck('open: bad path throws an exception', $bThrown);

/* The object can be reused after close() */
$oDb->open(':memory:');
TestCheck('open again after close', $oDb->querySingle('SELECT 5') == 5);
$oDb->close();


/* ------------------------------------------------------------------ */

echo "\nPassed: ", $iPassed, ", failed: ", $iFailed, "\n";
?>