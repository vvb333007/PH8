<?php




function getDayType(int $day): string
{
    return match ($day) {
        1, 7       => 'Weekend',
        2, 3, 4, 5, 6 => 'Working day',
        default    => 'Invalid day',
    };
}

foreach ([1, 3, 6, 7, 10] as $day) {
    echo "Day $day: " . getDayType($day) . PHP_EOL;
}


?>
