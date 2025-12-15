#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "FreeRTOS.h"
#include "task.h"

typedef struct {
    int id;
    int arrivalTime;
    int priority;
    int burstTime;
    int remainingTime;
    int arrivalTimestamp;
    int hasStarted;
    int deadline; // NEW: Tracks the exact time this task should timeout
    char name[16];
    TaskHandle_t handle;
} SimulationTask;

// Global functions
void vInitScheduler(void);
void vSchedulerStart(void);
void vTaskGenericFunction(void *pvParameters);
void printTaskLog(SimulationTask *task, const char *status); 

#endif