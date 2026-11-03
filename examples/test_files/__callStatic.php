<?php

class Proxy {

    public static function __callStatic($a,$b):string {
      
      var_dump($a); // 666
      var_dump($b); // Hello
      return 'Method '. __CALL__. ' was called';

    }
}


$z = Proxy::opa(666,'Hello');

var_dump($z);

?>
