#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering
    vInitScheduler();
    vSchedulerStart();
    return 0;
}