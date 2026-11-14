<?php

require 'ext/freertos';

/* FreeRTOS API: Queues test */

/* Create a queue for 10 messages */
$q = xQueueCreate(10);

/* Send number 10, an integer */
xQueueSend($q, 10);

/* Send string */
xQueueSend($q, 'Hello');

/* Receive number 10 (FIFO), wait for the message  */
$r = xQueueReceive($q, portMAX_DELAY);

echo "Received message: \n";
var_dump($r);
echo "\n";

/* Utility functions */
echo 'Messages waiting in the queue: ' . uxQueueMessagesWaiting($q) . "\n";
echo 'Vailable space in the queue: ' . uxQueueSpacesAvailable($q) . " slots\n";

?>
