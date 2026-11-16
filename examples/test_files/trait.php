<?php

trait Test {
  public function test(): void {
    echo 'Works!', $this->x, PHP_EOL;
  }
}

class Foo {
  public $x = 'Foo';
  use Test;
}

class Bar {
  public $x = 'Bar';
  use Test;
}

$a = new Foo;
$a->test();

$a = new Bar;
$a->test();

?>