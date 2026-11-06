<?php

/* FreeRTOS constants */

const pdPASS = 1;
const pdFAIL = 0;
const portMAX_DELAY = 0xffffffff;

const eNoAction = 0;
const eSetBits = 1;
const eIncrement = 2;
const eSetValueWithOverwrite = 3;
const eSetValueWithoutOverwrite = 4;


/** Global array emulating the FreeRTOS queue table.
 * The index in this array is the queue "handle" (analogue of QueueHandle_t which is a pointer 
 * in real FreeRTOS)
 *
 * The reason for this emulation:
 * 1. We do not want to expose raw pointers to the PHP code
 * 2. These queues are only used by PHP code so implementing them as real FreeRTOS bindings can
 * be slower (FF call, arg unpack/pack/parse, etc) - pure overhead
 *
 */

$_Queues = array();

/**
 * Creates a queue with the specified capacity.
 *
 * $item_type — optional element type checking:
 *   null            — type is automatically determined by the first xQueueSend()
 *   'int','string',
 *   'array','float',
 *   'bool','object'  — strict type checking via get_debug_type()
 *
 * @return int Queue handle, or -1 on error (equivalent to a NULL pointer in C)
 */
function xQueueCreate(int $que_len, string $item_type = ''): int {

    global $_Queues;

    if ($que_len <= 0) {
        return -1;
    }

    $handle = count($_Queues);
    $_Queues[$handle] = array(
        'capacity' => $que_len,
        'items'    => array(),       // Used as a FIFO: array_push/array_shift
        'type'     => $item_type, // null until determined
    );

    return $handle;
}

/**
 * Creates a queue with the specified capacity.
 *
 * $que — Queue handle (an interger index)
 *
 */
function vQueueDelete(int $que): void {
    global $_Queues;

    if (isset($_Queues[$que])) {
        unset($_Queues[$que]);
    }
}

/**
 * Adds an item to the end of the queue.
 * $ticks_wait is ignored semantically (single-threaded emulation,
 * no blocking), but the parameter is kept for API signature compatibility.
 */
function xQueueSend(int $que, mixed $item, int $ticks_wait = 0): int {
    global $_Queues;

    if (!isset($_Queues[$que])) {
        return pdFAIL;
    }

    $q = &$_Queues[$que];

    // Queue is full
    if (count($q['items']) >= $q['capacity']) {
        return pdFAIL;
    }

/*
    // TODO: implement get_debug_type()

    $actual_type = get_debug_type($item);

    if ($q['type'] === null) {
        $q['type'] = $actual_type;

    } elseif ($q['type'] !== $actual_type) {
        return pdFAIL;
    }
*/
    $q['items'][] = $item;

    return pdPASS;
}

/**
 * Alias, as in the original API (xQueueSend == xQueueSendToBack)
 */
function xQueueSendToBack(int $que, mixed $item, int $ticks_wait = 0): int {
    return xQueueSend($que, $item, $ticks_wait);
}

/**
 *  Push item to the front of the queue
 */
function xQueueSendToFront(int $que, mixed $item, int $ticks_wait = 0): int {
    global $_Queues;

    if (!isset($_Queues[$que])) {
        return pdFAIL;
    }

    $q = &$_Queues[$que];

    if (count($q['items']) >= $q['capacity']) {
        return pdFAIL;
    }

/*
    // TODO: implement get_debug_type()
    $actual_type = get_debug_type($item);

    if ($q['type'] === null) {
        $q['type'] = $actual_type;
    } elseif ($q['type'] !== $actual_type) {
        return pdFAIL;
    }
*/
    array_unshift($q['items'], $item);

    return pdPASS;
}

/**
 * Retrieves an item from the front of the queue.
 * Returns the item itself, or false if the queue is empty/non-existent
 * (real FreeRTOS returns pdFAIL via a separate parameter,
 * but since this function returns mixed, false is used to indicate failure).
 */
function xQueueReceive(int $que, int $ticks_wait = 0): mixed {
    global $_Queues;

    if (!isset($_Queues[$que]) || empty($_Queues[$que]['items'])) {
        // TODO: implement ticks_wait
        return false;
    }

    return array_shift($_Queues[$que]['items']);
}

/** Equivalent to uxQueueMessagesWaiting — number of items currently in the queue 
 *
 */
function uxQueueMessagesWaiting(int $que): int {
    global $_Queues;
    return isset($_Queues[$que]) ? count($_Queues[$que]['items']) : 0;
}

/** Equivalent to uxQueueSpacesAvailable — number of additional items that can fit
 *
 */
function uxQueueSpacesAvailable(int $que): int {
    global $_Queues;
    if (!isset($_Queues[$que])) {
        return 0;
    }
    return $_Queues[$que]['capacity'] - count($_Queues[$que]['items']);
}


/*
$q = xQueueCreate(10);

xQueueSend($q, 10);
xQueueSend($q, 'Hello');

var_dump($_Queues[$q]);
*/

?>
