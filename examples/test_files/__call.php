<?php

class Proxy {
    public function __call($arg1, $arg2, $arg3):string {
      return 'Works from __call: '.__CALL__;
    }
}

$p = new Proxy();
$z = $p->doSomething(1, 2, 3);
var_dump($z);

?>
