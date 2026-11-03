<?php

$W = 24;
$H = 12;
$GENERATIONS = 12;

$grid = [];
$next = [];

// Initialize the field.
for ($y = 0; $y < $H; $y++) {
    $grid[$y] = [];
    $next[$y] = [];

    for ($x = 0; $x < $W; $x++) {
        $grid[$y][$x] = 0;
        $next[$y][$x] = 0;
    }
}

// Glider.
$grid[0][1] = 1;
$grid[1][2] = 1;
$grid[2][0] = 1;
$grid[2][1] = 1;
$grid[2][2] = 1;

for ($gen = 0; $gen < $GENERATIONS; $gen++) {

    echo 'Generation ', $gen, PHP_EOL;

    // Print current frame.
    for ($y = 0; $y < $H; $y++) {
        for ($x = 0; $x < $W; $x++) {
            echo $grid[$y][$x] ? 'O' : '.';
        }
        echo PHP_EOL;
    }

    echo PHP_EOL;

    // Calculate the next generation.
    for ($y = 0; $y < $H; $y++) {
        for ($x = 0; $x < $W; $x++) {

            $neighbors = 0;

            for ($dy = -1; $dy <= 1; $dy++) {
                for ($dx = -1; $dx <= 1; $dx++) {

                    if ($dx == 0 && $dy == 0)
                        continue;

                    $nx = $x + $dx;
                    $ny = $y + $dy;

                    if ($nx < 0 || $nx >= $W ||
                        $ny < 0 || $ny >= $H)
                        continue;

                    $neighbors += $grid[$ny][$nx];
                }
            }

            if ($grid[$y][$x]) {
                $next[$y][$x] =
                    ($neighbors == 2 || $neighbors == 3) ? 1 : 0;
            } else {
                $next[$y][$x] =
                    ($neighbors == 3) ? 1 : 0;
            }
        }
    }

    // Swap the buffers.
    $tmp = $grid;
    $grid = $next;
    $next = $tmp;
}

?>
