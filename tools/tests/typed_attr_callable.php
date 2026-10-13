<?php

class Test {
  public ?string $a = '';
  public static int $b = 0;
  public object $z;

  public ?callable $c = 'echo';
  public callable $d;
  public function t(?string $z) {
    return $z;
  }
  //public function __toString() { return "Abject"; }
}
//aaa:
$cl = new Test();


$arr = array($cl, 't');

$cl->c = null;
if (!is_null($cl->c))
  die ('Nullable failed to accept null');

$cl->c = null;
if ($cl->c !== null)
  die ('!== operator fails on a nullable');
else
  if ($cl->c === null) {
  } else
    die ('=== operator fails on a nullable');

$cl->c = 11;
if ($cl->c !== '__badcallable') { 
  var_dump($cl->c);
  die('A bad callable is expected but got ['.$cl->c.']');
}


if ($cl->c === null)
  die ('=== operator fails on a nullable');
else
  if ($cl->c !== null) {
  } else
    die ('!== operator fails on a nullable');


$cl->c = null;
if ($cl->c !== null)
  die('Nullable failed to accept null');


$cl->c = 'strlen';
if ($cl->c != 'strlen') {
  die('A strlen is expected but got ['.$cl->c.']');
}

echo 'TEST 1 PASSED'.PHP_EOL;


$cl->d = 11;
if ($cl->d != '__badcallable') {
  die('A bad callable is expected but got ['.$cl->d.']');
}

$cl->d = 1;
if ($cl->d == 1)
  die('Uninitialized class callable attribute accepts a number!');

$cl->d = null;
if (!isset($cl->d))
  die('Non-nullable callable accepts null!');

if ($cl->d != '__badcallable') {
  die('__badcallable is expected but got ['.$cl->d.']');
}


$cl->d = 'strlen';
if ($cl->d != 'strlen') {
  die('strlen is expected but got ['.$cl->d.']');
}

$cl->d = $arr;
$z = $cl->d;
if ($z('Hello') != 'Hello')
  die('Hello is expected');

if ($z(null) != null)
  die('null is expected');

echo 'TEST 2 PASSED'.PHP_EOL;

//var_dump($cl->a);
//var_dump($cl->b);
//var_dump($cl->c);
//goto aaa;
?>