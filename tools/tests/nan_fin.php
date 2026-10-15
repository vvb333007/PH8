<?php

$var = NAN;
if (!is_nan($var))
  die("is_nan(NAN) failed");

if (is_infinite($var))
  die("is_infinite(NAN) failed");

if (is_finite($var))
  die("is_finite(NAN) failed");

echo 'TEST 1 of 4 PASSED'.PHP_EOL;

$var = INF;
if (is_nan($var))
  die("is_nan(INF) failed");

if (!is_infinite($var))
  die("is_infinite(INF) failed");

if (is_finite($var))
  die("is_finite(INF) failed");

echo 'TEST 2 of 4 PASSED'.PHP_EOL;

$var = 11.99;

if (is_nan($var))
  die("is_nan($var) failed");

if (is_infinite($var))
  die("is_infinite($var) failed");

if (!is_finite($var))
  die("is_finite($var) failed");

echo 'TEST 3 of 4 PASSED'.PHP_EOL;

// Negative infinity
$var = -INF;
if (is_nan($var) || !is_infinite($var) || is_finite($var))
    die("-INF test failed");

// Zero is finite
$var = 0.0;
if (is_nan($var) || is_infinite($var) || !is_finite($var))
    die("ZERO test failed");

// Integer is finite too
$var = 42;
if (is_nan($var) || is_infinite($var) || !is_finite($var))
    die("INT test failed");

echo 'TEST 4 of 4 PASSED'.PHP_EOL;
echo 'TEST PASSED'.PHP_EOL;

?>