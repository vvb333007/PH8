<?php

class Test {
  public ?string $c = '11';
  public ?string $d;

  public string $e;
}
//aaa:
$cl = new Test();


$cl->c = null;
if (!is_null($cl->d))
  die('Nullable rejects null');

$cl->c = 11;
if ($cl->c !== '11')
  die('Type mutated');

$cl->c = null;
if (!is_null($cl->d))
  die('Nullable rejects null');

$cl->c = 'strlen';
if ($cl->c !== 'strlen')
  die('Nullable rejects string');

echo 'TEST 1 PASSED'.PHP_EOL;




$cl->d = null;
if (!is_null($cl->d))
  die('Nullable rejects null');

$cl->d = 11;
if (!is_string($cl->d))
  die('Nullable type mutated');

$cl->d = null;
if (!is_null($cl->d))
  die('Nullable rejects null');

$cl->d = 'strlen';
if ($cl->d != 'strlen')
  die('Nullable rejects string');

echo 'TEST 2 PASSED'.PHP_EOL;

$cl->e = null;
if (is_null($cl->d))
  die('Non-Nullable acceptss null');

$cl->e = 11;
if (!is_string($cl->d))
  die('Non-Nullable type mutated');

$cl->e = 'strlen';
if ($cl->d != 'strlen')
  die('Non-Nullable rejects string');


echo 'TEST 3 PASSED'.PHP_EOL;

?>
