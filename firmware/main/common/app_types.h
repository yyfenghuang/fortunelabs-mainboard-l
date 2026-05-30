#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// The data that stream on queue
typedef struct
{
    float voltage;         // range 0,0 - 3V3, from ADC
    uint32_t timestamp_ms; // ms from boot
} sensor_reading_t;

// Queue handler
// Defined in main.c
extern QueueHandle_t g_queue_display;
extern QueueHandle_t g_queue_actuator;
