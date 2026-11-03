<?php

$db = sqlite3_open("test.db");

if (!$db) {
    die("Cannot open database\n");
}

// Create table
sqlite3_exec($db, "
    CREATE TABLE IF NOT EXISTS test (
        id INTEGER PRIMARY KEY,
        name TEXT,
        value INTEGER
    )
");

// Insert a record
sqlite3_exec($db, "
    INSERT INTO test (name, value)
    VALUES ('Hello SQLite', 123)
");

$id = sqlite3_last_insert_rowid($db);

echo "Inserted row ID: $id\n";

// Read the record
$stmt = sqlite3_prepare($db, "
    SELECT id, name, value FROM test WHERE id = ?
");

sqlite3_bind_value($stmt, 1, $id);
sqlite3_execute($stmt);

$row = sqlite3_fetch_array($stmt);

if ($row) {
    echo "ID:    " . $row['id'] . "\n";
    echo "Name:  " . $row['name'] . "\n";
    echo "Value: " . $row['value'] . "\n";
}

sqlite3_finalize($stmt);

// Close database
sqlite3_close($db);

echo "Done!\n";
?>