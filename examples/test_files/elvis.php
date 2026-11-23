<?php

function test($name, $expected, $actual)
{
    $ok = ($expected === $actual);
    echo ($ok ? "[ OK ] " : "[FAIL] ") . $name;

    if (!$ok) {
        echo PHP_EOL,'       Expected: ';
        var_export($expected);
        echo PHP_EOL,'       Actual:   ';
        var_export($actual);
    }

    echo PHP_EOL;
}

// Is ternary still works afetr our changes to ExprMakeTree?
test('ternary true',  'yes', true  ? 'yes' : 'no');
test('ternary false', 'no',  false ? 'yes' : 'no');
test('ternary integer', 10, 1 ? 10 : 20);
test('ternary zero',    20, 0 ? 10 : 20);
test('ternary null',    'B', null ? 'A' : 'B');
test('ternary empty',   'B', ''   ? 'A' : 'B');
test('ternary string',  'B', '0'  ? 'A' : 'B');
test('ternary comparison', 'yes', 5 > 3 ? 'yes' : 'no');
test('nested ternary 1', 'A',  true ? 'A' : false ? 'B' : 'C');
test('nested ternary 2', 'C', false ? 'A' : false ? 'B' : 'C');
test('nested ternary 3', 'B', false ? 'A' : true ? 'B' : 'C');

// Basic elvis operator tests
test('elvis true',  'hello', 'hello' ?: 'default');
test('elvis false', 'default', '' ?: 'default');
test('elvis zero',     'default', 0 ?: 'default');
test('elvis null',     'default', null ?: 'default');
test('elvis false',    'default', false ?: 'default');
test('elvis empty',    'default', [] ?: 'default');
test('elvis string', 'default', '0' ?: 'default');
test('elvis string 1', '1', '1' ?: 'default');


// ? + ?: (ternary + elvis in one expression)
test('ternary with elvis', 'hello', true ? ('hello' ?: 'default') : 'no');
test('elvis with ternary', 'yes',  (false ?: true) ? 'yes' : 'no');
test('nested elvis', 'fallback',   null ?: ('' ?: 'fallback'));
test('explicit paren 1', 'B', (true ? 'B' : 'B') ?: 'C');
test('explicit paren 2', 'B', false ? 'A' : ('B' ?: 'C'));


// side effects
$i = 0;

$result = true ? ++$i : ++$i;
test('ternary evaluates only one branch', 1, $i);

$i = 0;

$result = false ?: ++$i;
test('elvis evaluates else branch', 1, $i);

$i = 0;

$result = true ?: ++$i;
test('elvis skips else branch', 0, $i);

// chains. these will not compile in Zend PHP which requires explicit parens; it is a valid code in PH8
test('elvis chain 1', 'second',  null ?: 'second' ?: 'third');
test('elvis chain 2', 'third', null ?: '' ?: 'third');

echo PHP_EOL,'TEST DONE', PHP_EOL;


?>