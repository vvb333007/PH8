<?php

function fft(array $re, array $im): array
{
    $n = count($re);

    if ($n <= 1) {
        return [$re, $im];
    }

    $evenRe = [];
    $evenIm = [];
    $oddRe  = [];
    $oddIm  = [];

    for ($i = 0; $i < $n / 2; $i++) {
        $evenRe[] = $re[2 * $i];
        $evenIm[] = $im[2 * $i];

        $oddRe[] = $re[2 * $i + 1];
        $oddIm[] = $im[2 * $i + 1];
    }

    $even = fft($evenRe, $evenIm);
    $odd  = fft($oddRe, $oddIm);

    $outRe = array_fill(0, $n, 0.0);
    $outIm = array_fill(0, $n, 0.0);

    for ($k = 0; $k < $n / 2; $k++) {
        $angle = -2.0 * M_PI * $k / $n;

        $wr = cos($angle);
        $wi = sin($angle);

        $tr = $wr * $odd[0][$k]
            - $wi * $odd[1][$k];

        $ti = $wr * $odd[1][$k]
            + $wi * $odd[0][$k];

        $outRe[$k] = $even[0][$k] + $tr;
        $outIm[$k] = $even[1][$k] + $ti;

        $outRe[$k + $n / 2] = $even[0][$k] - $tr;
        $outIm[$k + $n / 2] = $even[1][$k] - $ti;
    }

    return [$outRe, $outIm];
}

function magnitude(float $re, float $im): float
{
    return sqrt($re * $re + $im * $im);
}

// Generate test signal
$N = 16;
$signal = [];

for ($n = 0; $n < $N; $n++) {
    $signal[] =
        sin(2.0 * M_PI * 3 * $n / $N)
        + 0.5 * sin(2.0 * M_PI * 5 * $n / $N);
}

$imag = array_fill(0, $N, 0.0);

echo "Input signal:\n";

for ($n = 0; $n < $N; $n++) {
    printf("%2d: % .6f\n", $n, $signal[$n]);
}

$result = fft($signal, $imag);

echo "\nFrequency spectrum:\n";
echo "Bin       Magnitude\n";
echo "-------------------\n";

for ($k = 0; $k <= $N / 2; $k++) {
    $mag = magnitude(
        $result[0][$k],
        $result[1][$k]
    );

    // Single-sided amplitude spectrum
    if ($k > 0 && $k < $N / 2) {
        $mag *= 2.0 / $N;
    } else {
        $mag /= $N;
    }

    printf("%3d       %.6f\n", $k, $mag);
}

echo "\nExpected peaks:\n";
echo "Bin 3: amplitude 1.0\n";
echo "Bin 5: amplitude 0.5\n";
echo "Done.\n";
?>
