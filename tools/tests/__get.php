<?php

// __get() semantics
// __set() is never called, no such magic method in PH8 at all
// __get() is called in both cases (store and load). there is
//         no way to determine which operation is requested, so __get() must return
//         either a reference to a attribute OR array (which is always returned by reference )
//
// Two builtin class attributes added by the compiler when __get() method is implemented by the class
//
class Proxy {

    public array $arr = [];  // BUG: array must be initialized

    public function &__get($arg) {

      echo '__get('.$arg.') called'.PHP_EOL;
      if ($arg == 'opa')
        return $this->__get;  
       else
        return $this->arr;  
    }
}

$cl = new Proxy;


// __get() called as right operand
$z = $cl->opa;   
var_dump($z); // displays 'null'

// __get() called (not set()) again, as a left operand
// returning a reference allows storing to the internal variable 
$cl->opa = 88;   
var_dump($cl->opa); // displays '88'

$z = $cl->waka;   
$z[1] = 'Boom';

$cl->waka[1] ='Hello';
var_dump($cl->waka[1]); // displays 'Hello'

?>
