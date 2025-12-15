#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

#define MAX_TASKS 100

SimulationTask taskList[MAX_TASKS];
int taskCount = 0;
int globalTimer = 0;

int lastRRIndex = -1;

void loadTasks(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Hata: %s dosyasi acilamadi.\n", filename);
        exit(1);
    }
    
    // Add the check (&& taskCount < MAX_TASKS)
    while (taskCount < MAX_TASKS && fscanf(file, "%d, %d, %d", 
        &taskList[taskCount].arrivalTime, 
        &taskList[taskCount].priority, 
        &taskList[taskCount].burstTime) != EOF) {        
        taskList[taskCount].id = taskCount; 
        taskList[taskCount].remainingTime = taskList[taskCount].burstTime;
        taskList[taskCount].handle = NULL;
        taskList[taskCount].arrivalTimestamp = -1;
        taskList[taskCount].hasStarted = 0;
        
        taskList[taskCount].deadline = taskList[taskCount].arrivalTime + 20;

        strcpy(taskList[taskCount].name, "proses");
        taskCount++;
    }
    
    if (!feof(file) && taskCount == MAX_TASKS) {
        printf("Uyarı: Maksimum görev sayısına (%d) ulaşıldı, bazı görevler yüklenmedi.\n", MAX_TASKS);
    }
    
    fclose(file);
}

// Global Timeout Check
void checkGlobalTimeouts() {
    for (int i = 0; i < taskCount; i++) {
        // Only check tasks that are waiting (handle != NULL)
        if (taskList[i].handle != NULL && taskList[i].remainingTime > 0 && taskList[i].arrivalTimestamp != -1) {
            // Compare current time against the task's specific deadline
            if (globalTimer >= taskList[i].deadline) {
                printTaskLog(&taskList[i], "zamanaşımı");
                vTaskDelete(taskList[i].handle);
                taskList[i].handle = NULL; 
            }
        }
    }
}

SimulationTask* selectNextTask() {
    // 1. Real-Time (Priority 0)
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i].priority == 0 && taskList[i].remainingTime > 0 && taskList[i].handle != NULL) {
            return &taskList[i];
        }
    }
    // 2. User Tasks (Priority 1 & 2)
    for (int p = 1; p <= 2; p++) {
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].priority == p && taskList[i].remainingTime > 0 && taskList[i].handle != NULL) {
                return &taskList[i];
            }
        }
    }
    // 3. Round Robin (Priority 3)
    for (int count = 0; count < taskCount; count++) {
        int idx = (lastRRIndex + 1 + count) % taskCount;
        if (taskList[idx].priority == 3 && taskList[idx].remainingTime > 0 && taskList[idx].handle != NULL) {
            lastRRIndex = idx;
            return &taskList[idx];
        }
    }
    return NULL;
}

void vSchedulerTask(void *pvParameters) {
    (void) pvParameters;

    for (;;) {
        // 1. Admit
        for (int i = 0; i < taskCount; i++) {
            if (taskList[i].arrivalTime == globalTimer) {
                xTaskCreate(vTaskGenericFunction, taskList[i].name, 128, &taskList[i], 1, &taskList[i].handle);
                vTaskSuspend(taskList[i].handle);
                taskList[i].arrivalTimestamp = globalTimer;
                taskList[i].hasStarted = 0;
            }
        }

        // 2. Dispatch
        SimulationTask *current = selectNextTask();

        if (current != NULL) {
            if (current->hasStarted == 0) {
                printTaskLog(current, "başladı");
                current->hasStarted = 1;
            } else {
                printTaskLog(current, "yürütülüyor");
            }
            
            // CHECK 1: Fixes 21.0000 log (Running -> Timeout)
            checkGlobalTimeouts();

            // Run Simulation
            vTaskResume(current->handle);
            vTaskDelay(pdMS_TO_TICKS(1000));
            vTaskSuspend(current->handle);

            // Extend deadline because it ran
            current->deadline++;

            globalTimer++;       
            current->remainingTime--;

            if (current->remainingTime <= 0) {
                printTaskLog(current, "sonlandı");
                vTaskDelete(current->handle);
                current->handle = NULL;
            }
            // Check if current task timed out DURING execution
            else if (globalTimer >= current->deadline) {
                printTaskLog(current, "zamanaşımı");
                vTaskDelete(current->handle);
                current->handle = NULL;
            }
            else if (current->priority > 0 && current->priority < 3) {
                printTaskLog(current, "askıda"); 
                current->priority++;
            }
            
            // CHECK 2: Fixes 22.0000 log (Finished -> Timeout -> Start)
            // This catches tasks expiring exactly at the new second
            checkGlobalTimeouts();

        } else {
            // Idle
            vTaskDelay(pdMS_TO_TICKS(1000));
            globalTimer++;
            checkGlobalTimeouts();
        }

        // 3. Exit Check
        int allDone = 1;
        for(int i=0; i<taskCount; i++) {
            if(taskList[i].remainingTime > 0 && taskList[i].handle != NULL) allDone = 0;
            if(taskList[i].arrivalTime > globalTimer) allDone = 0;
        }
        if(allDone) {
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