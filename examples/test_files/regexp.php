<?php

function t($name, $ok) {
    echo ($ok ? "[ok]   " : "[FAIL] ") . $name . "\n";
}

/* ------------------------------------------------------------------ */
echo "--- preg_match\n";
$m = [];
$rc = preg_match('/([0-9]+)-([0-9]+)/', 'tel 123-456 end', $m);
t('match returns 1',            $rc === 1);
t('whole match',                $m[0] === '123-456');
t('group 1',                    $m[1] === '123');
t('group 2',                    $m[2] === '456');
t('3 elements',                 count($m) === 3);

$m = [];
$rc = preg_match('/xyz/', 'hello', $m);
t('no match returns 0',         $rc === 0);
t('no match -> empty array',    count($m) === 0);

$m = [];
$rc = preg_match('/([a-z]+)/', 'abc', $m, PREG_OFFSET_CAPTURE, 0);
t('offset capture: text',       $m[1][0] === 'abc');
t('offset capture: position',   $m[1][1] === 0);

/* ------------------------------------------------------------------ */
echo "--- preg_match_all\n";
$m = [];
$n = preg_match_all('/([a-z])([0-9])/', 'a1 b2 c3', $m);
t('3 matches',                  $n === 3);
t('pattern order: [0][1]',      $m[0][1] === 'b2');
t('pattern order: [1][2]',      $m[1][2] === 'c');
t('pattern order: [2][0]',      $m[2][0] === '1');

$m = [];
$n = preg_match_all('/([a-z])([0-9])/', 'a1 b2 c3', $m, PREG_SET_ORDER);
t('set order: count',           count($m) === 3);
t('set order: [1][0]',          $m[1][0] === 'b2');
t('set order: [2][2]',          $m[2][2] === '3');

$n = preg_match_all('/[0-9]/', 'a1 b2 c3');
t('count only (no $matches)',   $n === 3);

/* ------------------------------------------------------------------ */
echo "--- preg_replace\n";
$cnt = 3;
$r = preg_replace('/o/', '0', 'Hello World', -1, $cnt);
t('replace all',                $r === 'Hell0 W0rld');
t('count = 2',                  $cnt === 2);

$r = preg_replace('/o/', '0', 'Hello World', 1);
t('limit 1',                    $r === 'Hell0 World');

$r = preg_replace('/([a-z]+)@/', '$1 at ', 'bob@x');
t('backreference $1',           $r === 'bob at x');

$r = preg_replace(array('/a/', '/b/'), array('1', '2'), 'aabb');
t('pattern + replacement arrays', $r === '1122');


$r = preg_replace('/1/', 'X', ['x' => 'a1', 'y' => 'b1']);
t('array subject: key x',       $r['x'] === 'aX');
t('array subject: key y',       $r['y'] === 'bX');

/* ------------------------------------------------------------------ */
echo "--- preg_split\n";
$p = preg_split('/[,;]/', 'a,b;c');
t('3 pieces',                   count($p) === 3);
t('piece 0',                    $p[0] === 'a');
t('piece 2',                    $p[2] === 'c');

$p = preg_split('/,/', 'a,,b');
t('empty piece kept',           count($p) === 3);
$p = preg_split('/,/', 'a,,b', -1, PREG_SPLIT_NO_EMPTY);
t('NO_EMPTY drops it',          count($p) === 2);

/* ------------------------------------------------------------------ */
echo "--- preg_grep\n";
$src = ['apple', 'banana', 'avocado', 'cherry'];
$r = preg_grep('/^a/', $src);
t('2 matches',                  count($r) === 2);
t('key 0 preserved',            $r[0] === 'apple');
t('key 2 preserved',            $r[2] === 'avocado');
t('key 1 absent',               !isset($r[1]));

$r = preg_grep('/^a/', $src, PREG_GREP_INVERT);
t('invert: 2 elements',         count($r) === 2);
t('invert: key 1',              $r[1] === 'banana');
t('invert: key 3',              $r[3] === 'cherry');

/* ------------------------------------------------------------------ */
echo "--- preg_quote\n";
t('special characters',         preg_quote('Hello.World?(1+1)') === 'Hello\.World\?\(1\+1\)');
t('delimiter',                  preg_quote('a/b', '/') === 'a\/b');
t('plain text untouched',       preg_quote('abc123') === 'abc123');
t('empty string',               preg_quote('') === '');

/* ------------------------------------------------------------------ */
echo "--- preg_last_error / preg_last_error_msg\n";
$rc = preg_match('abc', 'x');   /* no delimiters -> error */
t('bad pattern returns false',  $rc === false);
t('last_error != 0',            preg_last_error() !== 0);
t('last_error_msg is text',     preg_last_error_msg() !== 'No error');

$rc = preg_match('/x/', 'x');
t('good pattern: error reset',  preg_last_error() === 0);
t('good pattern: msg',          preg_last_error_msg() === 'No error');

echo "done\n";
?>
