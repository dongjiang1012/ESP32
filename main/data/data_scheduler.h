#pragma once

#include "freertos/FreeRTOS.h"

/** FreeRTOS entry point for the SW6306 periodic collector. */
void data_collector_task(void *arg);
