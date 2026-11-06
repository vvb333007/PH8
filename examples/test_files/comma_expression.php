<?php

function f(): int {
    //return   fn($x) => $x;
     return ({ $a = 2, $b = 10, $a + $b });  // curly brackets are optional.
}

var_dump( f() );

?>