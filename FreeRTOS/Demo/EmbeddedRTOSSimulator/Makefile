# EmbeddedRTOSSimulator Makefile

# Default: native build (for Pi 4 or x86 Linux)
CC = gcc
# For cross-compiling to Raspberry Pi 4 (aarch64):
#CC = aarch64-linux-gnu-gcc
CFLAGS = -I. -I../../Source/include -I../../Source/portable/GCC/Posix -Wall -g
LDFLAGS = -lpthread

SRCS = main.c board.c uart.c spi.c pci.c task_scheduler.c lru_cache.c
OBJS = $(SRCS:.c=.o)

# Path to FreeRTOS kernel source (adjust as needed)
FREERTOS_SRC = ../../Source
FREERTOS_OBJS = \
	$(FREERTOS_SRC)/tasks.o \
	$(FREERTOS_SRC)/queue.o \
	$(FREERTOS_SRC)/list.o \
	$(FREERTOS_SRC)/timers.o \
	$(FREERTOS_SRC)/event_groups.o \
	$(FREERTOS_SRC)/portable/MemMang/heap_4.o \
	$(FREERTOS_SRC)/portable/GCC/Posix/port.o

TARGET = EmbeddedRTOSSimulator

all: $(TARGET)

$(TARGET): $(OBJS) $(FREERTOS_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.log sim_output.log

.PHONY: all clean