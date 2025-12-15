# Compiler
CC = gcc

# Directories
FREERTOS_DIR = FreeRTOS
SRC_DIR = src

# Compiler Flags
# -D_GNU_SOURCE turns on all standard Linux/POSIX features needed (fixes locale_t and pthreads)
CFLAGS = -I$(SRC_DIR) \
         -I$(FREERTOS_DIR)/include \
         -I$(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix \
         -Wall \
         -g \
        -D_GNU_SOURCE

# Source Files
PROJECT_SOURCES = $(SRC_DIR)/main.c \
				$(SRC_DIR)/scheduler.c \
				$(SRC_DIR)/task.c

# Kernel Files
KERNEL_SOURCES =  $(FREERTOS_DIR)/event_groups.c \
				$(FREERTOS_DIR)/list.c \
				$(FREERTOS_DIR)/queue.c \
				$(FREERTOS_DIR)/stream_buffer.c \
				$(FREERTOS_DIR)/tasks.c \
				$(FREERTOS_DIR)/timers.c \
				$(FREERTOS_DIR)/portable/MemMang/heap_3.c

# The Linux Port File
PORT_SOURCE = $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix/port.c \
        $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c

# Combine
SOURCES = $(PROJECT_SOURCES) $(KERNEL_SOURCES) $(PORT_SOURCE)
OBJECTS = $(SOURCES:.c=.o)

TARGET = freertos_sim

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	rm -f $(OBJECTS) $(TARGET)