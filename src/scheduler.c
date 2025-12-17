#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define MAX_TASKS 100
#define TIMEOUT_WINDOW 20   // seconds

SimulationTask taskList[MAX_TASKS];
int taskCount = 0;
int globalTimer = 0;

static int lastRRIndex = -1;

void loadTasks(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Hata: %s dosyasi acilamadi.\n", filename);
        exit(1);
    }

    taskCount = 0;
    while (taskCount < MAX_TASKS &&
           fscanf(file, "%d, %d, %d",
                  &taskList[taskCount].arrivalTime,
                  &taskList[taskCount].priority,
                  &taskList[taskCount].burstTime) == 3) {

        taskList[taskCount].id = taskCount;
        taskList[taskCount].remainingTime = taskList[taskCount].burstTime;

        taskList[taskCount].handle = NULL;
        taskList[taskCount].arrivalTimestamp = -1;
        taskList[taskCount].hasStarted = 0;

        // If it never runs, it times out arrival + 20
        taskList[taskCount].deadline = taskList[taskCount].arrivalTime + TIMEOUT_WINDOW;

        strcpy(taskList[taskCount].name, "proses");
        taskCount++;
    }

    fclose(file);
}

static void checkGlobalTimeouts(void) {
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i].handle != NULL && taskList[i].remainingTime > 0) {
            if (globalTimer >= taskList[i].deadline) {
                printTaskLog(&taskList[i], "zamanaşımı");
                vTaskDelete(taskList[i].handle);
                taskList[i].handle = NULL;
            }
        }
    }
}

static SimulationTask* selectNextTask(void) {
    // 1) Priority 0
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i].handle != NULL &&
            taskList[i].remainingTime > 0 &&
            taskList[i].priority == 0) {
            return &taskList[i];
        }
    }

    // 2) Priority 1-2
    for (int p = 1; p <= 2; p++) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].handle != NULL &&
                taskList[i].remainingTime > 0 &&
                taskList[i].priority == p) {
                return &taskList[i];
            }
        }
    }

    // 3) Priority 3 (RR)
    for (int k = 0; k < taskCount; k++) {
        int idx = (lastRRIndex + 1 + k) % taskCount;
        if (taskList[idx].handle != NULL &&
            taskList[idx].remainingTime > 0 &&
            taskList[idx].priority == 3) {
            lastRRIndex = idx;
            return &taskList[idx];
        }
    }

    // 4) Priority 4-5
    for (int p = 4; p <= 5; p++) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].handle != NULL &&
                taskList[i].remainingTime > 0 &&
                taskList[i].priority == p) {
                return &taskList[i];
            }
        }
    }

    return NULL;
}

void vSchedulerTask(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        // Admit arrivals at this second
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].arrivalTime == globalTimer) {
                xTaskCreate(vTaskGenericFunction,
                            taskList[i].name,
                            128,
                            &taskList[i],
                            1,
                            &taskList[i].handle);

                vTaskSuspend(taskList[i].handle);
                taskList[i].arrivalTimestamp = globalTimer;
                taskList[i].hasStarted = 0;
            }
        }

        SimulationTask *current = selectNextTask();

        if (current != NULL) {
            // Print state at time T
            if (current->hasStarted == 0) {
                printTaskLog(current, "başladı");
                current->hasStarted = 1;
            } else {
                printTaskLog(current, "yürütülüyor");
            }

            // Print timeouts at time T (same style as report)
            checkGlobalTimeouts();

            // Run one second
            vTaskResume(current->handle);
            vTaskDelay(pdMS_TO_TICKS(1000));
            vTaskSuspend(current->handle);

            // Advance time to T+1
            globalTimer++;
            current->remainingTime--;

            if (current->remainingTime <= 0) {
                printTaskLog(current, "sonlandı");
                vTaskDelete(current->handle);
                current->handle = NULL;
            } else {
                // KEY FIX: after getting CPU, reset timeout window
                current->deadline = globalTimer + TIMEOUT_WINDOW;

                // KEY FIX: askıda allowed up to priority 5, and increment before printing
                if (current->priority > 0 && current->priority < 5) {
                    current->priority++;
                    printTaskLog(current, "askıda");
                }
            }
        } else {
            // Idle second
            checkGlobalTimeouts();
            vTaskDelay(pdMS_TO_TICKS(1000));
            globalTimer++;
        }

        // Stop condition
        int allDone = 1;
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].arrivalTime > globalTimer) { allDone = 0; break; }
            if (taskList[i].handle != NULL && taskList[i].remainingTime > 0) { allDone = 0; break; }
        }
        if (allDone) {
            printf("Simulasyon Tamamlandi.\n");
            exit(0);
        }
    }
}

void vInitScheduler(void) {
    loadTasks("giris.txt");
}

void vSchedulerStart(void) {
    xTaskCreate(vSchedulerTask, "Controller", 1000, NULL, configMAX_PRIORITIES - 1, NULL);
    vTaskStartScheduler();
}
