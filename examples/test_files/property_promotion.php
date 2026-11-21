<?php

$msg = '1. Public promoted properties';
class Point {
  public function __construct(public $x, public $y) {}
}

$o = new Point(1, 2);

if (!($o->x === 1 && $o->y === 2))
  die($msg.': FAIL'); 

echo $msg, ': PASS', PHP_EOL;

$msg ='2. Default values';
class Defaults {
  public function __construct(public $nFirst, public $nSecond = 5, public $zThird = 'abc') {}
}
$o = new Defaults(10);

if (!($o->nFirst === 10))
  die($msg.': FAIL (given)'); 

if (!($o->nSecond === 5 && $o->zThird === 'abc'))
  die($msg.': FAIL (used)'); 

$o = new Defaults(10, 20);

if (!($o->nSecond === 20 && $o->zThird === 'abc'))
  die($msg.': overridden'); 

echo $msg, ': PASS', PHP_EOL;

$msg ='3. Mix of promoted and ordinary parameters';
class Mixed {
  public $nSum = 0;
  public function __construct($nPlain, public $nPromoted, $nOther = 3) {
    $this->nSum = $nPlain + $nPromoted + $nOther;
  }
}

$o = new Mixed(1, 2);

if(!( $o->nPromoted === 2))
  die($msg.': FAIL (promoted)'); 

if(!( $o->nSum === 6))
  die($msg.': FAIL (body sees params)');

if(!( !property_exists($o, 'nPlain')))
  die($msg.': FAIL (plain is not a property)');

echo $msg, ': PASS', PHP_EOL;

$msg ='4. change in constructor does not affect the property value';
class Shadow {
  public function __construct(public $nValue) {
    $nValue = 99;
  }
}

$o = new Shadow(7);
if(!($o->nValue === 7)) 
  die($msg, ': FAIL (local change affects property)'); 

echo $msg, ': PASS', PHP_EOL;
$msg= '5. Property is assigned BEFORE the body runs';

class Early {
  public $nSeenInBody = null;
  public function __construct(public int $nValue) {
    $this->nSeenInBody = $this->nValue;
  }
}
$o = new Early(42);
if (!($o->nSeenInBody === 42))
  die($msg, ': FAIL (not assigned before body)'); 

echo $msg, ': PASS', PHP_EOL;

$msg ='6. Typed properties still compile?';

class F {
  function __construct(public int $nCount = 0, public ?string $zName = null) {}
}

$o = new F(3, 'x');  

if (!($o->nCount === 3 && $o->zName === 'x'))
  die($msg, ': FAIL (typed properties)'); 

echo 'TEST PASSED', PHP_EOL;
?>
