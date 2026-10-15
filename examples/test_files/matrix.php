<?php

function matmul(array $a, array $b): array
{
    $c = [
        [0, 0, 0],
        [0, 0, 0],
        [0, 0, 0]
    ];

    for ($i = 0; $i < 3; $i++) {
        for ($j = 0; $j < 3; $j++) {
            for ($k = 0; $k < 3; $k++) {
                $c[$i][$j] += $a[$i][$k] * $b[$k][$j];
            }
        }
    }

    return $c;
}

function matprint(array $m): void
{
    foreach ($m as $row) {
        printf(
            "| %8.2f %8.2f %8.2f |\n",
            $row[0], $row[1], $row[2]
        );
    }
}

$a = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
];

$b = [
    [9, 8, 7],
    [6, 5, 4],
    [3, 2, 1]
];

echo "Matrix A:\n";
matprint($a);

echo "\nMatrix B:\n";
matprint($b);

$c = matmul($a, $b);

echo "\nA x B =\n";
matprint($c);

echo "\nCheck C[0][0]: ";
echo "1*9 + 2*6 + 3*3 = ";
echo $c[0][0], "\n";

echo "\nCheck C[2][2]: ";
echo "7*7 + 8*4 + 9*1 = ";
echo $c[2][2], "\n";

echo "\nDone.\n";
?>
