<?php


// lambda function is a callable
//
$f = function ($g): int { 
    return $g + 10;
};


// Class with a method is a callable
//
class A {
  public static function hello($arg) { }
  public function goodbye($arg) { }
}
class B extends A {
  public static function hello($arg) { echo 'Jopa'; }
}

// string is a callable
//
$func = 'test';


// Array is a callable when class method is to be called:
//
$class_method1 = array('A','hello');
$class_method2 = array(new A(),'hello');

////////////////////////////////////////////////////////////////////

function test(callable $c) {
  

  echo is_callable($c) ? $c . ' is callable: ' : $c . ' is junk: ';
  var_dump($c);
  echo "\n";

  
}

// check if lamda function is callable, PASS
test($f);     

// check if integer is callable, PASS
test(111);    

// check if checker itself is callable, PASS
test($func); 

// check if class method can be converted to callable, PASS
test(array('A','hello')); 

// MUST FAIL
test(array('A','hello22')); 

$cl = new B();

// MUST FAIL
test(array($cl,'hello22')); 

// MUST PASS
test(array($cl,'goodbye')); 


?>
