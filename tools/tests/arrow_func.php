<?php
/*
 * Arrow function tests for PH8.
 * PH7/PH8 arrow functions do not capture the parent scope implicitly,
 * variables are captured explicitly with use().
 */

$nPassed = 0;
$nFailed = 0;

function fnCheck($zName, $mExpected, $mActual) {
  global $nPassed, $nFailed;
  if (json_encode($mExpected) === json_encode($mActual)) {
    $nPassed++;
    echo "PASS  $zName\n";
  } else {
    $nFailed++;
    echo "FAIL  $zName: expected " . json_encode($mExpected) . ", got " . json_encode($mActual) . "\n";
  }
}

/* 1. Plain arrow function stored in a variable, terminated by ';' */
$fAdd = fn($nA, $nB) => $nA + $nB;
fnCheck("assign to variable", 7, $fAdd(3, 4));

/* 2. Explicit capture with use() */
$nBase = 10;
$fAddBase = fn($nX) use ($nBase) => $nX + $nBase;
fnCheck("use() capture", 15, $fAddBase(5));

/* 3. Return type before the arrow */
$fTyped = fn($nX): int => $nX + 1;
fnCheck("return type", 6, $fTyped(5));

/* 4. Named arrow function statement */
fn fnSquare($nX) => $nX * $nX;
fnCheck("named arrow function", 36, fnSquare(6));

/* 5. Body ends at ')' : the only argument of a call */
fnCheck("only call argument", 7, call_user_func(fn() => 7));

/* 6. Body ends at ',' : followed by another argument */
fnCheck("call argument followed by another", [2, 4, 6], array_map(fn($nV) => $nV * 2, [1, 2, 3]));

/* 7. Commas inside the body must not end it */
fnCheck("commas inside body", 9, call_user_func(fn($nA, $nB) => max($nA, $nB), 3, 9));

/* 8. Array literal, value without key */
$aList = [fn() => 1, fn() => 2];
fnCheck("array entries without keys", [1, 2], [call_user_func($aList[0]), call_user_func($aList[1])]);

/* 9. Array literal, value with key */
$aMap = ['one' => fn() => 1, 'two' => fn() => 2];
fnCheck("array entries with keys", [1, 2], [call_user_func($aMap['one']), call_user_func($aMap['two'])]);

/* 10. Default value of a function parameter, followed by more parameters */
function fnWithDefault($fCallback = fn() => 42, $nExtra = 5) {
  return call_user_func($fCallback) + $nExtra;
}
fnCheck("default parameter", 47, fnWithDefault());
fnCheck("default parameter overridden", 15, fnWithDefault(fn() => 10));

/* 11. Nested arrow functions */
$fOuter = fn($nX) => fn($nY) use ($nX) => $nX + $nY;
fnCheck("nested arrow functions", 7, call_user_func($fOuter(3), 4));

/* 12. Ternary operator in the body */
$fSign = fn($nX) => $nX > 0 ? 'pos' : 'neg';
fnCheck("ternary in body", ['pos', 'neg'], [$fSign(1), $fSign(-1)]);

/* 13. Returned from a function */
function fnMakeAdder($nStep) {
  return fn($nX) use ($nStep) => $nX + $nStep;
}
$fAddThree = fnMakeAdder(3);
fnCheck("returned from function", 8, $fAddThree(5));

/* 14. Callback of usort() */
$aNumbers = [3, 1, 2];
usort($aNumbers, fn($nA, $nB) => $nA - $nB);
fnCheck("usort callback", [1, 2, 3], $aNumbers);

/* 15. Nested brackets and braces in the body */
$fPick = fn($aItems, $nIndex) => [$aItems[$nIndex], count($aItems)];
fnCheck("brackets in body", [20, 3], $fPick([10, 20, 30], 1));

/* 16. Closure with a block body inside an arrow function body */
$fWrap = fn($nX) => call_user_func(function() use ($nX) { $nY = $nX * 2; return $nY; });
fnCheck("block closure in body", 8, $fWrap(4));

/* 17. Immediately invoked, may not be supported by the call syntax itself */
fnCheck("immediately invoked", 1, (fn() => 1)());

echo "\nPassed: $nPassed, failed: $nFailed\n";
?>