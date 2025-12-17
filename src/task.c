#include "scheduler.h"
#include <stdio.h>
#include <string.h>

// Colors
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"

const char* TASK_COLORS[] = {
    COLOR_BLUE, COLOR_RED, COLOR_GREEN, 
    COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN
};

extern int globalTimer;

void printTaskLog(SimulationTask *task, const char *status) {
    int colorIndex = task->id % 6;
    const char *color = TASK_COLORS[colorIndex];

    // REMOVED \t to fix cut-off issue
    // Adjusted padding: %-6s for name, %-13s for status
    printf("%s%d.0000 sn %-6s %-13s (id:%04d  öncelik:%d  kalan süre:%d sn)%s\n",
        color,
        globalTimer,
        task->name,
        status,
        task->id, 
        task->priority,
        task->remainingTime,
        COLOR_RESET);        
    fflush(stdout);
}

void vTaskGenericFunction(void *pvParameters) {
    (void)pvParameters;
    while(1) { vTaskDelay(pdMS_TO_TICKS(100)); }
}
