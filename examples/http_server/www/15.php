<?php

$solved = '123456789ABCDEF_';
$allowed = '123456789ABCDEF_';

/* Generate a solveable position by making moves */
function new_game()
{
    $fields = '123456789ABCDEF_';
    $blank = 15;
    $previousBlank = -1;

    for ($step = 0; $step < 200; ++$step) {
        $moves = array();

        for ($i = 0; $i < 16; ++$i) {
            if ($i == $blank || $i == $previousBlank) {
                continue;
            }

            $horizontal =
                ($i - ($i % 4)) == ($blank - ($blank % 4))
                && abs($i - $blank) == 1;

            $vertical = abs($i - $blank) == 4;

            if ($horizontal || $vertical) {
                $moves[] = $i;
            }
        }

        $next = $moves[mt_rand(0, count($moves) - 1)];

        $fields[$blank] = $fields[$next];
        $fields[$next] = '_';

        $previousBlank = $blank;
        $blank = $next;
    }

    return $fields;
}

/* Validate the complete board before using it. */
$valid = false;

if (isset($_GET['fields']) && is_string($_GET['fields'])) {
    $candidate = $_GET['fields'];

    if (strlen($candidate) == 16) {
        $seen = array();
        $valid = true;

        for ($i = 0; $i < 16; ++$i) {
            $tile = $candidate[$i];

            if (strpos($allowed, $tile) === false
                || isset($seen[$tile])) {
                $valid = false;
                break;
            }

            $seen[$tile] = true;
        }

        if ($valid) {
            $fields = $candidate;
        }
    }
}

if (!$valid) {
    $fields = new_game();
}

/* A tile is identified by its label, not its position. */
if (isset($_GET['pressed']) && is_string($_GET['pressed'])
    && strlen($_GET['pressed']) == 1) {

    $tile = $_GET['pressed'];

    if ($tile != '_' && strpos($allowed, $tile) !== false) {
        $tilePos = strpos($fields, $tile);
        $blankPos = strpos($fields, '_');

        $horizontal =
            ($tilePos - ($tilePos % 4))
                == ($blankPos - ($blankPos % 4))
            && abs($tilePos - $blankPos) == 1;

        $vertical = abs($tilePos - $blankPos) == 4;

        if ($horizontal || $vertical) {
            $fields[$blankPos] = $tile;
            $fields[$tilePos] = '_';
        }
    }
}

$won = ($fields === $solved);

?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Fifteen</title>
<style>
    body {
        font-family: sans-serif;
        text-align: center;
        margin: 30px auto;
        max-width: 440px;
        padding: 0 12px;
        background: #f4f5f7;
    }

    h1 { margin-bottom: 8px; }

    .board {
        display: grid;
        grid-template-columns: repeat(4, 1fr);
        gap: 6px;
        margin: 24px auto;
    }

    .tile {
        display: flex;
        align-items: center;
        justify-content: center;
        aspect-ratio: 1;
        border-radius: 9px;
        background: #356b9a;
        color: white;
        font-size: clamp(22px, 7vw, 36px);
        font-weight: bold;
        text-decoration: none;
    }

    a.tile:hover { background: #24537b; }

    .empty {
        background: #dfe3e8;
    }

    .message {
        color: #26743b;
        font-size: 20px;
        font-weight: bold;
    }

    .restart {
        display: inline-block;
        margin: 8px;
        padding: 10px 18px;
        background: #333;
        color: white;
        text-decoration: none;
        border-radius: 6px;
    }
</style>
</head>
<body>

<h1>Fifteen</h1>

<?php if ($won) { ?>
    <div class="message">Congratulations!</div>
<?php } ?>

<div class="board">
<?php
for ($i = 0; $i < 16; ++$i) {
    $tile = $fields[$i];

    if ($tile == '_') {
        echo '<div class="tile empty"></div>';
        continue;
    }

    $blankPos = strpos($fields, '_');

    $horizontal =
        ($i - ($i % 4)) == ($blankPos - ($blankPos % 4))
        && abs($i - $blankPos) == 1;

    $vertical = abs($i - $blankPos) == 4;

    if (!$won && ($horizontal || $vertical)) {
        $href = '15.php?fields=' . rawurlencode($fields)
              . '&amp;pressed=' . rawurlencode($tile);

        echo '<a class="tile" href="' . $href . '">'
           . $tile . '</a>';
    } else {
        echo '<div class="tile">' . $tile . '</div>';
    }
}
?>
</div>
<a class="restart" href="15.php">New game</a>
</body>
</html>
